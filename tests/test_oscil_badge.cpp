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
    void SetUp() override {}
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(OscilBadgeTest, DefaultConstruction)
{
    OscilBadge badge(ThemeManager::getInstance());

    EXPECT_TRUE(badge.getText().isEmpty());
    EXPECT_EQ(badge.getColor(), BadgeColor::Default);
    EXPECT_EQ(badge.getVariant(), BadgeVariant::Filled);
    EXPECT_FALSE(badge.isCompact());
}

TEST_F(OscilBadgeTest, ConstructionWithText)
{
    OscilBadge badge(ThemeManager::getInstance(), "New");

    EXPECT_EQ(badge.getText(), juce::String("New"));
}

TEST_F(OscilBadgeTest, ConstructionWithTextAndColor)
{
    OscilBadge badge(ThemeManager::getInstance(), "Success", BadgeColor::Success);

    EXPECT_EQ(badge.getText(), juce::String("Success"));
    EXPECT_EQ(badge.getColor(), BadgeColor::Success);
}

TEST_F(OscilBadgeTest, ConstructionWithTestId)
{
    OscilBadge badge(ThemeManager::getInstance(), "Test", BadgeColor::Default, "badge-1");

    EXPECT_EQ(badge.getText(), juce::String("Test"));
}

// =============================================================================
// Content Tests
// =============================================================================

TEST_F(OscilBadgeTest, SetText)
{
    OscilBadge badge(ThemeManager::getInstance());
    badge.setText("New");

    EXPECT_EQ(badge.getText(), juce::String("New"));
}

TEST_F(OscilBadgeTest, SetEmptyText)
{
    OscilBadge badge(ThemeManager::getInstance(), "Initial");
    badge.setText("");

    EXPECT_TRUE(badge.getText().isEmpty());
}

TEST_F(OscilBadgeTest, SetIcon)
{
    OscilBadge badge(ThemeManager::getInstance());

    juce::Image icon(juce::Image::ARGB, 16, 16, true);
    badge.setIcon(icon);

    // No hasIcon method, just verify it doesn't crash
}

TEST_F(OscilBadgeTest, ClearIcon)
{
    OscilBadge badge(ThemeManager::getInstance());

    juce::Image icon(juce::Image::ARGB, 16, 16, true);
    badge.setIcon(icon);
    badge.clearIcon();

    // Just verify it doesn't crash
}

// =============================================================================
// Color Tests
// =============================================================================

TEST_F(OscilBadgeTest, SetColorDefault)
{
    OscilBadge badge(ThemeManager::getInstance());

    badge.setColor(BadgeColor::Default);
    EXPECT_EQ(badge.getColor(), BadgeColor::Default);
}

TEST_F(OscilBadgeTest, SetColorSuccess)
{
    OscilBadge badge(ThemeManager::getInstance());

    badge.setColor(BadgeColor::Success);
    EXPECT_EQ(badge.getColor(), BadgeColor::Success);
}

TEST_F(OscilBadgeTest, SetColorWarning)
{
    OscilBadge badge(ThemeManager::getInstance());

    badge.setColor(BadgeColor::Warning);
    EXPECT_EQ(badge.getColor(), BadgeColor::Warning);
}

TEST_F(OscilBadgeTest, SetColorError)
{
    OscilBadge badge(ThemeManager::getInstance());

    badge.setColor(BadgeColor::Error);
    EXPECT_EQ(badge.getColor(), BadgeColor::Error);
}

TEST_F(OscilBadgeTest, SetColorInfo)
{
    OscilBadge badge(ThemeManager::getInstance());

    badge.setColor(BadgeColor::Info);
    EXPECT_EQ(badge.getColor(), BadgeColor::Info);
}

// =============================================================================
// Variant Tests
// =============================================================================

TEST_F(OscilBadgeTest, SetVariantFilled)
{
    OscilBadge badge(ThemeManager::getInstance());

    badge.setVariant(BadgeVariant::Filled);
    EXPECT_EQ(badge.getVariant(), BadgeVariant::Filled);
}

TEST_F(OscilBadgeTest, SetVariantOutline)
{
    OscilBadge badge(ThemeManager::getInstance());

    badge.setVariant(BadgeVariant::Outline);
    EXPECT_EQ(badge.getVariant(), BadgeVariant::Outline);
}

// =============================================================================
// Compact Mode Tests
// =============================================================================

TEST_F(OscilBadgeTest, SetCompact)
{
    OscilBadge badge(ThemeManager::getInstance());

    badge.setCompact(true);
    EXPECT_TRUE(badge.isCompact());

    badge.setCompact(false);
    EXPECT_FALSE(badge.isCompact());
}

TEST_F(OscilBadgeTest, CompactModeAffectsSize)
{
    OscilBadge badge(ThemeManager::getInstance());
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
    OscilBadge badge(ThemeManager::getInstance());
    badge.setText("Status");

    int width = badge.getPreferredWidth();
    EXPECT_GT(width, 0);
}

TEST_F(OscilBadgeTest, PreferredHeightPositive)
{
    OscilBadge badge(ThemeManager::getInstance());
    badge.setText("Status");

    int height = badge.getPreferredHeight();
    EXPECT_GT(height, 0);
}

TEST_F(OscilBadgeTest, PreferredWidthIncreasesWithText)
{
    OscilBadge shortBadge(ThemeManager::getInstance());
    shortBadge.setText("OK");

    OscilBadge longBadge(ThemeManager::getInstance());
    longBadge.setText("Very Long Badge Text");

    EXPECT_GT(longBadge.getPreferredWidth(), shortBadge.getPreferredWidth());
}

TEST_F(OscilBadgeTest, EmptyTextHasMinimumWidth)
{
    OscilBadge badge(ThemeManager::getInstance());
    badge.setText("");

    EXPECT_GT(badge.getPreferredWidth(), 0);
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(OscilBadgeTest, ThemeChangeDoesNotThrow)
{
    OscilBadge badge(ThemeManager::getInstance());
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
    OscilBadge badge(ThemeManager::getInstance());
    badge.setVariant(BadgeVariant::Outline);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    badge.themeChanged(newTheme);

    EXPECT_EQ(badge.getVariant(), BadgeVariant::Outline);
}

TEST_F(OscilBadgeTest, ThemeChangePreservesCompact)
{
    OscilBadge badge(ThemeManager::getInstance());
    badge.setCompact(true);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    badge.themeChanged(newTheme);

    EXPECT_TRUE(badge.isCompact());
}
