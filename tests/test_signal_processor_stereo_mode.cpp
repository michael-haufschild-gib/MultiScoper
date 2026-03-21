/*
    Oscil - Signal Processor Tests (Stereo Modes & AudioBuffer Input)
    AdaptiveDecimator, fuzz testing, AudioBuffer input edge cases
*/

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <limits>
#include <random>
#include <gtest/gtest.h>
#include "core/dsp/SignalProcessor.h"
#include <span>

using namespace oscil;

class SignalProcessorStereoTest : public ::testing::Test
{
protected:
    SignalProcessor processor;
    ProcessedSignal output;

    std::vector<float> generateRandom(int numSamples, float min, float max, int seed = 12345)
    {
        std::vector<float> samples(numSamples);
        std::mt19937 gen(seed);
        std::uniform_real_distribution<float> dist(min, max);
        for(int i=0; i<numSamples; ++i) {
            samples[i] = dist(gen);
        }
        return samples;
    }
};

// =============================================================================
// Edge Case Tests - AdaptiveDecimator
// =============================================================================

// Test: AdaptiveDecimator default state
TEST_F(SignalProcessorStereoTest, AdaptiveDecimatorDefault)
{
    AdaptiveDecimator decimator;

    EXPECT_GT(decimator.getTargetSampleCount(), 0);
}

// Test: AdaptiveDecimator set display width
TEST_F(SignalProcessorStereoTest, AdaptiveDecimatorSetWidth)
{
    AdaptiveDecimator decimator;

    decimator.setDisplayWidth(1920);
    EXPECT_GT(decimator.getTargetSampleCount(), 0);

    decimator.setDisplayWidth(100);
    // Target sample count may be a multiple of display width for envelope display
    EXPECT_GT(decimator.getTargetSampleCount(), 0);
}

// Test: AdaptiveDecimator process
TEST_F(SignalProcessorStereoTest, AdaptiveDecimatorProcess)
{
    AdaptiveDecimator decimator;
    decimator.setDisplayWidth(100);

    std::vector<float> input(1000);
    for (int i = 0; i < 1000; ++i)
    {
        input[i] = std::sin(2.0f * M_PI * i / 100.0f);
    }

    std::vector<float> output;
    decimator.process(input, output);

    // Output should be reduced from input, exact ratio depends on implementation
    EXPECT_GT(output.size(), 0u);
    EXPECT_LE(output.size(), 1000u);
}

// Test: AdaptiveDecimator envelope processing
TEST_F(SignalProcessorStereoTest, AdaptiveDecimatorEnvelope)
{
    AdaptiveDecimator decimator;
    decimator.setDisplayWidth(50);

    std::vector<float> input(500);
    for (int i = 0; i < 500; ++i)
    {
        input[i] = std::sin(2.0f * M_PI * i / 50.0f);
    }

    std::vector<float> minEnv, maxEnv;
    decimator.processWithEnvelope(input, minEnv, maxEnv);

    EXPECT_EQ(minEnv.size(), maxEnv.size());
    EXPECT_GT(minEnv.size(), 0u);

    // Max should be >= min at each point
    for (size_t i = 0; i < minEnv.size(); ++i)
    {
        EXPECT_GE(maxEnv[i], minEnv[i]);
    }
}

// =============================================================================
// Fuzz Testing & Stress Tests
// =============================================================================

TEST_F(SignalProcessorStereoTest, FuzzTest_RandomBuffers)
{
    // Iterate many times with random data
    for(int i=0; i<100; ++i) {
        int numSamples = 10 + (i * 10); // Variable size

        // Generate very loud noise, possibly outside [-1, 1]
        auto left = generateRandom(numSamples, -2.0f, 2.0f, 1000 + i);
        auto right = generateRandom(numSamples, -2.0f, 2.0f, 2000 + i);

        // Random mode
        ProcessingMode mode = static_cast<ProcessingMode>(i % 6);

        processor.process(left, right, mode, output);

        ASSERT_EQ(output.numSamples, numSamples);

        // Basic sanity check
        if (output.numSamples > 0) {
            EXPECT_TRUE(std::isfinite(output.channel1[0]));
        }
    }
}

TEST_F(SignalProcessorStereoTest, DenormalStress)
{
    // Denormal numbers (very small values near zero) can cause performance issues
    // on some architectures if not handled (flush-to-zero).
    // We just want to ensure they don't cause crashes or weird output logic.

    const int numSamples = 1024;
    std::vector<float> denormals(numSamples);

    // Create denormal values: ~ 1e-40
    float val = 1.0e-30f;
    for(int i=0; i<numSamples; ++i) {
        denormals[i] = val;
        val *= 0.9f; // Quickly becomes denormal
    }

    processor.process(denormals, denormals, ProcessingMode::FullStereo, output);

    // Verify output is finite and consistent
    for(int i=0; i<numSamples; ++i) {
        EXPECT_TRUE(std::isfinite(output.channel1[i]));
        // If FTZ is on, these might become zero, which is fine.
    }
}

// =============================================================================
// Edge Case Tests - AudioBuffer Input
// =============================================================================

// Test: Process from AudioBuffer mono
TEST_F(SignalProcessorStereoTest, ProcessFromAudioBufferMono)
{
    juce::AudioBuffer<float> buffer(1, 100);

    for (int i = 0; i < 100; ++i)
    {
        buffer.setSample(0, i, 0.5f);
    }

    processor.process(buffer, ProcessingMode::Mono, output);

    EXPECT_EQ(output.numSamples, 100);
}

// Test: Process from AudioBuffer stereo
TEST_F(SignalProcessorStereoTest, ProcessFromAudioBufferStereo)
{
    juce::AudioBuffer<float> buffer(2, 100);

    for (int i = 0; i < 100; ++i)
    {
        buffer.setSample(0, i, 0.5f);
        buffer.setSample(1, i, 0.3f);
    }

    processor.process(buffer, ProcessingMode::FullStereo, output);

    EXPECT_EQ(output.numSamples, 100);
    EXPECT_TRUE(output.isStereo);
}

// Test: Process from empty AudioBuffer
TEST_F(SignalProcessorStereoTest, ProcessFromEmptyAudioBuffer)
{
    juce::AudioBuffer<float> buffer(2, 0);

    processor.process(buffer, ProcessingMode::FullStereo, output);

    EXPECT_EQ(output.numSamples, 0);
}

TEST_F(SignalProcessorStereoTest, ProcessFromEmptyAudioBufferClearsPreviousOutputState)
{
    juce::AudioBuffer<float> populated(2, 16);
    for (int i = 0; i < 16; ++i)
    {
        populated.setSample(0, i, 0.5f);
        populated.setSample(1, i, -0.5f);
    }

    processor.process(populated, ProcessingMode::FullStereo, output);
    ASSERT_EQ(output.numSamples, 16);
    ASSERT_TRUE(output.isStereo);

    juce::AudioBuffer<float> empty(2, 0);
    processor.process(empty, ProcessingMode::FullStereo, output);

    EXPECT_EQ(output.numSamples, 0);
    EXPECT_FALSE(output.isStereo);
    EXPECT_TRUE(output.channel1.empty());
    EXPECT_TRUE(output.channel2.empty());
}
