/*
    Oscil - Dropdown Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilDropdown.h"

using namespace oscil;

class OscilDropdownTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction
TEST_F(OscilDropdownTest, DefaultConstruction)
{
    OscilDropdown dropdown;

    EXPECT_EQ(dropdown.getNumItems(), 0);
    EXPECT_TRUE(dropdown.isEnabled());
    EXPECT_FALSE(dropdown.isOpen());
}

// Test: Add items
TEST_F(OscilDropdownTest, AddItems)
{
    OscilDropdown dropdown;

    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");
    dropdown.addItem("Option C", "c");

    EXPECT_EQ(dropdown.getNumItems(), 3);
}

// Test: Get selected value
TEST_F(OscilDropdownTest, GetSelectedValue)
{
    OscilDropdown dropdown;
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    dropdown.setSelectedValue("b");
    EXPECT_EQ(dropdown.getSelectedValue(), juce::String("b"));
}

// Test: Get selected index
TEST_F(OscilDropdownTest, GetSelectedIndex)
{
    OscilDropdown dropdown;
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");
    dropdown.addItem("Option C", "c");

    dropdown.setSelectedIndex(2);
    EXPECT_EQ(dropdown.getSelectedIndex(), 2);
    EXPECT_EQ(dropdown.getSelectedValue(), juce::String("c"));
}

// Test: Get selected text
TEST_F(OscilDropdownTest, GetSelectedText)
{
    OscilDropdown dropdown;
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    dropdown.setSelectedValue("a");
    EXPECT_EQ(dropdown.getSelectedText(), juce::String("Option A"));
}

// Test: Set placeholder
TEST_F(OscilDropdownTest, SetPlaceholder)
{
    OscilDropdown dropdown;
    dropdown.setPlaceholder("Select an option...");

    EXPECT_EQ(dropdown.getPlaceholder(), juce::String("Select an option..."));
}

// Test: Clear items
TEST_F(OscilDropdownTest, ClearItems)
{
    OscilDropdown dropdown;
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    dropdown.clearItems();
    EXPECT_EQ(dropdown.getNumItems(), 0);
}

// Test: Enabled/disabled state
TEST_F(OscilDropdownTest, EnabledState)
{
    OscilDropdown dropdown;

    EXPECT_TRUE(dropdown.isEnabled());

    dropdown.setEnabled(false);
    EXPECT_FALSE(dropdown.isEnabled());

    dropdown.setEnabled(true);
    EXPECT_TRUE(dropdown.isEnabled());
}

// Test: Multi-select mode
TEST_F(OscilDropdownTest, MultiSelectMode)
{
    OscilDropdown dropdown;

    EXPECT_FALSE(dropdown.isMultiSelect());

    dropdown.setMultiSelect(true);
    EXPECT_TRUE(dropdown.isMultiSelect());

    dropdown.setMultiSelect(false);
    EXPECT_FALSE(dropdown.isMultiSelect());
}

// Test: Multi-select values
TEST_F(OscilDropdownTest, MultiSelectValues)
{
    OscilDropdown dropdown;
    dropdown.setMultiSelect(true);

    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");
    dropdown.addItem("Option C", "c");

    std::vector<juce::String> selections = {"a", "c"};
    dropdown.setSelectedValues(selections);

    auto selected = dropdown.getSelectedValues();
    EXPECT_EQ(selected.size(), 2);
    EXPECT_NE(std::find(selected.begin(), selected.end(), "a"), selected.end());
    EXPECT_NE(std::find(selected.begin(), selected.end(), "c"), selected.end());
}

// Test: Searchable mode
TEST_F(OscilDropdownTest, SearchableMode)
{
    OscilDropdown dropdown;

    EXPECT_FALSE(dropdown.isSearchable());

    dropdown.setSearchable(true);
    EXPECT_TRUE(dropdown.isSearchable());

    dropdown.setSearchable(false);
    EXPECT_FALSE(dropdown.isSearchable());
}

// Test: Callback invocation
TEST_F(OscilDropdownTest, CallbackInvocation)
{
    OscilDropdown dropdown;
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    int changeCount = 0;
    juce::String lastValue;

    dropdown.onSelectionChanged = [&changeCount, &lastValue](const juce::String& value) {
        changeCount++;
        lastValue = value;
    };

    dropdown.setSelectedValue("b", true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastValue, juce::String("b"));
}

// Test: Multi-select callback
TEST_F(OscilDropdownTest, MultiSelectCallback)
{
    OscilDropdown dropdown;
    dropdown.setMultiSelect(true);
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    int changeCount = 0;

    dropdown.onMultiSelectionChanged = [&changeCount](const std::vector<juce::String>&) {
        changeCount++;
    };

    dropdown.setSelectedValues({"a", "b"}, true);
    EXPECT_EQ(changeCount, 1);
}

// Test: Invalid value does nothing
TEST_F(OscilDropdownTest, InvalidValueDoesNothing)
{
    OscilDropdown dropdown;
    dropdown.addItem("Option A", "a");
    dropdown.setSelectedValue("a");

    dropdown.setSelectedValue("nonexistent");
    EXPECT_EQ(dropdown.getSelectedValue(), juce::String("a"));
}

// Test: Label
TEST_F(OscilDropdownTest, Label)
{
    OscilDropdown dropdown;
    dropdown.setLabel("Select Option");

    EXPECT_EQ(dropdown.getLabel(), juce::String("Select Option"));
}

// Test: Max visible items
TEST_F(OscilDropdownTest, MaxVisibleItems)
{
    OscilDropdown dropdown;

    dropdown.setMaxVisibleItems(5);
    EXPECT_EQ(dropdown.getMaxVisibleItems(), 5);
}

// Test: Preferred size
TEST_F(OscilDropdownTest, PreferredSize)
{
    OscilDropdown dropdown;
    dropdown.addItem("Option A", "a");

    int width = dropdown.getPreferredWidth();
    int height = dropdown.getPreferredHeight();

    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

// Test: Theme changes
TEST_F(OscilDropdownTest, ThemeChanges)
{
    OscilDropdown dropdown;
    dropdown.addItem("Option A", "a");
    dropdown.setSelectedValue("a");

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    dropdown.themeChanged(newTheme);

    // Selection should be preserved
    EXPECT_EQ(dropdown.getSelectedValue(), juce::String("a"));
}

// Test: Accessibility
TEST_F(OscilDropdownTest, Accessibility)
{
    OscilDropdown dropdown;
    dropdown.setLabel("Test Dropdown");

    auto* handler = dropdown.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);

    EXPECT_EQ(handler->getRole(), juce::AccessibilityRole::comboBox);
}

// Test: Focus handling
TEST_F(OscilDropdownTest, FocusHandling)
{
    OscilDropdown dropdown;
    EXPECT_TRUE(dropdown.getWantsKeyboardFocus());
}

// Test: Add separator
TEST_F(OscilDropdownTest, AddSeparator)
{
    OscilDropdown dropdown;

    dropdown.addItem("Group 1", "g1");
    dropdown.addSeparator();
    dropdown.addItem("Group 2", "g2");

    // Should not throw, separator is valid
    EXPECT_GE(dropdown.getNumItems(), 2);
}

// Test: Add item with icon
TEST_F(OscilDropdownTest, AddItemWithIcon)
{
    OscilDropdown dropdown;

    juce::Path iconPath;
    iconPath.addEllipse(0, 0, 10, 10);

    dropdown.addItem("With Icon", "icon", iconPath);

    EXPECT_EQ(dropdown.getNumItems(), 1);
}
