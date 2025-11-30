/*
    Oscil - Color Picker Component Tests
    Tests for OscilColorPicker UI component
*/

#include <gtest/gtest.h>
#include "ui/components/OscilColorPicker.h"

using namespace oscil;

class OscilColorPickerTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(OscilColorPickerTest, DefaultConstruction)
{
    OscilColorPicker picker;

    // Should have a default color
    EXPECT_NE(picker.getColor().getARGB(), 0u);
}

TEST_F(OscilColorPickerTest, ConstructionWithTestId)
{
    OscilColorPicker picker("colorpicker-1");

    EXPECT_NE(picker.getColor().getARGB(), 0u);
}

// =============================================================================
// Color Tests
// =============================================================================

TEST_F(OscilColorPickerTest, SetColor)
{
    OscilColorPicker picker;

    picker.setColor(juce::Colours::purple, false);
    EXPECT_EQ(picker.getColor().getARGB(), juce::Colours::purple.getARGB());
}

TEST_F(OscilColorPickerTest, SetOriginalColor)
{
    OscilColorPicker picker;

    picker.setOriginalColor(juce::Colours::red);
    EXPECT_EQ(picker.getOriginalColor().getARGB(), juce::Colours::red.getARGB());
}

TEST_F(OscilColorPickerTest, ColorWithAlpha)
{
    OscilColorPicker picker;
    picker.setShowAlpha(true);

    auto colorWithAlpha = juce::Colours::blue.withAlpha(0.5f);
    picker.setColor(colorWithAlpha, false);

    EXPECT_NEAR(picker.getColor().getFloatAlpha(), 0.5f, 0.01f);
}

// =============================================================================
// Mode Tests
// =============================================================================

TEST_F(OscilColorPickerTest, SetModeSquare)
{
    OscilColorPicker picker;

    picker.setMode(OscilColorPicker::Mode::Square);
    EXPECT_EQ(picker.getMode(), OscilColorPicker::Mode::Square);
}

TEST_F(OscilColorPickerTest, SetModeWheel)
{
    OscilColorPicker picker;

    picker.setMode(OscilColorPicker::Mode::Wheel);
    EXPECT_EQ(picker.getMode(), OscilColorPicker::Mode::Wheel);
}

// =============================================================================
// Display Options Tests
// =============================================================================

TEST_F(OscilColorPickerTest, ShowAlpha)
{
    OscilColorPicker picker;

    picker.setShowAlpha(true);
    EXPECT_TRUE(picker.getShowAlpha());

    picker.setShowAlpha(false);
    EXPECT_FALSE(picker.getShowAlpha());
}

TEST_F(OscilColorPickerTest, ShowHexInput)
{
    OscilColorPicker picker;

    picker.setShowHexInput(true);
    EXPECT_TRUE(picker.getShowHexInput());

    picker.setShowHexInput(false);
    EXPECT_FALSE(picker.getShowHexInput());
}

TEST_F(OscilColorPickerTest, ShowPreview)
{
    OscilColorPicker picker;

    picker.setShowPreview(true);
    EXPECT_TRUE(picker.getShowPreview());

    picker.setShowPreview(false);
    EXPECT_FALSE(picker.getShowPreview());
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(OscilColorPickerTest, OnColorChangedCallback)
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

TEST_F(OscilColorPickerTest, NoCallbackWhenNotifyFalse)
{
    OscilColorPicker picker;

    int changeCount = 0;

    picker.onColorChanged = [&changeCount](juce::Colour) {
        changeCount++;
    };

    picker.setColor(juce::Colours::green, false);
    EXPECT_EQ(changeCount, 0);
}

TEST_F(OscilColorPickerTest, OnColorChangingCallback)
{
    OscilColorPicker picker;

    bool changingCalled = false;
    picker.onColorChanging = [&changingCalled](juce::Colour) {
        changingCalled = true;
    };

    // Callback would be triggered during drag
    // Just verify callback can be set
    EXPECT_FALSE(changingCalled);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(OscilColorPickerTest, PreferredWidthPositive)
{
    OscilColorPicker picker;

    EXPECT_GT(picker.getPreferredWidth(), 0);
}

TEST_F(OscilColorPickerTest, PreferredHeightPositive)
{
    OscilColorPicker picker;

    EXPECT_GT(picker.getPreferredHeight(), 0);
}

TEST_F(OscilColorPickerTest, PreferredHeightDependsOnOptions)
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

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(OscilColorPickerTest, ThemeChangeDoesNotThrow)
{
    OscilColorPicker picker;
    picker.setColor(juce::Colours::blue, false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    picker.themeChanged(newTheme);

    // Color should be preserved
    EXPECT_EQ(picker.getColor().getARGB(), juce::Colours::blue.getARGB());
}

TEST_F(OscilColorPickerTest, ThemeChangePreservesMode)
{
    OscilColorPicker picker;
    picker.setMode(OscilColorPicker::Mode::Wheel);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    picker.themeChanged(newTheme);

    EXPECT_EQ(picker.getMode(), OscilColorPicker::Mode::Wheel);
}

TEST_F(OscilColorPickerTest, ThemeChangePreservesDisplayOptions)
{
    OscilColorPicker picker;
    picker.setShowAlpha(false);
    picker.setShowPreview(false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    picker.themeChanged(newTheme);

    EXPECT_FALSE(picker.getShowAlpha());
    EXPECT_FALSE(picker.getShowPreview());
}
