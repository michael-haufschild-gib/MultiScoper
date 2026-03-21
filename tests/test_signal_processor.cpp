/*
    Oscil - Signal Processor Tests
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

class SignalProcessorTest : public ::testing::Test
{
protected:
    SignalProcessor processor;
    ProcessedSignal output;

    // Generate test signals
    std::vector<float> generateSineWave(int numSamples, float frequency, float sampleRate)
    {
        std::vector<float> samples(numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            samples[i] = std::sin(2.0f * M_PI * frequency * i / sampleRate);
        }
        return samples;
    }

    std::vector<float> generateDC(int numSamples, float level)
    {
        return std::vector<float>(numSamples, level);
    }
    
    // Generate random signal
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

// Test: Full Stereo mode passes through L/R correctly
TEST_F(SignalProcessorTest, FullStereoPassthrough)
{
    const int numSamples = 1024;
    auto left = generateSineWave(numSamples, 440.0f, 44100.0f);
    auto right = generateSineWave(numSamples, 880.0f, 44100.0f);

    processor.process(left, right, ProcessingMode::FullStereo, output);

    ASSERT_EQ(output.numSamples, numSamples);
    ASSERT_TRUE(output.isStereo);

    // Verify left channel
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_FLOAT_EQ(output.channel1[i], left[i]);
    }

    // Verify right channel
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_FLOAT_EQ(output.channel2[i], right[i]);
    }
}

// Test: Mono mode produces (L+R)/2
TEST_F(SignalProcessorTest, MonoSumming)
{
    const int numSamples = 1024;
    auto left = generateDC(numSamples, 0.5f);
    auto right = generateDC(numSamples, 0.3f);

    processor.process(left, right, ProcessingMode::Mono, output);

    ASSERT_EQ(output.numSamples, numSamples);
    ASSERT_FALSE(output.isStereo);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR(output.channel1[i], 0.4f, 0.001f); // (0.5 + 0.3) / 2 = 0.4
    }
}

// Test: Mid component M = (L+R)/2
TEST_F(SignalProcessorTest, MidComponent)
{
    const int numSamples = 1024;
    auto left = generateDC(numSamples, 1.0f);
    auto right = generateDC(numSamples, 1.0f);

    processor.process(left, right, ProcessingMode::Mid, output);

    ASSERT_FALSE(output.isStereo);

    // Identical signals should have mid = full level
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR(output.channel1[i], 1.0f, 0.001f);
    }
}

// Test: Side component S = (L-R)/2
TEST_F(SignalProcessorTest, SideComponent)
{
    const int numSamples = 1024;
    auto left = generateDC(numSamples, 1.0f);
    auto right = generateDC(numSamples, -1.0f); // Inverted

    processor.process(left, right, ProcessingMode::Side, output);

    ASSERT_FALSE(output.isStereo);

    // Opposite signals should have maximum side content
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR(output.channel1[i], 1.0f, 0.001f); // (1 - (-1)) / 2 = 1
    }
}

// Test: Side component is zero for identical L/R (mono content)
TEST_F(SignalProcessorTest, SideZeroForMono)
{
    const int numSamples = 1024;
    auto left = generateSineWave(numSamples, 440.0f, 44100.0f);
    auto right = left; // Same as left

    processor.process(left, right, ProcessingMode::Side, output);

    // Identical signals should have zero side content
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR(output.channel1[i], 0.0f, 0.001f);
    }
}

// Test: Left channel extraction
TEST_F(SignalProcessorTest, LeftChannelOnly)
{
    const int numSamples = 1024;
    auto left = generateSineWave(numSamples, 440.0f, 44100.0f);
    auto right = generateSineWave(numSamples, 880.0f, 44100.0f);

    processor.process(left, right, ProcessingMode::Left, output);

    ASSERT_FALSE(output.isStereo);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_FLOAT_EQ(output.channel1[i], left[i]);
    }
}

// Test: Right channel extraction
TEST_F(SignalProcessorTest, RightChannelOnly)
{
    const int numSamples = 1024;
    auto left = generateSineWave(numSamples, 440.0f, 44100.0f);
    auto right = generateSineWave(numSamples, 880.0f, 44100.0f);

    processor.process(left, right, ProcessingMode::Right, output);

    ASSERT_FALSE(output.isStereo);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_FLOAT_EQ(output.channel1[i], right[i]);
    }
}

// Test: Correlation coefficient for identical signals = 1.0
TEST_F(SignalProcessorTest, CorrelationIdenticalSignals)
{
    const int numSamples = 1024;
    auto signal = generateSineWave(numSamples, 440.0f, 44100.0f);

    float correlation = SignalProcessor::calculateCorrelation(signal, signal);

    EXPECT_NEAR(correlation, 1.0f, 0.01f);
}

// Test: Correlation coefficient for inverted signals = -1.0
TEST_F(SignalProcessorTest, CorrelationInvertedSignals)
{
    const int numSamples = 1024;
    auto left = generateSineWave(numSamples, 440.0f, 44100.0f);
    std::vector<float> right(numSamples);

    // Invert the signal
    for (int i = 0; i < numSamples; ++i)
    {
        right[i] = -left[i];
    }

    float correlation = SignalProcessor::calculateCorrelation(left, right);

    EXPECT_NEAR(correlation, -1.0f, 0.01f);
}

// Test: Peak level calculation
TEST_F(SignalProcessorTest, PeakLevelCalculation)
{
    const int numSamples = 100;
    std::vector<float> samples(numSamples, 0.0f);
    samples[50] = 0.75f;  // Peak positive
    samples[75] = -0.5f;  // Peak negative

    float peak = SignalProcessor::calculatePeak(samples);

    EXPECT_FLOAT_EQ(peak, 0.75f);
}

// Test: RMS level calculation
TEST_F(SignalProcessorTest, RMSLevelCalculation)
{
    const int numSamples = 1000;
    std::vector<float> samples(numSamples, 0.5f); // Constant DC

    float rms = SignalProcessor::calculateRMS(samples);

    EXPECT_FLOAT_EQ(rms, 0.5f);
}

// Test: Decimation preserves peaks
TEST_F(SignalProcessorTest, DecimationPreservesPeaks)
{
    const int inputLength = 1000;
    const int outputLength = 100;

    std::vector<float> input(inputLength, 0.0f);
    input[500] = 1.0f; // Peak in middle

    std::vector<float> output(outputLength);

    SignalProcessor::decimate(input, output, true);

    // Peak should be preserved somewhere in output
    float maxOutput = 0.0f;
    for (float val : output)
    {
        maxOutput = std::max(maxOutput, std::abs(val));
    }

    EXPECT_FLOAT_EQ(maxOutput, 1.0f);
}

// =============================================================================
// Edge Case Tests - Zero/Empty Samples
// =============================================================================

// Test: Process with zero samples
TEST_F(SignalProcessorTest, ProcessZeroSamples)
{
    std::vector<float> left;
    std::vector<float> right;

    processor.process(left, right, ProcessingMode::FullStereo, output);

    EXPECT_EQ(output.numSamples, 0);
}

TEST_F(SignalProcessorTest, ProcessZeroSamplesAfterPreviousFrameResetsOutputState)
{
    std::vector<float> left = { 0.5f, -0.25f };
    std::vector<float> right = { 0.2f, 0.8f };

    processor.process(left, right, ProcessingMode::FullStereo, output);
    ASSERT_EQ(output.numSamples, 2);
    ASSERT_TRUE(output.isStereo);

    std::vector<float> empty;
    processor.process(empty, empty, ProcessingMode::FullStereo, output);

    EXPECT_EQ(output.numSamples, 0);
    EXPECT_FALSE(output.isStereo);
    EXPECT_TRUE(output.channel1.empty());
    EXPECT_TRUE(output.channel2.empty());
}

// Test: Process with one sample
TEST_F(SignalProcessorTest, ProcessOneSample)
{
    std::vector<float> left = { 0.5f };
    std::vector<float> right = { -0.3f };

    processor.process(left, right, ProcessingMode::FullStereo, output);

    ASSERT_EQ(output.numSamples, 1);
    EXPECT_FLOAT_EQ(output.channel1[0], 0.5f);
    EXPECT_FLOAT_EQ(output.channel2[0], -0.3f);
}

// Test: Process silent buffer
TEST_F(SignalProcessorTest, ProcessSilentBuffer)
{
    const int numSamples = 1024;
    std::vector<float> silence(numSamples, 0.0f);

    processor.process(silence, silence, ProcessingMode::Mono, output);

    ASSERT_EQ(output.numSamples, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_FLOAT_EQ(output.channel1[i], 0.0f);
    }
}

// Test: Calculate peak of empty buffer
TEST_F(SignalProcessorTest, PeakEmptyBuffer)
{
    float peak = SignalProcessor::calculatePeak({});
    EXPECT_FLOAT_EQ(peak, 0.0f);
}

// Test: Calculate RMS of empty buffer
TEST_F(SignalProcessorTest, RMSEmptyBuffer)
{
    float rms = SignalProcessor::calculateRMS({});
    EXPECT_FLOAT_EQ(rms, 0.0f);
}

// Test: Correlation with zero samples
TEST_F(SignalProcessorTest, CorrelationZeroSamples)
{
    float correlation = SignalProcessor::calculateCorrelation({}, {});
    // Should return a neutral value (0.0 or handle gracefully)
    EXPECT_TRUE(std::isfinite(correlation));
}

// =============================================================================
// Edge Case Tests - Special Floating Point Values
// =============================================================================

// Test: Process with NaN values — output samples at non-NaN positions must remain finite
TEST_F(SignalProcessorTest, ProcessWithNaN)
{
    const int numSamples = 100;
    std::vector<float> left(numSamples, 0.5f);
    std::vector<float> right(numSamples, 0.5f);

    // Insert NaN values at known positions
    left[50] = std::nanf("");
    right[75] = std::nanf("");

    processor.process(left, right, ProcessingMode::FullStereo, output);

    ASSERT_EQ(output.numSamples, numSamples);

    // Non-NaN input positions must produce finite output
    for (int i = 0; i < numSamples; ++i)
    {
        if (i != 50)
            EXPECT_TRUE(std::isfinite(output.channel1[i])) << "channel1[" << i << "] is not finite";
        if (i != 75)
            EXPECT_TRUE(std::isfinite(output.channel2[i])) << "channel2[" << i << "] is not finite";
    }
}

// Test: Process with NaN in Mono mode — NaN must not spread to clean samples
TEST_F(SignalProcessorTest, ProcessWithNaNMonoContainment)
{
    const int numSamples = 100;
    std::vector<float> left(numSamples, 0.5f);
    std::vector<float> right(numSamples, 0.5f);
    left[50] = std::nanf("");

    processor.process(left, right, ProcessingMode::Mono, output);

    ASSERT_EQ(output.numSamples, numSamples);

    // Sample 50 may be NaN (legitimate propagation), but all other samples
    // must be finite since their inputs were both 0.5f
    for (int i = 0; i < numSamples; ++i)
    {
        if (i != 50)
            EXPECT_TRUE(std::isfinite(output.channel1[i])) << "channel1[" << i << "] is not finite";
    }
}

// Test: Process with Infinity values — non-Inf input positions must produce finite output
TEST_F(SignalProcessorTest, ProcessWithInfinity)
{
    const int numSamples = 100;
    std::vector<float> left(numSamples, 0.5f);
    std::vector<float> right(numSamples, 0.5f);

    left[25] = std::numeric_limits<float>::infinity();
    right[75] = -std::numeric_limits<float>::infinity();

    processor.process(left, right, ProcessingMode::Mid, output);

    ASSERT_EQ(output.numSamples, numSamples);

    // Mid = (L+R)/2. At non-Inf positions both L and R are 0.5f, so Mid must be 0.5f
    for (int i = 0; i < numSamples; ++i)
    {
        if (i != 25 && i != 75)
        {
            EXPECT_TRUE(std::isfinite(output.channel1[i])) << "channel1[" << i << "] is not finite";
            EXPECT_NEAR(output.channel1[i], 0.5f, 0.001f);
        }
    }
}

// Test: Peak with NaN — result must not be negative
TEST_F(SignalProcessorTest, PeakWithNaN)
{
    std::vector<float> samples = { 0.1f, 0.5f, std::nanf(""), 0.3f };

    float peak = SignalProcessor::calculatePeak(samples);

    // Result may be NaN (propagated) or a finite value (NaN ignored).
    // If finite, it must be non-negative and at least as large as the known peak (0.5).
    EXPECT_TRUE(std::isnan(peak) || peak >= 0.5f);
}

// Test: Peak with Infinity — must return infinity (it is the largest absolute value)
TEST_F(SignalProcessorTest, PeakWithInfinity)
{
    std::vector<float> samples = { 0.1f, 0.5f, std::numeric_limits<float>::infinity(), 0.3f };

    float peak = SignalProcessor::calculatePeak(samples);

    // Peak must be infinity since abs(inf) > any finite value
    EXPECT_EQ(peak, std::numeric_limits<float>::infinity());
}

// Test: RMS with NaN — result non-negative or NaN
TEST_F(SignalProcessorTest, RMSWithNaN)
{
    std::vector<float> samples = { 0.5f, 0.5f, std::nanf(""), 0.5f };

    float rms = SignalProcessor::calculateRMS(samples);

    // RMS with NaN input may propagate NaN or skip it.
    // If finite, must be non-negative.
    EXPECT_TRUE(std::isnan(rms) || rms >= 0.0f);
}

// Test: Correlation with NaN — result in valid range or NaN
TEST_F(SignalProcessorTest, CorrelationWithNaN)
{
    std::vector<float> left = { 0.5f, 0.5f, std::nanf(""), 0.5f };
    std::vector<float> right = { 0.5f, 0.5f, 0.5f, 0.5f };

    float correlation = SignalProcessor::calculateCorrelation(left, right);

    // Correlation with NaN input: result may be NaN or a finite value in [-1, 1].
    EXPECT_TRUE(std::isnan(correlation) || (correlation >= -1.0f && correlation <= 1.0f));
}

