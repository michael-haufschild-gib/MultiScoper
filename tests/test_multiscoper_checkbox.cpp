/*
    MultiScoper - Checkbox Component Tests
    Tests for MultiScoperCheckbox behavioral correctness

    Bug targets:
    - Toggle state machine incorrect transitions (Indeterminate→toggle should→Checked)
    - Callback fires when notify=false
    - Both callbacks (onCheckedChanged + onStateChanged) fire inconsistently
    - Indeterminate reports isChecked()=true incorrectly
    - State not preserved across theme change
*/

#include "ui/components/MultiScoperCheckbox.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace multiscoper;

class MultiScoperCheckboxTest : public ::testing::Test
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

TEST_F(MultiScoperCheckboxTest, DefaultConstructionState)
{
    MultiScoperCheckbox checkbox(getThemeManager());

    EXPECT_EQ(checkbox.getState(), CheckState::Unchecked);
    EXPECT_FALSE(checkbox.isChecked());
    EXPECT_TRUE(checkbox.isEnabled());
    EXPECT_TRUE(checkbox.getLabel().isEmpty());
    EXPECT_TRUE(checkbox.isLabelOnRight());
    EXPECT_TRUE(checkbox.getWantsKeyboardFocus());
}

// =============================================================================
// Toggle State Machine — the core behavior
// =============================================================================

TEST_F(MultiScoperCheckboxTest, ToggleFromUncheckedToChecked)
{
    MultiScoperCheckbox checkbox(getThemeManager());
    EXPECT_EQ(checkbox.getState(), CheckState::Unchecked);

    checkbox.toggle();
    EXPECT_EQ(checkbox.getState(), CheckState::Checked);
    EXPECT_TRUE(checkbox.isChecked());
}

TEST_F(MultiScoperCheckboxTest, ToggleFromCheckedToUnchecked)
{
    MultiScoperCheckbox checkbox(getThemeManager());
    checkbox.setState(CheckState::Checked, false);

    checkbox.toggle();
    EXPECT_EQ(checkbox.getState(), CheckState::Unchecked);
    EXPECT_FALSE(checkbox.isChecked());
}

TEST_F(MultiScoperCheckboxTest, ToggleFromIndeterminateToChecked)
{
    // Bug: Indeterminate→toggle might go to Unchecked instead of Checked
    MultiScoperCheckbox checkbox(getThemeManager());
    checkbox.setState(CheckState::Indeterminate, false);

    checkbox.toggle();
    EXPECT_EQ(checkbox.getState(), CheckState::Checked);
}

TEST_F(MultiScoperCheckboxTest, IndeterminateIsNotChecked)
{
    // Bug: isChecked() might return true for Indeterminate
    MultiScoperCheckbox checkbox(getThemeManager());
    checkbox.setState(CheckState::Indeterminate, false);

    EXPECT_FALSE(checkbox.isChecked());
    EXPECT_EQ(checkbox.getState(), CheckState::Indeterminate);
}

TEST_F(MultiScoperCheckboxTest, FullToggleCycle)
{
    MultiScoperCheckbox checkbox(getThemeManager());

    // Unchecked → Checked → Unchecked
    EXPECT_FALSE(checkbox.isChecked());
    checkbox.toggle();
    EXPECT_TRUE(checkbox.isChecked());
    checkbox.toggle();
    EXPECT_FALSE(checkbox.isChecked());
}

// =============================================================================
// Callback Behavior
// =============================================================================

TEST_F(MultiScoperCheckboxTest, BothCallbacksFireOnStateChange)
{
    MultiScoperCheckbox checkbox(getThemeManager());

    int checkedCallCount = 0;
    int stateCallCount = 0;

    checkbox.onCheckedChanged = [&](bool) { checkedCallCount++; };
    checkbox.onStateChanged = [&](CheckState) { stateCallCount++; };

    checkbox.setState(CheckState::Checked, true);
    EXPECT_EQ(stateCallCount, 1);
    EXPECT_EQ(checkedCallCount, 1);
}

TEST_F(MultiScoperCheckboxTest, NoCallbackWhenNotifyFalse)
{
    MultiScoperCheckbox checkbox(getThemeManager());

    int callCount = 0;
    checkbox.onCheckedChanged = [&](bool) { callCount++; };
    checkbox.onStateChanged = [&](CheckState) { callCount++; };

    checkbox.setState(CheckState::Checked, false);
    EXPECT_EQ(callCount, 0);
}

TEST_F(MultiScoperCheckboxTest, StateCallbackReceivesAllTransitions)
{
    MultiScoperCheckbox checkbox(getThemeManager());

    std::vector<CheckState> observed;
    checkbox.onStateChanged = [&](CheckState s) { observed.push_back(s); };

    checkbox.setState(CheckState::Checked, true);
    checkbox.setState(CheckState::Indeterminate, true);
    checkbox.setState(CheckState::Unchecked, true);

    ASSERT_EQ(observed.size(), 3u);
    EXPECT_EQ(observed[0], CheckState::Checked);
    EXPECT_EQ(observed[1], CheckState::Indeterminate);
    EXPECT_EQ(observed[2], CheckState::Unchecked);
}

// =============================================================================
// Size Behavior
// =============================================================================

TEST_F(MultiScoperCheckboxTest, LabelIncreasesPreferredWidth)
{
    MultiScoperCheckbox noLabel(getThemeManager());
    MultiScoperCheckbox withLabel(getThemeManager(), "A Reasonably Long Checkbox Label");

    EXPECT_GT(withLabel.getPreferredWidth(), noLabel.getPreferredWidth());
}

// =============================================================================
// Theme Resilience
// =============================================================================

TEST_F(MultiScoperCheckboxTest, ThemeChangePreservesCheckedState)
{
    MultiScoperCheckbox checkbox(getThemeManager());
    checkbox.setState(CheckState::Checked, false);
    checkbox.setLabel("Remember me");

    ColorTheme newTheme;
    newTheme.name = "New Theme";
    checkbox.themeChanged(newTheme);

    EXPECT_EQ(checkbox.getState(), CheckState::Checked);
    EXPECT_EQ(checkbox.getLabel(), juce::String("Remember me"));
}
