#include "ShimmerEngine.h"
#include <cmath>

namespace
{
float hann (float phase01) noexcept
{
    return 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * phase01));
}

int msToSamples (double sampleRate, float ms) noexcept
{
    return juce::jmax (2, (int) std::round (sampleRate * (double) ms * 0.001));
}

// Prime-ish diffuser / FDN lengths (ms) for dense, non-metallic bloom
constexpr float kDiffMs[4] = { 7.1f, 11.3f, 17.9f, 23.7f };
constexpr float kFdnMs[8]  = { 29.3f, 37.1f, 47.9f, 58.3f, 71.9f, 83.7f, 97.1f, 113.3f };
} // namespace

void ShimmerEngine::prepare (double newSampleRate, int /*maximumBlockSize*/, int newNumChannels)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    numChannels = juce::jlimit (1, 2, newNumChannels);

    for (int i = 0; i < kNumDiffusers; ++i)
    {
        auto setup = [&] (Diffuser& d)
        {
            d.size = msToSamples (sampleRate, kDiffMs[i]);
            d.buffer.assign ((size_t) d.size, 0.0f);
            d.index = 0;
            d.z = 0.0f;
        };
        setup (diffL[(size_t) i]);
        setup (diffR[(size_t) i]);
        // Slight L/R length skew for width without phase collapse
        diffR[(size_t) i].size = msToSamples (sampleRate, kDiffMs[i] * 1.07f);
        diffR[(size_t) i].buffer.assign ((size_t) diffR[(size_t) i].size, 0.0f);
    }

    for (int i = 0; i < kFdnSize; ++i)
    {
        fdn[(size_t) i].size = msToSamples (sampleRate, kFdnMs[i]);
        fdn[(size_t) i].buffer.assign ((size_t) fdn[(size_t) i].size, 0.0f);
        fdn[(size_t) i].index = 0;
        fdn[(size_t) i].filterState = 0.0f;
    }

    grainLengthSamples = juce::jmax (64.0f, kGrainMs * 0.001f * (float) sampleRate);
    pitchDelaySize = juce::jmax (256, (int) std::ceil (grainLengthSamples * kPitchRatio) + 64);
    pitchDelay.assign ((size_t) pitchDelaySize, 0.0f);
    pitchWrite = 0;

    for (int g = 0; g < kNumGrains; ++g)
        grains[(size_t) g].phase = (float) g / (float) kNumGrains;

    preDelaySize = msToSamples (sampleRate, 35.0f);
    preDelayL.assign ((size_t) preDelaySize, 0.0f);
    preDelayR.assign ((size_t) preDelaySize, 0.0f);
    preDelayIndex = 0;

    dampCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * kDampingHz / (float) sampleRate);
    inputHpCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * kInputHpfHz / (float) sampleRate);
    washHpCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * kWashHpfHz / (float) sampleRate);

    amountSmoothed.reset (sampleRate, 0.05f);
    amountSmoothed.setCurrentAndTargetValue (0.05f);

    reset();
}

void ShimmerEngine::reset()
{
    for (auto& d : diffL)
    {
        std::fill (d.buffer.begin(), d.buffer.end(), 0.0f);
        d.index = 0;
        d.z = 0.0f;
    }
    for (auto& d : diffR)
    {
        std::fill (d.buffer.begin(), d.buffer.end(), 0.0f);
        d.index = 0;
        d.z = 0.0f;
    }
    for (auto& line : fdn)
    {
        std::fill (line.buffer.begin(), line.buffer.end(), 0.0f);
        line.index = 0;
        line.filterState = 0.0f;
    }

    std::fill (pitchDelay.begin(), pitchDelay.end(), 0.0f);
    pitchWrite = 0;
    for (int g = 0; g < kNumGrains; ++g)
        grains[(size_t) g].phase = (float) g / (float) kNumGrains;

    std::fill (preDelayL.begin(), preDelayL.end(), 0.0f);
    std::fill (preDelayR.begin(), preDelayR.end(), 0.0f);
    preDelayIndex = 0;

    inputHpState = { 0.0f, 0.0f };
    washHpState = { 0.0f, 0.0f };
    pitchLpState = 0.0f;
}

void ShimmerEngine::setAmount (float amount01)
{
    amountSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, amount01));
}

float ShimmerEngine::processDiffuser (Diffuser& d, float input) noexcept
{
    // Schroeder-style allpass diffuser
    constexpr float g = 0.6f;
    const float bufOut = d.buffer[(size_t) d.index];
    const float y = -g * input + bufOut;
    d.buffer[(size_t) d.index] = input + g * bufOut;
    d.index = (d.index + 1) % d.size;

    // Tiny one-pole to soften diffuser edges
    d.z += 0.35f * (y - d.z);
    return d.z;
}

float ShimmerEngine::grainRead (float readPos) const noexcept
{
    while (readPos < 0.0f)
        readPos += (float) pitchDelaySize;
    while (readPos >= (float) pitchDelaySize)
        readPos -= (float) pitchDelaySize;

    const int i0 = (int) readPos;
    const float frac = readPos - (float) i0;
    const int i1 = (i0 + 1) % pitchDelaySize;

    // Hermite-ish 2-point with mild cubic flavor via neighboring taps
    const int im1 = (i0 - 1 + pitchDelaySize) % pitchDelaySize;
    const int i2  = (i0 + 2) % pitchDelaySize;
    const float y0 = pitchDelay[(size_t) im1];
    const float y1 = pitchDelay[(size_t) i0];
    const float y2 = pitchDelay[(size_t) i1];
    const float y3 = pitchDelay[(size_t) i2];
    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

float ShimmerEngine::pitchShift (float input) noexcept
{
    pitchDelay[(size_t) pitchWrite] = input;

    float shifted = 0.0f;
    float windowSum = 0.0f;

    for (auto& grain : grains)
    {
        const float window = hann (grain.phase);
        const float age = grain.phase * grainLengthSamples;
        // Read faster than write → +1 octave; long Hann grains keep it smooth
        const float readPos = (float) pitchWrite - age * kPitchRatio;
        shifted += grainRead (readPos) * window;
        windowSum += window;

        grain.phase += 1.0f / grainLengthSamples;
        if (grain.phase >= 1.0f)
            grain.phase -= 1.0f;
    }

    pitchWrite = (pitchWrite + 1) % pitchDelaySize;

    if (windowSum > 1.0e-4f)
        shifted /= windowSum;

    return shifted;
}

void ShimmerEngine::processFdn (float inL, float inR, float& outL, float& outR) noexcept
{
    // Read FDN taps
    std::array<float, kFdnSize> taps {};
    for (int i = 0; i < kFdnSize; ++i)
    {
        auto& line = fdn[(size_t) i];
        taps[(size_t) i] = line.buffer[(size_t) line.index];
    }

    // Householder-ish mixing: sum + local reflection → dense, non-metallic cloud
    float sum = 0.0f;
    for (float t : taps)
        sum += t;
    const float scale = 2.0f / (float) kFdnSize;

    std::array<float, kFdnSize> mixed {};
    for (int i = 0; i < kFdnSize; ++i)
        mixed[(size_t) i] = taps[(size_t) i] - scale * sum;

    // Pitch-shifted feedback into the cloud (mono sum → octave wash)
    const float monoTap = 0.5f * (taps[0] + taps[4]);
    float pitched = pitchShift (monoTap);

    // Soften pitch artifacts before reinjection
    pitchLpState += 0.18f * (pitched - pitchLpState);
    pitched = pitchLpState;

    const float injectL = inL + pitched * kPitchFeedback;
    const float injectR = inR + pitched * kPitchFeedback;

    // Write back with damping + decay
    for (int i = 0; i < kFdnSize; ++i)
    {
        auto& line = fdn[(size_t) i];
        const float input = (i & 1) ? injectR : injectL;
        float sample = mixed[(size_t) i] * kDecay + input * (0.15f + 0.02f * (float) i);

        line.filterState += dampCoeff * (sample - line.filterState);
        sample = line.filterState;

        line.buffer[(size_t) line.index] = sample;
        line.index = (line.index + 1) % line.size;
    }

    // Stereo pickup from interleaved lines
    outL = 0.0f;
    outR = 0.0f;
    for (int i = 0; i < kFdnSize; i += 2)
    {
        outL += taps[(size_t) i];
        outR += taps[(size_t) i + 1];
    }
    outL *= 0.35f;
    outR *= 0.35f;
}

void ShimmerEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int channels = juce::jmin (buffer.getNumChannels(), numChannels);

    for (int i = 0; i < numSamples; ++i)
    {
        const float amount = amountSmoothed.getNextValue();

        float inL = buffer.getSample (0, i);
        float inR = channels > 1 ? buffer.getSample (1, i) : inL;

        // Input HPF — wash stays behind the guitar, low end solid
        inputHpState[0] += inputHpCoeff * (inL - inputHpState[0]);
        inputHpState[1] += inputHpCoeff * (inR - inputHpState[1]);
        const float hpL = inL - inputHpState[0];
        const float hpR = inR - inputHpState[1];

        // Soft pre-delay into the bloom
        const float pdL = preDelayL[(size_t) preDelayIndex];
        const float pdR = preDelayR[(size_t) preDelayIndex];
        preDelayL[(size_t) preDelayIndex] = hpL;
        preDelayR[(size_t) preDelayIndex] = hpR;
        preDelayIndex = (preDelayIndex + 1) % preDelaySize;

        float dL = pdL;
        float dR = pdR;
        for (int d = 0; d < kNumDiffusers; ++d)
        {
            dL = processDiffuser (diffL[(size_t) d], dL);
            dR = processDiffuser (diffR[(size_t) d], dR);
        }

        float revL = 0.0f, revR = 0.0f;
        processFdn (dL, dR, revL, revR);

        washHpState[0] += washHpCoeff * (revL - washHpState[0]);
        washHpState[1] += washHpCoeff * (revR - washHpState[1]);
        revL -= washHpState[0];
        revR -= washHpState[1];

        // Shimmer amount = wet contribution of the pitch-shifted reverb cloud
        buffer.setSample (0, i, inL + amount * revL);
        if (channels > 1)
            buffer.setSample (1, i, inR + amount * revR);
    }
}
