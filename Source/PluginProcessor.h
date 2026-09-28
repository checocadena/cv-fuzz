#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "FuzzProcessor.h"
#include <array>
#include <atomic>

// CV Fuzz — Checo Cadena
// A digital model of Checo Cadena's NYU-grant hardware fuzz pedal,
// wrapped as a VST3/Standalone plugin. The DSP core (FuzzProcessor) is
// shared, unmodified, with the standalone-app version of this project —
// only this file and PluginEditor exist to bridge it into JUCE's plugin
// API.
class CVFuzzAudioProcessor : public juce::AudioProcessor
{
public:
    CVFuzzAudioProcessor();
    ~CVFuzzAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
    void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CV Fuzz"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String &) override {}

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Lock-free-ish scope buffer: the audio thread writes, the GUI timer
    // reads. Single writer / single reader, plain floats (not std::atomic)
    // for speed — worst case is a torn read on the visualizer, which is
    // cosmetically harmless and never touches audio.
    static constexpr int scopeBufferSize = 4096;
    std::array<float, scopeBufferSize> scopeBuffer{};
    std::atomic<int> scopeWritePos{0};

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::vector<FuzzProcessor> channelProcessors;
    std::atomic<float> *fuzzParam = nullptr;
    std::atomic<float> *toneParam = nullptr;
    std::atomic<float> *levelParam = nullptr;
    std::atomic<float> *bypassParam = nullptr;
    std::atomic<float> *biasParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CVFuzzAudioProcessor)
};
