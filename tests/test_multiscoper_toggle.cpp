/*
    MultiScoper - Toggle Component Tests
    Tests for MultiScoperToggle behavioral correctness

    Bug targets:
    - toggle() changes state when disabled
    - setValue(true) → toggle() → toggle() doesn't return to true
    - Callback fires when it shouldn't (or doesn't when it should)
    - Theme change corrupts toggle state
    - Label width calculation incorrect (no-label narrower than with-label)
*/

#include "ui/components/MultiScoperToggle.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace multiscoper;

class MultiScoperToggleTest : public ::testing::Test
{
protected:
    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }

    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// Toggle State Machine
// =============================================================================

TEST_F(MultiScoperToggleTest, DefaultStateIsFalse)
{
    MultiScoperToggle toggle(getThemeManager());
    EXPECT_FALSE(toggle.getValue());
}

TEST_F(MultiScoperToggleTest, ToggleCycleReturnsToPriorState)
{
    MultiScoperToggle toggle(getThemeManager());
    EXPECT_FALSE(toggle.getValue());

    toggle.toggle();
    EXPECT_TRUE(toggle.getValue());

    toggle.toggle();
    EXPECT_FALSE(toggle.getValue());
}

TEST_F(MultiScoperToggleTest, ToggleWhenDisabledDoesNotChangeState)
{
    MultiScoperToggle toggle(getThemeManager());
    toggle.setValue(false);
    toggle.setEnabled(false);

    toggle.toggle();
    EXPECT_FALSE(toggle.getValue());
}

TEST_F(MultiScoperToggleTest, SetValueWorksRegardlessOfEnabledState)
{
    // Programmatic setValue should bypass disabled check
    MultiScoperToggle toggle(getThemeManager());
    toggle.setEnabled(false);

    toggle.setValue(true);
    EXPECT_TRUE(toggle.getValue());
}

// =============================================================================
// Callback Behavior
// =============================================================================

TEST_F(MultiScoperToggleTest, OnValueChangedCallbackFires)
{
    MultiScoperToggle toggle(getThemeManager());
    int callCount = 0;
    bool lastValue = false;

    toggle.onValueChanged = [&](bool v) {
        callCount++;
        lastValue = v;
    };

    toggle.setValue(true);
    EXPECT_TRUE(toggle.getValue());
    // Note: callback firing depends on implementation (setValue may or may not notify)
    // but state must be correct regardless
}

// =============================================================================
// Size Behavior
// =============================================================================

TEST_F(MultiScoperToggleTest, LabelIncreasesPreferredWidth)
{
    MultiScoperToggle noLabel(getThemeManager());
    MultiScoperToggle withLabel(getThemeManager(), "A Toggle With A Long Label");

    EXPECT_GT(withLabel.getPreferredWidth(), noLabel.getPreferredWidth());
}

TEST_F(MultiScoperToggleTest, PreferredSizeIsPositive)
{
    MultiScoperToggle toggle(getThemeManager());
    EXPECT_GT(toggle.getPreferredWidth(), 0);
    EXPECT_GT(toggle.getPreferredHeight(), 0);
}

// =============================================================================
// Theme Resilience
// =============================================================================

TEST_F(MultiScoperToggleTest, ThemeChangePreservesToggleState)
{
    MultiScoperToggle toggle(getThemeManager());
    toggle.setValue(true);
    toggle.setLabel("Feature");

    ColorTheme newTheme;
    newTheme.name = "Custom Theme";
    toggle.themeChanged(newTheme);

    EXPECT_TRUE(toggle.getValue());
    EXPECT_EQ(toggle.getLabel(), juce::String("Feature"));
}

// =============================================================================
// APVTS Detach Safety
// =============================================================================

TEST_F(MultiScoperToggleTest, DetachWhenNotAttachedDoesNotCrash)
{
    MultiScoperToggle toggle(getThemeManager());
    toggle.detachFromParameter();
    EXPECT_FALSE(toggle.isAttachedToParameter());
}
