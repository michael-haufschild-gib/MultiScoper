/*
    Oscil - Tabs Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilTabs.h"

using namespace oscil;

class OscilTabsTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction
TEST_F(OscilTabsTest, DefaultConstruction)
{
    OscilTabs tabs;

    EXPECT_EQ(tabs.getNumTabs(), 0);
    EXPECT_EQ(tabs.getSelectedIndex(), -1);
}

// Test: Add tabs
TEST_F(OscilTabsTest, AddTabs)
{
    OscilTabs tabs;

    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");
    tabs.addTab("Tab C", "c");

    EXPECT_EQ(tabs.getNumTabs(), 3);
}

// Test: Select by index
TEST_F(OscilTabsTest, SelectByIndex)
{
    OscilTabs tabs;
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");
    tabs.addTab("Tab C", "c");

    tabs.setSelectedIndex(1);
    EXPECT_EQ(tabs.getSelectedIndex(), 1);
    EXPECT_EQ(tabs.getSelectedValue(), juce::String("b"));
}

// Test: Select by value
TEST_F(OscilTabsTest, SelectByValue)
{
    OscilTabs tabs;
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");

    tabs.setSelectedValue("b");
    EXPECT_EQ(tabs.getSelectedValue(), juce::String("b"));
    EXPECT_EQ(tabs.getSelectedIndex(), 1);
}

// Test: Get selected text
TEST_F(OscilTabsTest, GetSelectedText)
{
    OscilTabs tabs;
    tabs.addTab("First Tab", "first");
    tabs.addTab("Second Tab", "second");

    tabs.setSelectedValue("first");
    EXPECT_EQ(tabs.getSelectedText(), juce::String("First Tab"));
}

// Test: Orientation
TEST_F(OscilTabsTest, Orientation)
{
    OscilTabs tabs;

    tabs.setOrientation(Orientation::Horizontal);
    EXPECT_EQ(tabs.getOrientation(), Orientation::Horizontal);

    tabs.setOrientation(Orientation::Vertical);
    EXPECT_EQ(tabs.getOrientation(), Orientation::Vertical);
}

// Test: Tab variant
TEST_F(OscilTabsTest, TabVariant)
{
    OscilTabs tabs;

    tabs.setVariant(TabVariant::Default);
    EXPECT_EQ(tabs.getVariant(), TabVariant::Default);

    tabs.setVariant(TabVariant::Pills);
    EXPECT_EQ(tabs.getVariant(), TabVariant::Pills);

    tabs.setVariant(TabVariant::Bordered);
    EXPECT_EQ(tabs.getVariant(), TabVariant::Bordered);
}

// Test: Remove tab
TEST_F(OscilTabsTest, RemoveTab)
{
    OscilTabs tabs;
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");
    tabs.addTab("Tab C", "c");

    tabs.removeTab(1);
    EXPECT_EQ(tabs.getNumTabs(), 2);
}

// Test: Clear tabs
TEST_F(OscilTabsTest, ClearTabs)
{
    OscilTabs tabs;
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");

    tabs.clearTabs();
    EXPECT_EQ(tabs.getNumTabs(), 0);
}

// Test: Callback invocation
TEST_F(OscilTabsTest, CallbackInvocation)
{
    OscilTabs tabs;
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");

    int changeCount = 0;
    int lastIndex = -1;

    tabs.onTabChanged = [&changeCount, &lastIndex](int index, const juce::String&) {
        changeCount++;
        lastIndex = index;
    };

    tabs.setSelectedIndex(1, true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastIndex, 1);
}

// Test: Add tab with icon
TEST_F(OscilTabsTest, AddTabWithIcon)
{
    OscilTabs tabs;

    juce::Path iconPath;
    iconPath.addEllipse(0, 0, 10, 10);

    tabs.addTab("With Icon", "icon", iconPath);

    EXPECT_EQ(tabs.getNumTabs(), 1);
}

// Test: First tab auto-selected
TEST_F(OscilTabsTest, FirstTabAutoSelected)
{
    OscilTabs tabs;

    tabs.addTab("Tab A", "a");
    // First tab should be auto-selected
    EXPECT_EQ(tabs.getSelectedIndex(), 0);
}

// Test: Invalid index handling
TEST_F(OscilTabsTest, InvalidIndexHandling)
{
    OscilTabs tabs;
    tabs.addTab("Tab A", "a");

    tabs.setSelectedIndex(999);
    // Should handle gracefully, keep current or select last valid
}

// Test: Preferred size
TEST_F(OscilTabsTest, PreferredSize)
{
    OscilTabs tabs;
    tabs.addTab("Tab A", "a");
    tabs.addTab("Tab B", "b");

    int width = tabs.getPreferredWidth();
    int height = tabs.getPreferredHeight();

    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

// Test: Theme changes
TEST_F(OscilTabsTest, ThemeChanges)
{
    OscilTabs tabs;
    tabs.addTab("Tab A", "a");
    tabs.setSelectedIndex(0);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    tabs.themeChanged(newTheme);

    // Selection should be preserved
    EXPECT_EQ(tabs.getSelectedIndex(), 0);
}

// Test: Accessibility
TEST_F(OscilTabsTest, Accessibility)
{
    OscilTabs tabs;
    tabs.addTab("Tab A", "a");

    auto* handler = tabs.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);

    EXPECT_EQ(handler->getRole(), juce::AccessibilityRole::group);
}

// Test: Focus handling
TEST_F(OscilTabsTest, FocusHandling)
{
    OscilTabs tabs;
    EXPECT_TRUE(tabs.getWantsKeyboardFocus());
}

// Test: Stretch mode
TEST_F(OscilTabsTest, StretchMode)
{
    OscilTabs tabs;

    tabs.setStretchTabs(true);
    EXPECT_TRUE(tabs.getStretchTabs());

    tabs.setStretchTabs(false);
    EXPECT_FALSE(tabs.getStretchTabs());
}

// Test: Tab spacing
TEST_F(OscilTabsTest, TabSpacing)
{
    OscilTabs tabs;

    tabs.setTabSpacing(16);
    EXPECT_EQ(tabs.getTabSpacing(), 16);
}
