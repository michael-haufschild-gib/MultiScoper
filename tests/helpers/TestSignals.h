/*
    Oscil - Test Signal Generators
    Common signal generation utilities for unit tests
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <cmath>
#include <random>

namespace oscil::test
{

/**
 * Generate a sine wave signal
 * @param numSamples Number of samples to generate
 * @param frequency Frequency in Hz
 * @param amplitude Peak amplitude (0.0 to 1.0)
 * @param sampleRate Sample rate in Hz
 * @return AudioBuffer containing the sine wave (stereo, identical channels)
 */
inline juce::AudioBuffer<float> generateSineWave(int numSamples, float frequency, float amplitude, float sampleRate)
{
    juce::AudioBuffer<float> buffer(2, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        float value = amplitude * std::sin(2.0f * juce::MathConstants<float>::pi * frequency * static_cast<float>(i) / sampleRate);
        buffer.setSample(0, i, value);
        buffer.setSample(1, i, value);
    }

    return buffer;
}

/**
 * Generate a sine wave with separate left and right frequencies
 * @param numSamples Number of samples to generate
 * @param leftFreq Left channel frequency in Hz
 * @param rightFreq Right channel frequency in Hz
 * @param amplitude Peak amplitude (0.0 to 1.0)
 * @param sampleRate Sample rate in Hz
 * @return AudioBuffer containing the stereo sine waves
 */
inline juce::AudioBuffer<float> generateStereoSineWave(int numSamples, float leftFreq, float rightFreq, float amplitude, float sampleRate)
{
    juce::AudioBuffer<float> buffer(2, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        float leftValue = amplitude * std::sin(2.0f * juce::MathConstants<float>::pi * leftFreq * static_cast<float>(i) / sampleRate);
        float rightValue = amplitude * std::sin(2.0f * juce::MathConstants<float>::pi * rightFreq * static_cast<float>(i) / sampleRate);
        buffer.setSample(0, i, leftValue);
        buffer.setSample(1, i, rightValue);
    }

    return buffer;
}

/**
 * Generate a linear ramp signal
 * @param numSamples Number of samples to generate
 * @param startValue Starting value
 * @param endValue Ending value
 * @return AudioBuffer containing the ramp (stereo, identical channels)
 */
inline juce::AudioBuffer<float> generateRamp(int numSamples, float startValue, float endValue)
{
    juce::AudioBuffer<float> buffer(2, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(numSamples - 1);
        float value = startValue + t * (endValue - startValue);
        buffer.setSample(0, i, value);
        buffer.setSample(1, i, value);
    }

    return buffer;
}

/**
 * Generate a step signal that transitions at a specific position
 * @param numSamples Number of samples to generate
 * @param lowValue Value before step
 * @param highValue Value after step
 * @param stepPosition Sample index where step occurs
 * @return AudioBuffer containing the step signal (stereo, identical channels)
 */
inline juce::AudioBuffer<float> generateStep(int numSamples, float lowValue, float highValue, int stepPosition)
{
    juce::AudioBuffer<float> buffer(2, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        float value = (i < stepPosition) ? lowValue : highValue;
        buffer.setSample(0, i, value);
        buffer.setSample(1, i, value);
    }

    return buffer;
}

/**
 * Generate a square wave signal
 * @param numSamples Number of samples to generate
 * @param frequency Frequency in Hz
 * @param amplitude Peak amplitude (0.0 to 1.0)
 * @param sampleRate Sample rate in Hz
 * @return AudioBuffer containing the square wave (stereo, identical channels)
 */
inline juce::AudioBuffer<float> generateSquare(int numSamples, float frequency, float amplitude, float sampleRate)
{
    juce::AudioBuffer<float> buffer(2, numSamples);

    float period = sampleRate / frequency;

    for (int i = 0; i < numSamples; ++i)
    {
        float phase = std::fmod(static_cast<float>(i), period) / period;
        float value = (phase < 0.5f) ? amplitude : -amplitude;
        buffer.setSample(0, i, value);
        buffer.setSample(1, i, value);
    }

    return buffer;
}

/**
 * Generate a DC (constant) signal
 * @param numChannels Number of channels
 * @param numSamples Number of samples to generate
 * @param level Constant value
 * @return AudioBuffer filled with the constant value
 */
inline juce::AudioBuffer<float> generateDC(int numChannels, int numSamples, float level)
{
    juce::AudioBuffer<float> buffer(numChannels, numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            buffer.setSample(ch, i, level);
        }
    }

    return buffer;
}

/**
 * Generate a silent buffer (all zeros)
 * @param numChannels Number of channels
 * @param numSamples Number of samples to generate
 * @return AudioBuffer filled with zeros
 */
inline juce::AudioBuffer<float> generateSilence(int numChannels, int numSamples)
{
    juce::AudioBuffer<float> buffer(numChannels, numSamples);
    buffer.clear();
    return buffer;
}

/**
 * Generate random noise
 * @param numSamples Number of samples to generate
 * @param min Minimum value
 * @param max Maximum value
 * @param seed Random seed for reproducibility
 * @return AudioBuffer containing random values (stereo, identical channels)
 */
inline juce::AudioBuffer<float> generateRandom(int numSamples, float min, float max, int seed = 12345)
{
    juce::AudioBuffer<float> buffer(2, numSamples);
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist(min, max);

    for (int i = 0; i < numSamples; ++i)
    {
        float value = dist(gen);
        buffer.setSample(0, i, value);
        buffer.setSample(1, i, value);
    }

    return buffer;
}

/**
 * Generate white noise with independent channels
 * @param numSamples Number of samples to generate
 * @param min Minimum value
 * @param max Maximum value
 * @param seed Random seed for reproducibility
 * @return AudioBuffer containing random values with independent channels
 */
inline juce::AudioBuffer<float> generateStereoRandom(int numSamples, float min, float max, int seed = 12345)
{
    juce::AudioBuffer<float> buffer(2, numSamples);
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist(min, max);

    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            buffer.setSample(ch, i, dist(gen));
        }
    }

    return buffer;
}

/**
 * Generate a vector of samples (for non-JUCE buffer tests)
 * @param numSamples Number of samples to generate
 * @param frequency Frequency in Hz
 * @param sampleRate Sample rate in Hz
 * @return Vector of sine wave samples
 */
inline std::vector<float> generateSineVector(int numSamples, float frequency, float sampleRate)
{
    std::vector<float> samples(numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        samples[i] = std::sin(2.0f * juce::MathConstants<float>::pi * frequency * static_cast<float>(i) / sampleRate);
    }

    return samples;
}

/**
 * Generate a DC vector (for non-JUCE buffer tests)
 * @param numSamples Number of samples to generate
 * @param level Constant value
 * @return Vector filled with the constant value
 */
inline std::vector<float> generateDCVector(int numSamples, float level)
{
    return std::vector<float>(numSamples, level);
}

/**
 * Generate a random vector (for non-JUCE buffer tests)
 * @param numSamples Number of samples to generate
 * @param min Minimum value
 * @param max Maximum value
 * @param seed Random seed for reproducibility
 * @return Vector containing random values
 */
inline std::vector<float> generateRandomVector(int numSamples, float min, float max, int seed = 12345)
{
    std::vector<float> samples(numSamples);
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist(min, max);

    for (int i = 0; i < numSamples; ++i)
    {
        samples[i] = dist(gen);
    }

    return samples;
}

} // namespace oscil::test
