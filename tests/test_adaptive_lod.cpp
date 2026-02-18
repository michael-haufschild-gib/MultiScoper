/*
    Oscil - Adaptive LOD Tests
    Tests for the Level-of-Detail system including tier calculation,
    hysteresis, and peak preservation during decimation.
*/

#include <gtest/gtest.h>
#include "core/dsp/SignalProcessor.h"
#include <cmath>
#include <vector>

using namespace oscil;

//==============================================================================
// LODTier Helper Function Tests
//==============================================================================

TEST(LODTierTest, GetSampleCountReturnsCorrectValues)
{
    EXPECT_EQ(getLODSampleCount(LODTier::Full), 2048);
    EXPECT_EQ(getLODSampleCount(LODTier::High), 1024);
    EXPECT_EQ(getLODSampleCount(LODTier::Medium), 512);
    EXPECT_EQ(getLODSampleCount(LODTier::Preview), 256);
}

TEST(LODTierTest, GetTierNameReturnsValidStrings)
{
    EXPECT_STREQ(getLODTierName(LODTier::Full), "Full");
    EXPECT_STREQ(getLODTierName(LODTier::High), "High");
    EXPECT_STREQ(getLODTierName(LODTier::Medium), "Medium");
    EXPECT_STREQ(getLODTierName(LODTier::Preview), "Preview");
}

//==============================================================================
// AdaptiveDecimator LOD Tier Tests
//==============================================================================

class AdaptiveDecimatorTest : public ::testing::Test
{
protected:
    AdaptiveDecimator decimator;
    
    // Generate a simple test signal
    std::vector<float> generateTestSignal(size_t numSamples)
    {
        std::vector<float> samples(numSamples);
        for (size_t i = 0; i < numSamples; ++i)
        {
            samples[i] = std::sin(2.0f * M_PI * static_cast<float>(i) / 100.0f);
        }
        return samples;
    }
    
    // Generate a signal with known peaks
    std::vector<float> generatePeakySignal(size_t numSamples)
    {
        std::vector<float> samples(numSamples, 0.1f);
        
        // Add distinct peaks at known positions
        if (numSamples > 100) samples[50] = 1.0f;
        if (numSamples > 200) samples[150] = -0.9f;
        if (numSamples > 400) samples[300] = 0.8f;
        if (numSamples > 600) samples[500] = -0.7f;
        
        return samples;
    }
};

TEST_F(AdaptiveDecimatorTest, InitialTierIsFull)
{
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::Full);
    EXPECT_EQ(decimator.getTargetSampleCount(), 2048);
}

TEST_F(AdaptiveDecimatorTest, LargeWidthSelectsFullTier)
{
    decimator.setDisplayWidth(1000);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::Full);
    EXPECT_EQ(decimator.getTargetSampleCount(), 2048);
}

TEST_F(AdaptiveDecimatorTest, MediumLargeWidthSelectsHighTier)
{
    decimator.setDisplayWidth(600);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::High);
    EXPECT_EQ(decimator.getTargetSampleCount(), 1024);
}

TEST_F(AdaptiveDecimatorTest, MediumWidthSelectsMediumTier)
{
    decimator.setDisplayWidth(300);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::Medium);
    EXPECT_EQ(decimator.getTargetSampleCount(), 512);
}

TEST_F(AdaptiveDecimatorTest, SmallWidthSelectsPreviewTier)
{
    decimator.setDisplayWidth(100);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::Preview);
    EXPECT_EQ(decimator.getTargetSampleCount(), 256);
}

TEST_F(AdaptiveDecimatorTest, VerySmallWidthStillWorks)
{
    decimator.setDisplayWidth(10);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::Preview);
    EXPECT_EQ(decimator.getTargetSampleCount(), 256);
}

TEST_F(AdaptiveDecimatorTest, ZeroWidthDefaultsToMinimum)
{
    decimator.setDisplayWidth(0);
    // Should use minimum width of 1
    EXPECT_EQ(decimator.getDisplayWidth(), 1);
}

//==============================================================================
// Hysteresis Tests
//==============================================================================

TEST_F(AdaptiveDecimatorTest, HysteresisPreventsTierFlickeringAtFullHighBoundary)
{
    // Start at a width clearly in Full tier
    decimator.setDisplayWidth(900);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::Full);
    
    // Move to boundary (800) - should stay in current tier due to hysteresis
    decimator.setDisplayWidth(800);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::Full);
    
    // Move slightly below boundary but within hysteresis zone
    decimator.setDisplayWidth(790);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::Full);
    
    // Move clearly below boundary (outside hysteresis zone)
    decimator.setDisplayWidth(700);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::High);
}

TEST_F(AdaptiveDecimatorTest, HysteresisPreventsTierFlickeringAtHighMediumBoundary)
{
    // Start clearly in High tier
    decimator.setDisplayWidth(600);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::High);
    
    // Move to boundary area - should stay
    decimator.setDisplayWidth(410);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::High);
    
    // Move clearly below
    decimator.setDisplayWidth(300);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::Medium);
}

TEST_F(AdaptiveDecimatorTest, HysteresisWorksWhenScalingUp)
{
    // Start in Preview tier
    decimator.setDisplayWidth(100);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::Preview);
    
    // Move to boundary area - should stay in Preview
    decimator.setDisplayWidth(210);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::Preview);
    
    // Move clearly above
    decimator.setDisplayWidth(300);
    EXPECT_EQ(decimator.getCurrentTier(), LODTier::Medium);
}

//==============================================================================
// Decimation Output Tests
//==============================================================================

TEST_F(AdaptiveDecimatorTest, ProcessProducesCorrectOutputSize)
{
    decimator.setDisplayWidth(600); // High tier = 1024 samples
    
    auto input = generateTestSignal(4096);
    std::vector<float> output;
    
    decimator.process(input, output);
    
    EXPECT_EQ(output.size(), 1024u);
}

TEST_F(AdaptiveDecimatorTest, ProcessWithPeaksProducesCorrectStructure)
{
    decimator.setDisplayWidth(600); // High tier
    
    auto input = generateTestSignal(4096);
    DecimatedWaveform output;
    
    decimator.processWithPeaks(input, output);
    
    EXPECT_EQ(output.size(), 1024u);
    EXPECT_EQ(output.samples.size(), 1024u);
    EXPECT_EQ(output.minEnvelope.size(), 1024u);
    EXPECT_EQ(output.maxEnvelope.size(), 1024u);
    EXPECT_EQ(output.tier, LODTier::High);
}

//==============================================================================
// Peak Preservation Tests
//==============================================================================

TEST_F(AdaptiveDecimatorTest, MinMaxEnvelopePreservesPeaks)
{
    decimator.setDisplayWidth(100); // Preview tier = 256 samples
    
    // Create signal with known peaks
    auto input = generatePeakySignal(2048);
    DecimatedWaveform output;
    
    decimator.processWithPeaks(input, output);
    
    // Find max in output
    float maxInOutput = 0.0f;
    float minInOutput = 0.0f;
    
    for (size_t i = 0; i < output.size(); ++i)
    {
        maxInOutput = std::max(maxInOutput, output.maxEnvelope[i]);
        minInOutput = std::min(minInOutput, output.minEnvelope[i]);
    }
    
    // The peak of 1.0 should be preserved in the max envelope
    EXPECT_NEAR(maxInOutput, 1.0f, 0.01f);
    // The negative peak of -0.9 should be preserved in the min envelope
    EXPECT_NEAR(minInOutput, -0.9f, 0.01f);
}

TEST_F(AdaptiveDecimatorTest, EnvelopeMinIsAlwaysLessThanOrEqualToMax)
{
    decimator.setDisplayWidth(200); // Medium tier
    
    auto input = generateTestSignal(4096);
    DecimatedWaveform output;
    
    decimator.processWithPeaks(input, output);
    
    for (size_t i = 0; i < output.size(); ++i)
    {
        EXPECT_LE(output.minEnvelope[i], output.maxEnvelope[i]);
    }
}

TEST_F(AdaptiveDecimatorTest, SampleValueIsWithinEnvelope)
{
    decimator.setDisplayWidth(300); // Medium tier
    
    auto input = generateTestSignal(4096);
    DecimatedWaveform output;
    
    decimator.processWithPeaks(input, output);
    
    for (size_t i = 0; i < output.size(); ++i)
    {
        EXPECT_GE(output.samples[i], output.minEnvelope[i]);
        EXPECT_LE(output.samples[i], output.maxEnvelope[i]);
    }
}

//==============================================================================
// DecimatedWaveform Struct Tests
//==============================================================================

TEST(DecimatedWaveformTest, ResizeAllocatesAllArrays)
{
    DecimatedWaveform waveform;
    waveform.resize(512);
    
    EXPECT_EQ(waveform.samples.size(), 512u);
    EXPECT_EQ(waveform.minEnvelope.size(), 512u);
    EXPECT_EQ(waveform.maxEnvelope.size(), 512u);
}

TEST(DecimatedWaveformTest, ClearEmptiesAllArrays)
{
    DecimatedWaveform waveform;
    waveform.resize(512);
    waveform.clear();
    
    EXPECT_TRUE(waveform.empty());
    EXPECT_EQ(waveform.size(), 0u);
}

TEST(DecimatedWaveformTest, EmptyReturnsTrueForEmptyWaveform)
{
    DecimatedWaveform waveform;
    EXPECT_TRUE(waveform.empty());
    
    waveform.resize(100);
    EXPECT_FALSE(waveform.empty());
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_F(AdaptiveDecimatorTest, EmptyInputProducesZeroFilledOutput)
{
    decimator.setDisplayWidth(400);
    
    std::vector<float> emptyInput;
    DecimatedWaveform output;
    
    decimator.processWithPeaks(emptyInput, output);
    
    // Output should still have samples but all zeros
    EXPECT_EQ(output.size(), static_cast<size_t>(decimator.getTargetSampleCount()));
    
    for (size_t i = 0; i < output.size(); ++i)
    {
        EXPECT_FLOAT_EQ(output.samples[i], 0.0f);
        EXPECT_FLOAT_EQ(output.minEnvelope[i], 0.0f);
        EXPECT_FLOAT_EQ(output.maxEnvelope[i], 0.0f);
    }
}

TEST_F(AdaptiveDecimatorTest, SingleSampleInputHandledGracefully)
{
    decimator.setDisplayWidth(400);
    
    std::vector<float> singleSample = { 0.5f };
    DecimatedWaveform output;
    
    decimator.processWithPeaks(singleSample, output);
    
    // Should fill with the single sample value
    EXPECT_GT(output.size(), 0u);
    EXPECT_FLOAT_EQ(output.samples[0], 0.5f);
}

TEST_F(AdaptiveDecimatorTest, InputSmallerThanOutputUpsamples)
{
    decimator.setDisplayWidth(100); // Preview = 256 samples
    
    // Input smaller than output
    std::vector<float> smallInput(128);
    for (size_t i = 0; i < smallInput.size(); ++i)
    {
        smallInput[i] = static_cast<float>(i) / 128.0f;
    }
    
    DecimatedWaveform output;
    decimator.processWithPeaks(smallInput, output);
    
    EXPECT_EQ(output.size(), 256u);
    
    // Output should be interpolated, values should be smooth
    for (size_t i = 1; i < output.size(); ++i)
    {
        // Values should increase monotonically (since input is ramp)
        EXPECT_GE(output.samples[i], output.samples[i-1] - 0.01f);
    }
}













