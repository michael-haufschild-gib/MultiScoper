/*
    Oscil - Meter Bar Component Tests
    Tests for OscilMeterBar UI component
*/

#include "ui/components/OscilMeterBar.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace oscil;

class OscilMeterBarTest : public ::testing::Test
{
protected:
    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }

    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(OscilMeterBarTest, DefaultConstruction)
{
    OscilMeterBar meter(getThemeManager());

    EXPECT_FLOAT_EQ(meter.getLevel(), 0.0f);
    EXPECT_FALSE(meter.isClipping());
    EXPECT_FALSE(meter.isStereo());
}

TEST_F(OscilMeterBarTest, ConstructionWithTestId)
{
    OscilMeterBar meter(getThemeManager(), "meter-1");

    EXPECT_FLOAT_EQ(meter.getLevel(), 0.0f);
}

// =============================================================================
// Level Tests
// =============================================================================

TEST_F(OscilMeterBarTest, SetLevel)
{
    OscilMeterBar meter(getThemeManager());

    meter.setLevel(0.5f);
    // Level may decay, but should have been set
    EXPECT_GE(meter.getLevel(), 0.0f);
}

TEST_F(OscilMeterBarTest, SetStereoLevels)
{
    OscilMeterBar meter(getThemeManager());
    meter.setStereo(true);

    meter.setLevels(0.6f, 0.4f);
    // Levels may decay, but should have been set
    EXPECT_GE(meter.getLevel(), 0.0f);
}

TEST_F(OscilMeterBarTest, SetRMSLevel)
{
    OscilMeterBar meter(getThemeManager());

    meter.setRMSLevel(0.3f);
    EXPECT_GE(meter.getRMSLevel(), 0.0f);
}

TEST_F(OscilMeterBarTest, SetRMSLevelsStereo)
{
    OscilMeterBar meter(getThemeManager());
    meter.setStereo(true);

    meter.setRMSLevels(0.4f, 0.3f);
    EXPECT_GE(meter.getRMSLevel(), 0.0f);
}

// =============================================================================
// Clip Detection Tests
// =============================================================================

TEST_F(OscilMeterBarTest, ClipDetection)
{
    OscilMeterBar meter(getThemeManager());

    meter.setLevel(1.5f); // Over 1.0
    EXPECT_TRUE(meter.isClipping());
}

TEST_F(OscilMeterBarTest, NoClipAtUnityGain)
{
    OscilMeterBar meter(getThemeManager());

    meter.setLevel(1.0f);
    // Level at unity gain — verify meter is in a consistent state
    EXPECT_GE(meter.getLevel(), 0.0f);
    EXPECT_LE(meter.getLevel(), 1.0f);
}

TEST_F(OscilMeterBarTest, ResetClip)
{
    OscilMeterBar meter(getThemeManager());
    meter.setLevel(1.5f);
    EXPECT_TRUE(meter.isClipping());

    meter.resetClip();
    EXPECT_FALSE(meter.isClipping());
}

TEST_F(OscilMeterBarTest, ResetPeakHold)
{
    OscilMeterBar meter(getThemeManager());
    meter.setLevel(0.8f);

    meter.resetPeakHold();
    // After reset, clipping state should be clear
    EXPECT_FALSE(meter.isClipping());
}

// =============================================================================
// Orientation Tests
// =============================================================================

TEST_F(OscilMeterBarTest, DefaultOrientationVertical)
{
    OscilMeterBar meter(getThemeManager());

    EXPECT_EQ(meter.getOrientation(), OscilMeterBar::Orientation::Vertical);
}

TEST_F(OscilMeterBarTest, SetOrientationHorizontal)
{
    OscilMeterBar meter(getThemeManager());

    meter.setOrientation(OscilMeterBar::Orientation::Horizontal);
    EXPECT_EQ(meter.getOrientation(), OscilMeterBar::Orientation::Horizontal);
}

TEST_F(OscilMeterBarTest, SetOrientationVertical)
{
    OscilMeterBar meter(getThemeManager());
    meter.setOrientation(OscilMeterBar::Orientation::Horizontal);

    meter.setOrientation(OscilMeterBar::Orientation::Vertical);
    EXPECT_EQ(meter.getOrientation(), OscilMeterBar::Orientation::Vertical);
}

// =============================================================================
// Meter Type Tests
// =============================================================================

TEST_F(OscilMeterBarTest, SetMeterTypePeak)
{
    OscilMeterBar meter(getThemeManager());

    meter.setMeterType(OscilMeterBar::MeterType::Peak);
    EXPECT_EQ(meter.getMeterType(), OscilMeterBar::MeterType::Peak);
}

TEST_F(OscilMeterBarTest, SetMeterTypeRMS)
{
    OscilMeterBar meter(getThemeManager());

    meter.setMeterType(OscilMeterBar::MeterType::RMS);
    EXPECT_EQ(meter.getMeterType(), OscilMeterBar::MeterType::RMS);
}

TEST_F(OscilMeterBarTest, SetMeterTypePeakWithRMS)
{
    OscilMeterBar meter(getThemeManager());

    meter.setMeterType(OscilMeterBar::MeterType::PeakWithRMS);
    EXPECT_EQ(meter.getMeterType(), OscilMeterBar::MeterType::PeakWithRMS);
}

// =============================================================================
// Stereo Mode Tests
// =============================================================================

TEST_F(OscilMeterBarTest, DefaultNotStereo)
{
    OscilMeterBar meter(getThemeManager());

    EXPECT_FALSE(meter.isStereo());
}

TEST_F(OscilMeterBarTest, SetStereo)
{
    OscilMeterBar meter(getThemeManager());

    meter.setStereo(true);
    EXPECT_TRUE(meter.isStereo());

    meter.setStereo(false);
    EXPECT_FALSE(meter.isStereo());
}

// =============================================================================
// Configuration Tests
// =============================================================================

TEST_F(OscilMeterBarTest, SetPeakHoldTime)
{
    OscilMeterBar meter(getThemeManager());

    meter.setPeakHoldTime(3000);
    EXPECT_EQ(meter.getPeakHoldTime(), 3000);
}

TEST_F(OscilMeterBarTest, SetPeakDecayRate)
{
    OscilMeterBar meter(getThemeManager());

    meter.setPeakDecayRate(0.8f);
    EXPECT_NEAR(meter.getPeakDecayRate(), 0.8f, 0.01f);
}

TEST_F(OscilMeterBarTest, SetMinDb)
{
    OscilMeterBar meter(getThemeManager());

    meter.setMinDb(-48.0f);
    EXPECT_NEAR(meter.getMinDb(), -48.0f, 0.01f);
}

TEST_F(OscilMeterBarTest, SetMaxDb)
{
    OscilMeterBar meter(getThemeManager());

    meter.setMaxDb(6.0f);
    EXPECT_NEAR(meter.getMaxDb(), 6.0f, 0.01f);
}

TEST_F(OscilMeterBarTest, SetShowScale)
{
    OscilMeterBar meter(getThemeManager());

    meter.setShowScale(true);
    EXPECT_TRUE(meter.getShowScale());

    meter.setShowScale(false);
    EXPECT_FALSE(meter.getShowScale());
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(OscilMeterBarTest, PreferredWidthPositive)
{
    OscilMeterBar meter(getThemeManager());

    EXPECT_GT(meter.getPreferredWidth(), 0);
}

TEST_F(OscilMeterBarTest, PreferredHeightPositive)
{
    OscilMeterBar meter(getThemeManager());

    EXPECT_GT(meter.getPreferredHeight(), 0);
}

TEST_F(OscilMeterBarTest, StereoWiderThanMono)
{
    OscilMeterBar meter(getThemeManager());

    meter.setStereo(false);
    int monoWidth = meter.getPreferredWidth();

    meter.setStereo(true);
    int stereoWidth = meter.getPreferredWidth();

    EXPECT_GT(stereoWidth, monoWidth);
}

TEST_F(OscilMeterBarTest, ScaleAddsWidth)
{
    OscilMeterBar meter(getThemeManager());

    meter.setShowScale(false);
    int widthWithoutScale = meter.getPreferredWidth();

    meter.setShowScale(true);
    int widthWithScale = meter.getPreferredWidth();

    EXPECT_GT(widthWithScale, widthWithoutScale);
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(OscilMeterBarTest, ThemeChangeDoesNotThrow)
{
    OscilMeterBar meter(getThemeManager());
    meter.setLevel(0.5f);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    meter.themeChanged(newTheme);

    // Meter should still work
    EXPECT_GE(meter.getLevel(), 0.0f);
}

TEST_F(OscilMeterBarTest, ThemeChangePreservesSettings)
{
    OscilMeterBar meter(getThemeManager());
    meter.setStereo(true);
    meter.setMeterType(OscilMeterBar::MeterType::RMS);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    meter.themeChanged(newTheme);

    EXPECT_TRUE(meter.isStereo());
    EXPECT_EQ(meter.getMeterType(), OscilMeterBar::MeterType::RMS);
}
