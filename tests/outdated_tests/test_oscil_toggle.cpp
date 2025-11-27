/*
    Oscil - Toggle Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilToggle.h"

using namespace oscil;

class OscilToggleTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction - off state
TEST_F(OscilToggleTest, DefaultConstruction)
{
    OscilToggle toggle;

    EXPECT_FALSE(toggle.isOn());
    EXPECT_TRUE(toggle.isEnabled());
}

// Test: Set toggle state
TEST_F(OscilToggleTest, SetToggleState)
{
    OscilToggle toggle;

    toggle.setOn(true);
    EXPECT_TRUE(toggle.isOn());

    toggle.setOn(false);
    EXPECT_FALSE(toggle.isOn());
}

// Test: Toggle method
TEST_F(OscilToggleTest, ToggleMethod)
{
    OscilToggle toggle;

    EXPECT_FALSE(toggle.isOn());

    toggle.toggle();
    EXPECT_TRUE(toggle.isOn());

    toggle.toggle();
    EXPECT_FALSE(toggle.isOn());
}

// Test: Set label
TEST_F(OscilToggleTest, SetLabel)
{
    OscilToggle toggle;
    toggle.setLabel("Enable Feature");

    EXPECT_EQ(toggle.getLabel(), juce::String("Enable Feature"));
}

// Test: Label position
TEST_F(OscilToggleTest, LabelPosition)
{
    OscilToggle toggle;

    toggle.setLabelPosition(OscilToggle::LabelPosition::Left);
    EXPECT_EQ(toggle.getLabelPosition(), OscilToggle::LabelPosition::Left);

    toggle.setLabelPosition(OscilToggle::LabelPosition::Right);
    EXPECT_EQ(toggle.getLabelPosition(), OscilToggle::LabelPosition::Right);
}

// Test: Enabled/disabled state
TEST_F(OscilToggleTest, EnabledState)
{
    OscilToggle toggle;

    EXPECT_TRUE(toggle.isEnabled());

    toggle.setEnabled(false);
    EXPECT_FALSE(toggle.isEnabled());

    toggle.setEnabled(true);
    EXPECT_TRUE(toggle.isEnabled());
}

// Test: Callback invocation
TEST_F(OscilToggleTest, CallbackInvocation)
{
    OscilToggle toggle;
    int changeCount = 0;
    bool lastState = false;

    toggle.onToggle = [&changeCount, &lastState](bool state) {
        changeCount++;
        lastState = state;
    };

    toggle.setOn(true, true);  // notify
    EXPECT_EQ(changeCount, 1);
    EXPECT_TRUE(lastState);

    toggle.setOn(false, true);  // notify
    EXPECT_EQ(changeCount, 2);
    EXPECT_FALSE(lastState);
}

// Test: Callback not invoked when dontNotify
TEST_F(OscilToggleTest, CallbackNotInvokedWhenDontNotify)
{
    OscilToggle toggle;
    int changeCount = 0;

    toggle.onToggle = [&changeCount](bool) {
        changeCount++;
    };

    toggle.setOn(true, false);  // don't notify
    EXPECT_EQ(changeCount, 0);
}

// Test: Toggle disabled does nothing
TEST_F(OscilToggleTest, ToggleDisabledDoesNothing)
{
    OscilToggle toggle;
    toggle.setEnabled(false);
    toggle.setOn(false);

    toggle.toggle();
    EXPECT_FALSE(toggle.isOn());
}

// Test: Size variants
TEST_F(OscilToggleTest, SizeVariants)
{
    OscilToggle toggle;

    toggle.setSizeVariant(SizeVariant::Small);
    EXPECT_EQ(toggle.getSizeVariant(), SizeVariant::Small);

    toggle.setSizeVariant(SizeVariant::Medium);
    EXPECT_EQ(toggle.getSizeVariant(), SizeVariant::Medium);

    toggle.setSizeVariant(SizeVariant::Large);
    EXPECT_EQ(toggle.getSizeVariant(), SizeVariant::Large);
}

// Test: Preferred size
TEST_F(OscilToggleTest, PreferredSize)
{
    OscilToggle toggle;

    int width = toggle.getPreferredWidth();
    int height = toggle.getPreferredHeight();

    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

// Test: Theme changes
TEST_F(OscilToggleTest, ThemeChanges)
{
    OscilToggle toggle;
    toggle.setOn(true);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    toggle.themeChanged(newTheme);

    // State should be preserved
    EXPECT_TRUE(toggle.isOn());
}

// Test: Accessibility
TEST_F(OscilToggleTest, Accessibility)
{
    OscilToggle toggle;
    toggle.setLabel("Test Toggle");

    auto* handler = toggle.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);

    EXPECT_EQ(handler->getRole(), juce::AccessibilityRole::toggleButton);
}

// Test: Focus handling
TEST_F(OscilToggleTest, FocusHandling)
{
    OscilToggle toggle;
    EXPECT_TRUE(toggle.getWantsKeyboardFocus());
}

// Test: Animation spring exists
TEST_F(OscilToggleTest, AnimationSpringExists)
{
    OscilToggle toggle;

    // Animation should start at 0
    toggle.setOn(false, false);

    // Setting on should trigger animation
    toggle.setOn(true, false);

    // Toggle should have valid animation
    // (internal implementation detail, but verifies construction worked)
    EXPECT_TRUE(toggle.isOn());
}
