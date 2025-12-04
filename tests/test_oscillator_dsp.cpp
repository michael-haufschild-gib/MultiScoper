/*
    Oscil - Oscillator DSP Tests
*/

#include "core/Oscillator.h"

#include <gtest/gtest.h>

using namespace oscil;

class OscillatorDspTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Single trace detection
TEST_F(OscillatorDspTest, SingleTraceDetection)
{
    Oscillator osc;

    osc.setProcessingMode(ProcessingMode::FullStereo);
    EXPECT_FALSE(osc.isSingleTrace());

    osc.setProcessingMode(ProcessingMode::Mono);
    EXPECT_TRUE(osc.isSingleTrace());

    osc.setProcessingMode(ProcessingMode::Mid);
    EXPECT_TRUE(osc.isSingleTrace());

    osc.setProcessingMode(ProcessingMode::Side);
    EXPECT_TRUE(osc.isSingleTrace());

    osc.setProcessingMode(ProcessingMode::Left);
    EXPECT_TRUE(osc.isSingleTrace());

    osc.setProcessingMode(ProcessingMode::Right);
    EXPECT_TRUE(osc.isSingleTrace());
}

// Test: Processing mode string conversion
TEST_F(OscillatorDspTest, ProcessingModeStringConversion)
{
    EXPECT_EQ(processingModeToString(ProcessingMode::FullStereo), "FullStereo");
    EXPECT_EQ(processingModeToString(ProcessingMode::Mono), "Mono");
    EXPECT_EQ(processingModeToString(ProcessingMode::Mid), "Mid");
    EXPECT_EQ(processingModeToString(ProcessingMode::Side), "Side");
    EXPECT_EQ(processingModeToString(ProcessingMode::Left), "Left");
    EXPECT_EQ(processingModeToString(ProcessingMode::Right), "Right");

    EXPECT_EQ(stringToProcessingMode("FullStereo"), ProcessingMode::FullStereo);
    EXPECT_EQ(stringToProcessingMode("Mono"), ProcessingMode::Mono);
    EXPECT_EQ(stringToProcessingMode("Mid"), ProcessingMode::Mid);
    EXPECT_EQ(stringToProcessingMode("Side"), ProcessingMode::Side);
    EXPECT_EQ(stringToProcessingMode("Left"), ProcessingMode::Left);
    EXPECT_EQ(stringToProcessingMode("Right"), ProcessingMode::Right);

    // Unknown string should default to FullStereo
    EXPECT_EQ(stringToProcessingMode("Unknown"), ProcessingMode::FullStereo);
}

// Test: Line width clamping
TEST_F(OscillatorDspTest, LineWidthClampingLow)
{
    Oscillator osc;
    osc.setLineWidth(0.0f);
    EXPECT_EQ(osc.getLineWidth(), Oscillator::MIN_LINE_WIDTH);
}

TEST_F(OscillatorDspTest, LineWidthClampingHigh)
{
    Oscillator osc;
    osc.setLineWidth(100.0f);
    EXPECT_EQ(osc.getLineWidth(), Oscillator::MAX_LINE_WIDTH);
}

TEST_F(OscillatorDspTest, LineWidthNegative)
{
    Oscillator osc;
    osc.setLineWidth(-5.0f);
    EXPECT_EQ(osc.getLineWidth(), Oscillator::MIN_LINE_WIDTH);
}

TEST_F(OscillatorDspTest, LineWidthDefault)
{
    Oscillator osc;
    EXPECT_EQ(osc.getLineWidth(), Oscillator::DEFAULT_LINE_WIDTH);
}
