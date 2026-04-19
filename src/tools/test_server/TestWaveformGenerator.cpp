/*
    MultiScoper - Test Waveform Generator Implementation
*/

#include "tools/test_server/TestWaveformGenerator.h"

#include <cmath>

namespace multiscoper
{

namespace
{
using SampleFunc = float (*)(float phase, float amplitude, juce::Random& rng);

float sineSample(float phase, float amplitude, juce::Random& /*rng*/) { return std::sin(phase) * amplitude; }
float squareSample(float phase, float amplitude, juce::Random& /*rng*/)
{
    // Phase is kept in [0, 2π) by the caller, so the sign of sin(phase)
    // is simply "below or above π". Computing sin just to check the
    // sign is a trig call per sample for no reason.
    return (phase < juce::MathConstants<float>::pi ? 1.0f : -1.0f) * amplitude;
}
float triangleSample(float phase, float amplitude, juce::Random& /*rng*/)
{
    float const t = phase / (2.0f * juce::MathConstants<float>::pi);
    return ((2.0f * std::abs(2.0f * (t - std::floor(t + 0.5f)))) - 1.0f) * amplitude;
}
float sawtoothSample(float phase, float amplitude, juce::Random& /*rng*/)
{
    float const t = phase / (2.0f * juce::MathConstants<float>::pi);
    return (2.0f * (t - std::floor(t + 0.5f))) * amplitude;
}
float noiseSample(float /*phase*/, float amplitude, juce::Random& rng)
{
    return (rng.nextFloat() * 2.0f - 1.0f) * amplitude;
}
float dcSample(float /*phase*/, float amplitude, juce::Random& /*rng*/) { return amplitude; }
float silenceSample(float /*phase*/, float /*amplitude*/, juce::Random& /*rng*/) { return 0.0f; }

SampleFunc resolveWaveform(const std::string& type)
{
    if (type == "sine")
        return sineSample;
    if (type == "square")
        return squareSample;
    if (type == "triangle")
        return triangleSample;
    if (type == "sawtooth")
        return sawtoothSample;
    if (type == "noise")
        return noiseSample;
    if (type == "dc")
        return dcSample;
    return silenceSample;
}
} // namespace

bool isValidWaveformType(const std::string& waveformType)
{
    return waveformType == "sine" || waveformType == "square" || waveformType == "triangle" ||
           waveformType == "sawtooth" || waveformType == "noise" || waveformType == "dc" || waveformType == "silence";
}

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
    SampleFunc const generate = resolveWaveform(waveformType);

    for (int i = 0; i < numSamples; ++i)
    {
        float const sample = generate(phase, amplitude, rng);
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

} // namespace multiscoper
