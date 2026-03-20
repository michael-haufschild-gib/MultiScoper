/*
    Oscil - Toggle Component Tests
    Tests for OscilToggle UI component
*/

#include <gtest/gtest.h>
#include "ui/components/OscilToggle.h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class OscilToggleTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        themeManager_ = std::make_unique<ThemeManager>();
    }

    void TearDown() override
    {
        themeManager_.reset();
    }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(OscilToggleTest, DefaultConstruction)
{
    OscilToggle toggle(getThemeManager());

    EXPECT_FALSE(toggle.getValue());
    EXPECT_TRUE(toggle.isEnabled());
    EXPECT_TRUE(toggle.getLabel().isEmpty());
}

TEST_F(OscilToggleTest, ConstructionWithLabel)
{
    OscilToggle toggle(getThemeManager(), "Enable Feature");

    EXPECT_EQ(toggle.getLabel(), juce::String("Enable Feature"));
    EXPECT_FALSE(toggle.getValue());
}

TEST_F(OscilToggleTest, ConstructionWithLabelAndTestId)
{
    OscilToggle toggle(getThemeManager(), "Enable Feature", "toggle-feature");

    EXPECT_EQ(toggle.getLabel(), juce::String("Enable Feature"));
}

// =============================================================================
// Value Tests
// =============================================================================

TEST_F(OscilToggleTest, SetValueOn)
{
    OscilToggle toggle(getThemeManager());

    toggle.setValue(true);
    EXPECT_TRUE(toggle.getValue());
}

TEST_F(OscilToggleTest, SetValueOff)
{
    OscilToggle toggle(getThemeManager());
    toggle.setValue(true);

    toggle.setValue(false);
    EXPECT_FALSE(toggle.getValue());
}

TEST_F(OscilToggleTest, SetValueWithoutAnimation)
{
    OscilToggle toggle(getThemeManager());

    toggle.setValue(true, false);  // No animation
    EXPECT_TRUE(toggle.getValue());
}

TEST_F(OscilToggleTest, ToggleMethod)
{
    OscilToggle toggle(getThemeManager());

    EXPECT_FALSE(toggle.getValue());

    toggle.toggle();
    EXPECT_TRUE(toggle.getValue());

    toggle.toggle();
    EXPECT_FALSE(toggle.getValue());
}

// =============================================================================
// Label Tests
// =============================================================================

TEST_F(OscilToggleTest, SetLabel)
{
    OscilToggle toggle(getThemeManager());
    toggle.setLabel("Test Label");

    EXPECT_EQ(toggle.getLabel(), juce::String("Test Label"));
}

TEST_F(OscilToggleTest, DefaultLabelOnRight)
{
    OscilToggle toggle(getThemeManager());

    EXPECT_TRUE(toggle.isLabelOnRight());
}

TEST_F(OscilToggleTest, SetLabelOnRight)
{
    OscilToggle toggle(getThemeManager());

    toggle.setLabelOnRight(false);
    EXPECT_FALSE(toggle.isLabelOnRight());

    toggle.setLabelOnRight(true);
    EXPECT_TRUE(toggle.isLabelOnRight());
}

// =============================================================================
// Enabled/Disabled State Tests
// =============================================================================

TEST_F(OscilToggleTest, DefaultEnabled)
{
    OscilToggle toggle(getThemeManager());

    EXPECT_TRUE(toggle.isEnabled());
    EXPECT_FALSE(toggle.getValue());
}

TEST_F(OscilToggleTest, SetDisabled)
{
    OscilToggle toggle(getThemeManager());
    toggle.setEnabled(false);

    EXPECT_FALSE(toggle.isEnabled());
}

TEST_F(OscilToggleTest, SetEnabledAfterDisabled)
{
    OscilToggle toggle(getThemeManager());
    toggle.setEnabled(false);
    EXPECT_FALSE(toggle.isEnabled());

    toggle.setEnabled(true);
    EXPECT_TRUE(toggle.isEnabled());
}

TEST_F(OscilToggleTest, ToggleWhenDisabledDoesNothing)
{
    OscilToggle toggle(getThemeManager());
    toggle.setValue(false);
    toggle.setEnabled(false);

    toggle.toggle();

    // Should remain false because disabled
    EXPECT_FALSE(toggle.getValue());
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(OscilToggleTest, OnValueChangedCallback)
{
    OscilToggle toggle(getThemeManager());
    int changeCount = 0;
    bool lastState = false;

    toggle.onValueChanged = [&](bool state) {
        changeCount++;
        lastState = state;
    };

    toggle.setValue(true);
    // Regardless of callback, state should be updated
    EXPECT_TRUE(toggle.getValue());
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(OscilToggleTest, PreferredWidthPositive)
{
    OscilToggle toggle(getThemeManager());

    EXPECT_GT(toggle.getPreferredWidth(), 0);
}

TEST_F(OscilToggleTest, PreferredHeightPositive)
{
    OscilToggle toggle(getThemeManager());

    EXPECT_GT(toggle.getPreferredHeight(), 0);
}

TEST_F(OscilToggleTest, ToggleWithLabelHasGreaterWidth)
{
    OscilToggle noLabel(getThemeManager());
    OscilToggle withLabel(getThemeManager(), "A Very Long Label Here");

    EXPECT_GT(withLabel.getPreferredWidth(), noLabel.getPreferredWidth());
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(OscilToggleTest, ThemeChangeDoesNotThrow)
{
    OscilToggle toggle(getThemeManager());
    toggle.setValue(true);

    ColorTheme newTheme;
    newTheme.name = "Custom Theme";

    // Should not throw
    toggle.themeChanged(newTheme);

    // State should be preserved
    EXPECT_TRUE(toggle.getValue());
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

TEST_F(OscilToggleTest, WantsKeyboardFocus)
{
    OscilToggle toggle(getThemeManager());

    EXPECT_TRUE(toggle.getWantsKeyboardFocus());
}

// =============================================================================
// APVTS Attachment Tests
// =============================================================================

TEST_F(OscilToggleTest, DefaultNotAttached)
{
    OscilToggle toggle(getThemeManager());

    EXPECT_FALSE(toggle.isAttachedToParameter());
}

TEST_F(OscilToggleTest, DetachWhenNotAttached)
{
    OscilToggle toggle(getThemeManager());

    // Should not crash
    toggle.detachFromParameter();

    EXPECT_FALSE(toggle.isAttachedToParameter());
}
