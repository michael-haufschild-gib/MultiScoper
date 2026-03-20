/*
    Oscil - Badge Component Tests
    Tests for OscilBadge UI component
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

    ThemeManager& getThemeManager()
    {
        return *themeManager_;
    }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(OscilBadgeTest, DefaultConstruction)
{
    OscilBadge badge(getThemeManager());

    EXPECT_TRUE(badge.getText().isEmpty());
    EXPECT_EQ(badge.getColor(), BadgeColor::Default);
    EXPECT_EQ(badge.getVariant(), BadgeVariant::Filled);
    EXPECT_FALSE(badge.isCompact());
}

TEST_F(OscilBadgeTest, ConstructionWithText)
{
    OscilBadge badge(getThemeManager(), "New");

    EXPECT_EQ(badge.getText(), juce::String("New"));
}

TEST_F(OscilBadgeTest, ConstructionWithTextAndColor)
{
    OscilBadge badge(getThemeManager(), "Success", BadgeColor::Success);

    EXPECT_EQ(badge.getText(), juce::String("Success"));
    EXPECT_EQ(badge.getColor(), BadgeColor::Success);
}

TEST_F(OscilBadgeTest, ConstructionWithTestId)
{
    OscilBadge badge(getThemeManager(), "Test", BadgeColor::Default, "badge-1");

    EXPECT_EQ(badge.getText(), juce::String("Test"));
}

// =============================================================================
// Content Tests
// =============================================================================

TEST_F(OscilBadgeTest, SetText)
{
    OscilBadge badge(getThemeManager());
    badge.setText("New");

    EXPECT_EQ(badge.getText(), juce::String("New"));
}

TEST_F(OscilBadgeTest, SetEmptyText)
{
    OscilBadge badge(getThemeManager(), "Initial");
    badge.setText("");

    EXPECT_TRUE(badge.getText().isEmpty());
}

TEST_F(OscilBadgeTest, SetIcon)
{
    OscilBadge badge(getThemeManager());

    juce::Image icon(juce::Image::ARGB, 16, 16, true);
    badge.setIcon(icon);

    // Badge should remain enabled after setting icon
    EXPECT_TRUE(badge.isEnabled());
    EXPECT_EQ(badge.getWidth(), 0);  // Not yet sized
}

TEST_F(OscilBadgeTest, ClearIcon)
{
    OscilBadge badge(getThemeManager());

    juce::Image icon(juce::Image::ARGB, 16, 16, true);
    badge.setIcon(icon);
    badge.clearIcon();

    // Badge should still be enabled after clearing icon
    EXPECT_TRUE(badge.isEnabled());
    EXPECT_EQ(badge.getWidth(), 0);  // Not yet sized
}

// =============================================================================
// Color Tests
// =============================================================================

TEST_F(OscilBadgeTest, SetColorDefault)
{
    OscilBadge badge(getThemeManager());

    badge.setColor(BadgeColor::Default);
    EXPECT_EQ(badge.getColor(), BadgeColor::Default);
}

TEST_F(OscilBadgeTest, SetColorSuccess)
{
    OscilBadge badge(getThemeManager());

    badge.setColor(BadgeColor::Success);
    EXPECT_EQ(badge.getColor(), BadgeColor::Success);
}

TEST_F(OscilBadgeTest, SetColorWarning)
{
    OscilBadge badge(getThemeManager());

    badge.setColor(BadgeColor::Warning);
    EXPECT_EQ(badge.getColor(), BadgeColor::Warning);
}

TEST_F(OscilBadgeTest, SetColorError)
{
    OscilBadge badge(getThemeManager());

    badge.setColor(BadgeColor::Error);
    EXPECT_EQ(badge.getColor(), BadgeColor::Error);
}

TEST_F(OscilBadgeTest, SetColorInfo)
{
    OscilBadge badge(getThemeManager());

    badge.setColor(BadgeColor::Info);
    EXPECT_EQ(badge.getColor(), BadgeColor::Info);
}

// =============================================================================
// Variant Tests
// =============================================================================

TEST_F(OscilBadgeTest, SetVariantFilled)
{
    OscilBadge badge(getThemeManager());

    badge.setVariant(BadgeVariant::Filled);
    EXPECT_EQ(badge.getVariant(), BadgeVariant::Filled);
}

TEST_F(OscilBadgeTest, SetVariantOutline)
{
    OscilBadge badge(getThemeManager());

    badge.setVariant(BadgeVariant::Outline);
    EXPECT_EQ(badge.getVariant(), BadgeVariant::Outline);
}

// =============================================================================
// Compact Mode Tests
// =============================================================================

TEST_F(OscilBadgeTest, SetCompact)
{
    OscilBadge badge(getThemeManager());

    badge.setCompact(true);
    EXPECT_TRUE(badge.isCompact());

    badge.setCompact(false);
    EXPECT_FALSE(badge.isCompact());
}

TEST_F(OscilBadgeTest, CompactModeAffectsSize)
{
    OscilBadge badge(getThemeManager());
    badge.setText("Status");

    badge.setCompact(false);
    int normalHeight = badge.getPreferredHeight();

    badge.setCompact(true);
    int compactHeight = badge.getPreferredHeight();

    // Compact mode should result in smaller badge
    EXPECT_LE(compactHeight, normalHeight);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(OscilBadgeTest, PreferredWidthPositive)
{
    OscilBadge badge(getThemeManager());
    badge.setText("Status");

    int width = badge.getPreferredWidth();
    EXPECT_GT(width, 0);
}

TEST_F(OscilBadgeTest, PreferredHeightPositive)
{
    OscilBadge badge(getThemeManager());
    badge.setText("Status");

    int height = badge.getPreferredHeight();
    EXPECT_GT(height, 0);
}

TEST_F(OscilBadgeTest, PreferredWidthIncreasesWithText)
{
    OscilBadge shortBadge(getThemeManager());
    shortBadge.setText("OK");

    OscilBadge longBadge(getThemeManager());
    longBadge.setText("Very Long Badge Text");

    EXPECT_GT(longBadge.getPreferredWidth(), shortBadge.getPreferredWidth());
}

TEST_F(OscilBadgeTest, EmptyTextHasMinimumWidth)
{
    OscilBadge badge(getThemeManager());
    badge.setText("");

    EXPECT_GT(badge.getPreferredWidth(), 0);
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(OscilBadgeTest, ThemeChangeDoesNotThrow)
{
    OscilBadge badge(getThemeManager());
    badge.setText("Test");
    badge.setColor(BadgeColor::Success);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    badge.themeChanged(newTheme);

    // State should be preserved
    EXPECT_EQ(badge.getText(), juce::String("Test"));
    EXPECT_EQ(badge.getColor(), BadgeColor::Success);
}

TEST_F(OscilBadgeTest, ThemeChangePreservesVariant)
{
    OscilBadge badge(getThemeManager());
    badge.setVariant(BadgeVariant::Outline);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    badge.themeChanged(newTheme);

    EXPECT_EQ(badge.getVariant(), BadgeVariant::Outline);
}

TEST_F(OscilBadgeTest, ThemeChangePreservesCompact)
{
    OscilBadge badge(getThemeManager());
    badge.setCompact(true);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    badge.themeChanged(newTheme);

    EXPECT_TRUE(badge.isCompact());
}
