/*
    Oscil - Test Waveform Generator Implementation
*/

#include "tools/test_server/TestWaveformGenerator.h"

#include <cmath>

namespace oscil
{

void generateTestWaveform(juce::AudioBuffer<float>& buffer, const std::string& waveformType, float frequency,
                          float amplitude, float sampleRate)
{
    int const numSamples = buffer.getNumSamples();
    float phase = 0.0f;
    float const safeSampleRate = sampleRate > 0.0f ? sampleRate : 44100.0f;
    float const phaseIncrement = (2.0f * juce::MathConstants<float>::pi * frequency) / safeSampleRate;

    auto& rng = juce::Random::getSystemRandom();

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;

        if (waveformType == "sine")
        {
            sample = std::sin(phase) * amplitude;
        }
        else if (waveformType == "square")
        {
            sample = (std::sin(phase) > 0.0f ? 1.0f : -1.0f) * amplitude;
        }
        else if (waveformType == "triangle")
        {
            float const t = phase / (2.0f * juce::MathConstants<float>::pi);
            sample = ((2.0f * std::abs(2.0f * (t - std::floor(t + 0.5f)))) - 1.0f) * amplitude;
        }
        else if (waveformType == "sawtooth")
        {
            float const t = phase / (2.0f * juce::MathConstants<float>::pi);
            sample = (2.0f * (t - std::floor(t + 0.5f))) * amplitude;
        }
        else if (waveformType == "noise")
        {
            sample = (rng.nextFloat() * 2.0f - 1.0f) * amplitude;
        }
        else if (waveformType == "dc")
        {
            sample = amplitude;
        }
        // "silence" or unknown → sample remains 0.0f

        buffer.setSample(0, i, sample);
        if (buffer.getNumChannels() > 1)
            buffer.setSample(1, i, sample * 0.8f);

        phase += phaseIncrement;
        if (phase > 2.0f * juce::MathConstants<float>::pi)
            phase -= 2.0f * juce::MathConstants<float>::pi;
    }
}

CaptureFrameMetadata makeTestMetadata(float sampleRate, int numSamples)
{
    CaptureFrameMetadata metadata;
    metadata.sampleRate = sampleRate > 0.0f ? sampleRate : 44100.0f;
    metadata.numChannels = 2;
    metadata.numSamples = numSamples;
    metadata.isPlaying = true;
    metadata.bpm = 120.0f;
    metadata.timestamp = 0;
    return metadata;
}

} // namespace oscil
