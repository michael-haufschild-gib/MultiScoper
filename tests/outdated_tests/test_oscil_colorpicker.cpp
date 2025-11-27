/*
    Oscil - Color Picker Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilColorPicker.h"

using namespace oscil;

class OscilColorPickerTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction
TEST_F(OscilColorPickerTest, DefaultConstruction)
{
    OscilColorPicker picker;

    // Should have a default color
    EXPECT_NE(picker.getColor().getARGB(), 0);
}

// Test: Set color
TEST_F(OscilColorPickerTest, SetColor)
{
    OscilColorPicker picker;

    picker.setColor(juce::Colours::purple);
    EXPECT_EQ(picker.getColor().getARGB(), juce::Colours::purple.getARGB());
}

// Test: Set original color
TEST_F(OscilColorPickerTest, SetOriginalColor)
{
    OscilColorPicker picker;

    picker.setOriginalColor(juce::Colours::red);
    EXPECT_EQ(picker.getOriginalColor().getARGB(), juce::Colours::red.getARGB());
}

// Test: Picker mode
TEST_F(OscilColorPickerTest, PickerMode)
{
    OscilColorPicker picker;

    picker.setMode(OscilColorPicker::Mode::Square);
    EXPECT_EQ(picker.getMode(), OscilColorPicker::Mode::Square);

    picker.setMode(OscilColorPicker::Mode::Wheel);
    EXPECT_EQ(picker.getMode(), OscilColorPicker::Mode::Wheel);
}

// Test: Show alpha slider
TEST_F(OscilColorPickerTest, ShowAlphaSlider)
{
    OscilColorPicker picker;

    picker.setShowAlpha(true);
    EXPECT_TRUE(picker.getShowAlpha());

    picker.setShowAlpha(false);
    EXPECT_FALSE(picker.getShowAlpha());
}

// Test: Show hex input
TEST_F(OscilColorPickerTest, ShowHexInput)
{
    OscilColorPicker picker;

    picker.setShowHexInput(true);
    EXPECT_TRUE(picker.getShowHexInput());

    picker.setShowHexInput(false);
    EXPECT_FALSE(picker.getShowHexInput());
}

// Test: Show preview
TEST_F(OscilColorPickerTest, ShowPreview)
{
    OscilColorPicker picker;

    picker.setShowPreview(true);
    EXPECT_TRUE(picker.getShowPreview());

    picker.setShowPreview(false);
    EXPECT_FALSE(picker.getShowPreview());
}

// Test: Get HSV values
TEST_F(OscilColorPickerTest, GetHSVValues)
{
    OscilColorPicker picker;
    picker.setColor(juce::Colours::red);

    float hue = picker.getHue();
    float saturation = picker.getSaturation();
    float brightness = picker.getBrightness();

    EXPECT_GE(hue, 0.0f);
    EXPECT_LE(hue, 1.0f);
    EXPECT_GE(saturation, 0.0f);
    EXPECT_LE(saturation, 1.0f);
    EXPECT_GE(brightness, 0.0f);
    EXPECT_LE(brightness, 1.0f);
}

// Test: Set HSV values
TEST_F(OscilColorPickerTest, SetHSVValues)
{
    OscilColorPicker picker;

    picker.setHue(0.5f);
    EXPECT_NEAR(picker.getHue(), 0.5f, 0.01f);

    picker.setSaturation(0.75f);
    EXPECT_NEAR(picker.getSaturation(), 0.75f, 0.01f);

    picker.setBrightness(0.9f);
    EXPECT_NEAR(picker.getBrightness(), 0.9f, 0.01f);
}

// Test: Set alpha
TEST_F(OscilColorPickerTest, SetAlpha)
{
    OscilColorPicker picker;

    picker.setAlpha(0.5f);
    EXPECT_NEAR(picker.getAlpha(), 0.5f, 0.01f);
}

// Test: Color changed callback
TEST_F(OscilColorPickerTest, ColorChangedCallback)
{
    OscilColorPicker picker;

    int changeCount = 0;
    juce::Colour lastColor;

    picker.onColorChanged = [&changeCount, &lastColor](juce::Colour color) {
        changeCount++;
        lastColor = color;
    };

    picker.setColor(juce::Colours::green, true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastColor.getARGB(), juce::Colours::green.getARGB());
}

// Test: Color changing callback (while dragging)
TEST_F(OscilColorPickerTest, ColorChangingCallback)
{
    OscilColorPicker picker;

    bool changingCalled = false;
    picker.onColorChanging = [&changingCalled](juce::Colour) {
        changingCalled = true;
    };

    // This would be triggered during drag
    // Just verify callback can be set
    EXPECT_FALSE(changingCalled);
}

// Test: Get hex string
TEST_F(OscilColorPickerTest, GetHexString)
{
    OscilColorPicker picker;
    picker.setColor(juce::Colours::red);

    juce::String hex = picker.getHexString();
    EXPECT_TRUE(hex.startsWith("#") || hex.length() == 6 || hex.length() == 8);
}

// Test: Set from hex string
TEST_F(OscilColorPickerTest, SetFromHexString)
{
    OscilColorPicker picker;

    picker.setFromHexString("#FF0000");
    EXPECT_EQ(picker.getColor().getRed(), 255);
    EXPECT_EQ(picker.getColor().getGreen(), 0);
    EXPECT_EQ(picker.getColor().getBlue(), 0);
}

// Test: Preferred height based on options
TEST_F(OscilColorPickerTest, PreferredHeight)
{
    OscilColorPicker picker;

    picker.setShowAlpha(false);
    picker.setShowPreview(false);
    int heightMinimal = picker.getPreferredHeight();

    picker.setShowAlpha(true);
    picker.setShowPreview(true);
    int heightFull = picker.getPreferredHeight();

    EXPECT_GT(heightFull, heightMinimal);
}

// Test: Theme changes
TEST_F(OscilColorPickerTest, ThemeChanges)
{
    OscilColorPicker picker;
    picker.setColor(juce::Colours::blue);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    picker.themeChanged(newTheme);

    // Color should be preserved
    EXPECT_EQ(picker.getColor().getARGB(), juce::Colours::blue.getARGB());
}

// Test: Accessibility
TEST_F(OscilColorPickerTest, Accessibility)
{
    OscilColorPicker picker;

    auto* handler = picker.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);
}

// Test: Reset to original
TEST_F(OscilColorPickerTest, ResetToOriginal)
{
    OscilColorPicker picker;
    picker.setOriginalColor(juce::Colours::red);
    picker.setColor(juce::Colours::blue);

    picker.resetToOriginal();
    EXPECT_EQ(picker.getColor().getARGB(), juce::Colours::red.getARGB());
}

// Test: Color with alpha
TEST_F(OscilColorPickerTest, ColorWithAlpha)
{
    OscilColorPicker picker;
    picker.setShowAlpha(true);

    auto colorWithAlpha = juce::Colours::blue.withAlpha(0.5f);
    picker.setColor(colorWithAlpha);

    EXPECT_NEAR(picker.getColor().getFloatAlpha(), 0.5f, 0.01f);
}
