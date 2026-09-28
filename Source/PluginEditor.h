#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

// The editor is a thin host for the CV Fuzz interface (Source/ui/index.html),
// which is compiled into the binary and served to an embedded web view.
// Each knob is bound to a real AudioProcessorValueTreeState parameter through
// a WebSliderRelay, so DAW automation, presets and undo all work normally.
class CVFuzzEditor : public juce::AudioProcessorEditor
{
public:
    explicit CVFuzzEditor(CVFuzzAudioProcessor &);
    ~CVFuzzEditor() override;

    void resized() override;

private:
    void pushScope();
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String &url);

    CVFuzzAudioProcessor &fuzzProcessor;

    // Relays must be constructed before the web view that uses them.
    juce::WebSliderRelay fuzzRelay{"fuzz"}, toneRelay{"tone"}, levelRelay{"level"}, biasRelay{"bias"};

    juce::WebBrowserComponent webView;

    juce::WebSliderParameterAttachment fuzzAttachment, toneAttachment, levelAttachment, biasAttachment;

    // Fires once per display refresh (60 Hz, 120 Hz on ProMotion), so the scope
    // updates in step with the screen instead of on a fixed timer.
    juce::VBlankAttachment vblank { this, [this] { pushScope(); } };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CVFuzzEditor)
};
