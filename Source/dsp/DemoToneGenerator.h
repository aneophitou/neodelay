#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

/**
 * Clean plucked arpeggio for Demo / Test Tone mode.
 * Replaces silent/muted device input so Standalone is audible without a guitar.
 */
class DemoToneGenerator
{
public:
    void prepare (double sampleRate, int numChannels);
    void reset();

    /** Writes stereo (or mono) plucked notes into buffer, replacing contents. */
    void render (juce::AudioBuffer<float>& buffer);

private:
    void triggerNote (int patternIndex);

    double sampleRate = 44100.0;
    int numChannels = 2;

    static constexpr int kMaxPeriod = 2048;
    std::array<float, kMaxPeriod> delayLine {};
    int period = 100;
    int writePos = 0;
    float prev = 0.0f;
    float body = 0.0f;
    float env = 0.0f;
    float envInc = 0.0f;
    int samplesUntilNext = 0;
    int noteIndex = 0;
    float pan = 0.0f;

    juce::Random rng { 0xDE30701Eu };
};
