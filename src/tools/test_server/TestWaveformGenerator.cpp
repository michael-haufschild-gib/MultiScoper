/*
    Oscil - Test Waveform Generator Implementation
*/

#include "tools/test_server/TestWaveformGenerator.h"

#include <cmath>

namespace oscil
{

namespace
{
float generateSample(const std::string& waveformType, float phase, float amplitude, juce::Random& rng)
{
    if (waveformType == "sine")
        return std::sin(phase) * amplitude;
    if (waveformType == "square")
        return (std::sin(phase) > 0.0f ? 1.0f : -1.0f) * amplitude;
    if (waveformType == "triangle")
    {
        float const t = phase / (2.0f * juce::MathConstants<float>::pi);
        return ((2.0f * std::abs(2.0f * (t - std::floor(t + 0.5f)))) - 1.0f) * amplitude;
    }
    if (waveformType == "sawtooth")
    {
        float const t = phase / (2.0f * juce::MathConstants<float>::pi);
        return (2.0f * (t - std::floor(t + 0.5f))) * amplitude;
    }
    if (waveformType == "noise")
        return (rng.nextFloat() * 2.0f - 1.0f) * amplitude;
    if (waveformType == "dc")
        return amplitude;
    return 0.0f; // "silence" or unknown
}
} // namespace

void generateTestWaveform(juce::AudioBuffer<float>& buffer, const std::string& waveformType, float frequency,
                          float amplitude, float sampleRate)
{
    jassert(buffer.getNumChannels() >= 2);
    if (buffer.getNumChannels() < 2)
    {
        buffer.clear();
        return;
    }

    int const numSamples = buffer.getNumSamples();
    float phase = 0.0f;
    float const safeSampleRate = sampleRate > 0.0f ? sampleRate : 44100.0f;
    float const phaseIncrement = (2.0f * juce::MathConstants<float>::pi * frequency) / safeSampleRate;

    auto& rng = juce::Random::getSystemRandom();

    for (int i = 0; i < numSamples; ++i)
    {
        float const sample = generateSample(waveformType, phase, amplitude, rng);
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample * 0.8f);

        phase += phaseIncrement;
        phase = std::fmod(phase, 2.0f * juce::MathConstants<float>::pi);
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
