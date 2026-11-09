#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <functional>

class NeoDelayAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit NeoDelayAudioProcessorEditor (NeoDelayAudioProcessor&);
    ~NeoDelayAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label  title;
        juce::Label  value;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void timerCallback() override;
    void layoutToggles (juce::Rectangle<int> bounds, juce::TextButton* const* toggles, int count);
    void layoutKnobs (juce::Rectangle<int> bounds, Knob* const* knobs, int count, int cols);
    void refreshToggleLook (juce::TextButton& button, bool isOn);
    void refreshDemoLook();

    void setupKnob (Knob& knob,
                    const juce::String& name,
                    const juce::String& paramId,
                    std::function<juce::String (float)> formatter);

    void setupToggle (juce::TextButton& button,
                      const juce::String& label,
                      const juce::String& paramId,
                      std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attachment);

    NeoDelayAudioProcessor& audioProcessor;

    juce::TextButton demoToggle { "Demo" };
    juce::TextButton chorusToggle { "Chorus" };
    juce::TextButton delay1Toggle { "Delay 1" };
    juce::TextButton delay2Toggle { "Delay 2" };
    juce::TextButton etherealToggle { "Ethereal" };
    juce::TextButton delay1ReverseToggle { "D1 Rev" };
    juce::TextButton delay2ReverseToggle { "D2 Rev" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> demoOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chorusOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delay1OnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delay2OnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> etherealOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delay1ReverseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delay2ReverseAttachment;

    Knob rateKnob;
    Knob depthKnob;
    Knob delay1TimeKnob;
    Knob delay1FeedbackKnob;
    Knob delay2TimeKnob;
    Knob delay2FeedbackKnob;
    Knob shimmerKnob;
    Knob mixKnob;

    float lfoPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeoDelayAudioProcessorEditor)
};
