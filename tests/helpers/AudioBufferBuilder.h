/*
    Oscil - Audio Buffer Builder
    Fluent builder for creating JUCE AudioBuffer<float> objects in tests
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include "TestSignals.h"
#include <cmath>

namespace oscil::test
{

/**
 * Fluent builder for creating test audio buffers
 *
 * Example usage:
 *   auto buffer = AudioBufferBuilder()
 *       .withChannels(2)
 *       .withSamples(1024)
 *       .withSineWave(440.0f, 1.0f, 44100.0f)
 *       .build();
 */
class AudioBufferBuilder
{
public:
    AudioBufferBuilder()
        : numChannels_(2)
        , numSamples_(512)
        , sampleRate_(44100.0f)
    {
    }

    /**
     * Set the number of channels (default: 2)
     */
    AudioBufferBuilder& withChannels(int channels)
    {
        numChannels_ = channels;
        return *this;
    }

    /**
     * Set the number of samples (default: 512)
     */
    AudioBufferBuilder& withSamples(int samples)
    {
        numSamples_ = samples;
        return *this;
    }

    /**
     * Set the sample rate (default: 44100.0)
     * Used for signal generation calculations
     */
    AudioBufferBuilder& withSampleRate(float sampleRate)
    {
        sampleRate_ = sampleRate;
        return *this;
    }

    /**
     * Fill buffer with a sine wave
     * @param frequency Frequency in Hz
     * @param amplitude Peak amplitude (0.0 to 1.0)
     * @param sampleRate Sample rate (uses builder's sampleRate if not specified)
     */
    AudioBufferBuilder& withSineWave(float frequency, float amplitude = 1.0f, float sampleRate = 0.0f)
    {
        float sr = (sampleRate > 0.0f) ? sampleRate : sampleRate_;
        buffer_ = generateSineWave(numSamples_, frequency, amplitude, sr);

        // Adjust channels if needed
        if (buffer_.getNumChannels() != numChannels_)
        {
            juce::AudioBuffer<float> temp(numChannels_, numSamples_);
            for (int ch = 0; ch < numChannels_; ++ch)
            {
                temp.copyFrom(ch, 0, buffer_, ch % buffer_.getNumChannels(), 0, numSamples_);
            }
            buffer_ = std::move(temp);
        }

        return *this;
    }

    /**
     * Fill buffer with stereo sine waves (different frequencies per channel)
     * @param leftFreq Left channel frequency in Hz
     * @param rightFreq Right channel frequency in Hz
     * @param amplitude Peak amplitude (0.0 to 1.0)
     */
    AudioBufferBuilder& withStereoSineWave(float leftFreq, float rightFreq, float amplitude = 1.0f)
    {
        buffer_ = generateStereoSineWave(numSamples_, leftFreq, rightFreq, amplitude, sampleRate_);

        if (numChannels_ != 2)
        {
            juce::AudioBuffer<float> temp(numChannels_, numSamples_);
            for (int ch = 0; ch < numChannels_; ++ch)
            {
                temp.copyFrom(ch, 0, buffer_, ch % 2, 0, numSamples_);
            }
            buffer_ = std::move(temp);
        }

        return *this;
    }

    /**
     * Fill buffer with a linear ramp
     */
    AudioBufferBuilder& withRamp(float startValue, float endValue)
    {
        buffer_ = generateRamp(numSamples_, startValue, endValue);

        if (buffer_.getNumChannels() != numChannels_)
        {
            juce::AudioBuffer<float> temp(numChannels_, numSamples_);
            for (int ch = 0; ch < numChannels_; ++ch)
            {
                temp.copyFrom(ch, 0, buffer_, 0, 0, numSamples_);
            }
            buffer_ = std::move(temp);
        }

        return *this;
    }

    /**
     * Fill buffer with a step signal
     */
    AudioBufferBuilder& withStep(float lowValue, float highValue, int stepPosition)
    {
        buffer_ = generateStep(numSamples_, lowValue, highValue, stepPosition);

        if (buffer_.getNumChannels() != numChannels_)
        {
            juce::AudioBuffer<float> temp(numChannels_, numSamples_);
            for (int ch = 0; ch < numChannels_; ++ch)
            {
                temp.copyFrom(ch, 0, buffer_, 0, 0, numSamples_);
            }
            buffer_ = std::move(temp);
        }

        return *this;
    }

    /**
     * Fill buffer with a square wave
     */
    AudioBufferBuilder& withSquare(float frequency, float amplitude = 1.0f)
    {
        buffer_ = generateSquare(numSamples_, frequency, amplitude, sampleRate_);

        if (buffer_.getNumChannels() != numChannels_)
        {
            juce::AudioBuffer<float> temp(numChannels_, numSamples_);
            for (int ch = 0; ch < numChannels_; ++ch)
            {
                temp.copyFrom(ch, 0, buffer_, 0, 0, numSamples_);
            }
            buffer_ = std::move(temp);
        }

        return *this;
    }

    /**
     * Fill buffer with a DC (constant) value
     */
    AudioBufferBuilder& withDC(float level)
    {
        buffer_ = generateDC(numChannels_, numSamples_, level);
        return *this;
    }

    /**
     * Fill buffer with silence (zeros)
     */
    AudioBufferBuilder& withSilence()
    {
        buffer_ = generateSilence(numChannels_, numSamples_);
        return *this;
    }

    /**
     * Fill buffer with random noise
     */
    AudioBufferBuilder& withRandom(float min = -1.0f, float max = 1.0f, int seed = 12345)
    {
        buffer_ = generateRandom(numSamples_, min, max, seed);

        if (buffer_.getNumChannels() != numChannels_)
        {
            juce::AudioBuffer<float> temp(numChannels_, numSamples_);
            for (int ch = 0; ch < numChannels_; ++ch)
            {
                temp.copyFrom(ch, 0, buffer_, ch % buffer_.getNumChannels(), 0, numSamples_);
            }
            buffer_ = std::move(temp);
        }

        return *this;
    }

    /**
     * Fill buffer with stereo random noise (independent channels)
     */
    AudioBufferBuilder& withStereoRandom(float min = -1.0f, float max = 1.0f, int seed = 12345)
    {
        buffer_ = generateStereoRandom(numSamples_, min, max, seed);

        if (numChannels_ != 2)
        {
            juce::AudioBuffer<float> temp(numChannels_, numSamples_);
            for (int ch = 0; ch < numChannels_; ++ch)
            {
                temp.copyFrom(ch, 0, buffer_, ch % 2, 0, numSamples_);
            }
            buffer_ = std::move(temp);
        }

        return *this;
    }

    /**
     * Apply a gain multiplier to the buffer
     */
    AudioBufferBuilder& withGain(float gainLinear)
    {
        if (buffer_.getNumChannels() > 0 && buffer_.getNumSamples() > 0)
        {
            buffer_.applyGain(gainLinear);
        }
        return *this;
    }

    /**
     * Apply a DC offset to all channels
     */
    AudioBufferBuilder& withDCOffset(float offset)
    {
        for (int ch = 0; ch < buffer_.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer_.getNumSamples(); ++i)
            {
                buffer_.setSample(ch, i, buffer_.getSample(ch, i) + offset);
            }
        }
        return *this;
    }

    /**
     * Invert the phase of all channels
     */
    AudioBufferBuilder& withInvertedPhase()
    {
        if (buffer_.getNumChannels() > 0 && buffer_.getNumSamples() > 0)
        {
            buffer_.applyGain(-1.0f);
        }
        return *this;
    }

    /**
     * Build and return the audio buffer
     * If no signal was specified, returns a silent buffer
     */
    juce::AudioBuffer<float> build()
    {
        if (buffer_.getNumChannels() == 0 || buffer_.getNumSamples() == 0)
        {
            buffer_ = generateSilence(numChannels_, numSamples_);
        }

        return std::move(buffer_);
    }

    /**
     * Build and return a mono version (sum to mono)
     */
    juce::AudioBuffer<float> buildMono()
    {
        build();

        if (buffer_.getNumChannels() == 1)
        {
            return std::move(buffer_);
        }

        juce::AudioBuffer<float> mono(1, numSamples_);

        for (int i = 0; i < numSamples_; ++i)
        {
            float sum = 0.0f;
            for (int ch = 0; ch < buffer_.getNumChannels(); ++ch)
            {
                sum += buffer_.getSample(ch, i);
            }
            mono.setSample(0, i, sum / static_cast<float>(buffer_.getNumChannels()));
        }

        return mono;
    }

private:
    int numChannels_;
    int numSamples_;
    float sampleRate_;
    juce::AudioBuffer<float> buffer_;
};

} // namespace oscil::test
