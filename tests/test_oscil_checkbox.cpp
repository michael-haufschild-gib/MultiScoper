/*
    Oscil - Checkbox Component Tests
    Tests for OscilCheckbox UI component
*/

#include <gtest/gtest.h>
#include "ui/components/OscilCheckbox.h"

using namespace oscil;

class OscilCheckboxTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(OscilCheckboxTest, DefaultConstruction)
{
    OscilCheckbox checkbox;

    EXPECT_EQ(checkbox.getState(), CheckState::Unchecked);
    EXPECT_FALSE(checkbox.isChecked());
    EXPECT_TRUE(checkbox.isEnabled());
    EXPECT_TRUE(checkbox.getLabel().isEmpty());
}

TEST_F(OscilCheckboxTest, ConstructionWithLabel)
{
    OscilCheckbox checkbox("Accept Terms");

    EXPECT_EQ(checkbox.getLabel(), juce::String("Accept Terms"));
    EXPECT_FALSE(checkbox.isChecked());
}

TEST_F(OscilCheckboxTest, ConstructionWithLabelAndTestId)
{
    OscilCheckbox checkbox("Accept Terms", "checkbox-1");

    EXPECT_EQ(checkbox.getLabel(), juce::String("Accept Terms"));
}

// =============================================================================
// State Tests
// =============================================================================

TEST_F(OscilCheckboxTest, SetCheckedTrue)
{
    OscilCheckbox checkbox;

    checkbox.setChecked(true, false);
    EXPECT_TRUE(checkbox.isChecked());
    EXPECT_EQ(checkbox.getState(), CheckState::Checked);
}

TEST_F(OscilCheckboxTest, SetCheckedFalse)
{
    OscilCheckbox checkbox;
    checkbox.setChecked(true, false);

    checkbox.setChecked(false, false);
    EXPECT_FALSE(checkbox.isChecked());
    EXPECT_EQ(checkbox.getState(), CheckState::Unchecked);
}

TEST_F(OscilCheckboxTest, SetStateUnchecked)
{
    OscilCheckbox checkbox;

    checkbox.setState(CheckState::Unchecked, false);
    EXPECT_EQ(checkbox.getState(), CheckState::Unchecked);
    EXPECT_FALSE(checkbox.isChecked());
}

TEST_F(OscilCheckboxTest, SetStateChecked)
{
    OscilCheckbox checkbox;

    checkbox.setState(CheckState::Checked, false);
    EXPECT_EQ(checkbox.getState(), CheckState::Checked);
    EXPECT_TRUE(checkbox.isChecked());
}

TEST_F(OscilCheckboxTest, SetStateIndeterminate)
{
    OscilCheckbox checkbox;

    checkbox.setState(CheckState::Indeterminate, false);
    EXPECT_EQ(checkbox.getState(), CheckState::Indeterminate);
    EXPECT_FALSE(checkbox.isChecked());  // Indeterminate is not "checked"
}

TEST_F(OscilCheckboxTest, ToggleFromUnchecked)
{
    OscilCheckbox checkbox;
    checkbox.setState(CheckState::Unchecked, false);

    checkbox.toggle();
    EXPECT_EQ(checkbox.getState(), CheckState::Checked);
}

TEST_F(OscilCheckboxTest, ToggleFromChecked)
{
    OscilCheckbox checkbox;
    checkbox.setState(CheckState::Checked, false);

    checkbox.toggle();
    EXPECT_EQ(checkbox.getState(), CheckState::Unchecked);
}

TEST_F(OscilCheckboxTest, ToggleFromIndeterminate)
{
    OscilCheckbox checkbox;
    checkbox.setState(CheckState::Indeterminate, false);

    checkbox.toggle();
    // Should go to checked from indeterminate
    EXPECT_EQ(checkbox.getState(), CheckState::Checked);
}

// =============================================================================
// Label Tests
// =============================================================================

TEST_F(OscilCheckboxTest, SetLabel)
{
    OscilCheckbox checkbox;
    checkbox.setLabel("Accept Terms");

    EXPECT_EQ(checkbox.getLabel(), juce::String("Accept Terms"));
}

TEST_F(OscilCheckboxTest, DefaultLabelOnRight)
{
    OscilCheckbox checkbox;

    EXPECT_TRUE(checkbox.isLabelOnRight());
}

TEST_F(OscilCheckboxTest, SetLabelOnRight)
{
    OscilCheckbox checkbox;

    checkbox.setLabelOnRight(false);
    EXPECT_FALSE(checkbox.isLabelOnRight());

    checkbox.setLabelOnRight(true);
    EXPECT_TRUE(checkbox.isLabelOnRight());
}

// =============================================================================
// Enabled/Disabled Tests
// =============================================================================

TEST_F(OscilCheckboxTest, DefaultEnabled)
{
    OscilCheckbox checkbox;

    EXPECT_TRUE(checkbox.isEnabled());
}

TEST_F(OscilCheckboxTest, SetDisabled)
{
    OscilCheckbox checkbox;
    checkbox.setEnabled(false);

    EXPECT_FALSE(checkbox.isEnabled());
}

TEST_F(OscilCheckboxTest, SetEnabledAfterDisabled)
{
    OscilCheckbox checkbox;
    checkbox.setEnabled(false);
    checkbox.setEnabled(true);

    EXPECT_TRUE(checkbox.isEnabled());
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(OscilCheckboxTest, OnCheckedChangedCallback)
{
    OscilCheckbox checkbox;

    int changeCount = 0;
    bool lastState = false;

    checkbox.onCheckedChanged = [&changeCount, &lastState](bool checked) {
        changeCount++;
        lastState = checked;
    };

    checkbox.setChecked(true, true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_TRUE(lastState);
}

TEST_F(OscilCheckboxTest, OnStateChangedCallback)
{
    OscilCheckbox checkbox;

    int changeCount = 0;
    CheckState lastState = CheckState::Unchecked;

    checkbox.onStateChanged = [&changeCount, &lastState](CheckState state) {
        changeCount++;
        lastState = state;
    };

    checkbox.setState(CheckState::Checked, true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastState, CheckState::Checked);

    checkbox.setState(CheckState::Indeterminate, true);
    EXPECT_EQ(changeCount, 2);
    EXPECT_EQ(lastState, CheckState::Indeterminate);
}

TEST_F(OscilCheckboxTest, NoCallbackWhenNotifyFalse)
{
    OscilCheckbox checkbox;

    int changeCount = 0;

    checkbox.onStateChanged = [&changeCount](CheckState) {
        changeCount++;
    };

    checkbox.setState(CheckState::Checked, false);
    EXPECT_EQ(changeCount, 0);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(OscilCheckboxTest, PreferredWidthPositive)
{
    OscilCheckbox checkbox;

    EXPECT_GT(checkbox.getPreferredWidth(), 0);
}

TEST_F(OscilCheckboxTest, PreferredHeightPositive)
{
    OscilCheckbox checkbox;

    EXPECT_GT(checkbox.getPreferredHeight(), 0);
}

TEST_F(OscilCheckboxTest, CheckboxWithLabelHasGreaterWidth)
{
    OscilCheckbox noLabel;
    OscilCheckbox withLabel("A Very Long Label Here");

    EXPECT_GT(withLabel.getPreferredWidth(), noLabel.getPreferredWidth());
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(OscilCheckboxTest, ThemeChangeDoesNotThrow)
{
    OscilCheckbox checkbox;
    checkbox.setState(CheckState::Checked, false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    checkbox.themeChanged(newTheme);

    // State should be preserved
    EXPECT_EQ(checkbox.getState(), CheckState::Checked);
}

TEST_F(OscilCheckboxTest, ThemeChangePreservesLabel)
{
    OscilCheckbox checkbox;
    checkbox.setLabel("Test Label");

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    checkbox.themeChanged(newTheme);

    EXPECT_EQ(checkbox.getLabel(), juce::String("Test Label"));
}

// =============================================================================
// Focus Tests
// =============================================================================

TEST_F(OscilCheckboxTest, WantsKeyboardFocus)
{
    OscilCheckbox checkbox;

    EXPECT_TRUE(checkbox.getWantsKeyboardFocus());
}
