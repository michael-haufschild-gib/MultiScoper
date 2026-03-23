#pragma once

#include "core/dsp/TimingEngine.h"

#include <atomic>
#include <cmath>
#include <gtest/gtest.h>
#include <thread>

using namespace oscil;

class TimingEngineTest : public ::testing::Test
{
protected:
    TimingEngine engine;

    void SetUp() override
    {
        EngineTimingConfig defaultConfig;
        engine.setConfig(defaultConfig);
    }

    juce::AudioBuffer<float> generateSineWave(int numSamples, float frequency, float amplitude, float sampleRate)
    {
        juce::AudioBuffer<float> buffer(2, numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            float value = amplitude * std::sin(2.0f * juce::MathConstants<float>::pi * frequency * i / sampleRate);
            buffer.setSample(0, i, value);
            buffer.setSample(1, i, value);
        }
        return buffer;
    }

    juce::AudioBuffer<float> generateRamp(int numSamples, float startValue, float endValue)
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

    juce::AudioBuffer<float> generateStep(int numSamples, float lowValue, float highValue, int stepPosition)
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
};
