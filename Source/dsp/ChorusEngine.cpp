#include "ChorusEngine.h"
#include <cmath>

namespace
{
constexpr float kTwoPi = juce::MathConstants<float>::twoPi;
} // namespace

void ChorusEngine::prepare (double newSampleRate, int /*maximumBlockSize*/, int newNumChannels)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    numChannels = juce::jlimit (1, 2, newNumChannels);

    const float maxDelaySeconds = (kMaxDelayMs * 1.5f) / 1000.0f;
    delayBufferSize = juce::jmax (64, (int) std::ceil (maxDelaySeconds * (float) sampleRate) + 8);

    delayBuffer.assign ((size_t) numChannels, std::vector<float> ((size_t) delayBufferSize, 0.0f));
    writeIndex = 0;

    const float smoothSec = 0.03f;
    rateSmoothed.reset (sampleRate, smoothSec);
    depthSmoothed.reset (sampleRate, smoothSec);
    rateSmoothed.setCurrentAndTargetValue (0.70f);
    depthSmoothed.setCurrentAndTargetValue (0.35f);

    // Gentle stereo image — width without collapsing on mono sum
    voices[0] = { 0.0f, 0.00f, 1.00f, 0.78f, 0.48f };
    voices[1] = { 0.0f, 0.42f, 1.04f, 0.48f, 0.78f };

    hpfCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * kHpfHz / (float) sampleRate);
    hpfState = { 0.0f, 0.0f };

    reset();
}

void ChorusEngine::reset()
{
    for (auto& channel : delayBuffer)
        std::fill (channel.begin(), channel.end(), 0.0f);

    writeIndex = 0;
    hpfState = { 0.0f, 0.0f };

    for (auto& v : voices)
        v.phase = v.phaseOffset;
}

void ChorusEngine::setRateHz (float rateHz)
{
    // Musical CE range; upper end still available but skewed in the parameter map
    rateSmoothed.setTargetValue (juce::jlimit (0.1f, 4.0f, rateHz));
}

void ChorusEngine::setDepth (float depth01)
{
    depthSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, depth01));
}

float ChorusEngine::hermiteRead (int channel, float delaySamples) const
{
    float readPos = (float) writeIndex - delaySamples;
    while (readPos < 0.0f)
        readPos += (float) delayBufferSize;

    const int i1 = (int) readPos;
    const float frac = readPos - (float) i1;

    const auto& buf = delayBuffer[(size_t) channel];
    const int n = delayBufferSize;

    const int i0 = (i1 - 1 + n) % n;
    const int i2 = (i1 + 1) % n;
    const int i3 = (i1 + 2) % n;

    const float y0 = buf[(size_t) i0];
    const float y1 = buf[(size_t) i1];
    const float y2 = buf[(size_t) i2];
    const float y3 = buf[(size_t) i3];

    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

void ChorusEngine::writeSample (int channel, float sample)
{
    delayBuffer[(size_t) channel][(size_t) writeIndex] = sample;
}

void ChorusEngine::advanceWriteIndex()
{
    writeIndex = (writeIndex + 1) % delayBufferSize;
}

void ChorusEngine::process (juce::AudioBuffer<float>& buffer)
{
    if (delayBufferSize <= 0 || delayBuffer.empty())
        return;

    const int numSamples = buffer.getNumSamples();
    const int channels = juce::jmin (buffer.getNumChannels(), numChannels);
    const float voiceGain = 1.0f / std::sqrt ((float) kMaxVoices);

    for (int i = 0; i < numSamples; ++i)
    {
        const float rate = rateSmoothed.getNextValue();
        const float depth = depthSmoothed.getNextValue();

        const float baseDelay = kBaseDelayMs * 0.001f * (float) sampleRate;
        const float modSpan = kMaxModMs * 0.001f * (float) sampleRate * depth;

        float inL = buffer.getSample (0, i);
        float inR = channels > 1 ? buffer.getSample (1, i) : inL;

        writeSample (0, inL);
        if (channels > 1)
            writeSample (1, inR);
        else if (numChannels > 1)
            writeSample (1, inL);

        float wetL = 0.0f;
        float wetR = 0.0f;

        for (int v = 0; v < kMaxVoices; ++v)
        {
            auto& voice = voices[(size_t) v];
            const float lfo = std::sin (kTwoPi * (voice.phase + voice.phaseOffset));
            const float delaySamples = juce::jlimit (
                kMinDelayMs * 0.001f * (float) sampleRate,
                kMaxDelayMs * 0.001f * (float) sampleRate,
                baseDelay + lfo * modSpan);

            const float tapL = hermiteRead (0, delaySamples);
            const float tapR = hermiteRead (channels > 1 ? 1 : 0, delaySamples);

            wetL += tapL * voice.panL * voiceGain;
            wetR += tapR * voice.panR * voiceGain;

            voice.phase += (rate * voice.rateScale) / (float) sampleRate;
            if (voice.phase >= 1.0f)
                voice.phase -= 1.0f;
        }

        hpfState[0] += hpfCoeff * (wetL - hpfState[0]);
        wetL -= hpfState[0];
        hpfState[1] += hpfCoeff * (wetR - hpfState[1]);
        wetR -= hpfState[1];

        // CE-style: mostly dry through + moderate wet for thickening, not vibrato
        buffer.setSample (0, i, inL * 0.82f + wetL * 0.55f);
        if (channels > 1)
            buffer.setSample (1, i, inR * 0.82f + wetR * 0.55f);

        advanceWriteIndex();
    }
}
