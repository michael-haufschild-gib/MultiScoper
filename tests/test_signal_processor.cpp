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
#include "dsp/SignalProcessor.h"

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

// =============================================================================
// Edge Case Tests - Zero/Empty Samples
// =============================================================================

// Test: Process with zero samples
TEST_F(SignalProcessorTest, ProcessZeroSamples)
{
    std::vector<float> left;
    std::vector<float> right;

    processor.process(left.data(), right.data(), 0, ProcessingMode::FullStereo, output);

    EXPECT_EQ(output.numSamples, 0);
}

// Test: Process with one sample
TEST_F(SignalProcessorTest, ProcessOneSample)
{
    std::vector<float> left = { 0.5f };
    std::vector<float> right = { -0.3f };

    processor.process(left.data(), right.data(), 1, ProcessingMode::FullStereo, output);

    ASSERT_EQ(output.numSamples, 1);
    EXPECT_FLOAT_EQ(output.channel1[0], 0.5f);
    EXPECT_FLOAT_EQ(output.channel2[0], -0.3f);
}

// Test: Process silent buffer
TEST_F(SignalProcessorTest, ProcessSilentBuffer)
{
    const int numSamples = 1024;
    std::vector<float> silence(numSamples, 0.0f);

    processor.process(silence.data(), silence.data(), numSamples, ProcessingMode::Mono, output);

    ASSERT_EQ(output.numSamples, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_FLOAT_EQ(output.channel1[i], 0.0f);
    }
}

// Test: Calculate peak of empty buffer
TEST_F(SignalProcessorTest, PeakEmptyBuffer)
{
    float peak = SignalProcessor::calculatePeak(nullptr, 0);
    EXPECT_FLOAT_EQ(peak, 0.0f);
}

// Test: Calculate RMS of empty buffer
TEST_F(SignalProcessorTest, RMSEmptyBuffer)
{
    float rms = SignalProcessor::calculateRMS(nullptr, 0);
    EXPECT_FLOAT_EQ(rms, 0.0f);
}

// Test: Correlation with zero samples
TEST_F(SignalProcessorTest, CorrelationZeroSamples)
{
    float correlation = SignalProcessor::calculateCorrelation(nullptr, nullptr, 0);
    // Should return a neutral value (0.0 or handle gracefully)
    EXPECT_TRUE(std::isfinite(correlation));
}

// =============================================================================
// Edge Case Tests - Special Floating Point Values
// =============================================================================

// Test: Process with NaN values
TEST_F(SignalProcessorTest, ProcessWithNaN)
{
    const int numSamples = 100;
    std::vector<float> left(numSamples, 0.5f);
    std::vector<float> right(numSamples, 0.5f);

    // Insert NaN values
    left[50] = std::nanf("");
    right[75] = std::nanf("");

    // Should not crash
    processor.process(left.data(), right.data(), numSamples, ProcessingMode::FullStereo, output);

    EXPECT_EQ(output.numSamples, numSamples);
}

// Test: Process with Infinity values
TEST_F(SignalProcessorTest, ProcessWithInfinity)
{
    const int numSamples = 100;
    std::vector<float> left(numSamples, 0.5f);
    std::vector<float> right(numSamples, 0.5f);

    left[25] = std::numeric_limits<float>::infinity();
    right[75] = -std::numeric_limits<float>::infinity();

    // Should not crash
    processor.process(left.data(), right.data(), numSamples, ProcessingMode::Mid, output);

    EXPECT_EQ(output.numSamples, numSamples);
}

// Test: Peak with NaN
TEST_F(SignalProcessorTest, PeakWithNaN)
{
    std::vector<float> samples = { 0.1f, 0.5f, std::nanf(""), 0.3f };

    float peak = SignalProcessor::calculatePeak(samples.data(), static_cast<int>(samples.size()));

    // Should handle NaN gracefully - result may be NaN or should ignore it
    // Implementation-dependent, but shouldn't crash
    EXPECT_TRUE(true);
}

// Test: Peak with Infinity
TEST_F(SignalProcessorTest, PeakWithInfinity)
{
    std::vector<float> samples = { 0.1f, 0.5f, std::numeric_limits<float>::infinity(), 0.3f };

    float peak = SignalProcessor::calculatePeak(samples.data(), static_cast<int>(samples.size()));

    // Peak should be infinity or handled appropriately
    EXPECT_TRUE(peak == std::numeric_limits<float>::infinity() || std::isfinite(peak));
}

// Test: RMS with NaN
TEST_F(SignalProcessorTest, RMSWithNaN)
{
    std::vector<float> samples = { 0.5f, 0.5f, std::nanf(""), 0.5f };

    float rms = SignalProcessor::calculateRMS(samples.data(), static_cast<int>(samples.size()));

    // Should handle gracefully - result may be NaN
    EXPECT_TRUE(true);
}

// Test: Correlation with NaN
TEST_F(SignalProcessorTest, CorrelationWithNaN)
{
    std::vector<float> left = { 0.5f, 0.5f, std::nanf(""), 0.5f };
    std::vector<float> right = { 0.5f, 0.5f, 0.5f, 0.5f };

    float correlation = SignalProcessor::calculateCorrelation(
        left.data(), right.data(), static_cast<int>(left.size()));

    // Should not crash
    EXPECT_TRUE(true);
}

// =============================================================================
// Edge Case Tests - Out of Range Values
// =============================================================================

// Test: Process with out-of-range values (clipping)
TEST_F(SignalProcessorTest, ProcessOutOfRangeValues)
{
    const int numSamples = 100;
    std::vector<float> left(numSamples, 2.0f);   // Above normal range
    std::vector<float> right(numSamples, -3.0f); // Below normal range

    processor.process(left.data(), right.data(), numSamples, ProcessingMode::FullStereo, output);

    ASSERT_EQ(output.numSamples, numSamples);

    // Values should pass through (no clipping in SignalProcessor)
    EXPECT_FLOAT_EQ(output.channel1[0], 2.0f);
    EXPECT_FLOAT_EQ(output.channel2[0], -3.0f);
}

// Test: Peak with negative values
TEST_F(SignalProcessorTest, PeakNegativeValues)
{
    std::vector<float> samples = { -0.9f, -0.5f, -0.1f };

    float peak = SignalProcessor::calculatePeak(samples.data(), static_cast<int>(samples.size()));

    // Peak should be the maximum absolute value
    EXPECT_FLOAT_EQ(peak, 0.9f);
}

// Test: Mid/Side with extreme values
TEST_F(SignalProcessorTest, MidSideExtremeValues)
{
    const int numSamples = 10;
    std::vector<float> left(numSamples, 100.0f);
    std::vector<float> right(numSamples, -100.0f);

    processor.process(left.data(), right.data(), numSamples, ProcessingMode::Mid, output);

    // Mid = (100 + (-100)) / 2 = 0
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR(output.channel1[i], 0.0f, 0.001f);
    }

    processor.process(left.data(), right.data(), numSamples, ProcessingMode::Side, output);

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
TEST_F(SignalProcessorTest, DCOffsetMono)
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

    processor.process(left.data(), right.data(), numSamples, ProcessingMode::Mono, output);

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
TEST_F(SignalProcessorTest, DecimateSameLength)
{
    const int length = 100;
    std::vector<float> input(length);
    std::vector<float> output(length);

    for (int i = 0; i < length; ++i)
    {
        input[i] = static_cast<float>(i) / length;
    }

    SignalProcessor::decimate(input.data(), length, output.data(), length, true);

    // Should be roughly the same
    for (int i = 0; i < length; ++i)
    {
        EXPECT_NEAR(output[i], input[i], 0.01f);
    }
}

// Test: Decimate to single sample
TEST_F(SignalProcessorTest, DecimateToOneSample)
{
    const int inputLength = 1000;
    std::vector<float> input(inputLength);

    for (int i = 0; i < inputLength; ++i)
    {
        input[i] = 0.5f;
    }
    input[500] = 1.0f; // Peak

    std::vector<float> output(1);

    SignalProcessor::decimate(input.data(), inputLength, output.data(), 1, true);

    // Should have the peak
    EXPECT_FLOAT_EQ(output[0], 1.0f);
}

// Test: Decimate from single sample (upsampling case)
TEST_F(SignalProcessorTest, DecimateFromOneSample)
{
    std::vector<float> input = { 0.75f };
    std::vector<float> output(10, 0.0f);

    SignalProcessor::decimate(input.data(), 1, output.data(), 10, true);

    // All output samples should be filled with the input value (sample-and-hold)
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_FLOAT_EQ(output[i], 0.75f);
    }
}

// Test: Decimate zero samples
TEST_F(SignalProcessorTest, DecimateZeroSamples)
{
    std::vector<float> output(10, 1.0f);

    SignalProcessor::decimate(nullptr, 0, output.data(), 10, true);

    // Output might be unchanged or zeroed - implementation dependent
    // Main test is no crash
    EXPECT_TRUE(true);
}

// Test: Decimate without peak preservation
TEST_F(SignalProcessorTest, DecimateWithoutPeakPreservation)
{
    const int inputLength = 1000;
    const int outputLength = 100;

    std::vector<float> input(inputLength, 0.0f);
    input[500] = 1.0f;

    std::vector<float> output(outputLength);

    SignalProcessor::decimate(input.data(), inputLength, output.data(), outputLength, false);

    // Without peak preservation, peak might be averaged away
    // Test should not crash
    EXPECT_EQ(output.size(), static_cast<size_t>(outputLength));
}

// =============================================================================
// Edge Case Tests - All Processing Modes
// =============================================================================

// Test: All processing modes work with minimal buffer
TEST_F(SignalProcessorTest, AllModesMinimalBuffer)
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
        processor.process(left.data(), right.data(), 2, mode, output);
        EXPECT_EQ(output.numSamples, 2) << "Mode failed: " << static_cast<int>(mode);
    }
}

// Test: Process with null right channel (mono source)
TEST_F(SignalProcessorTest, ProcessNullRightChannel)
{
    const int numSamples = 100;
    std::vector<float> left(numSamples, 0.5f);

    processor.process(left.data(), nullptr, numSamples, ProcessingMode::Left, output);

    EXPECT_EQ(output.numSamples, numSamples);
    EXPECT_FALSE(output.isStereo);
}

// =============================================================================
// Edge Case Tests - ProcessedSignal Structure
// =============================================================================

// Test: ProcessedSignal resize
TEST_F(SignalProcessorTest, ProcessedSignalResize)
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

// Test: ProcessedSignal clear
TEST_F(SignalProcessorTest, ProcessedSignalClear)
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

// =============================================================================
// Edge Case Tests - AdaptiveDecimator
// =============================================================================

// Test: AdaptiveDecimator default state
TEST_F(SignalProcessorTest, AdaptiveDecimatorDefault)
{
    AdaptiveDecimator decimator;

    EXPECT_GT(decimator.getTargetSampleCount(), 0);
}

// Test: AdaptiveDecimator set display width
TEST_F(SignalProcessorTest, AdaptiveDecimatorSetWidth)
{
    AdaptiveDecimator decimator;

    decimator.setDisplayWidth(1920);
    EXPECT_GT(decimator.getTargetSampleCount(), 0);

    decimator.setDisplayWidth(100);
    // Target sample count may be a multiple of display width for envelope display
    EXPECT_GT(decimator.getTargetSampleCount(), 0);
}

// Test: AdaptiveDecimator process
TEST_F(SignalProcessorTest, AdaptiveDecimatorProcess)
{
    AdaptiveDecimator decimator;
    decimator.setDisplayWidth(100);

    std::vector<float> input(1000);
    for (int i = 0; i < 1000; ++i)
    {
        input[i] = std::sin(2.0f * M_PI * i / 100.0f);
    }

    std::vector<float> output;
    decimator.process(input.data(), 1000, output);

    // Output should be reduced from input, exact ratio depends on implementation
    EXPECT_GT(output.size(), 0u);
    EXPECT_LE(output.size(), 1000u);
}

// Test: AdaptiveDecimator envelope processing
TEST_F(SignalProcessorTest, AdaptiveDecimatorEnvelope)
{
    AdaptiveDecimator decimator;
    decimator.setDisplayWidth(50);

    std::vector<float> input(500);
    for (int i = 0; i < 500; ++i)
    {
        input[i] = std::sin(2.0f * M_PI * i / 50.0f);
    }

    std::vector<float> minEnv, maxEnv;
    decimator.processWithEnvelope(input.data(), 500, minEnv, maxEnv);

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

TEST_F(SignalProcessorTest, FuzzTest_RandomBuffers)
{
    // Iterate many times with random data
    for(int i=0; i<100; ++i) {
        int numSamples = 10 + (i * 10); // Variable size
        
        // Generate very loud noise, possibly outside [-1, 1]
        auto left = generateRandom(numSamples, -2.0f, 2.0f, 1000 + i);
        auto right = generateRandom(numSamples, -2.0f, 2.0f, 2000 + i);
        
        // Random mode
        ProcessingMode mode = static_cast<ProcessingMode>(i % 6);
        
        processor.process(left.data(), right.data(), numSamples, mode, output);
        
        ASSERT_EQ(output.numSamples, numSamples);
        
        // Basic sanity check
        if (output.numSamples > 0) {
            EXPECT_TRUE(std::isfinite(output.channel1[0]));
        }
    }
}

TEST_F(SignalProcessorTest, DenormalStress)
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
    
    processor.process(denormals.data(), denormals.data(), numSamples, ProcessingMode::FullStereo, output);
    
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
TEST_F(SignalProcessorTest, ProcessFromAudioBufferMono)
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
TEST_F(SignalProcessorTest, ProcessFromAudioBufferStereo)
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
TEST_F(SignalProcessorTest, ProcessFromEmptyAudioBuffer)
{
    juce::AudioBuffer<float> buffer(2, 0);

    processor.process(buffer, ProcessingMode::FullStereo, output);

    EXPECT_EQ(output.numSamples, 0);
}