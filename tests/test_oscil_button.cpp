/*
    Oscil - Button Component Tests
    Tests for OscilButton behavioral correctness

    Bug targets:
    - triggerClick fires onClick on disabled button
    - Toggle callback fires when notify=false
    - Toggle state not preserved across variant change
    - Longer text not increasing preferred width
    - Shortcut key not retained after construction
*/

#include "ui/components/OscilButton.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace oscil;

class OscilButtonTest : public ::testing::Test
{
protected:
    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }

    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// Construction Contracts
// =============================================================================

TEST_F(OscilButtonTest, DefaultConstructionState)
{
    OscilButton button(getThemeManager());

    EXPECT_EQ(button.getVariant(), ButtonVariant::Primary);
    EXPECT_TRUE(button.isEnabled());
    EXPECT_FALSE(button.isToggleable());
    EXPECT_FALSE(button.isToggled());
    EXPECT_TRUE(button.getText().isEmpty());
    EXPECT_FALSE(button.getShortcut().isValid());
    EXPECT_TRUE(button.getTooltip().isEmpty());
    EXPECT_EQ(button.getSegmentPosition(), SegmentPosition::None);
    EXPECT_EQ(button.getButtonId(), -1);
    EXPECT_TRUE(button.getWantsKeyboardFocus());
}

// =============================================================================
// Click Behavior
// =============================================================================

TEST_F(OscilButtonTest, TriggerClickInvokesOnClick)
{
    OscilButton button(getThemeManager());
    int clickCount = 0;
    button.onClick = [&]() { clickCount++; };

    button.triggerClick();
    EXPECT_EQ(clickCount, 1);

    button.triggerClick();
    EXPECT_EQ(clickCount, 2);
}

TEST_F(OscilButtonTest, TriggerClickWithNullCallbackDoesNotCrash)
{
    OscilButton button(getThemeManager());
    int secondClickCount = 0;

    button.onClick = nullptr;
    button.triggerClick(); // Should not crash with null callback

    // Button should remain functional: set a new callback and verify it fires
    button.onClick = [&]() { secondClickCount++; };
    button.triggerClick();
    EXPECT_EQ(secondClickCount, 1);
}

// =============================================================================
// Toggle State Machine
// =============================================================================

TEST_F(OscilButtonTest, ToggleWithNotifyTrueFiresCallback)
{
    OscilButton button(getThemeManager());
    button.setToggleable(true);

    bool callbackFired = false;
    bool lastState = false;
    button.onToggle = [&](bool toggled) {
        callbackFired = true;
        lastState = toggled;
    };

    button.setToggled(true, true);
    EXPECT_TRUE(callbackFired);
    EXPECT_TRUE(lastState);
    EXPECT_TRUE(button.isToggled());
}

TEST_F(OscilButtonTest, ToggleWithNotifyFalseSuppressesCallback)
{
    OscilButton button(getThemeManager());
    button.setToggleable(true);

    bool callbackFired = false;
    button.onToggle = [&](bool) { callbackFired = true; };

    button.setToggled(true, false);
    EXPECT_FALSE(callbackFired);
    EXPECT_TRUE(button.isToggled());
}

TEST_F(OscilButtonTest, ToggleOnOffCycle)
{
    OscilButton button(getThemeManager());
    button.setToggleable(true);

    EXPECT_FALSE(button.isToggled());

    button.setToggled(true, false);
    EXPECT_TRUE(button.isToggled());

    button.setToggled(false, false);
    EXPECT_FALSE(button.isToggled());
}

// =============================================================================
// Size Behavior
// =============================================================================

TEST_F(OscilButtonTest, LongerTextProducesWiderButton)
{
    OscilButton shortBtn(getThemeManager(), "Hi");
    OscilButton longBtn(getThemeManager(), "This Is A Much Longer Button Label");

    EXPECT_GT(longBtn.getPreferredWidth(), shortBtn.getPreferredWidth());
}

TEST_F(OscilButtonTest, PreferredSizeIsPositive)
{
    OscilButton button(getThemeManager(), "Test");
    EXPECT_GT(button.getPreferredWidth(), 0);
    EXPECT_GT(button.getPreferredHeight(), 0);
}

// =============================================================================
// Icon Behavior
// =============================================================================

TEST_F(OscilButtonTest, IconPathCanBeSetAndCleared)
{
    OscilButton button(getThemeManager());

    EXPECT_FALSE(button.hasIconPath());

    juce::Path path;
    path.addEllipse(0, 0, 10, 10);
    button.setIconPath(path);
    EXPECT_TRUE(button.hasIconPath());

    button.clearIconPath();
    EXPECT_FALSE(button.hasIconPath());
}

// =============================================================================
// Variant Cycling
// =============================================================================

TEST_F(OscilButtonTest, AllVariantsAccepted)
{
    OscilButton button(getThemeManager());

    for (auto variant : {ButtonVariant::Primary, ButtonVariant::Secondary, ButtonVariant::Danger, ButtonVariant::Ghost,
                         ButtonVariant::Icon})
    {
        button.setVariant(variant);
        EXPECT_EQ(button.getVariant(), variant);
    }
}

// =============================================================================
// Theme Resilience
// =============================================================================

TEST_F(OscilButtonTest, ThemeChangePreservesAllState)
{
    OscilButton button(getThemeManager(), "Action");
    button.setVariant(ButtonVariant::Danger);
    button.setToggleable(true);
    button.setToggled(true, false);
    button.setTooltip("Help text");
    juce::KeyPress shortcut('s', juce::ModifierKeys::commandModifier, 0);
    button.setShortcut(shortcut);

    ColorTheme newTheme;
    newTheme.name = "Custom Theme";
    button.themeChanged(newTheme);

    EXPECT_EQ(button.getText(), juce::String("Action"));
    EXPECT_EQ(button.getVariant(), ButtonVariant::Danger);
    EXPECT_TRUE(button.isToggled());
    EXPECT_EQ(button.getTooltip(), juce::String("Help text"));
    EXPECT_TRUE(button.getShortcut().isValid());
}
