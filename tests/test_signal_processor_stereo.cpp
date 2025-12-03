/*
    Oscil - Signal Processor Stereo/Mode Tests
    Tests for stereo processing, mono summing, mid/side, and L/R extraction
*/

#include <gtest/gtest.h>
#include "core/dsp/SignalProcessor.h"
#include "helpers/TestSignals.h"
#include "helpers/AudioBufferBuilder.h"
#include <cmath>

using namespace oscil;
using namespace oscil::test;

class SignalProcessorStereoTest : public ::testing::Test
{
protected:
    SignalProcessor processor;
    ProcessedSignal output;
};

// =============================================================================
// Full Stereo Mode Tests
// =============================================================================

TEST_F(SignalProcessorStereoTest, FullStereoPassthrough)
{
    const int numSamples = 1024;
    auto left = generateSineVector(numSamples, 440.0f, 44100.0f);
    auto right = generateSineVector(numSamples, 880.0f, 44100.0f);

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

// =============================================================================
// Mono Mode Tests
// =============================================================================

TEST_F(SignalProcessorStereoTest, MonoSumming)
{
    const int numSamples = 1024;
    auto left = generateDCVector(numSamples, 0.5f);
    auto right = generateDCVector(numSamples, 0.3f);

    processor.process(left, right, ProcessingMode::Mono, output);

    ASSERT_EQ(output.numSamples, numSamples);
    ASSERT_FALSE(output.isStereo);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR(output.channel1[i], 0.4f, 0.001f); // (0.5 + 0.3) / 2 = 0.4
    }
}

// =============================================================================
// Mid Component Tests
// =============================================================================

TEST_F(SignalProcessorStereoTest, MidComponent)
{
    const int numSamples = 1024;
    auto left = generateDCVector(numSamples, 1.0f);
    auto right = generateDCVector(numSamples, 1.0f);

    processor.process(left, right, ProcessingMode::Mid, output);

    ASSERT_FALSE(output.isStereo);

    // Identical signals should have mid = full level
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR(output.channel1[i], 1.0f, 0.001f);
    }
}

TEST_F(SignalProcessorStereoTest, MidComponentExtremeValues)
{
    const int numSamples = 10;
    auto left = generateDCVector(numSamples, 100.0f);
    auto right = generateDCVector(numSamples, -100.0f);

    processor.process(left, right, ProcessingMode::Mid, output);

    // Mid = (100 + (-100)) / 2 = 0
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR(output.channel1[i], 0.0f, 0.001f);
    }
}

// =============================================================================
// Side Component Tests
// =============================================================================

TEST_F(SignalProcessorStereoTest, SideComponent)
{
    const int numSamples = 1024;
    auto left = generateDCVector(numSamples, 1.0f);
    auto right = generateDCVector(numSamples, -1.0f); // Inverted

    processor.process(left, right, ProcessingMode::Side, output);

    ASSERT_FALSE(output.isStereo);

    // Opposite signals should have maximum side content
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR(output.channel1[i], 1.0f, 0.001f); // (1 - (-1)) / 2 = 1
    }
}

TEST_F(SignalProcessorStereoTest, SideZeroForMono)
{
    const int numSamples = 1024;
    auto left = generateSineVector(numSamples, 440.0f, 44100.0f);
    auto right = left; // Same as left

    processor.process(left, right, ProcessingMode::Side, output);

    // Identical signals should have zero side content
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR(output.channel1[i], 0.0f, 0.001f);
    }
}

TEST_F(SignalProcessorStereoTest, SideComponentExtremeValues)
{
    const int numSamples = 10;
    auto left = generateDCVector(numSamples, 100.0f);
    auto right = generateDCVector(numSamples, -100.0f);

    processor.process(left, right, ProcessingMode::Side, output);

    // Side = (100 - (-100)) / 2 = 100
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR(output.channel1[i], 100.0f, 0.001f);
    }
}

// =============================================================================
// Left Channel Extraction Tests
// =============================================================================

TEST_F(SignalProcessorStereoTest, LeftChannelOnly)
{
    const int numSamples = 1024;
    auto left = generateSineVector(numSamples, 440.0f, 44100.0f);
    auto right = generateSineVector(numSamples, 880.0f, 44100.0f);

    processor.process(left, right, ProcessingMode::Left, output);

    ASSERT_FALSE(output.isStereo);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_FLOAT_EQ(output.channel1[i], left[i]);
    }
}

// =============================================================================
// Right Channel Extraction Tests
// =============================================================================

TEST_F(SignalProcessorStereoTest, RightChannelOnly)
{
    const int numSamples = 1024;
    auto left = generateSineVector(numSamples, 440.0f, 44100.0f);
    auto right = generateSineVector(numSamples, 880.0f, 44100.0f);

    processor.process(left, right, ProcessingMode::Right, output);

    ASSERT_FALSE(output.isStereo);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_FLOAT_EQ(output.channel1[i], right[i]);
    }
}

// =============================================================================
// All Processing Modes Tests
// =============================================================================

TEST_F(SignalProcessorStereoTest, AllModesMinimalBuffer)
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

TEST_F(SignalProcessorStereoTest, ProcessNullRightChannel)
{
    const int numSamples = 100;
    auto left = generateDCVector(numSamples, 0.5f);

    processor.process(left, {}, ProcessingMode::Left, output);

    EXPECT_EQ(output.numSamples, numSamples);
    EXPECT_FALSE(output.isStereo);
}

// =============================================================================
// DC Offset Tests
// =============================================================================

TEST_F(SignalProcessorStereoTest, DCOffsetMono)
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
// AudioBuffer Input Tests
// =============================================================================

TEST_F(SignalProcessorStereoTest, ProcessFromAudioBufferMono)
{
    auto buffer = AudioBufferBuilder()
        .withChannels(1)
        .withSamples(100)
        .withDC(0.5f)
        .build();

    processor.process(buffer, ProcessingMode::Mono, output);

    EXPECT_EQ(output.numSamples, 100);
}

TEST_F(SignalProcessorStereoTest, ProcessFromAudioBufferStereo)
{
    auto buffer = AudioBufferBuilder()
        .withChannels(2)
        .withSamples(100)
        .withStereoSineWave(440.0f, 880.0f, 1.0f)
        .build();

    processor.process(buffer, ProcessingMode::FullStereo, output);

    EXPECT_EQ(output.numSamples, 100);
    EXPECT_TRUE(output.isStereo);
}

TEST_F(SignalProcessorStereoTest, ProcessFromEmptyAudioBuffer)
{
    juce::AudioBuffer<float> buffer(2, 0);

    processor.process(buffer, ProcessingMode::FullStereo, output);

    EXPECT_EQ(output.numSamples, 0);
}
