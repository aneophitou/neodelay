#pragma once

#include <JuceHeader.h>
#include "dsp/ChorusEngine.h"
#include "dsp/DelayEngine.h"
#include "dsp/DemoToneGenerator.h"
#include "dsp/ShimmerEngine.h"

class NeoDelayAudioProcessor : public juce::AudioProcessor
{
public:
    NeoDelayAudioProcessor();
    ~NeoDelayAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void updateEnginesFromParameters();

    juce::AudioProcessorValueTreeState apvts;
    ChorusEngine      chorusEngine;
    DelayEngine       delayEngine1;
    DelayEngine       delayEngine2;
    ShimmerEngine     shimmerEngine;
    DemoToneGenerator demoTone;

    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeoDelayAudioProcessor)
};
