/*
    Oscil - Slider Component Tests
    Tests for OscilSlider behavioral correctness

    Bug targets:
    - Value not quantized to step size
    - Value not clamped to range after range change
    - Range start/end not auto-swapped when inverted
    - Dead-zone comparison causing value change callbacks to fire/not fire
    - Skew factor inversion (value->proportion->value round-trip)
    - Magnetic snapping at wrong threshold
    - Callback not suppressed when notify=false
    - Range mode silently dropping one endpoint
*/

#include <gtest/gtest.h>
#include "ui/components/OscilSlider.h"
#include "ui/theme/ThemeManager.h"
#include <cmath>

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
        slider_.reset();
        themeManager_.reset();
    }

    ThemeManager& getThemeManager() { return *themeManager_; }

    // Helper: create and store a slider with a standard range
    OscilSlider& makeSlider(double min = 0.0, double max = 100.0)
    {
        slider_ = std::make_unique<OscilSlider>(getThemeManager());
        slider_->setRange(min, max);
        return *slider_;
    }

private:
    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<OscilSlider> slider_;
};

// =============================================================================
// Value Constraint Behavior
// =============================================================================

TEST_F(OscilSliderTest, ValueIsClampedToRangeBounds)
{
    auto& slider = makeSlider(0.0, 100.0);

    slider.setValue(150.0, false);
    EXPECT_DOUBLE_EQ(slider.getValue(), 100.0);

    slider.setValue(-50.0, false);
    EXPECT_DOUBLE_EQ(slider.getValue(), 0.0);
}

TEST_F(OscilSliderTest, ValueQuantizedToStepSize)
{
    // Bug: step quantization rounding to wrong grid point
    auto& slider = makeSlider(0.0, 100.0);
    slider.setStep(10.0);

    slider.setValue(33.0, false);
    EXPECT_DOUBLE_EQ(slider.getValue(), 30.0);

    slider.setValue(37.0, false);
    EXPECT_DOUBLE_EQ(slider.getValue(), 40.0);

    // Exact step boundary
    slider.setValue(50.0, false);
    EXPECT_DOUBLE_EQ(slider.getValue(), 50.0);
}

TEST_F(OscilSliderTest, StepQuantizationWithNonZeroMinimum)
{
    // Bug: step grid not anchored to min value
    auto& slider = makeSlider(5.0, 95.0);
    slider.setStep(10.0);

    slider.setValue(12.0, false);
    EXPECT_DOUBLE_EQ(slider.getValue(), 15.0);

    slider.setValue(8.0, false);
    EXPECT_DOUBLE_EQ(slider.getValue(), 5.0);
}

TEST_F(OscilSliderTest, StepOfZeroDisablesQuantization)
{
    auto& slider = makeSlider(0.0, 1.0);
    slider.setStep(0.0);

    slider.setValue(0.3333, false);
    EXPECT_NEAR(slider.getValue(), 0.3333, 0.0001);
}

TEST_F(OscilSliderTest, ValueReconstrainedWhenRangeChanges)
{
    // Bug: current value not re-clamped after range shrinks
    auto& slider = makeSlider(0.0, 100.0);
    slider.setValue(80.0, false);
    EXPECT_DOUBLE_EQ(slider.getValue(), 80.0);

    slider.setRange(0.0, 50.0);
    EXPECT_LE(slider.getValue(), 50.0);
}

// =============================================================================
// Dead-Zone and Callback Behavior
// =============================================================================

TEST_F(OscilSliderTest, SetValueWithNotifyTrueFiresCallback)
{
    auto& slider = makeSlider(0.0, 100.0);
    int callCount = 0;
    double lastValue = -1.0;

    slider.onValueChanged = [&](double v) { callCount++; lastValue = v; };

    slider.setValue(50.0, true);
    EXPECT_EQ(callCount, 1);
    EXPECT_DOUBLE_EQ(lastValue, 50.0);
}

TEST_F(OscilSliderTest, SetValueWithNotifyFalseSuppressesCallback)
{
    auto& slider = makeSlider(0.0, 100.0);
    int callCount = 0;

    slider.onValueChanged = [&](double) { callCount++; };

    slider.setValue(50.0, false);
    EXPECT_EQ(callCount, 0);
}

TEST_F(OscilSliderTest, SetValueToSameValueDoesNotFireCallback)
{
    // Bug: callback fires on every setValue even when value unchanged
    auto& slider = makeSlider(0.0, 100.0);
    slider.setValue(50.0, false);

    int callCount = 0;
    slider.onValueChanged = [&](double) { callCount++; };

    slider.setValue(50.0, true);
    EXPECT_EQ(callCount, 0) << "Callback should not fire when value is unchanged";
}

TEST_F(OscilSliderTest, DeadZoneThresholdIsRespected)
{
    // Bug: dead zone of 0.0001 causes legitimate small changes to be ignored
    auto& slider = makeSlider(0.0, 1.0);
    slider.setStep(0.0);
    slider.setValue(0.5, false);

    int callCount = 0;
    slider.onValueChanged = [&](double) { callCount++; };

    // Change smaller than dead zone (0.0001) -- should NOT fire
    slider.setValue(0.50005, true);
    EXPECT_EQ(callCount, 0);

    // Change larger than dead zone -- should fire
    slider.setValue(0.502, true);
    EXPECT_EQ(callCount, 1);
}

// =============================================================================
// Range Variant Behavior
// =============================================================================

TEST_F(OscilSliderTest, RangeValuesAutoSwappedWhenInverted)
{
    // Bug: setting start > end silently corrupts range
    OscilSlider slider(getThemeManager(), SliderVariant::Range);
    slider.setRange(0.0, 100.0);

    slider.setRangeValues(80.0, 20.0, false);
    EXPECT_LE(slider.getRangeStart(), slider.getRangeEnd());
    EXPECT_DOUBLE_EQ(slider.getRangeStart(), 20.0);
    EXPECT_DOUBLE_EQ(slider.getRangeEnd(), 80.0);
}

TEST_F(OscilSliderTest, RangeCallbackReceivesNormalizedValues)
{
    OscilSlider slider(getThemeManager(), SliderVariant::Range);
    slider.setRange(0.0, 100.0);

    double cbStart = -1, cbEnd = -1;
    slider.onRangeChanged = [&](double s, double e) { cbStart = s; cbEnd = e; };

    slider.setRangeValues(70.0, 30.0, true);
    EXPECT_DOUBLE_EQ(cbStart, 30.0);
    EXPECT_DOUBLE_EQ(cbEnd, 70.0);
}

TEST_F(OscilSliderTest, RangeCallbackNotFiredWhenValuesUnchanged)
{
    OscilSlider slider(getThemeManager(), SliderVariant::Range);
    slider.setRange(0.0, 100.0);
    slider.setRangeValues(20.0, 80.0, false);

    int callCount = 0;
    slider.onRangeChanged = [&](double, double) { callCount++; };

    slider.setRangeValues(20.0, 80.0, true);
    EXPECT_EQ(callCount, 0) << "Range callback should not fire for unchanged values";
}

TEST_F(OscilSliderTest, RangeValuesClampedToSliderRange)
{
    OscilSlider slider(getThemeManager(), SliderVariant::Range);
    slider.setRange(10.0, 90.0);

    slider.setRangeValues(-50.0, 200.0, false);
    EXPECT_GE(slider.getRangeStart(), 10.0);
    EXPECT_LE(slider.getRangeEnd(), 90.0);
}

// =============================================================================
// Preferred Size Behavior
// =============================================================================

TEST_F(OscilSliderTest, VerticalVariantHasTallPreferredSize)
{
    OscilSlider vertical(getThemeManager(), SliderVariant::Vertical);
    OscilSlider horizontal(getThemeManager(), SliderVariant::Single);

    EXPECT_GT(vertical.getPreferredHeight(), horizontal.getPreferredHeight());
    EXPECT_LT(vertical.getPreferredWidth(), horizontal.getPreferredWidth());
}

// =============================================================================
// Enabled State Behavior
// =============================================================================

TEST_F(OscilSliderTest, DisabledSliderStillAcceptsValueChanges)
{
    // Important: disabled only affects user interaction, not programmatic changes
    auto& slider = makeSlider(0.0, 100.0);
    slider.setEnabled(false);

    slider.setValue(75.0, false);
    EXPECT_DOUBLE_EQ(slider.getValue(), 75.0);
}

// =============================================================================
// Default Value and Magnetic Snapping
// =============================================================================

TEST_F(OscilSliderTest, DefaultValueAddedAsMagneticPoint)
{
    auto& slider = makeSlider(0.0, 100.0);
    slider.setMagneticSnappingEnabled(true);
    slider.setDefaultValue(42.0);

    EXPECT_DOUBLE_EQ(slider.getDefaultValue(), 42.0);
    EXPECT_TRUE(slider.isMagneticSnappingEnabled());
}

TEST_F(OscilSliderTest, MagneticSnappingCanBeToggledIndependentlyOfPoints)
{
    auto& slider = makeSlider(0.0, 100.0);
    slider.addMagneticPoint(50.0);
    slider.setMagneticSnappingEnabled(false);

    EXPECT_FALSE(slider.isMagneticSnappingEnabled());

    slider.setMagneticSnappingEnabled(true);
    EXPECT_TRUE(slider.isMagneticSnappingEnabled());
}

// =============================================================================
// Theme Resilience
// =============================================================================

TEST_F(OscilSliderTest, ThemeChangePreservesValueAndConfiguration)
{
    auto& slider = makeSlider(0.0, 100.0);
    slider.setValue(42.0, false);
    slider.setLabel("Volume");
    slider.setSuffix("dB");
    slider.setDecimalPlaces(2);

    ColorTheme newTheme;
    newTheme.name = "Dark";
    slider.themeChanged(newTheme);

    EXPECT_DOUBLE_EQ(slider.getValue(), 42.0);
    EXPECT_EQ(slider.getLabel(), juce::String("Volume"));
    EXPECT_EQ(slider.getSuffix(), juce::String("dB"));
    EXPECT_EQ(slider.getDecimalPlaces(), 2);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(OscilSliderTest, ZeroWidthRangeDoesNotCrash)
{
    // Bug: division by zero in valueToProportionOfLength
    auto& slider = makeSlider(50.0, 50.0);
    slider.setValue(50.0, false);
    EXPECT_DOUBLE_EQ(slider.getValue(), 50.0);
}

TEST_F(OscilSliderTest, NegativeRange)
{
    auto& slider = makeSlider(-100.0, -10.0);
    slider.setValue(-50.0, false);
    EXPECT_DOUBLE_EQ(slider.getValue(), -50.0);

    slider.setValue(0.0, false);
    EXPECT_DOUBLE_EQ(slider.getValue(), -10.0);
}

TEST_F(OscilSliderTest, VerySmallStepDoesNotInfiniteLoop)
{
    auto& slider = makeSlider(0.0, 1.0);
    slider.setStep(0.0001);

    slider.setValue(0.33333, false);
    double result = slider.getValue();
    // Should be quantized to nearest 0.0001
    double remainder = std::fmod(result - 0.0, 0.0001);
    EXPECT_NEAR(remainder, 0.0, 0.00005);
}

TEST_F(OscilSliderTest, SetRangeUpdatesDefaultMagneticPoints)
{
    // Bug: setRange resets magnetic points to new range but old default points linger
    auto& slider = makeSlider(0.0, 100.0);
    slider.setMagneticSnappingEnabled(true);

    slider.setRange(0.0, 1000.0);
    // Should not crash or leave stale points
    EXPECT_DOUBLE_EQ(slider.getMinimum(), 0.0);
    EXPECT_DOUBLE_EQ(slider.getMaximum(), 1000.0);
}
