/*
    Oscil - Checkbox Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilCheckbox.h"

using namespace oscil;

class OscilCheckboxTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction - unchecked
TEST_F(OscilCheckboxTest, DefaultConstruction)
{
    OscilCheckbox checkbox;

    EXPECT_EQ(checkbox.getState(), CheckboxState::Unchecked);
    EXPECT_TRUE(checkbox.isEnabled());
}

// Test: Set state
TEST_F(OscilCheckboxTest, SetState)
{
    OscilCheckbox checkbox;

    checkbox.setState(CheckboxState::Checked);
    EXPECT_EQ(checkbox.getState(), CheckboxState::Checked);

    checkbox.setState(CheckboxState::Unchecked);
    EXPECT_EQ(checkbox.getState(), CheckboxState::Unchecked);

    checkbox.setState(CheckboxState::Indeterminate);
    EXPECT_EQ(checkbox.getState(), CheckboxState::Indeterminate);
}

// Test: isChecked convenience method
TEST_F(OscilCheckboxTest, IsChecked)
{
    OscilCheckbox checkbox;

    checkbox.setState(CheckboxState::Unchecked);
    EXPECT_FALSE(checkbox.isChecked());

    checkbox.setState(CheckboxState::Checked);
    EXPECT_TRUE(checkbox.isChecked());

    checkbox.setState(CheckboxState::Indeterminate);
    EXPECT_FALSE(checkbox.isChecked());
}

// Test: Toggle state
TEST_F(OscilCheckboxTest, ToggleState)
{
    OscilCheckbox checkbox;
    checkbox.setState(CheckboxState::Unchecked);

    checkbox.toggle();
    EXPECT_EQ(checkbox.getState(), CheckboxState::Checked);

    checkbox.toggle();
    EXPECT_EQ(checkbox.getState(), CheckboxState::Unchecked);
}

// Test: Toggle from indeterminate
TEST_F(OscilCheckboxTest, ToggleFromIndeterminate)
{
    OscilCheckbox checkbox;
    checkbox.setState(CheckboxState::Indeterminate);

    checkbox.toggle();
    // Should go to checked from indeterminate
    EXPECT_EQ(checkbox.getState(), CheckboxState::Checked);
}

// Test: Set label
TEST_F(OscilCheckboxTest, SetLabel)
{
    OscilCheckbox checkbox;
    checkbox.setLabel("Accept Terms");

    EXPECT_EQ(checkbox.getLabel(), juce::String("Accept Terms"));
}

// Test: Label position
TEST_F(OscilCheckboxTest, LabelPosition)
{
    OscilCheckbox checkbox;

    checkbox.setLabelPosition(OscilCheckbox::LabelPosition::Left);
    EXPECT_EQ(checkbox.getLabelPosition(), OscilCheckbox::LabelPosition::Left);

    checkbox.setLabelPosition(OscilCheckbox::LabelPosition::Right);
    EXPECT_EQ(checkbox.getLabelPosition(), OscilCheckbox::LabelPosition::Right);
}

// Test: Enabled/disabled state
TEST_F(OscilCheckboxTest, EnabledState)
{
    OscilCheckbox checkbox;

    EXPECT_TRUE(checkbox.isEnabled());

    checkbox.setEnabled(false);
    EXPECT_FALSE(checkbox.isEnabled());

    checkbox.setEnabled(true);
    EXPECT_TRUE(checkbox.isEnabled());
}

// Test: Toggle disabled does nothing
TEST_F(OscilCheckboxTest, ToggleDisabledDoesNothing)
{
    OscilCheckbox checkbox;
    checkbox.setState(CheckboxState::Unchecked);
    checkbox.setEnabled(false);

    checkbox.toggle();
    EXPECT_EQ(checkbox.getState(), CheckboxState::Unchecked);
}

// Test: Callback invocation
TEST_F(OscilCheckboxTest, CallbackInvocation)
{
    OscilCheckbox checkbox;
    int changeCount = 0;
    CheckboxState lastState = CheckboxState::Unchecked;

    checkbox.onStateChanged = [&changeCount, &lastState](CheckboxState state) {
        changeCount++;
        lastState = state;
    };

    checkbox.setState(CheckboxState::Checked, true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastState, CheckboxState::Checked);

    checkbox.setState(CheckboxState::Indeterminate, true);
    EXPECT_EQ(changeCount, 2);
    EXPECT_EQ(lastState, CheckboxState::Indeterminate);
}

// Test: Callback not invoked when dontNotify
TEST_F(OscilCheckboxTest, CallbackNotInvokedWhenDontNotify)
{
    OscilCheckbox checkbox;
    int changeCount = 0;

    checkbox.onStateChanged = [&changeCount](CheckboxState) {
        changeCount++;
    };

    checkbox.setState(CheckboxState::Checked, false);
    EXPECT_EQ(changeCount, 0);
}

// Test: Size variants
TEST_F(OscilCheckboxTest, SizeVariants)
{
    OscilCheckbox checkbox;

    checkbox.setSizeVariant(SizeVariant::Small);
    EXPECT_EQ(checkbox.getSizeVariant(), SizeVariant::Small);

    checkbox.setSizeVariant(SizeVariant::Medium);
    EXPECT_EQ(checkbox.getSizeVariant(), SizeVariant::Medium);

    checkbox.setSizeVariant(SizeVariant::Large);
    EXPECT_EQ(checkbox.getSizeVariant(), SizeVariant::Large);
}

// Test: Preferred size
TEST_F(OscilCheckboxTest, PreferredSize)
{
    OscilCheckbox checkbox;
    checkbox.setLabel("Test Checkbox");

    int width = checkbox.getPreferredWidth();
    int height = checkbox.getPreferredHeight();

    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

// Test: Theme changes
TEST_F(OscilCheckboxTest, ThemeChanges)
{
    OscilCheckbox checkbox;
    checkbox.setState(CheckboxState::Checked);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    checkbox.themeChanged(newTheme);

    // State should be preserved
    EXPECT_EQ(checkbox.getState(), CheckboxState::Checked);
}

// Test: Accessibility
TEST_F(OscilCheckboxTest, Accessibility)
{
    OscilCheckbox checkbox;
    checkbox.setLabel("Test Checkbox");

    auto* handler = checkbox.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);

    EXPECT_EQ(handler->getRole(), juce::AccessibilityRole::checkBox);
}

// Test: Focus handling
TEST_F(OscilCheckboxTest, FocusHandling)
{
    OscilCheckbox checkbox;
    EXPECT_TRUE(checkbox.getWantsKeyboardFocus());
}

// Test: Allow indeterminate
TEST_F(OscilCheckboxTest, AllowIndeterminate)
{
    OscilCheckbox checkbox;

    checkbox.setAllowIndeterminate(true);
    EXPECT_TRUE(checkbox.getAllowIndeterminate());

    checkbox.setAllowIndeterminate(false);
    EXPECT_FALSE(checkbox.getAllowIndeterminate());
}

// Test: Cycle through states
TEST_F(OscilCheckboxTest, CycleThroughStates)
{
    OscilCheckbox checkbox;
    checkbox.setAllowIndeterminate(true);
    checkbox.setState(CheckboxState::Unchecked);

    // Toggle cycles: unchecked -> checked -> indeterminate -> unchecked
    checkbox.toggle();
    EXPECT_EQ(checkbox.getState(), CheckboxState::Checked);

    checkbox.toggle();
    EXPECT_EQ(checkbox.getState(), CheckboxState::Indeterminate);

    checkbox.toggle();
    EXPECT_EQ(checkbox.getState(), CheckboxState::Unchecked);
}
