/*
    Oscil - Signal Processor Tests
*/

#include <gtest/gtest.h>
#include "dsp/SignalProcessor.h"
#include <cmath>

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
};

// Test: Full Stereo mode passes through L/R correctly
TEST_F(SignalProcessorTest, FullStereoPassthrough)
{
    const int numSamples = 1024;
    auto left = generateSineWave(numSamples, 440.0f, 44100.0f);
    auto right = generateSineWave(numSamples, 880.0f, 44100.0f);

    processor.process(left.data(), right.data(), numSamples, ProcessingMode::FullStereo, output);

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

    processor.process(left.data(), right.data(), numSamples, ProcessingMode::Mono, output);

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

    processor.process(left.data(), right.data(), numSamples, ProcessingMode::Mid, output);

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

    processor.process(left.data(), right.data(), numSamples, ProcessingMode::Side, output);

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

    processor.process(left.data(), right.data(), numSamples, ProcessingMode::Side, output);

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

    processor.process(left.data(), right.data(), numSamples, ProcessingMode::Left, output);

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

    processor.process(left.data(), right.data(), numSamples, ProcessingMode::Right, output);

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

    float correlation = SignalProcessor::calculateCorrelation(
        signal.data(), signal.data(), numSamples);

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

    float correlation = SignalProcessor::calculateCorrelation(
        left.data(), right.data(), numSamples);

    EXPECT_NEAR(correlation, -1.0f, 0.01f);
}

// Test: Peak level calculation
TEST_F(SignalProcessorTest, PeakLevelCalculation)
{
    const int numSamples = 100;
    std::vector<float> samples(numSamples, 0.0f);
    samples[50] = 0.75f;  // Peak positive
    samples[75] = -0.5f;  // Peak negative

    float peak = SignalProcessor::calculatePeak(samples.data(), numSamples);

    EXPECT_FLOAT_EQ(peak, 0.75f);
}

// Test: RMS level calculation
TEST_F(SignalProcessorTest, RMSLevelCalculation)
{
    const int numSamples = 1000;
    std::vector<float> samples(numSamples, 0.5f); // Constant DC

    float rms = SignalProcessor::calculateRMS(samples.data(), numSamples);

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

    SignalProcessor::decimate(input.data(), inputLength, output.data(), outputLength, true);

    // Peak should be preserved somewhere in output
    float maxOutput = 0.0f;
    for (float val : output)
    {
        maxOutput = std::max(maxOutput, std::abs(val));
    }

    EXPECT_FLOAT_EQ(maxOutput, 1.0f);
}
