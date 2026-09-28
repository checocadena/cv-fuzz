#pragma once
//
// FuzzProcessor.h
//
// A framework-agnostic C++ model of a classic 3-knob (Fuzz / Tone / Level)
// fuzz circuit, e.g. a Fuzz Face-style two-transistor / diode-clipper
// topology. No JUCE, no framework dependency — this is the "core" class
// that gets wrapped by:
//   - a standalone app (live audio in/out)
//   - a JUCE AudioProcessor plugin (VST3/AU)
// so the actual circuit modeling only has to be written once.
//
// Circuit-modeling choices, so the "why" is documented alongside the "what":
//
//   1. Asymmetric soft clipping (tanh-based, different thresholds for the
//      positive and negative half-cycles) approximates the asymmetric
//      forward-voltage behavior of a biased diode/transistor clipping
//      stage (e.g. germanium vs. silicon halves, or a biased BJT stage
//      that clips harder on one side). Asymmetry is what gives a fuzz its
//      even-harmonic-rich, "fizzy"/farty character rather than a clean,
//      symmetric distortion.
//
//   2. 4x oversampling (zero-stuff upsample -> lowpass -> nonlinearity ->
//      lowpass -> decimate) is done because a hard nonlinearity generates
//      harmonics well above Nyquist; without oversampling those alias back
//      down into the audible band as inharmonic garbage. This is standard
//      practice for any waveshaper/distortion plugin.
//
//   3. A one-pole DC blocker after the clipper removes the DC offset that
//      asymmetric clipping introduces (the positive and negative halves no
//      longer average to zero).
//
//   4. The tone stage is a simplified single-knob crossfade between the
//      raw (bright) signal and a low-passed (dark) version — a common
//      approximation of a passive RC tone network like the one in a real
//      fuzz pedal's tone control. A future iteration could replace this
//      with a proper multi-pole model of the actual pedal's tone network
//      if the real schematic's component values are known.
//

#include <cmath>

class Biquad
{
public:
    void setLowpass(double sampleRate, double cutoffHz, double q = 0.70710678)
    {
        double omega = 2.0 * M_PI * cutoffHz / sampleRate;
        double alpha = std::sin(omega) / (2.0 * q);
        double cosw = std::cos(omega);
        double a0 = 1.0 + alpha;

        b0 = ((1.0 - cosw) / 2.0) / a0;
        b1 = (1.0 - cosw) / a0;
        b2 = b0;
        a1 = (-2.0 * cosw) / a0;
        a2 = (1.0 - alpha) / a0;
    }

    inline double process(double x)
    {
        double y = b0 * x + b1 * z1 + b2 * z2 - a1 * y1 - a2 * y2;
        z2 = z1; z1 = x;
        y2 = y1; y1 = y;
        return y;
    }

    void reset() { z1 = z2 = y1 = y2 = 0.0; }

private:
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    double z1 = 0.0, z2 = 0.0, y1 = 0.0, y2 = 0.0;
};

class FuzzProcessor
{
public:
    static constexpr int kOversample = 4;

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        double osSr = sr * kOversample;

        // Anti-imaging / anti-aliasing filters for the oversampled stage,
        // cut just under the original Nyquist.
        upFilter.setLowpass(osSr, sr * 0.45, 0.70710678);
        downFilter.setLowpass(osSr, sr * 0.45, 0.70710678);

        setTone(tone);
        reset();
    }

    void reset()
    {
        upFilter.reset();
        downFilter.reset();
        toneLow.reset();
        dcX1 = dcY1 = 0.0;
    }

    // params are 0..1 knob positions, mapped internally to real ranges
    void setFuzz(double f)  { fuzz = clamp01(f); }
    void setTone(double t)
    {
        tone = clamp01(t);
        // Map tone knob to the lowpass cutoff used for the "dark" path:
        // fully dark (tone=0) ~500 Hz, fully bright (tone=1) ~6 kHz.
        double cutoff = 500.0 * std::pow(12.0, tone);
        toneLow.setLowpass(sr, cutoff, 0.70710678);
    }
    void setLevel(double l) { level = clamp01(l); }

    // Bias models a starved transistor ("dying battery" fuzz). Two things happen:
    //   1. The operating point shifts relative to the INPUT signal, before the
    //      drive stage. Anything quieter than the offset can't swing past it,
    //      so it sits on one rail and the DC blocker turns it into silence.
    //      Drum decays and note tails choke off; loud hits break through and
    //      sputter as they cross the threshold.
    //   2. Headroom on the negative side collapses, pushing the stage toward
    //      half-wave rectification, which adds the ragged "velcro" edge.
    // The knob is curved (bias^1.5) so the first half stays usable.
    void setBias(double b)
    {
        bias = clamp01(b);
        const double starve = std::pow(bias, 1.5);   // computed once per audio block, not per sample
        biasOffset = starve * 0.25;                  // in input units, before drive
        negThresh = 0.46 * (1.0 - 0.85 * starve);    // negative-side headroom collapses
    }

    inline float processSample(float xin)
    {
        // Fuzz knob drives input gain into the clipper — up to ~60x.
        double drive = 1.0 + fuzz * 60.0;
        double xDriven = (static_cast<double>(xin) + biasOffset) * drive;

        double clippedOS = 0.0;
        for (int i = 0; i < kOversample; ++i)
        {
            // zero-stuff upsample: only the first of every 4 samples carries
            // signal energy, compensated by *kOversample so the lowpass
            // reconstruction restores correct amplitude.
            double stuffed = (i == 0) ? xDriven * kOversample : 0.0;
            double up = upFilter.process(stuffed);
            double clipped = asymmetricClip(up, negThresh);
            double down = downFilter.process(clipped);
            clippedOS = down; // last of the 4 = the decimated output
        }

        // DC blocker (one-pole highpass), R close to 1 = very low cutoff
        constexpr double R = 0.995;
        double dcOut = clippedOS - dcX1 + R * dcY1;
        dcX1 = clippedOS;
        dcY1 = dcOut;

        // Tone: crossfade between raw (bright) and low-passed (dark)
        double dark = toneLow.process(dcOut);
        double toned = dcOut * tone + dark * (1.0 - tone);

        return static_cast<float>(toned * level);
    }

private:
    static double asymmetricClip(double x, double negThresh)
    {
        // Different "forward voltage" thresholds per half-cycle == asymmetry.
        // negThresh shrinks as Bias starves the stage.
        constexpr double posThresh = 0.70;
        return x >= 0.0
            ? std::tanh(x / posThresh) * posThresh
            : std::tanh(x / negThresh) * negThresh;
    }

    static double clamp01(double v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }

    double sr = 48000.0;
    double fuzz = 0.5, tone = 0.5, level = 0.7, bias = 0.0;
    double biasOffset = 0.0, negThresh = 0.46;

    Biquad upFilter, downFilter, toneLow;
    double dcX1 = 0.0, dcY1 = 0.0;
};
