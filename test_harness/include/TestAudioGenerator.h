/*
    Oscil Test Harness - Audio Generator
    Generates test waveforms for simulating DAW audio
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>
#include <random>

namespace oscil::test
{

enum class Waveform
{
    Sine,
    Square,
    Saw,
    Triangle,
    Noise,
    Silence
};

/**
 * Generates test audio waveforms for E2E testing.
 * Thread-safe for real-time audio callback use.
 */
class TestAudioGenerator
{
public:
    TestAudioGenerator();
    ~TestAudioGenerator() = default;

    /**
     * Prepare the generator for a given sample rate
     */
    void prepare(double sampleRate);

    /**
     * Generate audio into the provided buffer
     */
    void generateBlock(juce::AudioBuffer<float>& buffer);

    /**
     * Set the waveform type
     */
    void setWaveform(Waveform type);
    Waveform getWaveform() const { return waveform_.load(); }

    /**
     * Set frequency in Hz (for tonal waveforms)
     */
    void setFrequency(float hz);
    float getFrequency() const { return frequency_.load(); }

    /**
     * Set amplitude (0.0 to 1.0)
     */
    void setAmplitude(float gain);
    float getAmplitude() const { return amplitude_.load(); }

    /**
     * Enable burst mode - generate for N samples then silence
     * Set to 0 to disable burst mode (continuous generation)
     */
    void setBurstSamples(int samples);

    /**
     * Reset phase and burst counter
     */
    void reset();

    /**
     * Check if currently generating (not in post-burst silence)
     */
    bool isGenerating() const { return generating_.load(); }

    /**
     * Convert waveform enum to string
     */
    static juce::String waveformToString(Waveform wf);

    /**
     * Convert string to waveform enum
     */
    static Waveform stringToWaveform(const juce::String& str);

private:
    float generateSample();

    std::atomic<Waveform> waveform_{Waveform::Sine};
    std::atomic<float> frequency_{440.0f};
    std::atomic<float> amplitude_{0.8f};
    std::atomic<int> burstSamples_{0};
    std::atomic<bool> generating_{true};

    double sampleRate_ = 44100.0;
    double phase_ = 0.0;
    double phaseIncrement_ = 0.0;
    int burstCounter_ = 0;

    // For noise generation
    std::mt19937 rng_;
    std::uniform_real_distribution<float> noiseDist_{-1.0f, 1.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestAudioGenerator)
};

} // namespace oscil::test
