#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

/**
 * Two-voice CE / Cyber Twin–style chorus for the wet chain.
 * Subtle-to-moderate thickening (not extreme vibrato). Rate/Depth
 * cover a Tears Don’t Fall intro–like setting. Global Mix is elsewhere.
 */
class ChorusEngine
{
public:
    ChorusEngine() = default;

    void prepare (double sampleRate, int maximumBlockSize, int numChannels);
    void reset();

    void setRateHz (float rateHz);
    void setDepth (float depth01);

    void process (juce::AudioBuffer<float>& buffer);

private:
    static constexpr int kMaxVoices = 2;
    // CE-ish short delay window — musical depth without seasick warble
    static constexpr float kMinDelayMs = 7.0f;
    static constexpr float kMaxDelayMs = 18.0f;
    static constexpr float kBaseDelayMs = 12.5f;
    static constexpr float kMaxModMs = 4.5f; // excursion at Depth=100%
    static constexpr float kHpfHz = 160.0f;

    struct Voice
    {
        float phase = 0.0f;
        float phaseOffset = 0.0f;
        float rateScale = 1.0f;
        float panL = 0.7071f;
        float panR = 0.7071f;
    };

    float hermiteRead (int channel, float delaySamples) const;
    void writeSample (int channel, float sample);
    void advanceWriteIndex();

    double sampleRate = 44100.0;
    int numChannels = 2;
    int writeIndex = 0;
    int delayBufferSize = 0;

    std::vector<std::vector<float>> delayBuffer;
    std::array<Voice, kMaxVoices> voices;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> rateSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> depthSmoothed;

    std::array<float, 2> hpfState { 0.0f, 0.0f };
    float hpfCoeff = 0.0f;
};
