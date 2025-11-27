/*
    Oscil - Slider Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilSlider.h"

using namespace oscil;

class OscilSliderTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction
TEST_F(OscilSliderTest, DefaultConstruction)
{
    OscilSlider slider;

    EXPECT_TRUE(slider.isEnabled());
    EXPECT_EQ(slider.getVariant(), SliderVariant::Linear);
}

// Test: Set value
TEST_F(OscilSliderTest, SetValue)
{
    OscilSlider slider;
    slider.setRange(0.0, 100.0);

    slider.setValue(50.0);
    EXPECT_NEAR(slider.getValue(), 50.0, 0.01);
}

// Test: Set range
TEST_F(OscilSliderTest, SetRange)
{
    OscilSlider slider;
    slider.setRange(10.0, 90.0);

    EXPECT_NEAR(slider.getMinimum(), 10.0, 0.01);
    EXPECT_NEAR(slider.getMaximum(), 90.0, 0.01);
}

// Test: Value clamping
TEST_F(OscilSliderTest, ValueClamping)
{
    OscilSlider slider;
    slider.setRange(0.0, 100.0);

    slider.setValue(150.0);
    EXPECT_LE(slider.getValue(), 100.0);

    slider.setValue(-50.0);
    EXPECT_GE(slider.getValue(), 0.0);
}

// Test: Variant types
TEST_F(OscilSliderTest, VariantTypes)
{
    OscilSlider slider;

    slider.setVariant(SliderVariant::Linear);
    EXPECT_EQ(slider.getVariant(), SliderVariant::Linear);

    slider.setVariant(SliderVariant::Rotary);
    EXPECT_EQ(slider.getVariant(), SliderVariant::Rotary);

    slider.setVariant(SliderVariant::VerticalBar);
    EXPECT_EQ(slider.getVariant(), SliderVariant::VerticalBar);

    slider.setVariant(SliderVariant::ThreeWay);
    EXPECT_EQ(slider.getVariant(), SliderVariant::ThreeWay);
}

// Test: Enabled/disabled state
TEST_F(OscilSliderTest, EnabledState)
{
    OscilSlider slider;

    EXPECT_TRUE(slider.isEnabled());

    slider.setEnabled(false);
    EXPECT_FALSE(slider.isEnabled());

    slider.setEnabled(true);
    EXPECT_TRUE(slider.isEnabled());
}

// Test: Label
TEST_F(OscilSliderTest, Label)
{
    OscilSlider slider;
    slider.setLabel("Volume");

    EXPECT_EQ(slider.getLabel(), juce::String("Volume"));
}

// Test: Unit text
TEST_F(OscilSliderTest, UnitText)
{
    OscilSlider slider;
    slider.setUnitText("dB");

    EXPECT_EQ(slider.getUnitText(), juce::String("dB"));
}

// Test: Default value
TEST_F(OscilSliderTest, DefaultValue)
{
    OscilSlider slider;
    slider.setRange(0.0, 100.0);
    slider.setDefaultValue(50.0);

    EXPECT_NEAR(slider.getDefaultValue(), 50.0, 0.01);
}

// Test: Reset to default
TEST_F(OscilSliderTest, ResetToDefault)
{
    OscilSlider slider;
    slider.setRange(0.0, 100.0);
    slider.setDefaultValue(50.0);
    slider.setValue(75.0);

    slider.resetToDefault();
    EXPECT_NEAR(slider.getValue(), 50.0, 0.01);
}

// Test: Skew factor
TEST_F(OscilSliderTest, SkewFactor)
{
    OscilSlider slider;
    slider.setSkewFactor(0.5);

    EXPECT_NEAR(slider.getSkewFactor(), 0.5, 0.01);
}

// Test: Step size
TEST_F(OscilSliderTest, StepSize)
{
    OscilSlider slider;
    slider.setRange(0.0, 100.0);
    slider.setStepSize(10.0);

    slider.setValue(25.0);
    // With step size of 10, value should snap
    double value = slider.getValue();
    double remainder = std::fmod(value, 10.0);
    EXPECT_NEAR(remainder, 0.0, 0.01);
}

// Test: Magnetic snap points
TEST_F(OscilSliderTest, MagneticSnapPoints)
{
    OscilSlider slider;
    slider.setRange(0.0, 100.0);

    std::vector<double> snapPoints = {0.0, 25.0, 50.0, 75.0, 100.0};
    slider.setMagneticSnapPoints(snapPoints);

    // Should store snap points
    EXPECT_EQ(slider.getMagneticSnapPoints().size(), 5);
}

// Test: Fine/coarse control sensitivity
TEST_F(OscilSliderTest, ControlSensitivity)
{
    OscilSlider slider;

    slider.setFineSensitivity(0.1);
    EXPECT_NEAR(slider.getFineSensitivity(), 0.1, 0.01);

    slider.setCoarseSensitivity(2.0);
    EXPECT_NEAR(slider.getCoarseSensitivity(), 2.0, 0.01);
}

// Test: Value changed callback
TEST_F(OscilSliderTest, ValueChangedCallback)
{
    OscilSlider slider;
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

// Test: Drag started/ended callbacks
TEST_F(OscilSliderTest, DragCallbacks)
{
    OscilSlider slider;

    bool dragStarted = false;
    bool dragEnded = false;

    slider.onDragStart = [&dragStarted]() {
        dragStarted = true;
    };

    slider.onDragEnd = [&dragEnded]() {
        dragEnded = true;
    };

    // These would be triggered by mouse interaction
    // Verifies callbacks exist and can be set
    EXPECT_FALSE(dragStarted);
    EXPECT_FALSE(dragEnded);
}

// Test: Preferred size
TEST_F(OscilSliderTest, PreferredSize)
{
    OscilSlider slider;

    int width = slider.getPreferredWidth();
    int height = slider.getPreferredHeight();

    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

// Test: Show value label
TEST_F(OscilSliderTest, ShowValueLabel)
{
    OscilSlider slider;

    slider.setShowValueLabel(true);
    EXPECT_TRUE(slider.getShowValueLabel());

    slider.setShowValueLabel(false);
    EXPECT_FALSE(slider.getShowValueLabel());
}

// Test: Text from value function
TEST_F(OscilSliderTest, TextFromValueFunction)
{
    OscilSlider slider;
    slider.setRange(0.0, 100.0);

    slider.setTextFromValueFunction([](double value) {
        return juce::String(value, 1) + " dB";
    });

    slider.setValue(50.0);
    juce::String text = slider.getValueText();
    EXPECT_TRUE(text.contains("50"));
}

// Test: Theme changes
TEST_F(OscilSliderTest, ThemeChanges)
{
    OscilSlider slider;
    slider.setRange(0.0, 100.0);
    slider.setValue(50.0);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    slider.themeChanged(newTheme);

    // Value should be preserved
    EXPECT_NEAR(slider.getValue(), 50.0, 0.01);
}

// Test: Accessibility
TEST_F(OscilSliderTest, Accessibility)
{
    OscilSlider slider;
    slider.setLabel("Test Slider");

    auto* handler = slider.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);

    EXPECT_EQ(handler->getRole(), juce::AccessibilityRole::slider);
}

// Test: Focus handling
TEST_F(OscilSliderTest, FocusHandling)
{
    OscilSlider slider;
    EXPECT_TRUE(slider.getWantsKeyboardFocus());
}

// Test: Bipolar mode
TEST_F(OscilSliderTest, BipolarMode)
{
    OscilSlider slider;
    slider.setRange(-100.0, 100.0);

    slider.setBipolar(true);
    EXPECT_TRUE(slider.isBipolar());

    slider.setBipolar(false);
    EXPECT_FALSE(slider.isBipolar());
}
