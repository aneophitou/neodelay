#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr auto kDemoOnId         = "demoOn";
constexpr auto kChorusOnId       = "chorusOn";
constexpr auto kDelay1OnId       = "delay1On";
constexpr auto kDelay2OnId       = "delay2On";
constexpr auto kEtherealOnId     = "etherealOn";
constexpr auto kRateId           = "rate";
constexpr auto kDepthId          = "depth";
constexpr auto kDelay1TimeId     = "delay1Time";
constexpr auto kDelay1FeedbackId = "delay1Feedback";
constexpr auto kDelay1ReverseId  = "delay1Reverse";
constexpr auto kDelay2TimeId     = "delay2Time";
constexpr auto kDelay2FeedbackId = "delay2Feedback";
constexpr auto kDelay2ReverseId  = "delay2Reverse";
constexpr auto kShimmerId        = "shimmer";
constexpr auto kMixId            = "mix";
} // namespace

NeoDelayAudioProcessor::NeoDelayAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout NeoDelayAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDemoOnId, 1 }, "Demo", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kChorusOnId, 1 }, "Chorus On", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelay1OnId, 1 }, "Delay 1 On", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelay2OnId, 1 }, "Delay 2 On", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kEtherealOnId, 1 }, "Ethereal On", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kRateId, 1 },
        "Rate",
        juce::NormalisableRange<float> { 0.1f, 4.0f, 0.001f, 0.5f },
        0.70f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kDepthId, 1 },
        "Depth",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.35f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float v, int) {
                return juce::String (juce::roundToInt (v * 100.0f));
            })
            .withValueFromStringFunction ([] (const juce::String& s) {
                return s.getFloatValue() / 100.0f;
            })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kDelay1TimeId, 1 },
        "Delay 1 Time",
        juce::NormalisableRange<float> { 50.0f, 1000.0f, 0.1f, 0.4f },
        380.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kDelay1FeedbackId, 1 },
        "Delay 1 Feedback",
        juce::NormalisableRange<float> { 0.0f, 0.85f, 0.001f },
        0.32f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float v, int) {
                return juce::String (juce::roundToInt (v * 100.0f));
            })
            .withValueFromStringFunction ([] (const juce::String& s) {
                return s.getFloatValue() / 100.0f;
            })));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelay1ReverseId, 1 }, "Delay 1 Reverse", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kDelay2TimeId, 1 },
        "Delay 2 Time",
        juce::NormalisableRange<float> { 50.0f, 1000.0f, 0.1f, 0.4f },
        560.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kDelay2FeedbackId, 1 },
        "Delay 2 Feedback",
        juce::NormalisableRange<float> { 0.0f, 0.85f, 0.001f },
        0.28f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float v, int) {
                return juce::String (juce::roundToInt (v * 100.0f));
            })
            .withValueFromStringFunction ([] (const juce::String& s) {
                return s.getFloatValue() / 100.0f;
            })));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelay2ReverseId, 1 }, "Delay 2 Reverse", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kShimmerId, 1 },
        "Shimmer",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.05f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float v, int) {
                return juce::String (juce::roundToInt (v * 100.0f));
            })
            .withValueFromStringFunction ([] (const juce::String& s) {
                return s.getFloatValue() / 100.0f;
            })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kMixId, 1 },
        "Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.45f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float v, int) {
                return juce::String (juce::roundToInt (v * 100.0f));
            })
            .withValueFromStringFunction ([] (const juce::String& s) {
                return s.getFloatValue() / 100.0f;
            })));

    return { params.begin(), params.end() };
}

void NeoDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const int channels = getTotalNumOutputChannels();
    chorusEngine.prepare (sampleRate, samplesPerBlock, channels);
    delayEngine1.prepare (sampleRate, samplesPerBlock, channels);
    delayEngine2.prepare (sampleRate, samplesPerBlock, channels);
    shimmerEngine.prepare (sampleRate, samplesPerBlock, channels);

    dryBuffer.setSize (channels, samplesPerBlock, false, false, true);
    demoTone.prepare (sampleRate, channels);
    mixSmoothed.reset (sampleRate, 0.03);
    mixSmoothed.setCurrentAndTargetValue (0.45f);

    updateEnginesFromParameters();
}

void NeoDelayAudioProcessor::releaseResources()
{
    chorusEngine.reset();
    delayEngine1.reset();
    delayEngine2.reset();
    shimmerEngine.reset();
    demoTone.reset();
}

bool NeoDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn  = layouts.getMainInputChannelSet();

    if (mainOut != mainIn)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void NeoDelayAudioProcessor::updateEnginesFromParameters()
{
    chorusEngine.setRateHz (apvts.getRawParameterValue (kRateId)->load());
    chorusEngine.setDepth  (apvts.getRawParameterValue (kDepthId)->load());

    delayEngine1.setDelayMs (apvts.getRawParameterValue (kDelay1TimeId)->load());
    delayEngine1.setFeedback (apvts.getRawParameterValue (kDelay1FeedbackId)->load());
    delayEngine1.setReverse (apvts.getRawParameterValue (kDelay1ReverseId)->load() > 0.5f);

    delayEngine2.setDelayMs (apvts.getRawParameterValue (kDelay2TimeId)->load());
    delayEngine2.setFeedback (apvts.getRawParameterValue (kDelay2FeedbackId)->load());
    delayEngine2.setReverse (apvts.getRawParameterValue (kDelay2ReverseId)->load() > 0.5f);

    shimmerEngine.setAmount (apvts.getRawParameterValue (kShimmerId)->load());
    mixSmoothed.setTargetValue (apvts.getRawParameterValue (kMixId)->load());
}

void NeoDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    updateEnginesFromParameters();

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    const bool demoOn = apvts.getRawParameterValue (kDemoOnId)->load() > 0.5f;
    if (demoOn)
        demoTone.render (buffer);

    dryBuffer.setSize (numChannels, numSamples, false, false, true);
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    const bool chorusOn   = apvts.getRawParameterValue (kChorusOnId)->load() > 0.5f;
    const bool delay1On   = apvts.getRawParameterValue (kDelay1OnId)->load() > 0.5f;
    const bool delay2On   = apvts.getRawParameterValue (kDelay2OnId)->load() > 0.5f;
    const bool etherealOn = apvts.getRawParameterValue (kEtherealOnId)->load() > 0.5f;

    if (chorusOn)
        chorusEngine.process (buffer);

    // Series taps: Delay 1 → Delay 2
    if (delay1On)
        delayEngine1.process (buffer);
    if (delay2On)
        delayEngine2.process (buffer);

    if (etherealOn)
        shimmerEngine.process (buffer);

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = mixSmoothed.getNextValue();
        const float dryGain = 1.0f - mix;
        const float wetGain = mix;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float dry = dryBuffer.getSample (ch, i);
            const float wet = buffer.getSample (ch, i);
            buffer.setSample (ch, i, dry * dryGain + wet * wetGain);
        }
    }
}

juce::AudioProcessorEditor* NeoDelayAudioProcessor::createEditor()
{
    return new NeoDelayAudioProcessorEditor (*this);
}

void NeoDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void NeoDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NeoDelayAudioProcessor();
}
