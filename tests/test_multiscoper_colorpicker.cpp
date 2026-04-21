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

// Regression: in wheel mode a vertical drag must update brightness. The old
// implementation froze brightness_ on switching to wheel mode, so a top-to-
// bottom drag left the value at its original level — exercise that path by
// seeding a mid-value colour, simulating two drags on opposite ends of the
// gradient rectangle, and verifying the brightness differs noticeably.
TEST_F(MultiScoperColorPickerTest, WheelModeDragUpdatesBrightness)
{
    MultiScoperColorPicker picker(getThemeManager());
    picker.setShowAlpha(false);
    picker.setShowPreview(false);
    picker.setShowHexInput(false);
    picker.setBounds(0, 0, 200, picker.getPreferredHeight());
    picker.setMode(MultiScoperColorPicker::Mode::Wheel);

    // Pure red at mid-brightness so there's room to go brighter or darker.
    picker.setColor(juce::Colour::fromHSV(0.0f, 1.0f, 0.5f, 1.0f), false);

    auto makeClickAt = [&picker](juce::Point<int> position) {
        const auto now = juce::Time::getCurrentTime();
        return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), position.toFloat(),
                                juce::ModifierKeys::leftButtonModifier, juce::MouseInputSource::defaultPressure,
                                juce::MouseInputSource::defaultOrientation, juce::MouseInputSource::defaultRotation,
                                juce::MouseInputSource::defaultTiltX, juce::MouseInputSource::defaultTiltY, &picker,
                                &picker, now, position.toFloat(), now, 1, /*mouseWasDragged*/ false);
    };

    // Click at the top of the gradient → brightness should climb near 1.
    picker.mouseDown(makeClickAt({100, 1}));
    const float brightnessTop = picker.getColor().getBrightness();

    // And at the bottom → brightness should drop near 0.
    picker.mouseDown(makeClickAt({100, 178}));
    const float brightnessBottom = picker.getColor().getBrightness();

    // With the fix, vertical drags drive brightness; difference must be non-trivial.
    EXPECT_GT(brightnessTop - brightnessBottom, 0.5f);
}
