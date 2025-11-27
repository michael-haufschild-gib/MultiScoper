/*
    Oscil - Button Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilButton.h"

using namespace oscil;

class OscilButtonTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction
TEST_F(OscilButtonTest, DefaultConstruction)
{
    OscilButton button;

    EXPECT_FALSE(button.isToggleable());
    EXPECT_FALSE(button.getToggleState());
    EXPECT_TRUE(button.isEnabled());
}

// Test: Set button text
TEST_F(OscilButtonTest, SetButtonText)
{
    OscilButton button;
    button.setButtonText("Click Me");

    EXPECT_EQ(button.getButtonText(), juce::String("Click Me"));
}

// Test: Set variant
TEST_F(OscilButtonTest, SetVariant)
{
    OscilButton button;

    button.setVariant(ButtonVariant::Primary);
    EXPECT_EQ(button.getVariant(), ButtonVariant::Primary);

    button.setVariant(ButtonVariant::Secondary);
    EXPECT_EQ(button.getVariant(), ButtonVariant::Secondary);

    button.setVariant(ButtonVariant::Outline);
    EXPECT_EQ(button.getVariant(), ButtonVariant::Outline);

    button.setVariant(ButtonVariant::Ghost);
    EXPECT_EQ(button.getVariant(), ButtonVariant::Ghost);

    button.setVariant(ButtonVariant::Danger);
    EXPECT_EQ(button.getVariant(), ButtonVariant::Danger);
}

// Test: Toggleable state
TEST_F(OscilButtonTest, ToggleableState)
{
    OscilButton button;

    EXPECT_FALSE(button.isToggleable());

    button.setToggleable(true);
    EXPECT_TRUE(button.isToggleable());

    button.setToggleable(false);
    EXPECT_FALSE(button.isToggleable());
}

// Test: Toggle state changes
TEST_F(OscilButtonTest, ToggleStateChanges)
{
    OscilButton button;
    button.setToggleable(true);

    EXPECT_FALSE(button.getToggleState());

    button.setToggleState(true, juce::dontSendNotification);
    EXPECT_TRUE(button.getToggleState());

    button.setToggleState(false, juce::dontSendNotification);
    EXPECT_FALSE(button.getToggleState());
}

// Test: Enabled/disabled state
TEST_F(OscilButtonTest, EnabledDisabledState)
{
    OscilButton button;

    EXPECT_TRUE(button.isEnabled());

    button.setEnabled(false);
    EXPECT_FALSE(button.isEnabled());

    button.setEnabled(true);
    EXPECT_TRUE(button.isEnabled());
}

// Test: Set icon
TEST_F(OscilButtonTest, SetIcon)
{
    OscilButton button;

    juce::Path iconPath;
    iconPath.addEllipse(0, 0, 10, 10);

    button.setIcon(iconPath);
    EXPECT_TRUE(button.hasIcon());
}

// Test: Icon position
TEST_F(OscilButtonTest, IconPosition)
{
    OscilButton button;

    button.setIconPosition(OscilButton::IconPosition::Left);
    EXPECT_EQ(button.getIconPosition(), OscilButton::IconPosition::Left);

    button.setIconPosition(OscilButton::IconPosition::Right);
    EXPECT_EQ(button.getIconPosition(), OscilButton::IconPosition::Right);

    button.setIconPosition(OscilButton::IconPosition::Only);
    EXPECT_EQ(button.getIconPosition(), OscilButton::IconPosition::Only);
}

// Test: Loading state
TEST_F(OscilButtonTest, LoadingState)
{
    OscilButton button;

    EXPECT_FALSE(button.isLoading());

    button.setLoading(true);
    EXPECT_TRUE(button.isLoading());

    button.setLoading(false);
    EXPECT_FALSE(button.isLoading());
}

// Test: Size variants
TEST_F(OscilButtonTest, SizeVariants)
{
    OscilButton button;

    button.setSizeVariant(SizeVariant::Small);
    EXPECT_EQ(button.getSizeVariant(), SizeVariant::Small);

    button.setSizeVariant(SizeVariant::Medium);
    EXPECT_EQ(button.getSizeVariant(), SizeVariant::Medium);

    button.setSizeVariant(SizeVariant::Large);
    EXPECT_EQ(button.getSizeVariant(), SizeVariant::Large);
}

// Test: Callback invocation
TEST_F(OscilButtonTest, CallbackInvocation)
{
    OscilButton button;
    int clickCount = 0;

    button.onClick = [&clickCount]() {
        clickCount++;
    };

    button.triggerClick();
    EXPECT_EQ(clickCount, 1);

    button.triggerClick();
    EXPECT_EQ(clickCount, 2);
}

// Test: Toggle callback
TEST_F(OscilButtonTest, ToggleCallback)
{
    OscilButton button;
    button.setToggleable(true);

    bool lastState = false;
    button.onStateChange = [&lastState, &button]() {
        lastState = button.getToggleState();
    };

    button.setToggleState(true, juce::sendNotification);
    EXPECT_TRUE(lastState);

    button.setToggleState(false, juce::sendNotification);
    EXPECT_FALSE(lastState);
}

// Test: Preferred width/height
TEST_F(OscilButtonTest, PreferredSize)
{
    OscilButton button;
    button.setButtonText("Test Button");

    int width = button.getPreferredWidth();
    int height = button.getPreferredHeight();

    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

// Test: Minimum width
TEST_F(OscilButtonTest, MinimumWidth)
{
    OscilButton button;
    button.setButtonText("X");

    button.setMinimumWidth(100);
    EXPECT_GE(button.getPreferredWidth(), 100);
}

// Test: Theme changes
TEST_F(OscilButtonTest, ThemeChanges)
{
    OscilButton button;
    button.setButtonText("Test");

    // Theme change should not throw
    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    button.themeChanged(newTheme);

    // Button should still be functional
    EXPECT_EQ(button.getButtonText(), juce::String("Test"));
}

// Test: Accessibility
TEST_F(OscilButtonTest, Accessibility)
{
    OscilButton button;
    button.setButtonText("Accessible Button");

    auto* handler = button.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);

    EXPECT_EQ(handler->getRole(), juce::AccessibilityRole::button);
}

// Test: Focus handling
TEST_F(OscilButtonTest, FocusHandling)
{
    OscilButton button;

    // Should accept focus
    EXPECT_TRUE(button.getWantsKeyboardFocus());
}
