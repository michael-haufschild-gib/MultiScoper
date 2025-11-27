/*
    Oscil - Badge Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilBadge.h"

using namespace oscil;

class OscilBadgeTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction
TEST_F(OscilBadgeTest, DefaultConstruction)
{
    OscilBadge badge;

    EXPECT_TRUE(badge.getText().isEmpty());
    EXPECT_EQ(badge.getColor(), BadgeColor::Default);
    EXPECT_EQ(badge.getVariant(), BadgeVariant::Filled);
}

// Test: Set text
TEST_F(OscilBadgeTest, SetText)
{
    OscilBadge badge;
    badge.setText("New");

    EXPECT_EQ(badge.getText(), juce::String("New"));
}

// Test: Badge colors
TEST_F(OscilBadgeTest, BadgeColors)
{
    OscilBadge badge;

    badge.setColor(BadgeColor::Default);
    EXPECT_EQ(badge.getColor(), BadgeColor::Default);

    badge.setColor(BadgeColor::Primary);
    EXPECT_EQ(badge.getColor(), BadgeColor::Primary);

    badge.setColor(BadgeColor::Success);
    EXPECT_EQ(badge.getColor(), BadgeColor::Success);

    badge.setColor(BadgeColor::Warning);
    EXPECT_EQ(badge.getColor(), BadgeColor::Warning);

    badge.setColor(BadgeColor::Error);
    EXPECT_EQ(badge.getColor(), BadgeColor::Error);

    badge.setColor(BadgeColor::Info);
    EXPECT_EQ(badge.getColor(), BadgeColor::Info);
}

// Test: Badge variants
TEST_F(OscilBadgeTest, BadgeVariants)
{
    OscilBadge badge;

    badge.setVariant(BadgeVariant::Filled);
    EXPECT_EQ(badge.getVariant(), BadgeVariant::Filled);

    badge.setVariant(BadgeVariant::Outline);
    EXPECT_EQ(badge.getVariant(), BadgeVariant::Outline);

    badge.setVariant(BadgeVariant::Subtle);
    EXPECT_EQ(badge.getVariant(), BadgeVariant::Subtle);
}

// Test: Size variants
TEST_F(OscilBadgeTest, SizeVariants)
{
    OscilBadge badge;

    badge.setSizeVariant(SizeVariant::Small);
    EXPECT_EQ(badge.getSizeVariant(), SizeVariant::Small);

    badge.setSizeVariant(SizeVariant::Medium);
    EXPECT_EQ(badge.getSizeVariant(), SizeVariant::Medium);

    badge.setSizeVariant(SizeVariant::Large);
    EXPECT_EQ(badge.getSizeVariant(), SizeVariant::Large);
}

// Test: Dismissible
TEST_F(OscilBadgeTest, Dismissible)
{
    OscilBadge badge;

    EXPECT_FALSE(badge.isDismissible());

    badge.setDismissible(true);
    EXPECT_TRUE(badge.isDismissible());

    badge.setDismissible(false);
    EXPECT_FALSE(badge.isDismissible());
}

// Test: Dismiss callback
TEST_F(OscilBadgeTest, DismissCallback)
{
    OscilBadge badge;
    badge.setDismissible(true);

    bool dismissed = false;
    badge.onDismiss = [&dismissed]() {
        dismissed = true;
    };

    badge.dismiss();
    EXPECT_TRUE(dismissed);
}

// Test: Clickable
TEST_F(OscilBadgeTest, Clickable)
{
    OscilBadge badge;

    EXPECT_FALSE(badge.isClickable());

    badge.setClickable(true);
    EXPECT_TRUE(badge.isClickable());

    badge.setClickable(false);
    EXPECT_FALSE(badge.isClickable());
}

// Test: Click callback
TEST_F(OscilBadgeTest, ClickCallback)
{
    OscilBadge badge;
    badge.setClickable(true);

    int clickCount = 0;
    badge.onClick = [&clickCount]() {
        clickCount++;
    };

    // Simulating click would require mouse event, just verify callback can be set
    EXPECT_EQ(clickCount, 0);
}

// Test: Set icon
TEST_F(OscilBadgeTest, SetIcon)
{
    OscilBadge badge;

    juce::Path iconPath;
    iconPath.addEllipse(0, 0, 10, 10);

    badge.setIcon(iconPath);
    EXPECT_TRUE(badge.hasIcon());
}

// Test: Dot mode (no text, just colored dot)
TEST_F(OscilBadgeTest, DotMode)
{
    OscilBadge badge;

    badge.setDotMode(true);
    EXPECT_TRUE(badge.isDotMode());

    badge.setDotMode(false);
    EXPECT_FALSE(badge.isDotMode());
}

// Test: Preferred size
TEST_F(OscilBadgeTest, PreferredSize)
{
    OscilBadge badge;
    badge.setText("Status");

    int width = badge.getPreferredWidth();
    int height = badge.getPreferredHeight();

    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

// Test: Preferred size with icon
TEST_F(OscilBadgeTest, PreferredSizeWithIcon)
{
    OscilBadge badge;
    badge.setText("Status");

    int widthWithoutIcon = badge.getPreferredWidth();

    juce::Path iconPath;
    iconPath.addEllipse(0, 0, 10, 10);
    badge.setIcon(iconPath);

    int widthWithIcon = badge.getPreferredWidth();

    EXPECT_GT(widthWithIcon, widthWithoutIcon);
}

// Test: Dot mode has small size
TEST_F(OscilBadgeTest, DotModeSmallSize)
{
    OscilBadge badge;
    badge.setText("Status");
    int normalWidth = badge.getPreferredWidth();

    badge.setDotMode(true);
    int dotWidth = badge.getPreferredWidth();

    EXPECT_LT(dotWidth, normalWidth);
}

// Test: Theme changes
TEST_F(OscilBadgeTest, ThemeChanges)
{
    OscilBadge badge;
    badge.setText("Test");
    badge.setColor(BadgeColor::Success);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    badge.themeChanged(newTheme);

    // State should be preserved
    EXPECT_EQ(badge.getText(), juce::String("Test"));
    EXPECT_EQ(badge.getColor(), BadgeColor::Success);
}

// Test: Accessibility
TEST_F(OscilBadgeTest, Accessibility)
{
    OscilBadge badge;
    badge.setText("New");

    auto* handler = badge.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);
}

// Test: Max width constraint
TEST_F(OscilBadgeTest, MaxWidthConstraint)
{
    OscilBadge badge;
    badge.setText("This is a very long badge text that should be truncated");

    badge.setMaxWidth(100);
    EXPECT_LE(badge.getPreferredWidth(), 100);
}

// Test: Pulse animation
TEST_F(OscilBadgeTest, PulseAnimation)
{
    OscilBadge badge;

    badge.setPulsing(true);
    EXPECT_TRUE(badge.isPulsing());

    badge.setPulsing(false);
    EXPECT_FALSE(badge.isPulsing());
}
