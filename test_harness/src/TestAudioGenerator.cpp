/*
    MultiScoper Test Harness - Audio Generator Implementation
*/

#include "TestAudioGenerator.h"

#include <cmath>

namespace multiscoper::test
{

TestAudioGenerator::TestAudioGenerator() : rng_(std::random_device{}()) {}

void TestAudioGenerator::prepare(double sampleRate)
{
    // Guard against invalid sample rate
    const double sr = sampleRate > 0.0 ? sampleRate : 48000.0;
    sampleRate_.store(sr, std::memory_order_relaxed);
    phaseIncrement_.store(frequency_.load() / sr, std::memory_order_relaxed);
    reset();
}

void TestAudioGenerator::generateBlock(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    const float amp = amplitude_.load();
    const Waveform wf = waveform_.load();
    const int burst = burstSamples_.load();

    // Update phase increment in case frequency changed (sampleRate_ validated in prepare()).
    // Snapshot the atomic into a local so the per-sample loop below doesn't reload.
    const double sr = sampleRate_.load(std::memory_order_relaxed);
    double phaseInc = phaseIncrement_.load(std::memory_order_relaxed);
    if (sr > 0.0)
    {
        phaseInc = frequency_.load() / sr;
        phaseIncrement_.store(phaseInc, std::memory_order_relaxed);
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float value = 0.0f;

        // Check burst mode
        if (burst > 0)
        {
            if (burstCounter_ >= burst)
            {
                generating_.store(false);
                value = 0.0f;
            }
            else
            {
                generating_.store(true);
                value = generateSample() * amp;
                burstCounter_++;
            }
        }
        else
        {
            generating_.store(true);
            value = generateSample() * amp;
        }

        // Write to all channels
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.setSample(ch, sample, value);
        }

        // Advance phase (for tonal waveforms) using the snapshotted
        // phaseInc — avoids per-sample atomic loads.
        if (wf != Waveform::Noise && wf != Waveform::Silence)
        {
            phase_ += phaseInc;
            if (phase_ >= 1.0)
                phase_ -= 1.0;
        }
    }
}

float TestAudioGenerator::generateSample()
{
    const Waveform wf = waveform_.load();

    switch (wf)
    {
        case Waveform::Sine:
            return static_cast<float>(std::sin(phase_ * 2.0 * juce::MathConstants<double>::pi));

        case Waveform::Square:
            return phase_ < 0.5 ? 1.0f : -1.0f;

        case Waveform::Saw:
            return static_cast<float>(2.0 * phase_ - 1.0);

        case Waveform::Triangle:
        {
            if (phase_ < 0.25)
                return static_cast<float>(4.0 * phase_);
            else if (phase_ < 0.75)
                return static_cast<float>(2.0 - 4.0 * phase_);
            else
                return static_cast<float>(4.0 * phase_ - 4.0);
        }

        case Waveform::Noise:
            return noiseDist_(rng_);

        case Waveform::Silence:
        default:
            return 0.0f;
    }
}

void TestAudioGenerator::setWaveform(Waveform type) { waveform_.store(type); }

void TestAudioGenerator::setFrequency(float hz)
{
    // Allow LFO rates (sub-audio frequencies) for test visualization
    frequency_.store(juce::jlimit(0.01f, 20000.0f, hz));
    // Guard against division by zero if prepare() wasn't called
    const double sr = sampleRate_.load(std::memory_order_relaxed);
    if (sr > 0.0)
        phaseIncrement_.store(frequency_.load() / sr, std::memory_order_relaxed);
}

void TestAudioGenerator::setAmplitude(float gain) { amplitude_.store(juce::jlimit(0.0f, 1.0f, gain)); }

void TestAudioGenerator::setBurstSamples(int samples)
{
    burstSamples_.store(std::max(0, samples));
    if (samples > 0)
    {
        burstCounter_ = 0;
        generating_.store(true);
    }
}

void TestAudioGenerator::reset()
{
    phase_ = 0.0;
    burstCounter_ = 0;
    generating_.store(true);
}

juce::String TestAudioGenerator::waveformToString(Waveform wf)
{
    switch (wf)
    {
        case Waveform::Sine:
            return "sine";
        case Waveform::Square:
            return "square";
        case Waveform::Saw:
            return "saw";
        case Waveform::Triangle:
            return "triangle";
        case Waveform::Noise:
            return "noise";
        case Waveform::Silence:
            return "silence";
        default:
            return "unknown";
    }
}

Waveform TestAudioGenerator::stringToWaveform(const juce::String& str)
{
    auto lower = str.toLowerCase();
    if (lower == "sine")
        return Waveform::Sine;
    if (lower == "square")
        return Waveform::Square;
    if (lower == "saw")
        return Waveform::Saw;
    if (lower == "triangle")
        return Waveform::Triangle;
    if (lower == "noise")
        return Waveform::Noise;
    if (lower == "silence")
        return Waveform::Silence;
    return Waveform::Silence;
}

} // namespace multiscoper::test
