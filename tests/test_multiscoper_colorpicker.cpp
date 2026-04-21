/*
    MultiScoper - Color Picker Component Tests
    Tests for MultiScoperColorPicker UI component
*/

#include "ui/components/MultiScoperColorPicker.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace multiscoper;

class MultiScoperColorPickerTest : public ::testing::Test
{
protected:
    std::unique_ptr<ThemeManager> themeManager_;

    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }

    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(MultiScoperColorPickerTest, DefaultConstruction)
{
    MultiScoperColorPicker picker(getThemeManager());

    // Should have a default color
    EXPECT_NE(picker.getColor().getARGB(), 0u);
}

TEST_F(MultiScoperColorPickerTest, ConstructionWithTestId)
{
    MultiScoperColorPicker picker(getThemeManager(), "colorpicker-1");

    EXPECT_NE(picker.getColor().getARGB(), 0u);
}

// =============================================================================
// Color Tests
// =============================================================================

TEST_F(MultiScoperColorPickerTest, SetColor)
{
    MultiScoperColorPicker picker(getThemeManager());

    picker.setColor(juce::Colours::purple, false);
    EXPECT_EQ(picker.getColor().getARGB(), juce::Colours::purple.getARGB());
}

TEST_F(MultiScoperColorPickerTest, SetOriginalColor)
{
    MultiScoperColorPicker picker(getThemeManager());

    picker.setOriginalColor(juce::Colours::red);
    EXPECT_EQ(picker.getOriginalColor().getARGB(), juce::Colours::red.getARGB());
}

TEST_F(MultiScoperColorPickerTest, ColorWithAlpha)
{
    MultiScoperColorPicker picker(getThemeManager());
    picker.setShowAlpha(true);

    auto colorWithAlpha = juce::Colours::blue.withAlpha(0.5f);
    picker.setColor(colorWithAlpha, false);

    EXPECT_NEAR(picker.getColor().getFloatAlpha(), 0.5f, 0.01f);
}

// =============================================================================
// Mode Tests
// =============================================================================

TEST_F(MultiScoperColorPickerTest, SetModeSquare)
{
    MultiScoperColorPicker picker(getThemeManager());

    picker.setMode(MultiScoperColorPicker::Mode::Square);
    EXPECT_EQ(picker.getMode(), MultiScoperColorPicker::Mode::Square);
}

TEST_F(MultiScoperColorPickerTest, SetModeWheel)
{
    MultiScoperColorPicker picker(getThemeManager());

    picker.setMode(MultiScoperColorPicker::Mode::Wheel);
    EXPECT_EQ(picker.getMode(), MultiScoperColorPicker::Mode::Wheel);
}

// =============================================================================
// Display Options Tests
// =============================================================================

TEST_F(MultiScoperColorPickerTest, ShowAlpha)
{
    MultiScoperColorPicker picker(getThemeManager());

    picker.setShowAlpha(true);
    EXPECT_TRUE(picker.getShowAlpha());

    picker.setShowAlpha(false);
    EXPECT_FALSE(picker.getShowAlpha());
}

TEST_F(MultiScoperColorPickerTest, ShowHexInput)
{
    MultiScoperColorPicker picker(getThemeManager());

    picker.setShowHexInput(true);
    EXPECT_TRUE(picker.getShowHexInput());

    picker.setShowHexInput(false);
    EXPECT_FALSE(picker.getShowHexInput());
}

TEST_F(MultiScoperColorPickerTest, ShowPreview)
{
    MultiScoperColorPicker picker(getThemeManager());

    picker.setShowPreview(true);
    EXPECT_TRUE(picker.getShowPreview());

    picker.setShowPreview(false);
    EXPECT_FALSE(picker.getShowPreview());
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(MultiScoperColorPickerTest, OnColorChangedCallback)
{
    MultiScoperColorPicker picker(getThemeManager());

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

TEST_F(MultiScoperColorPickerTest, NoCallbackWhenNotifyFalse)
{
    MultiScoperColorPicker picker(getThemeManager());

    int changeCount = 0;

    picker.onColorChanged = [&changeCount](juce::Colour) { changeCount++; };

    picker.setColor(juce::Colours::green, false);
    EXPECT_EQ(changeCount, 0);
}

TEST_F(MultiScoperColorPickerTest, OnColorChangingCallback)
{
    MultiScoperColorPicker picker(getThemeManager());

    bool changingCalled = false;
    picker.onColorChanging = [&changingCalled](juce::Colour) { changingCalled = true; };

    // Callback would be triggered during drag
    // Just verify callback can be set
    EXPECT_FALSE(changingCalled);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(MultiScoperColorPickerTest, PreferredWidthPositive)
{
    MultiScoperColorPicker picker(getThemeManager());

    EXPECT_GT(picker.getPreferredWidth(), 0);
}

TEST_F(MultiScoperColorPickerTest, PreferredHeightPositive)
{
    MultiScoperColorPicker picker(getThemeManager());

    EXPECT_GT(picker.getPreferredHeight(), 0);
}

TEST_F(MultiScoperColorPickerTest, PreferredHeightDependsOnOptions)
{
    MultiScoperColorPicker picker(getThemeManager());

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

TEST_F(MultiScoperColorPickerTest, ThemeChangeDoesNotThrow)
{
    MultiScoperColorPicker picker(getThemeManager());
    picker.setColor(juce::Colours::blue, false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    picker.themeChanged(newTheme);

    // Color should be preserved
    EXPECT_EQ(picker.getColor().getARGB(), juce::Colours::blue.getARGB());
}

TEST_F(MultiScoperColorPickerTest, ThemeChangePreservesMode)
{
    MultiScoperColorPicker picker(getThemeManager());
    picker.setMode(MultiScoperColorPicker::Mode::Wheel);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    picker.themeChanged(newTheme);

    EXPECT_EQ(picker.getMode(), MultiScoperColorPicker::Mode::Wheel);
}

TEST_F(MultiScoperColorPickerTest, ThemeChangePreservesDisplayOptions)
{
    MultiScoperColorPicker picker(getThemeManager());
    picker.setShowAlpha(false);
    picker.setShowPreview(false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    picker.themeChanged(newTheme);

    EXPECT_FALSE(picker.getShowAlpha());
    EXPECT_FALSE(picker.getShowPreview());
}

// Regression: setColor with a non-trivial alpha must roundtrip through the
// internal state (hue/saturation/brightness/alpha) and back to getColor.
// Previously untested — covers the alpha path through updateFromHSV.
TEST_F(MultiScoperColorPickerTest, SetColorRoundtripPreservesAlpha)
{
    MultiScoperColorPicker picker(getThemeManager());
    picker.setShowAlpha(true);

    auto input = juce::Colour::fromFloatRGBA(0.7f, 0.2f, 0.1f, 0.5f);
    picker.setColor(input, false);

    EXPECT_NEAR(picker.getColor().getFloatAlpha(), 0.5f, 0.01f);
    EXPECT_NEAR(picker.getColor().getFloatRed(), 0.7f, 0.01f);
    EXPECT_NEAR(picker.getColor().getFloatGreen(), 0.2f, 0.01f);
    EXPECT_NEAR(picker.getColor().getFloatBlue(), 0.1f, 0.01f);
}
