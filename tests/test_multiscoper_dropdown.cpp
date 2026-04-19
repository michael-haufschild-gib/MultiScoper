/*
    MultiScoper - Dropdown Component Tests
    Tests for MultiScoperDropdown UI component
*/

#include "ui/components/MultiScoperDropdown.h"

#include "MultiScoperTestFixtures.h"

using namespace multiscoper;
using namespace multiscoper::test;

class MultiScoperDropdownTest : public MultiScoperComponentTestFixture
{
protected:
    void SetUp() override { MultiScoperComponentTestFixture::SetUp(); }
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(MultiScoperDropdownTest, DefaultConstruction)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    EXPECT_EQ(dropdown.getNumItems(), 0);
    EXPECT_TRUE(dropdown.isEnabled());
    EXPECT_FALSE(dropdown.isPopupVisible());
    EXPECT_FALSE(dropdown.isMultiSelect());
    EXPECT_FALSE(dropdown.isSearchable());
}

TEST_F(MultiScoperDropdownTest, ConstructionWithPlaceholder)
{
    MultiScoperDropdown dropdown(*mockThemeService, "Select an option...");

    EXPECT_EQ(dropdown.getPlaceholder(), juce::String("Select an option..."));
}

TEST_F(MultiScoperDropdownTest, ConstructionWithPlaceholderAndTestId)
{
    MultiScoperDropdown dropdown(*mockThemeService, "Select...", "dropdown-1");

    EXPECT_EQ(dropdown.getPlaceholder(), juce::String("Select..."));
}

// =============================================================================
// Items Management Tests
// =============================================================================

TEST_F(MultiScoperDropdownTest, AddItemByLabel)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    dropdown.addItem("Option A");
    dropdown.addItem("Option B");
    dropdown.addItem("Option C");

    EXPECT_EQ(dropdown.getNumItems(), 3);
}

TEST_F(MultiScoperDropdownTest, AddItemByLabelAndId)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    EXPECT_EQ(dropdown.getNumItems(), 2);
    EXPECT_EQ(dropdown.getItem(0).id, juce::String("a"));
    EXPECT_EQ(dropdown.getItem(0).label, juce::String("Option A"));
}

TEST_F(MultiScoperDropdownTest, AddDropdownItem)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    DropdownItem item;
    item.id = "test";
    item.label = "Test Item";
    item.description = "A test item";

    dropdown.addItem(item);

    EXPECT_EQ(dropdown.getNumItems(), 1);
    EXPECT_EQ(dropdown.getItem(0).description, juce::String("A test item"));
}

TEST_F(MultiScoperDropdownTest, AddMultipleItems)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    std::vector<juce::String> labels = {"One", "Two", "Three"};
    dropdown.addItems(labels);

    EXPECT_EQ(dropdown.getNumItems(), 3);
}

TEST_F(MultiScoperDropdownTest, AddMultipleDropdownItems)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    std::vector<DropdownItem> items;
    items.push_back({.id = "a", .label = "A"});
    items.push_back({.id = "b", .label = "B"});

    dropdown.addItems(items);

    EXPECT_EQ(dropdown.getNumItems(), 2);
}

TEST_F(MultiScoperDropdownTest, AddSeparator)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    dropdown.addItem("Before");
    dropdown.addItem(DropdownItem::separator());
    dropdown.addItem("After");

    EXPECT_EQ(dropdown.getNumItems(), 3);
    EXPECT_TRUE(dropdown.getItem(1).isSeparator);
}

TEST_F(MultiScoperDropdownTest, ClearItems)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    dropdown.clearItems();

    EXPECT_EQ(dropdown.getNumItems(), 0);
}

// =============================================================================
// Single Selection Tests
// =============================================================================

TEST_F(MultiScoperDropdownTest, SetSelectedIndex)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");
    dropdown.addItem("Option C", "c");

    dropdown.setSelectedIndex(1, false);

    EXPECT_EQ(dropdown.getSelectedIndex(), 1);
}

TEST_F(MultiScoperDropdownTest, GetSelectedId)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    dropdown.setSelectedIndex(1, false);

    EXPECT_EQ(dropdown.getSelectedId(), juce::String("b"));
}

TEST_F(MultiScoperDropdownTest, GetSelectedLabel)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    dropdown.setSelectedIndex(0, false);

    EXPECT_EQ(dropdown.getSelectedLabel(), juce::String("Option A"));
}

TEST_F(MultiScoperDropdownTest, NoSelectionReturnsMinusOne)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");

    EXPECT_EQ(dropdown.getSelectedIndex(), -1);
}

TEST_F(MultiScoperDropdownTest, NoSelectionReturnsEmptyId)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");

    EXPECT_TRUE(dropdown.getSelectedId().isEmpty());
}

// =============================================================================
// Multi-Selection Tests
// =============================================================================

TEST_F(MultiScoperDropdownTest, DefaultNotMultiSelect)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    EXPECT_FALSE(dropdown.isMultiSelect());
}

TEST_F(MultiScoperDropdownTest, SetMultiSelect)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    dropdown.setMultiSelect(true);
    EXPECT_TRUE(dropdown.isMultiSelect());

    dropdown.setMultiSelect(false);
    EXPECT_FALSE(dropdown.isMultiSelect());
}

TEST_F(MultiScoperDropdownTest, SetSelectedIndices)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.setMultiSelect(true);
    dropdown.addItem("A", "a");
    dropdown.addItem("B", "b");
    dropdown.addItem("C", "c");

    std::set<int> indices = {0, 2};
    dropdown.setSelectedIndices(indices, false);

    auto selected = dropdown.getSelectedIndices();
    EXPECT_EQ(selected.size(), 2);
    EXPECT_TRUE(selected.count(0) > 0);
    EXPECT_TRUE(selected.count(2) > 0);
}

TEST_F(MultiScoperDropdownTest, GetSelectedIds)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.setMultiSelect(true);
    dropdown.addItem("A", "a");
    dropdown.addItem("B", "b");
    dropdown.addItem("C", "c");

    std::set<int> indices = {0, 2};
    dropdown.setSelectedIndices(indices, false);

    auto ids = dropdown.getSelectedIds();
    EXPECT_EQ(ids.size(), 2);

    bool foundA = std::find(ids.begin(), ids.end(), "a") != ids.end();
    bool foundC = std::find(ids.begin(), ids.end(), "c") != ids.end();
    EXPECT_TRUE(foundA);
    EXPECT_TRUE(foundC);
}

TEST_F(MultiScoperDropdownTest, GetSelectedLabels)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.setMultiSelect(true);
    dropdown.addItem("Alpha", "a");
    dropdown.addItem("Beta", "b");
    dropdown.addItem("Gamma", "c");

    std::set<int> indices = {1};
    dropdown.setSelectedIndices(indices, false);

    auto labels = dropdown.getSelectedLabels();
    EXPECT_EQ(labels.size(), 1);
    EXPECT_EQ(labels[0], juce::String("Beta"));
}

// =============================================================================
// Placeholder Tests
// =============================================================================

TEST_F(MultiScoperDropdownTest, DefaultPlaceholder)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    EXPECT_EQ(dropdown.getPlaceholder(), juce::String("Select..."));
}

TEST_F(MultiScoperDropdownTest, SetPlaceholder)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.setPlaceholder("Choose an option");

    EXPECT_EQ(dropdown.getPlaceholder(), juce::String("Choose an option"));
}

// =============================================================================
// Searchable Tests
// =============================================================================

TEST_F(MultiScoperDropdownTest, DefaultNotSearchable)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    EXPECT_FALSE(dropdown.isSearchable());
}

TEST_F(MultiScoperDropdownTest, SetSearchable)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    dropdown.setSearchable(true);
    EXPECT_TRUE(dropdown.isSearchable());

    dropdown.setSearchable(false);
    EXPECT_FALSE(dropdown.isSearchable());
}

// =============================================================================
// Enabled/Disabled Tests
// =============================================================================

TEST_F(MultiScoperDropdownTest, DefaultEnabled)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    EXPECT_TRUE(dropdown.isEnabled());
    EXPECT_EQ(dropdown.getSelectedIndex(), -1);
}

TEST_F(MultiScoperDropdownTest, SetDisabled)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.setEnabled(false);

    EXPECT_FALSE(dropdown.isEnabled());
}

TEST_F(MultiScoperDropdownTest, SetEnabledAfterDisabled)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.setEnabled(false);
    EXPECT_FALSE(dropdown.isEnabled());

    dropdown.setEnabled(true);
    EXPECT_TRUE(dropdown.isEnabled());
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(MultiScoperDropdownTest, OnSelectionChangedCallback)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    int changeCount = 0;
    int lastIndex = -1;

    dropdown.onSelectionChanged = [&](int index) {
        changeCount++;
        lastIndex = index;
    };

    dropdown.setSelectedIndex(1, true);

    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastIndex, 1);
}

TEST_F(MultiScoperDropdownTest, OnSelectionChangedIdCallback)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    juce::String lastId;

    dropdown.onSelectionChangedId = [&](const juce::String& id) { lastId = id; };

    dropdown.setSelectedIndex(1, true);

    EXPECT_EQ(lastId, juce::String("b"));
}

TEST_F(MultiScoperDropdownTest, OnMultiSelectionChangedCallback)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.setMultiSelect(true);
    dropdown.addItem("A", "a");
    dropdown.addItem("B", "b");

    int changeCount = 0;
    size_t lastSize = 0;

    dropdown.onMultiSelectionChanged = [&](const std::set<int>& indices) {
        changeCount++;
        lastSize = indices.size();
    };

    dropdown.setSelectedIndices({0, 1}, true);

    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastSize, 2);
}

TEST_F(MultiScoperDropdownTest, NoCallbackWhenNotifyFalse)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");

    int changeCount = 0;

    dropdown.onSelectionChanged = [&](int) { changeCount++; };

    dropdown.setSelectedIndex(0, false);

    EXPECT_EQ(changeCount, 0);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(MultiScoperDropdownTest, PreferredWidthPositive)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");

    EXPECT_GT(dropdown.getPreferredWidth(), 0);
}

TEST_F(MultiScoperDropdownTest, PreferredHeightPositive)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    EXPECT_GT(dropdown.getPreferredHeight(), 0);
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(MultiScoperDropdownTest, ThemeChangeDoesNotThrow)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");
    dropdown.setSelectedIndex(0, false);

    ColorTheme newTheme;
    newTheme.name = "Custom Theme";

    // Should not throw
    dropdown.themeChanged(newTheme);

    // Selection should be preserved
    EXPECT_EQ(dropdown.getSelectedIndex(), 0);
}

// =============================================================================
// Focus Tests
// =============================================================================

TEST_F(MultiScoperDropdownTest, WantsKeyboardFocus)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    EXPECT_TRUE(dropdown.getWantsKeyboardFocus());
}

// =============================================================================
// Popup Tests
// =============================================================================

TEST_F(MultiScoperDropdownTest, DefaultPopupNotVisible)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    EXPECT_FALSE(dropdown.isPopupVisible());
}

TEST_F(MultiScoperDropdownTest, HidePopupWhenNotVisible)
{
    MultiScoperDropdown dropdown(*mockThemeService);

    // Should not crash
    dropdown.hidePopup();

    EXPECT_FALSE(dropdown.isPopupVisible());
}
