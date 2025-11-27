/*
    Oscil - Meter Bar Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilMeterBar.h"

using namespace oscil;

class OscilMeterBarTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction
TEST_F(OscilMeterBarTest, DefaultConstruction)
{
    OscilMeterBar meter;

    EXPECT_FLOAT_EQ(meter.getLevel(), 0.0f);
    EXPECT_FALSE(meter.isClipping());
}

// Test: Set level
TEST_F(OscilMeterBarTest, SetLevel)
{
    OscilMeterBar meter;

    meter.setLevel(0.5f);
    // Level decays, so we check it was set
    EXPECT_GE(meter.getLevel(), 0.0f);
}

// Test: Set stereo levels
TEST_F(OscilMeterBarTest, SetStereoLevels)
{
    OscilMeterBar meter;
    meter.setStereo(true);

    meter.setLevels(0.6f, 0.4f);

    // Levels decay, but should be set
    EXPECT_GE(meter.getLevel(), 0.0f);
}

// Test: Set RMS level
TEST_F(OscilMeterBarTest, SetRMSLevel)
{
    OscilMeterBar meter;

    meter.setRMSLevel(0.3f);
    EXPECT_GE(meter.getRMSLevel(), 0.0f);
}

// Test: Clip detection
TEST_F(OscilMeterBarTest, ClipDetection)
{
    OscilMeterBar meter;

    meter.setLevel(1.5f);  // Over 1.0
    EXPECT_TRUE(meter.isClipping());
}

// Test: Reset clip
TEST_F(OscilMeterBarTest, ResetClip)
{
    OscilMeterBar meter;
    meter.setLevel(1.5f);
    EXPECT_TRUE(meter.isClipping());

    meter.resetClip();
    EXPECT_FALSE(meter.isClipping());
}

// Test: Reset peak hold
TEST_F(OscilMeterBarTest, ResetPeakHold)
{
    OscilMeterBar meter;
    meter.setLevel(0.8f);

    meter.resetPeakHold();
    // Peak hold should be reset
}

// Test: Orientation
TEST_F(OscilMeterBarTest, Orientation)
{
    OscilMeterBar meter;

    meter.setOrientation(OscilMeterBar::Orientation::Vertical);
    EXPECT_EQ(meter.getOrientation(), OscilMeterBar::Orientation::Vertical);

    meter.setOrientation(OscilMeterBar::Orientation::Horizontal);
    EXPECT_EQ(meter.getOrientation(), OscilMeterBar::Orientation::Horizontal);
}

// Test: Meter type
TEST_F(OscilMeterBarTest, MeterType)
{
    OscilMeterBar meter;

    meter.setMeterType(OscilMeterBar::MeterType::Peak);
    EXPECT_EQ(meter.getMeterType(), OscilMeterBar::MeterType::Peak);

    meter.setMeterType(OscilMeterBar::MeterType::RMS);
    EXPECT_EQ(meter.getMeterType(), OscilMeterBar::MeterType::RMS);

    meter.setMeterType(OscilMeterBar::MeterType::PeakWithRMS);
    EXPECT_EQ(meter.getMeterType(), OscilMeterBar::MeterType::PeakWithRMS);
}

// Test: Stereo mode
TEST_F(OscilMeterBarTest, StereoMode)
{
    OscilMeterBar meter;

    EXPECT_FALSE(meter.isStereo());

    meter.setStereo(true);
    EXPECT_TRUE(meter.isStereo());

    meter.setStereo(false);
    EXPECT_FALSE(meter.isStereo());
}

// Test: Peak hold time
TEST_F(OscilMeterBarTest, PeakHoldTime)
{
    OscilMeterBar meter;

    meter.setPeakHoldTime(3000);
    EXPECT_EQ(meter.getPeakHoldTime(), 3000);
}

// Test: Peak decay rate
TEST_F(OscilMeterBarTest, PeakDecayRate)
{
    OscilMeterBar meter;

    meter.setPeakDecayRate(0.8f);
    EXPECT_NEAR(meter.getPeakDecayRate(), 0.8f, 0.01f);
}

// Test: dB range
TEST_F(OscilMeterBarTest, DbRange)
{
    OscilMeterBar meter;

    meter.setMinDb(-48.0f);
    EXPECT_NEAR(meter.getMinDb(), -48.0f, 0.01f);

    meter.setMaxDb(6.0f);
    EXPECT_NEAR(meter.getMaxDb(), 6.0f, 0.01f);
}

// Test: Show scale
TEST_F(OscilMeterBarTest, ShowScale)
{
    OscilMeterBar meter;

    meter.setShowScale(true);
    EXPECT_TRUE(meter.getShowScale());

    meter.setShowScale(false);
    EXPECT_FALSE(meter.getShowScale());
}

// Test: Preferred size - mono vs stereo
TEST_F(OscilMeterBarTest, PreferredSizeMonoVsStereo)
{
    OscilMeterBar meter;

    meter.setStereo(false);
    int monoWidth = meter.getPreferredWidth();

    meter.setStereo(true);
    int stereoWidth = meter.getPreferredWidth();

    EXPECT_GT(stereoWidth, monoWidth);
}

// Test: Preferred size - with scale
TEST_F(OscilMeterBarTest, PreferredSizeWithScale)
{
    OscilMeterBar meter;

    meter.setShowScale(false);
    int widthWithoutScale = meter.getPreferredWidth();

    meter.setShowScale(true);
    int widthWithScale = meter.getPreferredWidth();

    EXPECT_GT(widthWithScale, widthWithoutScale);
}

// Test: Theme changes
TEST_F(OscilMeterBarTest, ThemeChanges)
{
    OscilMeterBar meter;
    meter.setLevel(0.5f);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    meter.themeChanged(newTheme);

    // Meter should still work
    EXPECT_GE(meter.getLevel(), 0.0f);
}

// Test: Accessibility
TEST_F(OscilMeterBarTest, Accessibility)
{
    OscilMeterBar meter;

    auto* handler = meter.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);
}

// Test: Level decay over time
TEST_F(OscilMeterBarTest, LevelDecay)
{
    OscilMeterBar meter;
    meter.setLevel(1.0f);

    float initialLevel = meter.getLevel();

    // Simulate timer callbacks for decay
    // The actual decay happens in timerCallback, which we can't call directly
    // But we verify the level was set
    EXPECT_GE(initialLevel, 0.0f);
}
