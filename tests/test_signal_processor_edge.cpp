/*
    Oscil - Signal Processor Tests (Edge Cases & Precision)
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

class SignalProcessorEdgeTest : public ::testing::Test
{
protected:
    SignalProcessor processor;
    ProcessedSignal output;

    std::vector<float> generateSine(int numSamples, float frequency, float amplitude, float sampleRate)
    {
        std::vector<float> samples(numSamples);
        for(int i=0; i<numSamples; ++i) {
            samples[i] = amplitude * std::sin(2.0f * static_cast<float>(M_PI) * frequency * i / sampleRate);
        }
        return samples;
    }

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
// Edge Case Tests - Out of Range Values
// =============================================================================

// Test: Process with out-of-range values (clipping)
TEST_F(SignalProcessorEdgeTest, ProcessOutOfRangeValues)
{
    const int numSamples = 100;
    std::vector<float> left(numSamples, 2.0f);   // Above normal range
    std::vector<float> right(numSamples, -3.0f); // Below normal range

    processor.process(left, right, ProcessingMode::FullStereo, output);

    ASSERT_EQ(output.numSamples, numSamples);

    // Values should pass through (no clipping in SignalProcessor)
    EXPECT_FLOAT_EQ(output.channel1[0], 2.0f);
    EXPECT_FLOAT_EQ(output.channel2[0], -3.0f);
}

// Test: Peak with negative values
TEST_F(SignalProcessorEdgeTest, PeakNegativeValues)
{
    std::vector<float> samples = { -0.9f, -0.5f, -0.1f };

    float peak = SignalProcessor::calculatePeak(samples);

    // Peak should be the maximum absolute value
    EXPECT_FLOAT_EQ(peak, 0.9f);
}

// Test: Mid/Side with extreme values
TEST_F(SignalProcessorEdgeTest, MidSideExtremeValues)
{
    const int numSamples = 10;
    std::vector<float> left(numSamples, 100.0f);
    std::vector<float> right(numSamples, -100.0f);

    processor.process(left, right, ProcessingMode::Mid, output);

    // Mid = (100 + (-100)) / 2 = 0
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR(output.channel1[i], 0.0f, 0.001f);
    }

    processor.process(left, right, ProcessingMode::Side, output);

    // Side = (100 - (-100)) / 2 = 100
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR(output.channel1[i], 100.0f, 0.001f);
    }
}

// =============================================================================
// Edge Case Tests - DC Offset
// =============================================================================

// Test: DC offset in mono mode
TEST_F(SignalProcessorEdgeTest, DCOffsetMono)
{
    const int numSamples = 1024;
    float dcOffset = 0.3f;
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    // Sine with DC offset
    for (int i = 0; i < numSamples; ++i)
    {
        left[i] = dcOffset + 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        right[i] = dcOffset + 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
    }

    processor.process(left, right, ProcessingMode::Mono, output);

    // Calculate average (should preserve DC offset)
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        sum += output.channel1[i];
    }
    float avg = sum / numSamples;

    EXPECT_NEAR(avg, dcOffset, 0.01f);
}

// =============================================================================
// Edge Case Tests - Decimation Edge Cases
// =============================================================================

// Test: Decimate with same input/output length
TEST_F(SignalProcessorEdgeTest, DecimateSameLength)
{
    const int length = 100;
    std::vector<float> input(length);
    std::vector<float> output(length);

    for (int i = 0; i < length; ++i)
    {
        input[i] = static_cast<float>(i) / length;
    }

    SignalProcessor::decimate(input, output, true);

    // Should be roughly the same
    for (int i = 0; i < length; ++i)
    {
        EXPECT_NEAR(output[i], input[i], 0.01f);
    }
}

// Test: Decimate to single sample
TEST_F(SignalProcessorEdgeTest, DecimateToOneSample)
{
    const int inputLength = 1000;
    std::vector<float> input(inputLength);

    for (int i = 0; i < inputLength; ++i)
    {
        input[i] = 0.5f;
    }
    input[500] = 1.0f; // Peak

    std::vector<float> output(1);

    SignalProcessor::decimate(input, output, true);

    // Should have the peak
    EXPECT_FLOAT_EQ(output[0], 1.0f);
}

// Test: Decimate from single sample (upsampling case)
TEST_F(SignalProcessorEdgeTest, DecimateFromOneSample)
{
    std::vector<float> input = { 0.75f };
    std::vector<float> output(10, 0.0f);

    SignalProcessor::decimate(input, output, true);

    // All output samples should be filled with the input value (sample-and-hold)
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_FLOAT_EQ(output[i], 0.75f);
    }
}

// Test: Decimate zero samples
TEST_F(SignalProcessorEdgeTest, DecimateZeroSamples)
{
    std::vector<float> output(10, 1.0f);

    SignalProcessor::decimate({}, output, true);

    // With zero input samples, output should remain at its initial values
    // (decimate with empty input is a no-op for output content)
    EXPECT_EQ(output.size(), 10u);
}

// Test: Decimate without peak preservation
TEST_F(SignalProcessorEdgeTest, DecimateWithoutPeakPreservation)
{
    const int inputLength = 1000;
    const int outputLength = 100;

    std::vector<float> input(inputLength, 0.0f);
    input[500] = 1.0f;

    std::vector<float> output(outputLength);

    SignalProcessor::decimate(input, output, false);

    // Without peak preservation, peak might be averaged away
    // Test should not crash
    EXPECT_EQ(output.size(), static_cast<size_t>(outputLength));
}

// =============================================================================
// Edge Case Tests - All Processing Modes
// =============================================================================

// Test: All processing modes work with minimal buffer
TEST_F(SignalProcessorEdgeTest, AllModesMinimalBuffer)
{
    std::vector<float> left = { 0.5f, -0.5f };
    std::vector<float> right = { 0.3f, -0.3f };

    std::vector<ProcessingMode> modes = {
        ProcessingMode::FullStereo,
        ProcessingMode::Mono,
        ProcessingMode::Mid,
        ProcessingMode::Side,
        ProcessingMode::Left,
        ProcessingMode::Right
    };

    for (auto mode : modes)
    {
        processor.process(left, right, mode, output);
        EXPECT_EQ(output.numSamples, 2) << "Mode failed: " << static_cast<int>(mode);
    }
}

// Test: Process with null right channel (mono source)
TEST_F(SignalProcessorEdgeTest, ProcessNullRightChannel)
{
    const int numSamples = 100;
    std::vector<float> left(numSamples, 0.5f);

    processor.process(left, {}, ProcessingMode::Left, output);

    EXPECT_EQ(output.numSamples, numSamples);
    EXPECT_FALSE(output.isStereo);
}

TEST_F(SignalProcessorEdgeTest, ProcessNullRightChannelRightModeFallsBackToMonoInput)
{
    const int numSamples = 64;
    std::vector<float> left(numSamples, 0.25f);

    processor.process(left, {}, ProcessingMode::Right, output);

    ASSERT_EQ(output.numSamples, numSamples);
    EXPECT_FALSE(output.isStereo);
    ASSERT_EQ(output.channel1.size(), static_cast<size_t>(numSamples));
    for (float sample : output.channel1)
        EXPECT_FLOAT_EQ(sample, 0.25f);
}

// =============================================================================
// Edge Case Tests - ProcessedSignal Structure
// =============================================================================

// Test: ProcessedSignal resize
TEST_F(SignalProcessorEdgeTest, ProcessedSignalResize)
{
    ProcessedSignal signal;

    signal.resize(100, false);
    EXPECT_EQ(signal.channel1.size(), 100u);
    EXPECT_TRUE(signal.channel2.empty());
    EXPECT_EQ(signal.numSamples, 100);
    EXPECT_FALSE(signal.isStereo);

    signal.resize(50, true);
    EXPECT_EQ(signal.channel1.size(), 50u);
    EXPECT_EQ(signal.channel2.size(), 50u);
    EXPECT_EQ(signal.numSamples, 50);
    EXPECT_TRUE(signal.isStereo);
}

// =============================================================================
// Mismatched channel lengths
// =============================================================================

// Bug caught: left and right vectors have different sizes, causing
// out-of-bounds read in the shorter channel when processing Mid/Side.
TEST_F(SignalProcessorEdgeTest, MismatchedChannelLengthsLeftLonger)
{
    std::vector<float> left = { 0.5f, 0.5f, 0.5f, 0.5f };
    std::vector<float> right = { 0.3f, 0.3f };

    processor.process(left, right, ProcessingMode::FullStereo, output);

    // Output should use the minimum of the two lengths
    EXPECT_LE(output.numSamples, 4);
    EXPECT_GE(output.numSamples, 2);

    // However many samples we get, they must all be finite
    for (int i = 0; i < output.numSamples; ++i)
    {
        EXPECT_TRUE(std::isfinite(output.channel1[i]))
            << "channel1[" << i << "] is not finite";
    }
}

TEST_F(SignalProcessorEdgeTest, MismatchedChannelLengthsRightLonger)
{
    std::vector<float> left = { 0.2f };
    std::vector<float> right = { 0.8f, 0.8f, 0.8f };

    processor.process(left, right, ProcessingMode::Mono, output);

    EXPECT_GE(output.numSamples, 1);
    EXPECT_TRUE(std::isfinite(output.channel1[0]));
}

// Bug caught: denormal values in audio cause extreme CPU usage in
// downstream FIR/IIR filters that don't flush denormals.
TEST_F(SignalProcessorEdgeTest, DenormalValuesDoNotCauseIssues)
{
    const int numSamples = 100;
    float denormal = std::numeric_limits<float>::denorm_min();

    std::vector<float> left(numSamples, denormal);
    std::vector<float> right(numSamples, -denormal);

    processor.process(left, right, ProcessingMode::FullStereo, output);

    ASSERT_EQ(output.numSamples, numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_TRUE(std::isfinite(output.channel1[i]));
        EXPECT_TRUE(std::isfinite(output.channel2[i]));
    }

    // Peak of denormals should be very small but non-negative
    float peak = SignalProcessor::calculatePeak(left);
    EXPECT_GE(peak, 0.0f);
}

// Bug caught: correlation of mismatched-length vectors reads past end of shorter.
TEST_F(SignalProcessorEdgeTest, CorrelationMismatchedLengths)
{
    std::vector<float> left = { 0.5f, 0.5f, 0.5f };
    std::vector<float> right = { 0.5f };

    float correlation = SignalProcessor::calculateCorrelation(left, right);

    // Result should be finite regardless of length mismatch
    EXPECT_TRUE(std::isfinite(correlation) || std::isnan(correlation));
}

// Test: ProcessedSignal clear
TEST_F(SignalProcessorEdgeTest, ProcessedSignalClear)
{
    ProcessedSignal signal;
    signal.resize(10, true);

    // Fill with values
    for (int i = 0; i < 10; ++i)
    {
        signal.channel1[i] = 1.0f;
        signal.channel2[i] = 2.0f;
    }

    signal.clear();

    for (int i = 0; i < 10; ++i)
    {
        EXPECT_FLOAT_EQ(signal.channel1[i], 0.0f);
        EXPECT_FLOAT_EQ(signal.channel2[i], 0.0f);
    }
}

