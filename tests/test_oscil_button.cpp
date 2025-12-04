/*
    Oscil - Button Component Tests
    Tests for OscilButton UI component
*/

#include <gtest/gtest.h>
#include "ui/components/OscilButton.h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class OscilButtonTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(OscilButtonTest, DefaultConstruction)
{
    OscilButton button(ThemeManager::getInstance());

    EXPECT_EQ(button.getVariant(), ButtonVariant::Primary);
    EXPECT_TRUE(button.isEnabled());
    EXPECT_FALSE(button.isToggleable());
    EXPECT_FALSE(button.isToggled());
    EXPECT_EQ(button.getText(), juce::String());
}

TEST_F(OscilButtonTest, ConstructionWithText)
{
    OscilButton button(ThemeManager::getInstance(), "Click Me");

    EXPECT_EQ(button.getText(), juce::String("Click Me"));
}

TEST_F(OscilButtonTest, ConstructionWithTextAndTestId)
{
    OscilButton button(ThemeManager::getInstance(), "Click Me", "test-button-1");

    EXPECT_EQ(button.getText(), juce::String("Click Me"));
}

// =============================================================================
// Text Tests
// =============================================================================

TEST_F(OscilButtonTest, SetText)
{
    OscilButton button(ThemeManager::getInstance());
    button.setText("New Label");

    EXPECT_EQ(button.getText(), juce::String("New Label"));
}

TEST_F(OscilButtonTest, SetEmptyText)
{
    OscilButton button(ThemeManager::getInstance(), "Initial");
    button.setText("");

    EXPECT_EQ(button.getText(), juce::String());
}

// =============================================================================
// Variant Tests
// =============================================================================

TEST_F(OscilButtonTest, SetVariantPrimary)
{
    OscilButton button(ThemeManager::getInstance());
    button.setVariant(ButtonVariant::Primary);

    EXPECT_EQ(button.getVariant(), ButtonVariant::Primary);
}

TEST_F(OscilButtonTest, SetVariantSecondary)
{
    OscilButton button(ThemeManager::getInstance());
    button.setVariant(ButtonVariant::Secondary);

    EXPECT_EQ(button.getVariant(), ButtonVariant::Secondary);
}

TEST_F(OscilButtonTest, SetVariantDanger)
{
    OscilButton button(ThemeManager::getInstance());
    button.setVariant(ButtonVariant::Danger);

    EXPECT_EQ(button.getVariant(), ButtonVariant::Danger);
}

TEST_F(OscilButtonTest, SetVariantGhost)
{
    OscilButton button(ThemeManager::getInstance());
    button.setVariant(ButtonVariant::Ghost);

    EXPECT_EQ(button.getVariant(), ButtonVariant::Ghost);
}

TEST_F(OscilButtonTest, SetVariantIcon)
{
    OscilButton button(ThemeManager::getInstance());
    button.setVariant(ButtonVariant::Icon);

    EXPECT_EQ(button.getVariant(), ButtonVariant::Icon);
}

// =============================================================================
// Enabled/Disabled State Tests
// =============================================================================

TEST_F(OscilButtonTest, DefaultEnabled)
{
    OscilButton button(ThemeManager::getInstance());

    EXPECT_TRUE(button.isEnabled());
}

TEST_F(OscilButtonTest, SetDisabled)
{
    OscilButton button(ThemeManager::getInstance());
    button.setEnabled(false);

    EXPECT_FALSE(button.isEnabled());
}

TEST_F(OscilButtonTest, SetEnabledAfterDisabled)
{
    OscilButton button(ThemeManager::getInstance());
    button.setEnabled(false);
    button.setEnabled(true);

    EXPECT_TRUE(button.isEnabled());
}

// =============================================================================
// Toggle Tests
// =============================================================================

TEST_F(OscilButtonTest, DefaultNotToggleable)
{
    OscilButton button(ThemeManager::getInstance());

    EXPECT_FALSE(button.isToggleable());
}

TEST_F(OscilButtonTest, SetToggleable)
{
    OscilButton button(ThemeManager::getInstance());
    button.setToggleable(true);

    EXPECT_TRUE(button.isToggleable());
}

TEST_F(OscilButtonTest, ToggleState)
{
    OscilButton button(ThemeManager::getInstance());
    button.setToggleable(true);

    EXPECT_FALSE(button.isToggled());

    button.setToggled(true, false);  // No notify
    EXPECT_TRUE(button.isToggled());

    button.setToggled(false, false);
    EXPECT_FALSE(button.isToggled());
}

TEST_F(OscilButtonTest, ToggleCallbackNotified)
{
    OscilButton button(ThemeManager::getInstance());
    button.setToggleable(true);

    bool callbackInvoked = false;
    bool lastToggleState = false;

    button.onToggle = [&](bool toggled) {
        callbackInvoked = true;
        lastToggleState = toggled;
    };

    button.setToggled(true, true);  // With notify

    EXPECT_TRUE(callbackInvoked);
    EXPECT_TRUE(lastToggleState);
}

TEST_F(OscilButtonTest, ToggleCallbackNotNotifiedWhenNoNotify)
{
    OscilButton button(ThemeManager::getInstance());
    button.setToggleable(true);

    bool callbackInvoked = false;

    button.onToggle = [&](bool /*toggled*/) {
        callbackInvoked = true;
    };

    button.setToggled(true, false);  // Without notify

    EXPECT_FALSE(callbackInvoked);
}

// =============================================================================
// Icon Path Tests
// =============================================================================

TEST_F(OscilButtonTest, DefaultNoIconPath)
{
    OscilButton button(ThemeManager::getInstance());

    EXPECT_FALSE(button.hasIconPath());
}

TEST_F(OscilButtonTest, SetIconPath)
{
    OscilButton button(ThemeManager::getInstance());

    juce::Path iconPath;
    iconPath.addEllipse(0, 0, 10, 10);

    button.setIconPath(iconPath);

    EXPECT_TRUE(button.hasIconPath());
}

TEST_F(OscilButtonTest, ClearIconPath)
{
    OscilButton button(ThemeManager::getInstance());

    juce::Path iconPath;
    iconPath.addEllipse(0, 0, 10, 10);
    button.setIconPath(iconPath);

    EXPECT_TRUE(button.hasIconPath());

    button.clearIconPath();

    EXPECT_FALSE(button.hasIconPath());
}

// =============================================================================
// Shortcut Key Tests
// =============================================================================

TEST_F(OscilButtonTest, DefaultNoShortcut)
{
    OscilButton button(ThemeManager::getInstance());

    EXPECT_FALSE(button.getShortcut().isValid());
}

TEST_F(OscilButtonTest, SetShortcut)
{
    OscilButton button(ThemeManager::getInstance());
    juce::KeyPress shortcut('s', juce::ModifierKeys::commandModifier, 0);

    button.setShortcut(shortcut);

    EXPECT_TRUE(button.getShortcut().isValid());
}

// =============================================================================
// Tooltip Tests
// =============================================================================

TEST_F(OscilButtonTest, DefaultNoTooltip)
{
    OscilButton button(ThemeManager::getInstance());

    EXPECT_TRUE(button.getTooltip().isEmpty());
}

TEST_F(OscilButtonTest, SetTooltip)
{
    OscilButton button(ThemeManager::getInstance());
    button.setTooltip("This is a tooltip");

    EXPECT_EQ(button.getTooltip(), juce::String("This is a tooltip"));
}

// =============================================================================
// Segment Position Tests
// =============================================================================

TEST_F(OscilButtonTest, DefaultSegmentPositionNone)
{
    OscilButton button(ThemeManager::getInstance());

    EXPECT_EQ(button.getSegmentPosition(), SegmentPosition::None);
}

TEST_F(OscilButtonTest, SetSegmentPositionFirst)
{
    OscilButton button(ThemeManager::getInstance());
    button.setSegmentPosition(SegmentPosition::First);

    EXPECT_EQ(button.getSegmentPosition(), SegmentPosition::First);
}

TEST_F(OscilButtonTest, SetSegmentPositionMiddle)
{
    OscilButton button(ThemeManager::getInstance());
    button.setSegmentPosition(SegmentPosition::Middle);

    EXPECT_EQ(button.getSegmentPosition(), SegmentPosition::Middle);
}

TEST_F(OscilButtonTest, SetSegmentPositionLast)
{
    OscilButton button(ThemeManager::getInstance());
    button.setSegmentPosition(SegmentPosition::Last);

    EXPECT_EQ(button.getSegmentPosition(), SegmentPosition::Last);
}

// =============================================================================
// Button ID Tests
// =============================================================================

TEST_F(OscilButtonTest, DefaultButtonId)
{
    OscilButton button(ThemeManager::getInstance());

    EXPECT_EQ(button.getButtonId(), -1);
}

TEST_F(OscilButtonTest, SetButtonId)
{
    OscilButton button(ThemeManager::getInstance());
    button.setButtonId(42);

    EXPECT_EQ(button.getButtonId(), 42);
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(OscilButtonTest, OnClickCallback)
{
    OscilButton button(ThemeManager::getInstance());
    int clickCount = 0;

    button.onClick = [&]() {
        clickCount++;
    };

    button.triggerClick();
    EXPECT_EQ(clickCount, 1);

    button.triggerClick();
    EXPECT_EQ(clickCount, 2);
}

// Note: triggerClick() bypasses the enabled check for accessibility purposes
// In real usage, mouseDown/mouseUp check isEnabled() before triggering clicks

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(OscilButtonTest, PreferredWidthPositive)
{
    OscilButton button(ThemeManager::getInstance(), "Test Button");

    EXPECT_GT(button.getPreferredWidth(), 0);
}

TEST_F(OscilButtonTest, PreferredHeightPositive)
{
    OscilButton button(ThemeManager::getInstance(), "Test Button");

    EXPECT_GT(button.getPreferredHeight(), 0);
}

TEST_F(OscilButtonTest, LongerTextHasGreaterWidth)
{
    OscilButton shortButton(ThemeManager::getInstance(), "Hi");
    OscilButton longButton(ThemeManager::getInstance(), "This is a much longer button text");

    EXPECT_GT(longButton.getPreferredWidth(), shortButton.getPreferredWidth());
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(OscilButtonTest, ThemeChangeDoesNotThrow)
{
    OscilButton button(ThemeManager::getInstance(), "Test");

    ColorTheme newTheme;
    newTheme.name = "Custom Theme";

    // Should not throw
    button.themeChanged(newTheme);

    // Button should still be functional
    EXPECT_EQ(button.getText(), juce::String("Test"));
}

// =============================================================================
// Accessibility Tests
// =============================================================================

// Note: Accessibility handler tests require the component to be added to the desktop
// or a visible window. In headless test environments, getAccessibilityHandler()
// returns nullptr until the component is properly displayed.

// =============================================================================
// Focus Tests
// =============================================================================

TEST_F(OscilButtonTest, WantsKeyboardFocus)
{
    OscilButton button(ThemeManager::getInstance());

    EXPECT_TRUE(button.getWantsKeyboardFocus());
}
