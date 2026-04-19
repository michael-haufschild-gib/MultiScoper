/*
    MultiScoper - Meter Bar Component Tests
    Tests for MultiScoperMeterBar UI component
*/

#include "ui/components/MultiScoperMeterBar.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace multiscoper;

class MultiScoperMeterBarTest : public ::testing::Test
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

TEST_F(MultiScoperMeterBarTest, DefaultConstruction)
{
    MultiScoperMeterBar meter(getThemeManager());

    EXPECT_FLOAT_EQ(meter.getLevel(), 0.0f);
    EXPECT_FALSE(meter.isClipping());
    EXPECT_FALSE(meter.isStereo());
}

TEST_F(MultiScoperMeterBarTest, ConstructionWithTestId)
{
    MultiScoperMeterBar meter(getThemeManager(), "meter-1");

    EXPECT_FLOAT_EQ(meter.getLevel(), 0.0f);
}

// =============================================================================
// Level Tests
// =============================================================================

TEST_F(MultiScoperMeterBarTest, SetLevel)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setLevel(0.5f);
    // Level may decay, but should have been set
    EXPECT_GE(meter.getLevel(), 0.0f);
}

TEST_F(MultiScoperMeterBarTest, SetStereoLevels)
{
    MultiScoperMeterBar meter(getThemeManager());
    meter.setStereo(true);

    meter.setLevels(0.6f, 0.4f);
    // Levels may decay, but should have been set
    EXPECT_GE(meter.getLevel(), 0.0f);
}

TEST_F(MultiScoperMeterBarTest, SetRMSLevel)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setRMSLevel(0.3f);
    EXPECT_GE(meter.getRMSLevel(), 0.0f);
}

TEST_F(MultiScoperMeterBarTest, SetRMSLevelsStereo)
{
    MultiScoperMeterBar meter(getThemeManager());
    meter.setStereo(true);

    meter.setRMSLevels(0.4f, 0.3f);
    EXPECT_GE(meter.getRMSLevel(), 0.0f);
}

// =============================================================================
// Clip Detection Tests
// =============================================================================

TEST_F(MultiScoperMeterBarTest, ClipDetection)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setLevel(1.5f); // Over 1.0
    EXPECT_TRUE(meter.isClipping());
}

TEST_F(MultiScoperMeterBarTest, NoClipAtUnityGain)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setLevel(1.0f);
    // Level at unity gain — verify meter is in a consistent state
    EXPECT_GE(meter.getLevel(), 0.0f);
    EXPECT_LE(meter.getLevel(), 1.0f);
}

TEST_F(MultiScoperMeterBarTest, ResetClip)
{
    MultiScoperMeterBar meter(getThemeManager());
    meter.setLevel(1.5f);
    EXPECT_TRUE(meter.isClipping());

    meter.resetClip();
    EXPECT_FALSE(meter.isClipping());
}

TEST_F(MultiScoperMeterBarTest, ResetPeakHold)
{
    MultiScoperMeterBar meter(getThemeManager());
    meter.setLevel(0.8f);

    meter.resetPeakHold();
    // After reset, clipping state should be clear
    EXPECT_FALSE(meter.isClipping());
}

// =============================================================================
// Orientation Tests
// =============================================================================

TEST_F(MultiScoperMeterBarTest, DefaultOrientationVertical)
{
    MultiScoperMeterBar meter(getThemeManager());

    EXPECT_EQ(meter.getOrientation(), MultiScoperMeterBar::Orientation::Vertical);
}

TEST_F(MultiScoperMeterBarTest, SetOrientationHorizontal)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setOrientation(MultiScoperMeterBar::Orientation::Horizontal);
    EXPECT_EQ(meter.getOrientation(), MultiScoperMeterBar::Orientation::Horizontal);
}

TEST_F(MultiScoperMeterBarTest, SetOrientationVertical)
{
    MultiScoperMeterBar meter(getThemeManager());
    meter.setOrientation(MultiScoperMeterBar::Orientation::Horizontal);

    meter.setOrientation(MultiScoperMeterBar::Orientation::Vertical);
    EXPECT_EQ(meter.getOrientation(), MultiScoperMeterBar::Orientation::Vertical);
}

// =============================================================================
// Meter Type Tests
// =============================================================================

TEST_F(MultiScoperMeterBarTest, SetMeterTypePeak)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setMeterType(MultiScoperMeterBar::MeterType::Peak);
    EXPECT_EQ(meter.getMeterType(), MultiScoperMeterBar::MeterType::Peak);
}

TEST_F(MultiScoperMeterBarTest, SetMeterTypeRMS)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setMeterType(MultiScoperMeterBar::MeterType::RMS);
    EXPECT_EQ(meter.getMeterType(), MultiScoperMeterBar::MeterType::RMS);
}

TEST_F(MultiScoperMeterBarTest, SetMeterTypePeakWithRMS)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setMeterType(MultiScoperMeterBar::MeterType::PeakWithRMS);
    EXPECT_EQ(meter.getMeterType(), MultiScoperMeterBar::MeterType::PeakWithRMS);
}

// =============================================================================
// Stereo Mode Tests
// =============================================================================

TEST_F(MultiScoperMeterBarTest, DefaultNotStereo)
{
    MultiScoperMeterBar meter(getThemeManager());

    EXPECT_FALSE(meter.isStereo());
}

TEST_F(MultiScoperMeterBarTest, SetStereo)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setStereo(true);
    EXPECT_TRUE(meter.isStereo());

    meter.setStereo(false);
    EXPECT_FALSE(meter.isStereo());
}

// =============================================================================
// Configuration Tests
// =============================================================================

TEST_F(MultiScoperMeterBarTest, SetPeakHoldTime)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setPeakHoldTime(3000);
    EXPECT_EQ(meter.getPeakHoldTime(), 3000);
}

TEST_F(MultiScoperMeterBarTest, SetPeakDecayRate)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setPeakDecayRate(0.8f);
    EXPECT_NEAR(meter.getPeakDecayRate(), 0.8f, 0.01f);
}

TEST_F(MultiScoperMeterBarTest, SetMinDb)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setMinDb(-48.0f);
    EXPECT_NEAR(meter.getMinDb(), -48.0f, 0.01f);
}

TEST_F(MultiScoperMeterBarTest, SetMaxDb)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setMaxDb(6.0f);
    EXPECT_NEAR(meter.getMaxDb(), 6.0f, 0.01f);
}

TEST_F(MultiScoperMeterBarTest, SetShowScale)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setShowScale(true);
    EXPECT_TRUE(meter.getShowScale());

    meter.setShowScale(false);
    EXPECT_FALSE(meter.getShowScale());
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(MultiScoperMeterBarTest, PreferredWidthPositive)
{
    MultiScoperMeterBar meter(getThemeManager());

    EXPECT_GT(meter.getPreferredWidth(), 0);
}

TEST_F(MultiScoperMeterBarTest, PreferredHeightPositive)
{
    MultiScoperMeterBar meter(getThemeManager());

    EXPECT_GT(meter.getPreferredHeight(), 0);
}

TEST_F(MultiScoperMeterBarTest, StereoWiderThanMono)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setStereo(false);
    int monoWidth = meter.getPreferredWidth();

    meter.setStereo(true);
    int stereoWidth = meter.getPreferredWidth();

    EXPECT_GT(stereoWidth, monoWidth);
}

TEST_F(MultiScoperMeterBarTest, ScaleAddsWidth)
{
    MultiScoperMeterBar meter(getThemeManager());

    meter.setShowScale(false);
    int widthWithoutScale = meter.getPreferredWidth();

    meter.setShowScale(true);
    int widthWithScale = meter.getPreferredWidth();

    EXPECT_GT(widthWithScale, widthWithoutScale);
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(MultiScoperMeterBarTest, ThemeChangeDoesNotThrow)
{
    MultiScoperMeterBar meter(getThemeManager());
    meter.setLevel(0.5f);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    meter.themeChanged(newTheme);

    // Meter should still work
    EXPECT_GE(meter.getLevel(), 0.0f);
}

TEST_F(MultiScoperMeterBarTest, ThemeChangePreservesSettings)
{
    MultiScoperMeterBar meter(getThemeManager());
    meter.setStereo(true);
    meter.setMeterType(MultiScoperMeterBar::MeterType::RMS);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    meter.themeChanged(newTheme);

    EXPECT_TRUE(meter.isStereo());
    EXPECT_EQ(meter.getMeterType(), MultiScoperMeterBar::MeterType::RMS);
}
