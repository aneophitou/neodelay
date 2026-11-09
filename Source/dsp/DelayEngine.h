#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

/**
 * Clean digital stereo delay with optional reverse playback.
 * Time range includes ~370–400 ms (Tears Don’t Fall quarter-ish).
 * Feedback capped for stability; light HPF keeps repeats pristine.
 */
class DelayEngine
{
public:
    DelayEngine() = default;

    void prepare (double sampleRate, int maximumBlockSize, int numChannels);
    void reset();

    void setDelayMs (float delayMs);
    void setFeedback (float feedback01);
    void setReverse (bool shouldReverse);

    void process (juce::AudioBuffer<float>& buffer);

private:
    float readAt (int channel, float delaySamples) const;

    double sampleRate = 44100.0;
    int numChannels = 2;
    int writeIndex = 0;
    int bufferSize = 0;
    bool reverse = false;

    static constexpr float kMaxDelayMs = 1000.0f;
    static constexpr float kMinDelayMs = 50.0f;
    static constexpr float kMaxFeedback = 0.85f;

    std::vector<std::vector<float>> delayLine;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delaySamplesSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> feedbackSmoothed;

    std::array<float, 2> fbLpState { 0.0f, 0.0f };
    float fbHpfCoeff = 0.0f;
};
