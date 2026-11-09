#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

/**
 * Valhalla Shimmer–inspired pitch-shifted reverb wash (original DSP).
 *
 * Dense FDN diffusion with feedback through a +12 semitone pitch shifter,
 * damping in the loop, soft bloom / long ethereal tail. The Shimmer amount
 * blends this wash behind the dry-through wet-chain signal.
 */
class ShimmerEngine
{
public:
    ShimmerEngine() = default;

    void prepare (double sampleRate, int maximumBlockSize, int numChannels);
    void reset();

    void setAmount (float amount01);

    void process (juce::AudioBuffer<float>& buffer);

private:
    static constexpr int kNumDiffusers = 4;
    static constexpr int kFdnSize = 8;
    static constexpr int kNumGrains = 3;
    static constexpr float kPitchRatio = 2.0f; // +12 semitones
    static constexpr float kGrainMs = 90.0f;   // long grains → less robotic

    // Fixed v1 “size/color” defaults (Valhalla-like bloom)
    static constexpr float kDecay = 0.88f;         // long tail
    static constexpr float kPitchFeedback = 0.55f; // how hard the octave feeds the cloud
    static constexpr float kDampingHz = 4200.0f;   // HF damping in FDN
    static constexpr float kInputHpfHz = 180.0f;   // keep guitar low end clean
    static constexpr float kWashHpfHz = 250.0f;    // airy wash, less mud

    struct Diffuser
    {
        std::vector<float> buffer;
        int index = 0;
        int size = 1;
        float z = 0.0f; // one-pole for mild smoothing
    };

    struct FdnLine
    {
        std::vector<float> buffer;
        int index = 0;
        int size = 1;
        float filterState = 0.0f;
    };

    struct Grain
    {
        float phase = 0.0f;
    };

    float processDiffuser (Diffuser& d, float input) noexcept;
    void  processFdn (float inL, float inR, float& outL, float& outR) noexcept;
    float pitchShift (float input) noexcept;
    float grainRead (float readPos) const noexcept;

    double sampleRate = 44100.0;
    int numChannels = 2;

    std::array<Diffuser, kNumDiffusers> diffL {};
    std::array<Diffuser, kNumDiffusers> diffR {};
    std::array<FdnLine, kFdnSize> fdn {};

    // Mono pitch-shift delay used in the feedback path
    std::vector<float> pitchDelay;
    int pitchDelaySize = 0;
    int pitchWrite = 0;
    float grainLengthSamples = 1.0f;
    std::array<Grain, kNumGrains> grains {};

    float dampCoeff = 0.0f;
    float inputHpCoeff = 0.0f;
    float washHpCoeff = 0.0f;
    float pitchLpState = 0.0f;
    std::array<float, 2> inputHpState { 0.0f, 0.0f };
    std::array<float, 2> washHpState { 0.0f, 0.0f };

    // Soft pre-delay into the cloud (~35 ms) for bloom onset
    std::vector<float> preDelayL;
    std::vector<float> preDelayR;
    int preDelaySize = 0;
    int preDelayIndex = 0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> amountSmoothed;
};
