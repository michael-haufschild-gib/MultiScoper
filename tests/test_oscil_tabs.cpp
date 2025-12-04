/*
    Oscil - Tabs Component Tests
    Tests for OscilTabs UI component
*/

#include <gtest/gtest.h>
#include "ui/components/OscilTabs.h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class OscilTabsTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(OscilTabsTest, DefaultConstruction)
{
    OscilTabs tabs(ThemeManager::getInstance());

    EXPECT_EQ(tabs.getNumTabs(), 0);
    EXPECT_EQ(tabs.getSelectedIndex(), 0);  // Default selected index
}

TEST_F(OscilTabsTest, ConstructionWithOrientation)
{
    OscilTabs tabs(ThemeManager::getInstance(), OscilTabs::Orientation::Vertical);

    EXPECT_EQ(tabs.getOrientation(), OscilTabs::Orientation::Vertical);
}

TEST_F(OscilTabsTest, ConstructionWithOrientationAndTestId)
{
    OscilTabs tabs(ThemeManager::getInstance(), OscilTabs::Orientation::Horizontal, "tabs-1");

    EXPECT_EQ(tabs.getOrientation(), OscilTabs::Orientation::Horizontal);
}

// =============================================================================
// Tab Management Tests
// =============================================================================

TEST_F(OscilTabsTest, AddTabWithLabel)
{
    OscilTabs tabs(ThemeManager::getInstance());

    tabs.addTab("Tab A");
    tabs.addTab("Tab B");
    tabs.addTab("Tab C");

    EXPECT_EQ(tabs.getNumTabs(), 3);
}

TEST_F(OscilTabsTest, AddTabWithLabelAndId)
{
    OscilTabs tabs(ThemeManager::getInstance());

    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");

    EXPECT_EQ(tabs.getNumTabs(), 2);
}

TEST_F(OscilTabsTest, AddTabItem)
{
    OscilTabs tabs(ThemeManager::getInstance());

    TabItem item;
    item.id = "test";
    item.label = "Test Tab";
    tabs.addTab(item);

    EXPECT_EQ(tabs.getNumTabs(), 1);
    EXPECT_EQ(tabs.getTab(0).label, juce::String("Test Tab"));
}

TEST_F(OscilTabsTest, AddMultipleTabs)
{
    OscilTabs tabs(ThemeManager::getInstance());

    tabs.addTabs({"One", "Two", "Three"});

    EXPECT_EQ(tabs.getNumTabs(), 3);
}

TEST_F(OscilTabsTest, ClearTabs)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.addTab("Tab A");
    tabs.addTab("Tab B");

    tabs.clearTabs();
    EXPECT_EQ(tabs.getNumTabs(), 0);
}

TEST_F(OscilTabsTest, GetTab)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");

    const auto& tab = tabs.getTab(1);
    EXPECT_EQ(tab.label, juce::String("Tab B"));
    EXPECT_EQ(tab.id, juce::String("b"));
}

TEST_F(OscilTabsTest, SetTabBadge)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.addTab("Tab A", "a");

    tabs.setTabBadge(0, 5);
    EXPECT_EQ(tabs.getTab(0).badgeCount, 5);
}

TEST_F(OscilTabsTest, SetTabEnabled)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.addTab("Tab A", "a");

    tabs.setTabEnabled(0, false);
    EXPECT_FALSE(tabs.getTab(0).enabled);

    tabs.setTabEnabled(0, true);
    EXPECT_TRUE(tabs.getTab(0).enabled);
}

// =============================================================================
// Selection Tests
// =============================================================================

TEST_F(OscilTabsTest, SetSelectedIndex)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");
    tabs.addTab("Tab C", "c");

    tabs.setSelectedIndex(1, false);
    EXPECT_EQ(tabs.getSelectedIndex(), 1);
}

TEST_F(OscilTabsTest, GetSelectedId)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");

    tabs.setSelectedIndex(1, false);
    EXPECT_EQ(tabs.getSelectedId(), juce::String("b"));
}

TEST_F(OscilTabsTest, SetSelectedById)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");

    tabs.setSelectedById("b", false);
    EXPECT_EQ(tabs.getSelectedIndex(), 1);
}

TEST_F(OscilTabsTest, FirstTabAutoSelected)
{
    OscilTabs tabs(ThemeManager::getInstance());

    tabs.addTab("Tab A", "a");
    // First tab should be auto-selected (index 0)
    EXPECT_EQ(tabs.getSelectedIndex(), 0);
}

TEST_F(OscilTabsTest, InvalidIndexHandling)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.addTab("Tab A", "a");
    tabs.setSelectedIndex(0, false);

    tabs.setSelectedIndex(999, false);
    // Should handle gracefully
}

// =============================================================================
// Configuration Tests
// =============================================================================

TEST_F(OscilTabsTest, SetOrientationHorizontal)
{
    OscilTabs tabs(ThemeManager::getInstance());

    tabs.setOrientation(OscilTabs::Orientation::Horizontal);
    EXPECT_EQ(tabs.getOrientation(), OscilTabs::Orientation::Horizontal);
}

TEST_F(OscilTabsTest, SetOrientationVertical)
{
    OscilTabs tabs(ThemeManager::getInstance());

    tabs.setOrientation(OscilTabs::Orientation::Vertical);
    EXPECT_EQ(tabs.getOrientation(), OscilTabs::Orientation::Vertical);
}

TEST_F(OscilTabsTest, SetVariantDefault)
{
    OscilTabs tabs(ThemeManager::getInstance());

    tabs.setVariant(OscilTabs::Variant::Default);
    EXPECT_EQ(tabs.getVariant(), OscilTabs::Variant::Default);
}

TEST_F(OscilTabsTest, SetVariantPills)
{
    OscilTabs tabs(ThemeManager::getInstance());

    tabs.setVariant(OscilTabs::Variant::Pills);
    EXPECT_EQ(tabs.getVariant(), OscilTabs::Variant::Pills);
}

TEST_F(OscilTabsTest, SetVariantBordered)
{
    OscilTabs tabs(ThemeManager::getInstance());

    tabs.setVariant(OscilTabs::Variant::Bordered);
    EXPECT_EQ(tabs.getVariant(), OscilTabs::Variant::Bordered);
}

TEST_F(OscilTabsTest, SetTabWidth)
{
    OscilTabs tabs(ThemeManager::getInstance());

    tabs.setTabWidth(100);
    EXPECT_EQ(tabs.getTabWidth(), 100);
}

TEST_F(OscilTabsTest, SetTabWidthZeroForAuto)
{
    OscilTabs tabs(ThemeManager::getInstance());

    tabs.setTabWidth(0);  // Auto width
    EXPECT_EQ(tabs.getTabWidth(), 0);
}

TEST_F(OscilTabsTest, SetTabHeight)
{
    OscilTabs tabs(ThemeManager::getInstance());

    tabs.setTabHeight(50);
    EXPECT_EQ(tabs.getTabHeight(), 50);
}

TEST_F(OscilTabsTest, SetStretchTabs)
{
    OscilTabs tabs(ThemeManager::getInstance());

    tabs.setStretchTabs(true);
    EXPECT_TRUE(tabs.isStretchTabs());

    tabs.setStretchTabs(false);
    EXPECT_FALSE(tabs.isStretchTabs());
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(OscilTabsTest, OnTabChangedCallback)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");

    int changeCount = 0;
    int lastIndex = -1;

    tabs.onTabChanged = [&changeCount, &lastIndex](int index) {
        changeCount++;
        lastIndex = index;
    };

    tabs.setSelectedIndex(1, true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastIndex, 1);
}

TEST_F(OscilTabsTest, OnTabChangedIdCallback)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");

    int changeCount = 0;
    juce::String lastId;

    tabs.onTabChangedId = [&changeCount, &lastId](const juce::String& id) {
        changeCount++;
        lastId = id;
    };

    tabs.setSelectedIndex(1, true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastId, juce::String("b"));
}

TEST_F(OscilTabsTest, NoCallbackWhenNotifyFalse)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");

    int changeCount = 0;

    tabs.onTabChanged = [&changeCount](int) {
        changeCount++;
    };

    tabs.setSelectedIndex(1, false);
    EXPECT_EQ(changeCount, 0);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(OscilTabsTest, PreferredWidthPositive)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");

    int width = tabs.getPreferredWidth();
    EXPECT_GT(width, 0);
}

TEST_F(OscilTabsTest, PreferredHeightPositive)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");

    int height = tabs.getPreferredHeight();
    EXPECT_GT(height, 0);
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(OscilTabsTest, ThemeChangeDoesNotThrow)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.addTab("Tab A", "a");
    tabs.setSelectedIndex(0, false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    tabs.themeChanged(newTheme);

    // Selection should be preserved
    EXPECT_EQ(tabs.getSelectedIndex(), 0);
}

TEST_F(OscilTabsTest, ThemeChangePreservesOrientation)
{
    OscilTabs tabs(ThemeManager::getInstance());
    tabs.setOrientation(OscilTabs::Orientation::Vertical);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    tabs.themeChanged(newTheme);

    EXPECT_EQ(tabs.getOrientation(), OscilTabs::Orientation::Vertical);
}

// =============================================================================
// Focus Tests
// =============================================================================

TEST_F(OscilTabsTest, WantsKeyboardFocus)
{
    OscilTabs tabs(ThemeManager::getInstance());

    EXPECT_TRUE(tabs.getWantsKeyboardFocus());
}
