/*
    Oscil - Badge Component Tests
    Tests for OscilBadge behavioral correctness

    Bug targets:
    - Compact mode not reducing size
    - Longer text not increasing width
    - Empty text badge having zero width (invisible)
    - Theme change corrupting color/variant state
*/

#include <gtest/gtest.h>
#include "ui/components/OscilBadge.h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class OscilBadgeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        themeManager_ = std::make_unique<ThemeManager>();
    }

    void TearDown() override
    {
        themeManager_.reset();
    }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// Construction Contracts
// =============================================================================

TEST_F(OscilBadgeTest, DefaultConstructionInitializesValidState)
{
    OscilBadge badge(getThemeManager());

    EXPECT_TRUE(badge.getText().isEmpty());
    EXPECT_EQ(badge.getColor(), BadgeColor::Default);
    EXPECT_EQ(badge.getVariant(), BadgeVariant::Filled);
    EXPECT_FALSE(badge.isCompact());
}

TEST_F(OscilBadgeTest, ConstructionWithParametersPreservesAll)
{
    OscilBadge badge(getThemeManager(), "Alert", BadgeColor::Error, "badge-err");

    EXPECT_EQ(badge.getText(), juce::String("Alert"));
    EXPECT_EQ(badge.getColor(), BadgeColor::Error);
}

// =============================================================================
// Size Behavior — these test actual layout-affecting behavior
// =============================================================================

TEST_F(OscilBadgeTest, LongerTextProducesWiderBadge)
{
    OscilBadge shortBadge(getThemeManager());
    shortBadge.setText("OK");

    OscilBadge longBadge(getThemeManager());
    longBadge.setText("This Is A Significantly Longer Badge");

    EXPECT_GT(longBadge.getPreferredWidth(), shortBadge.getPreferredWidth());
}

TEST_F(OscilBadgeTest, EmptyTextBadgeStillHasMinimumWidth)
{
    // Bug: empty badge renders as zero-width, invisible in layout
    OscilBadge badge(getThemeManager());
    badge.setText("");

    EXPECT_GT(badge.getPreferredWidth(), 0);
    EXPECT_GT(badge.getPreferredHeight(), 0);
}

TEST_F(OscilBadgeTest, CompactModeReducesOrMaintainsHeight)
{
    OscilBadge badge(getThemeManager());
    badge.setText("Status");

    badge.setCompact(false);
    int normalHeight = badge.getPreferredHeight();

    badge.setCompact(true);
    int compactHeight = badge.getPreferredHeight();

    EXPECT_LE(compactHeight, normalHeight);
}

// =============================================================================
// State Transitions
// =============================================================================

TEST_F(OscilBadgeTest, SetTextUpdatesWidth)
{
    OscilBadge badge(getThemeManager());
    badge.setText("A");
    int shortWidth = badge.getPreferredWidth();

    badge.setText("A Much Longer Text String");
    int longWidth = badge.getPreferredWidth();

    EXPECT_GT(longWidth, shortWidth);
}

TEST_F(OscilBadgeTest, AllColorVariantsAccepted)
{
    OscilBadge badge(getThemeManager());

    for (auto color : {BadgeColor::Default, BadgeColor::Success,
                       BadgeColor::Warning, BadgeColor::Error, BadgeColor::Info})
    {
        badge.setColor(color);
        EXPECT_EQ(badge.getColor(), color);
    }
}

// =============================================================================
// Theme Resilience
// =============================================================================

TEST_F(OscilBadgeTest, ThemeChangePreservesAllState)
{
    OscilBadge badge(getThemeManager());
    badge.setText("Status");
    badge.setColor(BadgeColor::Warning);
    badge.setVariant(BadgeVariant::Outline);
    badge.setCompact(true);

    ColorTheme newTheme;
    newTheme.name = "Different Theme";
    badge.themeChanged(newTheme);

    EXPECT_EQ(badge.getText(), juce::String("Status"));
    EXPECT_EQ(badge.getColor(), BadgeColor::Warning);
    EXPECT_EQ(badge.getVariant(), BadgeVariant::Outline);
    EXPECT_TRUE(badge.isCompact());
}
