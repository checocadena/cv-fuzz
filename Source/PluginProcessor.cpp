#include "PluginProcessor.h"
#include "PluginEditor.h"

CVFuzzAudioProcessor::CVFuzzAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    fuzzParam = apvts.getRawParameterValue("fuzz");
    toneParam = apvts.getRawParameterValue("tone");
    levelParam = apvts.getRawParameterValue("level");
    bypassParam = apvts.getRawParameterValue("bypass");
    biasParam = apvts.getRawParameterValue("bias");
}

juce::AudioProcessorValueTreeState::ParameterLayout CVFuzzAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"fuzz", 1}, "Fuzz",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"tone", 1}, "Tone",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"level", 1}, "Level",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"bias", 1}, "Bias",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypass", 1}, "Bypass", false));

    return { params.begin(), params.end() };
}

void CVFuzzAudioProcessor::prepareToPlay(double sampleRate, int)
{
    auto numChannels = static_cast<size_t>(getTotalNumInputChannels());
    channelProcessors.assign(numChannels, FuzzProcessor{});
    for (auto &fp : channelProcessors)
        fp.prepare(sampleRate);
}

bool CVFuzzAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet()
        && !layouts.getMainOutputChannelSet().isDisabled();
}

void CVFuzzAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &)
{
    juce::ScopedNoDenormals noDenormals;

    const float fuzz = fuzzParam->load();
    const float tone = toneParam->load();
    const float level = levelParam->load();
    const bool bypassed = bypassParam->load() > 0.5f;
    const float bias = biasParam->load();

    auto numChannels = static_cast<size_t>(buffer.getNumChannels());
    for (size_t ch = 0; ch < numChannels && ch < channelProcessors.size(); ++ch)
    {
        auto &fp = channelProcessors[ch];
        fp.setFuzz(fuzz);
        fp.setTone(tone);
        fp.setLevel(level);
        fp.setBias(bias);

        auto *data = buffer.getWritePointer(static_cast<int>(ch));
        if (!bypassed)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                data[i] = fp.processSample(data[i]);
        // bypassed: leave data untouched, straight passthrough
    }

    // Feed the scope with whatever actually left the plugin (channel 0),
    // so the GUI shows the real output whether bypassed or not.
    if (buffer.getNumChannels() > 0)
    {
        const auto *out = buffer.getReadPointer(0);
        int pos = scopeWritePos.load(std::memory_order_relaxed);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            scopeBuffer[static_cast<size_t>(pos)] = out[i];
            pos = (pos + 1) % scopeBufferSize;
        }
        scopeWritePos.store(pos, std::memory_order_relaxed);
    }
}

juce::AudioProcessorEditor *CVFuzzAudioProcessor::createEditor()
{
    return new CVFuzzEditor(*this);
}

void CVFuzzAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    if (auto state = apvts.copyState(); true)
    {
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }
}

void CVFuzzAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// This creates new instances of the plugin
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new CVFuzzAudioProcessor();
}
