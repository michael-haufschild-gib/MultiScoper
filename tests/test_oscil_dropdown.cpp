/*
    Oscil - Dropdown Component Tests
    Tests for OscilDropdown UI component
*/

#include "OscilTestFixtures.h"
#include "ui/components/OscilDropdown.h"

using namespace oscil;
using namespace oscil::test;

class OscilDropdownTest : public OscilComponentTestFixture
{
protected:
    void SetUp() override { OscilComponentTestFixture::SetUp(); }
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(OscilDropdownTest, DefaultConstruction)
{
    OscilDropdown dropdown(*mockThemeService);

    EXPECT_EQ(dropdown.getNumItems(), 0);
    EXPECT_TRUE(dropdown.isEnabled());
    EXPECT_FALSE(dropdown.isPopupVisible());
    EXPECT_FALSE(dropdown.isMultiSelect());
    EXPECT_FALSE(dropdown.isSearchable());
}

TEST_F(OscilDropdownTest, ConstructionWithPlaceholder)
{
    OscilDropdown dropdown(*mockThemeService, "Select an option...");

    EXPECT_EQ(dropdown.getPlaceholder(), juce::String("Select an option..."));
}

TEST_F(OscilDropdownTest, ConstructionWithPlaceholderAndTestId)
{
    OscilDropdown dropdown(*mockThemeService, "Select...", "dropdown-1");

    EXPECT_EQ(dropdown.getPlaceholder(), juce::String("Select..."));
}

// =============================================================================
// Items Management Tests
// =============================================================================

TEST_F(OscilDropdownTest, AddItemByLabel)
{
    OscilDropdown dropdown(*mockThemeService);

    dropdown.addItem("Option A");
    dropdown.addItem("Option B");
    dropdown.addItem("Option C");

    EXPECT_EQ(dropdown.getNumItems(), 3);
}

TEST_F(OscilDropdownTest, AddItemByLabelAndId)
{
    OscilDropdown dropdown(*mockThemeService);

    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    EXPECT_EQ(dropdown.getNumItems(), 2);
    EXPECT_EQ(dropdown.getItem(0).id, juce::String("a"));
    EXPECT_EQ(dropdown.getItem(0).label, juce::String("Option A"));
}

TEST_F(OscilDropdownTest, AddDropdownItem)
{
    OscilDropdown dropdown(*mockThemeService);

    DropdownItem item;
    item.id = "test";
    item.label = "Test Item";
    item.description = "A test item";

    dropdown.addItem(item);

    EXPECT_EQ(dropdown.getNumItems(), 1);
    EXPECT_EQ(dropdown.getItem(0).description, juce::String("A test item"));
}

TEST_F(OscilDropdownTest, AddMultipleItems)
{
    OscilDropdown dropdown(*mockThemeService);

    std::vector<juce::String> labels = {"One", "Two", "Three"};
    dropdown.addItems(labels);

    EXPECT_EQ(dropdown.getNumItems(), 3);
}

TEST_F(OscilDropdownTest, AddMultipleDropdownItems)
{
    OscilDropdown dropdown(*mockThemeService);

    std::vector<DropdownItem> items;
    items.push_back({.id = "a", .label = "A"});
    items.push_back({.id = "b", .label = "B"});

    dropdown.addItems(items);

    EXPECT_EQ(dropdown.getNumItems(), 2);
}

TEST_F(OscilDropdownTest, AddSeparator)
{
    OscilDropdown dropdown(*mockThemeService);

    dropdown.addItem("Before");
    dropdown.addItem(DropdownItem::separator());
    dropdown.addItem("After");

    EXPECT_EQ(dropdown.getNumItems(), 3);
    EXPECT_TRUE(dropdown.getItem(1).isSeparator);
}

TEST_F(OscilDropdownTest, ClearItems)
{
    OscilDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    dropdown.clearItems();

    EXPECT_EQ(dropdown.getNumItems(), 0);
}

// =============================================================================
// Single Selection Tests
// =============================================================================

TEST_F(OscilDropdownTest, SetSelectedIndex)
{
    OscilDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");
    dropdown.addItem("Option C", "c");

    dropdown.setSelectedIndex(1, false);

    EXPECT_EQ(dropdown.getSelectedIndex(), 1);
}

TEST_F(OscilDropdownTest, GetSelectedId)
{
    OscilDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    dropdown.setSelectedIndex(1, false);

    EXPECT_EQ(dropdown.getSelectedId(), juce::String("b"));
}

TEST_F(OscilDropdownTest, GetSelectedLabel)
{
    OscilDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    dropdown.setSelectedIndex(0, false);

    EXPECT_EQ(dropdown.getSelectedLabel(), juce::String("Option A"));
}

TEST_F(OscilDropdownTest, NoSelectionReturnsMinusOne)
{
    OscilDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");

    EXPECT_EQ(dropdown.getSelectedIndex(), -1);
}

TEST_F(OscilDropdownTest, NoSelectionReturnsEmptyId)
{
    OscilDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");

    EXPECT_TRUE(dropdown.getSelectedId().isEmpty());
}

// =============================================================================
// Multi-Selection Tests
// =============================================================================

TEST_F(OscilDropdownTest, DefaultNotMultiSelect)
{
    OscilDropdown dropdown(*mockThemeService);

    EXPECT_FALSE(dropdown.isMultiSelect());
}

TEST_F(OscilDropdownTest, SetMultiSelect)
{
    OscilDropdown dropdown(*mockThemeService);

    dropdown.setMultiSelect(true);
    EXPECT_TRUE(dropdown.isMultiSelect());

    dropdown.setMultiSelect(false);
    EXPECT_FALSE(dropdown.isMultiSelect());
}

TEST_F(OscilDropdownTest, SetSelectedIndices)
{
    OscilDropdown dropdown(*mockThemeService);
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

TEST_F(OscilDropdownTest, GetSelectedIds)
{
    OscilDropdown dropdown(*mockThemeService);
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

TEST_F(OscilDropdownTest, GetSelectedLabels)
{
    OscilDropdown dropdown(*mockThemeService);
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

TEST_F(OscilDropdownTest, DefaultPlaceholder)
{
    OscilDropdown dropdown(*mockThemeService);

    EXPECT_EQ(dropdown.getPlaceholder(), juce::String("Select..."));
}

TEST_F(OscilDropdownTest, SetPlaceholder)
{
    OscilDropdown dropdown(*mockThemeService);
    dropdown.setPlaceholder("Choose an option");

    EXPECT_EQ(dropdown.getPlaceholder(), juce::String("Choose an option"));
}

// =============================================================================
// Searchable Tests
// =============================================================================

TEST_F(OscilDropdownTest, DefaultNotSearchable)
{
    OscilDropdown dropdown(*mockThemeService);

    EXPECT_FALSE(dropdown.isSearchable());
}

TEST_F(OscilDropdownTest, SetSearchable)
{
    OscilDropdown dropdown(*mockThemeService);

    dropdown.setSearchable(true);
    EXPECT_TRUE(dropdown.isSearchable());

    dropdown.setSearchable(false);
    EXPECT_FALSE(dropdown.isSearchable());
}

// =============================================================================
// Enabled/Disabled Tests
// =============================================================================

TEST_F(OscilDropdownTest, DefaultEnabled)
{
    OscilDropdown dropdown(*mockThemeService);

    EXPECT_TRUE(dropdown.isEnabled());
}

TEST_F(OscilDropdownTest, SetDisabled)
{
    OscilDropdown dropdown(*mockThemeService);
    dropdown.setEnabled(false);

    EXPECT_FALSE(dropdown.isEnabled());
}

TEST_F(OscilDropdownTest, SetEnabledAfterDisabled)
{
    OscilDropdown dropdown(*mockThemeService);
    dropdown.setEnabled(false);
    dropdown.setEnabled(true);

    EXPECT_TRUE(dropdown.isEnabled());
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(OscilDropdownTest, OnSelectionChangedCallback)
{
    OscilDropdown dropdown(*mockThemeService);
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

TEST_F(OscilDropdownTest, OnSelectionChangedIdCallback)
{
    OscilDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");
    dropdown.addItem("Option B", "b");

    juce::String lastId;

    dropdown.onSelectionChangedId = [&](const juce::String& id) {
        lastId = id;
    };

    dropdown.setSelectedIndex(1, true);

    EXPECT_EQ(lastId, juce::String("b"));
}

TEST_F(OscilDropdownTest, OnMultiSelectionChangedCallback)
{
    OscilDropdown dropdown(*mockThemeService);
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

TEST_F(OscilDropdownTest, NoCallbackWhenNotifyFalse)
{
    OscilDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");

    int changeCount = 0;

    dropdown.onSelectionChanged = [&](int) {
        changeCount++;
    };

    dropdown.setSelectedIndex(0, false);

    EXPECT_EQ(changeCount, 0);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(OscilDropdownTest, PreferredWidthPositive)
{
    OscilDropdown dropdown(*mockThemeService);
    dropdown.addItem("Option A", "a");

    EXPECT_GT(dropdown.getPreferredWidth(), 0);
}

TEST_F(OscilDropdownTest, PreferredHeightPositive)
{
    OscilDropdown dropdown(*mockThemeService);

    EXPECT_GT(dropdown.getPreferredHeight(), 0);
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(OscilDropdownTest, ThemeChangeDoesNotThrow)
{
    OscilDropdown dropdown(*mockThemeService);
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

TEST_F(OscilDropdownTest, WantsKeyboardFocus)
{
    OscilDropdown dropdown(*mockThemeService);

    EXPECT_TRUE(dropdown.getWantsKeyboardFocus());
}

// =============================================================================
// Popup Tests
// =============================================================================

TEST_F(OscilDropdownTest, DefaultPopupNotVisible)
{
    OscilDropdown dropdown(*mockThemeService);

    EXPECT_FALSE(dropdown.isPopupVisible());
}

TEST_F(OscilDropdownTest, HidePopupWhenNotVisible)
{
    OscilDropdown dropdown(*mockThemeService);

    // Should not crash
    dropdown.hidePopup();

    EXPECT_FALSE(dropdown.isPopupVisible());
}
