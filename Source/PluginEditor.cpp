#include "PluginEditor.h"

namespace
{
constexpr int kMinHit = 48;
constexpr int kPreferredWidth = 860;
constexpr int kPreferredHeight = 640;

const juce::Colour kBgTop     { 0xff1a1c1e };
const juce::Colour kBgBottom  { 0xff101214 };
const juce::Colour kPanel     { 0xff24282c };
const juce::Colour kAccent    { 0xffc4a574 };
const juce::Colour kDemoAccent { 0xff8fbc8f };
const juce::Colour kText      { 0xffe6e2da };
const juce::Colour kMuted     { 0xff9a9590 };
const juce::Colour kToggleOff { 0xff1c1f22 };

juce::String formatPercent (float v)
{
    return juce::String (juce::roundToInt (v * 100.0f)) + " %";
}
} // namespace

NeoDelayAudioProcessorEditor::NeoDelayAudioProcessorEditor (NeoDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (kPreferredWidth, kPreferredHeight);
    setResizable (true, true);
    setResizeLimits (640, 520, 1600, 1200);

    setupToggle (demoToggle,           "Demo",     "demoOn",         demoOnAttachment);
    setupToggle (chorusToggle,         "Chorus",   "chorusOn",       chorusOnAttachment);
    setupToggle (delay1Toggle,         "Delay 1",  "delay1On",       delay1OnAttachment);
    setupToggle (delay2Toggle,         "Delay 2",  "delay2On",       delay2OnAttachment);
    setupToggle (etherealToggle,       "Ethereal", "etherealOn",     etherealOnAttachment);
    setupToggle (delay1ReverseToggle,  "D1 Rev",   "delay1Reverse",  delay1ReverseAttachment);
    setupToggle (delay2ReverseToggle,  "D2 Rev",   "delay2Reverse",  delay2ReverseAttachment);
    refreshDemoLook();

    setupKnob (rateKnob, "Rate", "rate", [] (float v) {
        return juce::String (v, 2) + " Hz";
    });
    setupKnob (depthKnob, "Depth", "depth", formatPercent);
    setupKnob (delay1TimeKnob, "D1 Time", "delay1Time", [] (float v) {
        return juce::String (juce::roundToInt (v)) + " ms";
    });
    setupKnob (delay1FeedbackKnob, "D1 Fb", "delay1Feedback", formatPercent);
    setupKnob (delay2TimeKnob, "D2 Time", "delay2Time", [] (float v) {
        return juce::String (juce::roundToInt (v)) + " ms";
    });
    setupKnob (delay2FeedbackKnob, "D2 Fb", "delay2Feedback", formatPercent);
    setupKnob (shimmerKnob, "Shimmer", "shimmer", formatPercent);
    setupKnob (mixKnob, "Mix", "mix", formatPercent);

    startTimerHz (30);
}

void NeoDelayAudioProcessorEditor::setupToggle (
    juce::TextButton& button,
    const juce::String& label,
    const juce::String& paramId,
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attachment)
{
    button.setButtonText (label);
    button.setClickingTogglesState (true);
    button.setColour (juce::TextButton::textColourOffId, kMuted);
    button.setColour (juce::TextButton::textColourOnId, kBgBottom);
    button.setColour (juce::TextButton::buttonColourId, kToggleOff);
    button.setColour (juce::TextButton::buttonOnColourId, kAccent);
    button.setConnectedEdges (0);
    addAndMakeVisible (button);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), paramId, button);

    button.onClick = [this, &button]()
    {
        if (&button == &demoToggle)
            refreshDemoLook();
        else
            refreshToggleLook (button, button.getToggleState());
    };

    refreshToggleLook (button, button.getToggleState());
}

void NeoDelayAudioProcessorEditor::refreshToggleLook (juce::TextButton& button, bool isOn)
{
    button.setColour (juce::TextButton::buttonColourId, isOn ? kAccent : kToggleOff);
    button.setColour (juce::TextButton::buttonOnColourId, kAccent);
    button.setColour (juce::TextButton::textColourOffId, isOn ? kBgBottom : kMuted);
    button.setColour (juce::TextButton::textColourOnId, kBgBottom);
}

void NeoDelayAudioProcessorEditor::refreshDemoLook()
{
    const bool isOn = demoToggle.getToggleState();
    demoToggle.setColour (juce::TextButton::buttonColourId, isOn ? kDemoAccent : kToggleOff);
    demoToggle.setColour (juce::TextButton::buttonOnColourId, kDemoAccent);
    demoToggle.setColour (juce::TextButton::textColourOffId, isOn ? kBgBottom : kMuted);
    demoToggle.setColour (juce::TextButton::textColourOnId, kBgBottom);
}

void NeoDelayAudioProcessorEditor::setupKnob (Knob& knob,
                                              const juce::String& name,
                                              const juce::String& paramId,
                                              std::function<juce::String (float)> formatter)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.slider.setRotaryParameters (juce::degreesToRadians (225.0f),
                                     juce::degreesToRadians (495.0f),
                                     true);
    knob.slider.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    knob.slider.setColour (juce::Slider::rotarySliderOutlineColourId, kMuted.withAlpha (0.55f));
    knob.slider.setColour (juce::Slider::thumbColourId, kText);
    addAndMakeVisible (knob.slider);

    knob.title.setText (name, juce::dontSendNotification);
    knob.title.setJustificationType (juce::Justification::centred);
    knob.title.setColour (juce::Label::textColourId, kText);
    knob.title.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (knob.title);

    knob.value.setJustificationType (juce::Justification::centred);
    knob.value.setColour (juce::Label::textColourId, kMuted);
    knob.value.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (knob.value);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), paramId, knob.slider);

    knob.slider.onValueChange = [&knob, formatter]()
    {
        knob.value.setText (formatter ((float) knob.slider.getValue()), juce::dontSendNotification);
    };

    knob.slider.onValueChange();
}

void NeoDelayAudioProcessorEditor::timerCallback()
{
    const float rate = audioProcessor.getAPVTS().getRawParameterValue ("rate")->load();
    lfoPhase += rate / 30.0f;
    if (lfoPhase >= 1.0f)
        lfoPhase -= 1.0f;

    refreshDemoLook();
    refreshToggleLook (chorusToggle, chorusToggle.getToggleState());
    refreshToggleLook (delay1Toggle, delay1Toggle.getToggleState());
    refreshToggleLook (delay2Toggle, delay2Toggle.getToggleState());
    refreshToggleLook (etherealToggle, etherealToggle.getToggleState());
    refreshToggleLook (delay1ReverseToggle, delay1ReverseToggle.getToggleState());
    refreshToggleLook (delay2ReverseToggle, delay2ReverseToggle.getToggleState());

    repaint (getLocalBounds().removeFromTop (56));
}

void NeoDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (kBgTop, 0.0f, 0.0f,
                                             kBgBottom, 0.0f, bounds.getHeight(), false));
    g.fillAll();

    auto panel = bounds.reduced (18.0f, 14.0f);
    g.setColour (kPanel);
    g.fillRoundedRectangle (panel, 14.0f);
    g.setColour (kMuted.withAlpha (0.25f));
    g.drawRoundedRectangle (panel, 14.0f, 1.2f);

    g.setColour (kText);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("NeoDelay", 28, 18, 260, 28, juce::Justification::centredLeft);

    g.setColour (kMuted);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("Andreas Neophitou  ·  chorus · dual delay · ethereal",
                28, 42, 420, 16, juce::Justification::centredLeft);

    const float indicatorX = (float) getWidth() - 168.0f;
    const float indicatorY = 40.0f;
    const float wobble = std::sin (lfoPhase * juce::MathConstants<float>::twoPi) * 6.0f;
    g.setColour (kAccent.withAlpha (0.85f));
    g.fillEllipse (indicatorX + wobble, indicatorY - 3.0f, 6.0f, 6.0f);
}

void NeoDelayAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (28, 20);

    auto brandRow = area.removeFromTop (52);
    demoToggle.setBounds (brandRow.removeFromRight (120).withSizeKeepingCentre (120, juce::jmax (kMinHit - 4, 44)));

    area.removeFromTop (4);
    auto toggleRow1 = area.removeFromTop (48);
    area.removeFromTop (8);
    auto toggleRow2 = area.removeFromTop (48);
    area.removeFromTop (10);

    juce::TextButton* row1[] = { &chorusToggle, &delay1Toggle, &delay2Toggle, &etherealToggle };
    juce::TextButton* row2[] = { &delay1ReverseToggle, &delay2ReverseToggle };
    layoutToggles (toggleRow1, row1, 4);
    layoutToggles (toggleRow2, row2, 2);

    Knob* knobs[] = {
        &rateKnob, &depthKnob, &delay1TimeKnob, &delay1FeedbackKnob,
        &delay2TimeKnob, &delay2FeedbackKnob, &shimmerKnob, &mixKnob
    };
    layoutKnobs (area, knobs, 8, 4);
}

void NeoDelayAudioProcessorEditor::layoutToggles (juce::Rectangle<int> bounds,
                                                  juce::TextButton* const* toggles,
                                                  int count)
{
    const int gap = 10;
    const int cellW = (bounds.getWidth() - gap * (count - 1)) / count;
    const int height = juce::jmax (kMinHit - 4, bounds.getHeight());

    for (int i = 0; i < count; ++i)
    {
        auto cell = juce::Rectangle<int> (bounds.getX() + i * (cellW + gap),
                                          bounds.getCentreY() - height / 2,
                                          cellW,
                                          height);
        toggles[i]->setBounds (cell);
    }
}

void NeoDelayAudioProcessorEditor::layoutKnobs (juce::Rectangle<int> bounds,
                                                Knob* const* knobs,
                                                int count,
                                                int cols)
{
    const int rows = (count + cols - 1) / cols;
    const int cellW = bounds.getWidth() / cols;
    const int cellH = bounds.getHeight() / rows;

    for (int i = 0; i < count; ++i)
    {
        const int row = i / cols;
        const int col = i % cols;
        auto cell = juce::Rectangle<int> (bounds.getX() + col * cellW,
                                          bounds.getY() + row * cellH,
                                          cellW,
                                          cellH).reduced (6);

        auto titleArea = cell.removeFromTop (20);
        auto valueArea = cell.removeFromBottom (18);
        auto dialArea  = cell;

        const int dialSize = juce::jmax (kMinHit, juce::jmin (dialArea.getWidth(), dialArea.getHeight()) - 4);
        auto dial = juce::Rectangle<int> (dialSize, dialSize).withCentre (dialArea.getCentre());

        knobs[i]->title.setBounds (titleArea);
        knobs[i]->slider.setBounds (dial);
        knobs[i]->value.setBounds (valueArea);
    }
}
