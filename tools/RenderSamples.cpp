#include <JuceHeader.h>
#include "../Source/dsp/ChorusEngine.h"
#include "../Source/dsp/DelayEngine.h"
#include "../Source/dsp/ShimmerEngine.h"
#include <cmath>
#include <iostream>

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kChannels = 2;
constexpr float kDurationSec = 6.0f;
constexpr int kBlockSize = 512;

struct Settings
{
    bool chorusOn = false;
    bool delayOn = false;
    bool shimmerOn = false;
    float rateHz = 0.70f;
    float depth = 0.35f;
    float delayMs = 380.0f;
    float feedback = 0.32f;
    float shimmerAmount = 0.05f;
    float mix = 0.45f;
};

juce::AudioBuffer<float> makeGuitarLikeDry (int numSamples)
{
    // Synthetic clean plucked arpeggio (not a real guitar recording).
    // Karplus-ish plucks on a Cmaj7-ish pattern around tempo ~95 BPM.
    juce::AudioBuffer<float> dry (kChannels, numSamples);
    dry.clear();

    const float bpm = 95.0f;
    const float beatSec = 60.0f / bpm;
    const float freqs[] = {
        130.81f, // C3
        164.81f, // E3
        196.00f, // G3
        246.94f, // B3
        261.63f, // C4
        329.63f, // E4
        392.00f, // G4
        493.88f  // B4
    };
    const int pattern[] = { 0, 2, 4, 5, 4, 2, 3, 1, 0, 4, 6, 7, 5, 4, 2, 0 };
    const int patternLen = (int) (sizeof (pattern) / sizeof (pattern[0]));

    // Soft noise seed buffers for each note attack
    juce::Random rng (0xE45C0F15u);

    for (int noteIndex = 0; noteIndex < 24; ++noteIndex)
    {
        const double startSec = 0.15 + (double) noteIndex * (double) beatSec * 0.5;
        const int start = (int) std::round (startSec * kSampleRate);
        if (start >= numSamples)
            break;

        const float freq = freqs[pattern[noteIndex % patternLen]];
        const int period = juce::jmax (2, (int) std::round (kSampleRate / (double) freq));
        std::vector<float> delay ((size_t) period);
        for (float& s : delay)
            s = (rng.nextFloat() * 2.0f - 1.0f) * 0.9f;

        int write = 0;
        float prev = 0.0f;
        float body = 0.0f;
        const int noteLen = juce::jmin (numSamples - start, (int) (1.35 * kSampleRate));

        for (int i = 0; i < noteLen; ++i)
        {
            const float filtered = 0.5f * (delay[(size_t) write] + prev);
            prev = delay[(size_t) write];
            delay[(size_t) write] = filtered * 0.996f;
            write = (write + 1) % period;

            // Soft pick envelope
            const float env = std::exp (-3.2f * (float) i / (float) noteLen);
            float sample = filtered * env * 0.55f;

            // Mild body resonance
            body = body * 0.95f + sample * 0.05f;
            sample = sample * 0.85f + body * 0.35f;

            // Slight stereo width
            const float pan = ((noteIndex & 1) != 0) ? 0.15f : -0.15f;
            dry.addSample (0, start + i, sample * (0.7f - pan));
            dry.addSample (1, start + i, sample * (0.7f + pan));
        }
    }

    // Soft limiter-ish peak normalize
    float peak = 0.0f;
    for (int ch = 0; ch < kChannels; ++ch)
        peak = juce::jmax (peak, dry.getMagnitude (ch, 0, numSamples));
    if (peak > 1.0e-6f)
        dry.applyGain (0.85f / peak);

    return dry;
}

void processOffline (juce::AudioBuffer<float>& buffer, const Settings& s)
{
    ChorusEngine chorus;
    DelayEngine delay;
    ShimmerEngine shimmer;

    chorus.prepare (kSampleRate, kBlockSize, kChannels);
    delay.prepare (kSampleRate, kBlockSize, kChannels);
    shimmer.prepare (kSampleRate, kBlockSize, kChannels);

    chorus.setRateHz (s.rateHz);
    chorus.setDepth (s.depth);
    delay.setDelayMs (s.delayMs);
    delay.setFeedback (s.feedback);
    shimmer.setAmount (s.shimmerAmount);

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
    mixSmoothed.reset (kSampleRate, 0.03);
    mixSmoothed.setCurrentAndTargetValue (s.mix);

    juce::AudioBuffer<float> dry (kChannels, kBlockSize);

    for (int pos = 0; pos < buffer.getNumSamples();)
    {
        const int n = juce::jmin (kBlockSize, buffer.getNumSamples() - pos);
        juce::AudioBuffer<float> block (buffer.getArrayOfWritePointers(), kChannels, pos, n);

        dry.setSize (kChannels, n, false, false, true);
        for (int ch = 0; ch < kChannels; ++ch)
            dry.copyFrom (ch, 0, block, ch, 0, n);

        if (s.chorusOn)
            chorus.process (block);
        if (s.delayOn)
            delay.process (block);
        if (s.shimmerOn)
            shimmer.process (block);

        for (int i = 0; i < n; ++i)
        {
            const float mix = mixSmoothed.getNextValue();
            for (int ch = 0; ch < kChannels; ++ch)
            {
                const float d = dry.getSample (ch, i);
                const float w = block.getSample (ch, i);
                block.setSample (ch, i, d * (1.0f - mix) + w * mix);
            }
        }

        pos += n;
    }
}

bool writeWav (const juce::File& file, const juce::AudioBuffer<float>& buffer)
{
    file.deleteFile();
    file.getParentDirectory().createDirectory();

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());
    if (stream == nullptr)
        return false;

    std::unique_ptr<juce::AudioFormatWriter> writer (
        wav.createWriterFor (stream.get(), kSampleRate, (unsigned int) kChannels, 24, {}, 0));
    if (writer == nullptr)
        return false;

    stream.release(); // writer owns it
    return writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
}
} // namespace

int main (int argc, char* argv[])
{
    const juce::File outDir (argc > 1 ? argv[1]
                                      : "/cursor/stores/self/media");
    outDir.createDirectory();

    const int numSamples = (int) std::round (kDurationSec * kSampleRate);
    const auto drySource = makeGuitarLikeDry (numSamples);

    struct Job
    {
        const char* name;
        Settings settings;
    };

    const Job jobs[] = {
        { "sample-dry.wav",
          { false, false, false, 0.70f, 0.35f, 380.0f, 0.32f, 0.05f, 0.0f } },
        { "sample-chorus.wav",
          { true, false, false, 0.70f, 0.35f, 380.0f, 0.32f, 0.05f, 0.50f } },
        { "sample-delay.wav",
          { false, true, false, 0.70f, 0.35f, 380.0f, 0.32f, 0.05f, 0.45f } },
        { "sample-shimmer.wav",
          { false, false, true, 0.70f, 0.35f, 380.0f, 0.32f, 0.65f, 0.55f } },
        { "sample-bfmv-clean.wav",
          { true, true, false, 0.70f, 0.35f, 380.0f, 0.32f, 0.05f, 0.45f } },
        { "sample-full.wav",
          { true, true, true, 0.65f, 0.40f, 380.0f, 0.30f, 0.45f, 0.50f } },
    };

    int failed = 0;
    for (const auto& job : jobs)
    {
        juce::AudioBuffer<float> buf;
        buf.makeCopyOf (drySource);
        processOffline (buf, job.settings);

        const juce::File out = outDir.getChildFile (job.name);
        if (! writeWav (out, buf) || out.getSize() < 1000)
        {
            std::cerr << "FAIL writing " << out.getFullPathName() << "\n";
            ++failed;
            continue;
        }

        std::cout << "OK " << out.getFullPathName()
                  << " (" << out.getSize() << " bytes)\n";
    }

    return failed == 0 ? 0 : 1;
}
