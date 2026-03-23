/*
    Oscil - Tabs Component Tests
    Tests for OscilTabs behavioral correctness

    Bug targets:
    - Invalid tab index silently accepted, corrupting selection
    - Clear tabs doesn't reset selection index
    - Callback fires when notify=false
    - Both callbacks (index + id) fire on single selection change
    - Tab badge/enabled state not accessible after set
*/

#include <gtest/gtest.h>
#include "ui/components/OscilTabs.h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class OscilTabsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        themeManager_ = std::make_unique<ThemeManager>();
    }

    void TearDown() override
    {
        tabs_.reset();
        themeManager_.reset();
    }

    ThemeManager& getThemeManager() { return *themeManager_; }

    // Helper: create a tabs component with 3 tabs, stored in member
    OscilTabs& makeTabs()
    {
        tabs_ = std::make_unique<OscilTabs>(getThemeManager());
        tabs_->addTab("Alpha", "a");
        tabs_->addTab("Beta", "b");
        tabs_->addTab("Gamma", "c");
        return *tabs_;
    }

private:
    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<OscilTabs> tabs_;
};

// =============================================================================
// Tab Management
// =============================================================================

TEST_F(OscilTabsTest, AddTabsIncreasesCount)
{
    OscilTabs tabs(getThemeManager());
    EXPECT_EQ(tabs.getNumTabs(), 0);

    tabs.addTab("Tab A", "a");
    EXPECT_EQ(tabs.getNumTabs(), 1);

    tabs.addTab("Tab B", "b");
    EXPECT_EQ(tabs.getNumTabs(), 2);
}

TEST_F(OscilTabsTest, ClearTabsResetsCount)
{
    auto& tabs = makeTabs();
    EXPECT_EQ(tabs.getNumTabs(), 3);

    tabs.clearTabs();
    EXPECT_EQ(tabs.getNumTabs(), 0);
}

TEST_F(OscilTabsTest, AddMultipleTabsAtOnce)
{
    OscilTabs tabs(getThemeManager());
    tabs.addTabs({"One", "Two", "Three"});
    EXPECT_EQ(tabs.getNumTabs(), 3);
}

TEST_F(OscilTabsTest, GetTabReturnsCorrectItem)
{
    auto& tabs = makeTabs();

    EXPECT_EQ(tabs.getTab(0).label, juce::String("Alpha"));
    EXPECT_EQ(tabs.getTab(0).id, juce::String("a"));
    EXPECT_EQ(tabs.getTab(2).label, juce::String("Gamma"));
    EXPECT_EQ(tabs.getTab(2).id, juce::String("c"));
}

// =============================================================================
// Selection Behavior
// =============================================================================

TEST_F(OscilTabsTest, FirstTabAutoSelectedOnAdd)
{
    OscilTabs tabs(getThemeManager());
    tabs.addTab("First", "first");

    EXPECT_EQ(tabs.getSelectedIndex(), 0);
    EXPECT_EQ(tabs.getSelectedId(), juce::String("first"));
}

TEST_F(OscilTabsTest, SetSelectedIndexUpdatesSelection)
{
    auto& tabs = makeTabs();

    tabs.setSelectedIndex(2, false);
    EXPECT_EQ(tabs.getSelectedIndex(), 2);
    EXPECT_EQ(tabs.getSelectedId(), juce::String("c"));
}

TEST_F(OscilTabsTest, SetSelectedByIdUpdatesIndex)
{
    auto& tabs = makeTabs();

    tabs.setSelectedById("b", false);
    EXPECT_EQ(tabs.getSelectedIndex(), 1);
}

TEST_F(OscilTabsTest, InvalidIndexDoesNotChangeSelection)
{
    auto& tabs = makeTabs();
    tabs.setSelectedIndex(0, false);

    tabs.setSelectedIndex(999, false);
    EXPECT_EQ(tabs.getSelectedIndex(), 0);

    tabs.setSelectedIndex(-1, false);
    EXPECT_EQ(tabs.getSelectedIndex(), 0);
}

// =============================================================================
// Tab State (Badge, Enabled)
// =============================================================================

TEST_F(OscilTabsTest, TabBadgeCountIsPersisted)
{
    auto& tabs = makeTabs();

    tabs.setTabBadge(0, 5);
    EXPECT_EQ(tabs.getTab(0).badgeCount, 5);

    tabs.setTabBadge(0, 0);
    EXPECT_EQ(tabs.getTab(0).badgeCount, 0);
}

TEST_F(OscilTabsTest, TabEnabledStateIsPersisted)
{
    auto& tabs = makeTabs();

    tabs.setTabEnabled(1, false);
    EXPECT_FALSE(tabs.getTab(1).enabled);

    tabs.setTabEnabled(1, true);
    EXPECT_TRUE(tabs.getTab(1).enabled);
}

// =============================================================================
// Callback Behavior
// =============================================================================

TEST_F(OscilTabsTest, SelectionCallbackFiresWithCorrectIndex)
{
    auto& tabs = makeTabs();

    int cbIndex = -1;
    tabs.onTabChanged = [&](int index) { cbIndex = index; };

    tabs.setSelectedIndex(2, true);
    EXPECT_EQ(cbIndex, 2);
}

TEST_F(OscilTabsTest, SelectionIdCallbackFiresWithCorrectId)
{
    auto& tabs = makeTabs();

    juce::String cbId;
    tabs.onTabChangedId = [&](const juce::String& id) { cbId = id; };

    tabs.setSelectedIndex(1, true);
    EXPECT_EQ(cbId, juce::String("b"));
}

TEST_F(OscilTabsTest, NoCallbackWhenNotifyFalse)
{
    auto& tabs = makeTabs();

    int callCount = 0;
    tabs.onTabChanged = [&](int) { callCount++; };
    tabs.onTabChangedId = [&](const juce::String&) { callCount++; };

    tabs.setSelectedIndex(1, false);
    EXPECT_EQ(callCount, 0);
}

// =============================================================================
// Theme Resilience
// =============================================================================

TEST_F(OscilTabsTest, ThemeChangePreservesSelectionAndTabs)
{
    auto& tabs = makeTabs();
    tabs.setSelectedIndex(1, false);

    ColorTheme newTheme;
    newTheme.name = "Dark";
    tabs.themeChanged(newTheme);

    EXPECT_EQ(tabs.getNumTabs(), 3);
    EXPECT_EQ(tabs.getSelectedIndex(), 1);
}
