/*
    Oscil - RadioButton Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilRadioButton.h"

using namespace oscil;

class OscilRadioButtonTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// ============ OscilRadioButton Tests ============

// Test: Default construction
TEST_F(OscilRadioButtonTest, DefaultConstruction)
{
    OscilRadioButton radio;

    EXPECT_FALSE(radio.isSelected());
    EXPECT_TRUE(radio.isEnabled());
}

// Test: Set selected
TEST_F(OscilRadioButtonTest, SetSelected)
{
    OscilRadioButton radio;

    radio.setSelected(true);
    EXPECT_TRUE(radio.isSelected());

    radio.setSelected(false);
    EXPECT_FALSE(radio.isSelected());
}

// Test: Set label
TEST_F(OscilRadioButtonTest, SetLabel)
{
    OscilRadioButton radio;
    radio.setLabel("Option A");

    EXPECT_EQ(radio.getLabel(), juce::String("Option A"));
}

// Test: Set value
TEST_F(OscilRadioButtonTest, SetValue)
{
    OscilRadioButton radio;
    radio.setValue("optionA");

    EXPECT_EQ(radio.getValue(), juce::String("optionA"));
}

// Test: Enabled/disabled state
TEST_F(OscilRadioButtonTest, EnabledState)
{
    OscilRadioButton radio;

    EXPECT_TRUE(radio.isEnabled());

    radio.setEnabled(false);
    EXPECT_FALSE(radio.isEnabled());

    radio.setEnabled(true);
    EXPECT_TRUE(radio.isEnabled());
}

// Test: Callback invocation
TEST_F(OscilRadioButtonTest, CallbackInvocation)
{
    OscilRadioButton radio;
    int selectCount = 0;

    radio.onSelected = [&selectCount]() {
        selectCount++;
    };

    radio.setSelected(true, true);
    EXPECT_EQ(selectCount, 1);
}

// Test: Size variants
TEST_F(OscilRadioButtonTest, SizeVariants)
{
    OscilRadioButton radio;

    radio.setSizeVariant(SizeVariant::Small);
    EXPECT_EQ(radio.getSizeVariant(), SizeVariant::Small);

    radio.setSizeVariant(SizeVariant::Medium);
    EXPECT_EQ(radio.getSizeVariant(), SizeVariant::Medium);

    radio.setSizeVariant(SizeVariant::Large);
    EXPECT_EQ(radio.getSizeVariant(), SizeVariant::Large);
}

// Test: Preferred size
TEST_F(OscilRadioButtonTest, PreferredSize)
{
    OscilRadioButton radio;
    radio.setLabel("Test Option");

    int width = radio.getPreferredWidth();
    int height = radio.getPreferredHeight();

    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

// Test: Theme changes
TEST_F(OscilRadioButtonTest, ThemeChanges)
{
    OscilRadioButton radio;
    radio.setSelected(true);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    radio.themeChanged(newTheme);

    // State should be preserved
    EXPECT_TRUE(radio.isSelected());
}

// Test: Accessibility
TEST_F(OscilRadioButtonTest, Accessibility)
{
    OscilRadioButton radio;
    radio.setLabel("Test Radio");

    auto* handler = radio.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);

    EXPECT_EQ(handler->getRole(), juce::AccessibilityRole::radioButton);
}

// ============ OscilRadioGroup Tests ============

class OscilRadioGroupTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction
TEST_F(OscilRadioGroupTest, DefaultConstruction)
{
    OscilRadioGroup group;

    EXPECT_EQ(group.getNumOptions(), 0);
    EXPECT_TRUE(group.getSelectedValue().isEmpty());
}

// Test: Add options
TEST_F(OscilRadioGroupTest, AddOptions)
{
    OscilRadioGroup group;

    group.addOption("Option A", "a");
    group.addOption("Option B", "b");
    group.addOption("Option C", "c");

    EXPECT_EQ(group.getNumOptions(), 3);
}

// Test: Set selected value
TEST_F(OscilRadioGroupTest, SetSelectedValue)
{
    OscilRadioGroup group;
    group.addOption("Option A", "a");
    group.addOption("Option B", "b");

    group.setSelectedValue("b");
    EXPECT_EQ(group.getSelectedValue(), juce::String("b"));
}

// Test: Set selected index
TEST_F(OscilRadioGroupTest, SetSelectedIndex)
{
    OscilRadioGroup group;
    group.addOption("Option A", "a");
    group.addOption("Option B", "b");
    group.addOption("Option C", "c");

    group.setSelectedIndex(1);
    EXPECT_EQ(group.getSelectedIndex(), 1);
    EXPECT_EQ(group.getSelectedValue(), juce::String("b"));
}

// Test: Mutual exclusivity
TEST_F(OscilRadioGroupTest, MutualExclusivity)
{
    OscilRadioGroup group;
    group.addOption("Option A", "a");
    group.addOption("Option B", "b");
    group.addOption("Option C", "c");

    group.setSelectedValue("a");
    EXPECT_EQ(group.getSelectedValue(), juce::String("a"));

    group.setSelectedValue("c");
    EXPECT_EQ(group.getSelectedValue(), juce::String("c"));
    // A should no longer be selected
}

// Test: Callback invocation
TEST_F(OscilRadioGroupTest, CallbackInvocation)
{
    OscilRadioGroup group;
    group.addOption("Option A", "a");
    group.addOption("Option B", "b");

    int changeCount = 0;
    juce::String lastValue;

    group.onSelectionChanged = [&changeCount, &lastValue](const juce::String& value) {
        changeCount++;
        lastValue = value;
    };

    group.setSelectedValue("b", true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastValue, juce::String("b"));
}

// Test: Clear options
TEST_F(OscilRadioGroupTest, ClearOptions)
{
    OscilRadioGroup group;
    group.addOption("Option A", "a");
    group.addOption("Option B", "b");

    group.clearOptions();
    EXPECT_EQ(group.getNumOptions(), 0);
}

// Test: Orientation
TEST_F(OscilRadioGroupTest, Orientation)
{
    OscilRadioGroup group;

    group.setOrientation(Orientation::Vertical);
    EXPECT_EQ(group.getOrientation(), Orientation::Vertical);

    group.setOrientation(Orientation::Horizontal);
    EXPECT_EQ(group.getOrientation(), Orientation::Horizontal);
}

// Test: Spacing
TEST_F(OscilRadioGroupTest, Spacing)
{
    OscilRadioGroup group;

    group.setSpacing(20);
    EXPECT_EQ(group.getSpacing(), 20);
}

// Test: Invalid index returns empty
TEST_F(OscilRadioGroupTest, InvalidIndexReturnsEmpty)
{
    OscilRadioGroup group;
    group.addOption("Option A", "a");

    group.setSelectedIndex(999);
    // Should handle gracefully
}

// Test: Invalid value does nothing
TEST_F(OscilRadioGroupTest, InvalidValueDoesNothing)
{
    OscilRadioGroup group;
    group.addOption("Option A", "a");
    group.setSelectedValue("a");

    group.setSelectedValue("nonexistent");
    // Should keep previous selection
    EXPECT_EQ(group.getSelectedValue(), juce::String("a"));
}

// Test: Preferred size
TEST_F(OscilRadioGroupTest, PreferredSize)
{
    OscilRadioGroup group;
    group.addOption("Option A", "a");
    group.addOption("Option B", "b");

    int width = group.getPreferredWidth();
    int height = group.getPreferredHeight();

    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

// Test: Theme changes
TEST_F(OscilRadioGroupTest, ThemeChanges)
{
    OscilRadioGroup group;
    group.addOption("Option A", "a");
    group.setSelectedValue("a");

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    group.themeChanged(newTheme);

    // Selection should be preserved
    EXPECT_EQ(group.getSelectedValue(), juce::String("a"));
}
