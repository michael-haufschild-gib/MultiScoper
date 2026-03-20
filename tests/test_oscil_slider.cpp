/*
    Oscil - Slider Component Tests
    Tests for OscilSlider UI component
*/

#include <gtest/gtest.h>
#include "ui/components/OscilSlider.h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class OscilSliderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        themeManager_ = std::make_unique<ThemeManager>();
    }

    void TearDown() override
    {
        themeManager_.reset();
    }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(OscilSliderTest, DefaultConstruction)
{
    OscilSlider slider(getThemeManager());

    EXPECT_TRUE(slider.isEnabled());
    EXPECT_EQ(slider.getVariant(), SliderVariant::Single);
}

TEST_F(OscilSliderTest, ConstructionWithVariant)
{
    OscilSlider slider(getThemeManager(), SliderVariant::Vertical);

    EXPECT_EQ(slider.getVariant(), SliderVariant::Vertical);
}

TEST_F(OscilSliderTest, ConstructionWithTestId)
{
    OscilSlider slider(getThemeManager(), "slider-1");

    EXPECT_TRUE(slider.isEnabled());
    EXPECT_EQ(slider.getVariant(), SliderVariant::Single);
}

TEST_F(OscilSliderTest, ConstructionWithVariantAndTestId)
{
    OscilSlider slider(getThemeManager(), SliderVariant::Range, "slider-1");

    EXPECT_EQ(slider.getVariant(), SliderVariant::Range);
}

// =============================================================================
// Value Tests
// =============================================================================

TEST_F(OscilSliderTest, SetValue)
{
    OscilSlider slider(getThemeManager());
    slider.setRange(0.0, 100.0);

    slider.setValue(50.0, false);
    EXPECT_NEAR(slider.getValue(), 50.0, 0.01);
}

TEST_F(OscilSliderTest, ValueClamping)
{
    OscilSlider slider(getThemeManager());
    slider.setRange(0.0, 100.0);

    slider.setValue(150.0, false);
    EXPECT_LE(slider.getValue(), 100.0);

    slider.setValue(-50.0, false);
    EXPECT_GE(slider.getValue(), 0.0);
}

TEST_F(OscilSliderTest, SetRange)
{
    OscilSlider slider(getThemeManager());
    slider.setRange(10.0, 90.0);

    EXPECT_NEAR(slider.getMinimum(), 10.0, 0.01);
    EXPECT_NEAR(slider.getMaximum(), 90.0, 0.01);
}

// =============================================================================
// Range Variant Tests
// =============================================================================

TEST_F(OscilSliderTest, SetRangeValues)
{
    OscilSlider slider(getThemeManager(), SliderVariant::Range);
    slider.setRange(0.0, 100.0);

    slider.setRangeValues(20.0, 80.0, false);
    EXPECT_NEAR(slider.getRangeStart(), 20.0, 0.01);
    EXPECT_NEAR(slider.getRangeEnd(), 80.0, 0.01);
}

// =============================================================================
// Variant Tests
// =============================================================================

TEST_F(OscilSliderTest, SetVariantSingle)
{
    OscilSlider slider(getThemeManager());

    slider.setVariant(SliderVariant::Single);
    EXPECT_EQ(slider.getVariant(), SliderVariant::Single);
}

TEST_F(OscilSliderTest, SetVariantRange)
{
    OscilSlider slider(getThemeManager());

    slider.setVariant(SliderVariant::Range);
    EXPECT_EQ(slider.getVariant(), SliderVariant::Range);
}

TEST_F(OscilSliderTest, SetVariantVertical)
{
    OscilSlider slider(getThemeManager());

    slider.setVariant(SliderVariant::Vertical);
    EXPECT_EQ(slider.getVariant(), SliderVariant::Vertical);
}

// =============================================================================
// Configuration Tests
// =============================================================================

TEST_F(OscilSliderTest, SetStep)
{
    OscilSlider slider(getThemeManager());
    slider.setRange(0.0, 100.0);

    slider.setStep(10.0);
    EXPECT_NEAR(slider.getStep(), 10.0, 0.01);
}

TEST_F(OscilSliderTest, SetDefaultValue)
{
    OscilSlider slider(getThemeManager());
    slider.setRange(0.0, 100.0);

    slider.setDefaultValue(50.0);
    EXPECT_NEAR(slider.getDefaultValue(), 50.0, 0.01);
}

TEST_F(OscilSliderTest, SetSkewFactor)
{
    OscilSlider slider(getThemeManager());

    slider.setSkewFactor(0.5);
    EXPECT_NEAR(slider.getSkewFactor(), 0.5, 0.01);
}

TEST_F(OscilSliderTest, SetLabel)
{
    OscilSlider slider(getThemeManager());
    slider.setLabel("Volume");

    EXPECT_EQ(slider.getLabel(), juce::String("Volume"));
}

TEST_F(OscilSliderTest, SetSuffix)
{
    OscilSlider slider(getThemeManager());
    slider.setSuffix("dB");

    EXPECT_EQ(slider.getSuffix(), juce::String("dB"));
}

TEST_F(OscilSliderTest, SetDecimalPlaces)
{
    OscilSlider slider(getThemeManager());

    slider.setDecimalPlaces(2);
    EXPECT_EQ(slider.getDecimalPlaces(), 2);
}

TEST_F(OscilSliderTest, SetShowValueOnHover)
{
    OscilSlider slider(getThemeManager());

    slider.setShowValueOnHover(true);
    EXPECT_TRUE(slider.getShowValueOnHover());

    slider.setShowValueOnHover(false);
    EXPECT_FALSE(slider.getShowValueOnHover());
}

// =============================================================================
// Magnetic Snapping Tests
// =============================================================================

TEST_F(OscilSliderTest, SetMagneticSnappingEnabled)
{
    OscilSlider slider(getThemeManager());

    slider.setMagneticSnappingEnabled(true);
    EXPECT_TRUE(slider.isMagneticSnappingEnabled());

    slider.setMagneticSnappingEnabled(false);
    EXPECT_FALSE(slider.isMagneticSnappingEnabled());
}

TEST_F(OscilSliderTest, SetMagneticPoints)
{
    OscilSlider slider(getThemeManager());
    slider.setRange(0.0, 100.0);
    slider.setMagneticSnappingEnabled(true);

    std::vector<double> points = {0.0, 25.0, 50.0, 75.0, 100.0};
    slider.setMagneticPoints(points);

    EXPECT_TRUE(slider.isMagneticSnappingEnabled());
    EXPECT_NEAR(slider.getMinimum(), 0.0, 0.01);
}

TEST_F(OscilSliderTest, AddMagneticPoint)
{
    OscilSlider slider(getThemeManager());
    slider.setRange(0.0, 100.0);
    slider.setMagneticSnappingEnabled(true);

    slider.addMagneticPoint(50.0);
    EXPECT_TRUE(slider.isMagneticSnappingEnabled());
}

TEST_F(OscilSliderTest, ClearMagneticPoints)
{
    OscilSlider slider(getThemeManager());
    slider.setRange(0.0, 100.0);
    slider.setMagneticSnappingEnabled(true);
    slider.addMagneticPoint(50.0);

    slider.clearMagneticPoints();
    // Magnetic snapping state should still be enabled even with no points
    EXPECT_TRUE(slider.isMagneticSnappingEnabled());
}

// =============================================================================
// Enabled/Disabled Tests
// =============================================================================

TEST_F(OscilSliderTest, SetEnabled)
{
    OscilSlider slider(getThemeManager());

    EXPECT_TRUE(slider.isEnabled());

    slider.setEnabled(false);
    EXPECT_FALSE(slider.isEnabled());

    slider.setEnabled(true);
    EXPECT_TRUE(slider.isEnabled());
}

// =============================================================================
// APVTS Tests
// =============================================================================

TEST_F(OscilSliderTest, DefaultNotAttached)
{
    OscilSlider slider(getThemeManager());

    EXPECT_FALSE(slider.isAttachedToParameter());
}

TEST_F(OscilSliderTest, DetachWhenNotAttached)
{
    OscilSlider slider(getThemeManager());

    // Should not crash
    slider.detachFromParameter();
    EXPECT_FALSE(slider.isAttachedToParameter());
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(OscilSliderTest, OnValueChangedCallback)
{
    OscilSlider slider(getThemeManager());
    slider.setRange(0.0, 100.0);

    int changeCount = 0;
    double lastValue = 0.0;

    slider.onValueChanged = [&changeCount, &lastValue](double value) {
        changeCount++;
        lastValue = value;
    };

    slider.setValue(50.0, true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_NEAR(lastValue, 50.0, 0.01);
}

TEST_F(OscilSliderTest, NoCallbackWhenNotifyFalse)
{
    OscilSlider slider(getThemeManager());
    slider.setRange(0.0, 100.0);

    int changeCount = 0;

    slider.onValueChanged = [&changeCount](double) {
        changeCount++;
    };

    slider.setValue(50.0, false);
    EXPECT_EQ(changeCount, 0);
}

TEST_F(OscilSliderTest, OnRangeChangedCallback)
{
    OscilSlider slider(getThemeManager(), SliderVariant::Range);
    slider.setRange(0.0, 100.0);

    int changeCount = 0;

    slider.onRangeChanged = [&changeCount](double, double) {
        changeCount++;
    };

    slider.setRangeValues(20.0, 80.0, true);
    EXPECT_EQ(changeCount, 1);
}

TEST_F(OscilSliderTest, OnDragStartCallback)
{
    OscilSlider slider(getThemeManager());

    bool dragStarted = false;
    slider.onDragStart = [&dragStarted]() {
        dragStarted = true;
    };

    // Callback would be triggered by mouse interaction
    // Just verify callback can be set
    EXPECT_FALSE(dragStarted);
}

TEST_F(OscilSliderTest, OnDragEndCallback)
{
    OscilSlider slider(getThemeManager());

    bool dragEnded = false;
    slider.onDragEnd = [&dragEnded]() {
        dragEnded = true;
    };

    // Callback would be triggered by mouse interaction
    // Just verify callback can be set
    EXPECT_FALSE(dragEnded);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(OscilSliderTest, PreferredWidthPositive)
{
    OscilSlider slider(getThemeManager());

    int width = slider.getPreferredWidth();
    EXPECT_GT(width, 0);
}

TEST_F(OscilSliderTest, PreferredHeightPositive)
{
    OscilSlider slider(getThemeManager());

    int height = slider.getPreferredHeight();
    EXPECT_GT(height, 0);
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(OscilSliderTest, ThemeChangeDoesNotThrow)
{
    OscilSlider slider(getThemeManager());
    slider.setRange(0.0, 100.0);
    slider.setValue(50.0, false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    slider.themeChanged(newTheme);

    // Value should be preserved
    EXPECT_NEAR(slider.getValue(), 50.0, 0.01);
}

TEST_F(OscilSliderTest, ThemeChangePreservesConfiguration)
{
    OscilSlider slider(getThemeManager());
    slider.setLabel("Volume");
    slider.setSuffix("dB");

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    slider.themeChanged(newTheme);

    EXPECT_EQ(slider.getLabel(), juce::String("Volume"));
    EXPECT_EQ(slider.getSuffix(), juce::String("dB"));
}

// =============================================================================
// Focus Tests
// =============================================================================

TEST_F(OscilSliderTest, WantsKeyboardFocus)
{
    OscilSlider slider(getThemeManager());

    EXPECT_TRUE(slider.getWantsKeyboardFocus());
}
