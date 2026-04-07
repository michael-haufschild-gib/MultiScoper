/*
    Oscil - Theme Accessibility Tests
    Tests for WCAG contrast validation, theme name safety, and glass-contrast checks
*/

#include "ui/components/GlassStyle.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace oscil;

class ThemeAccessibilityTest : public ::testing::Test
{
protected:
    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }

    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// Accessibility Validation
// =============================================================================

TEST_F(ThemeAccessibilityTest, SystemThemesPassAccessibilityChecks)
{
    auto themes = getThemeManager().getAvailableThemes();

    for (const auto& themeName : themes)
    {
        auto* theme = getThemeManager().getTheme(themeName);
        ASSERT_NE(theme, nullptr) << "Theme '" << themeName << "' is null";

        auto issues = theme->validateAccessibility();
        EXPECT_TRUE(issues.empty()) << "Theme '" << themeName << "' has accessibility violations: "
                                    << (issues.empty() ? "" : issues[0].toStdString());
    }
}

TEST_F(ThemeAccessibilityTest, ValidateAccessibilityDetectsLowContrastText)
{
    ColorTheme badTheme;
    badTheme.textPrimary = juce::Colour(0xFF202020);
    badTheme.backgroundPrimary = juce::Colour(0xFF1E1E1E);

    auto issues = badTheme.validateAccessibility();
    EXPECT_FALSE(issues.empty());

    bool foundTextIssue = false;
    for (const auto& issue : issues)
    {
        if (issue.contains("textPrimary"))
            foundTextIssue = true;
    }
    EXPECT_TRUE(foundTextIssue);
}

TEST_F(ThemeAccessibilityTest, ValidateAccessibilityPassesWithHighContrast)
{
    ColorTheme goodTheme;
    goodTheme.textPrimary = juce::Colour(0xFFFFFFFF);
    goodTheme.textSecondary = juce::Colour(0xFFCCCCCC);
    goodTheme.backgroundPrimary = juce::Colour(0xFF000000);
    goodTheme.backgroundSecondary = juce::Colour(0xFF111111);
    goodTheme.btnPrimaryText = juce::Colour(0xFFFFFFFF);
    goodTheme.btnPrimaryBg = juce::Colour(0xFF0000AA);
    goodTheme.btnSecondaryText = juce::Colour(0xFFFFFFFF);
    goodTheme.btnSecondaryBg = juce::Colour(0xFF333333);
    goodTheme.statusActive = juce::Colour(0xFF00FF00);
    goodTheme.statusWarning = juce::Colour(0xFFFFAA00);
    goodTheme.statusError = juce::Colour(0xFFFF0000);

    auto issues = goodTheme.validateAccessibility();
    EXPECT_TRUE(issues.empty());
}

TEST_F(ThemeAccessibilityTest, ContrastRatioCalculation)
{
    float ratio = ColorTheme::calculateContrastRatio(juce::Colours::white, juce::Colours::black);
    EXPECT_NEAR(ratio, 21.0f, 0.1f);

    float sameRatio = ColorTheme::calculateContrastRatio(juce::Colours::red, juce::Colours::red);
    EXPECT_NEAR(sameRatio, 1.0f, 0.01f);

    EXPECT_TRUE(ColorTheme::meetsContrastAA(juce::Colours::white, juce::Colours::black));
    EXPECT_FALSE(ColorTheme::meetsContrastAA(juce::Colour(0xFF808080), juce::Colour(0xFF909090)));
}

TEST_F(ThemeAccessibilityTest, ValidThemeNameRejectsPathTraversal)
{
    EXPECT_FALSE(ThemeManager::isValidThemeName("../etc/passwd"));
    EXPECT_FALSE(ThemeManager::isValidThemeName("..\\windows\\system32"));
    EXPECT_FALSE(ThemeManager::isValidThemeName("theme/with/slashes"));
    EXPECT_FALSE(ThemeManager::isValidThemeName(""));
    EXPECT_TRUE(ThemeManager::isValidThemeName("My Custom Theme"));
    EXPECT_TRUE(ThemeManager::isValidThemeName("Dark Mode v2"));
}

// =============================================================================
// Glass-Contrast Validation
// =============================================================================

// Test: textPrimary on effective glass background passes AA for all system themes
TEST_F(ThemeAccessibilityTest, TextOnGlassBackgroundPassesAAForAllThemes)
{
    auto themes = getThemeManager().getAvailableThemes();

    for (const auto& themeName : themes)
    {
        auto* theme = getThemeManager().getTheme(themeName);
        ASSERT_NE(theme, nullptr) << "Theme '" << themeName << "' is null";

        // Effective glass bg: backgroundPrimary blended with backgroundPane at glassAlpha
        auto effectiveGlassBg = theme->backgroundPrimary.interpolatedWith(theme->backgroundPane, theme->glassAlpha);

        EXPECT_TRUE(ColorTheme::meetsContrastAA(theme->textPrimary, effectiveGlassBg))
            << "Theme '" << themeName << "': textPrimary (0x"
            << juce::String::toHexString(static_cast<int>(theme->textPrimary.getARGB())) << ") on glass bg (0x"
            << juce::String::toHexString(static_cast<int>(effectiveGlassBg.getARGB()))
            << ") fails AA. Ratio: " << ColorTheme::calculateContrastRatio(theme->textPrimary, effectiveGlassBg);
    }
}

// Test: all four new glass system themes individually pass full accessibility validation
TEST_F(ThemeAccessibilityTest, GlassSystemThemesPassAccessibility)
{
    auto validateTheme = [&](const juce::String& name) {
        auto* theme = getThemeManager().getTheme(name);
        ASSERT_NE(theme, nullptr) << "Theme '" << name << "' not found";

        auto issues = theme->validateAccessibility();
        EXPECT_TRUE(issues.empty()) << "Theme '" << name
                                    << "' fails: " << (issues.empty() ? "" : issues[0].toStdString());
    };

    validateTheme("Glass Dark Blue");
    validateTheme("Glass Dark Purple");
    validateTheme("Glass Dark Brown");
    validateTheme("Glass Dark Black");
}

// Test: High Contrast theme with glassAlpha=1.0 still passes accessibility
TEST_F(ThemeAccessibilityTest, HighContrastFullOpacityGlassPassesAccessibility)
{
    auto* theme = getThemeManager().getTheme("High Contrast");
    ASSERT_NE(theme, nullptr);

    // Verify it uses full opacity
    EXPECT_NEAR(theme->glassAlpha, 1.0f, 0.001f);

    auto issues = theme->validateAccessibility();
    EXPECT_TRUE(issues.empty()) << "High Contrast fails: " << (issues.empty() ? "" : issues[0].toStdString());
}

// Test: glass validateAccessibility catches low contrast on glass bg
TEST_F(ThemeAccessibilityTest, GlassContrastCheckCatchesLowContrast)
{
    ColorTheme bad;
    // Near-identical background and text on glass
    bad.backgroundPrimary = juce::Colour(0xFF202020);
    bad.backgroundPane = juce::Colour(0xFF222222);
    bad.textPrimary = juce::Colour(0xFF303030); // very low contrast against ~0xFF212121
    bad.textSecondary = juce::Colour(0xFF808080);
    bad.glassAlpha = 0.55f;

    // Need valid button colors to avoid button false positives
    bad.btnPrimaryText = juce::Colour(0xFFFFFFFF);
    bad.btnPrimaryBg = juce::Colour(0xFF0000AA);
    bad.btnSecondaryText = juce::Colour(0xFFFFFFFF);
    bad.btnSecondaryBg = juce::Colour(0xFF333333);

    auto issues = bad.validateAccessibility();

    // The effective glass bg is ~0xFF212121 and textPrimary is 0xFF303030: ratio < 4.5
    auto effectiveGlassBg = bad.backgroundPrimary.interpolatedWith(bad.backgroundPane, bad.glassAlpha);
    float glassRatio = ColorTheme::calculateContrastRatio(bad.textPrimary, effectiveGlassBg);
    EXPECT_LT(glassRatio, 4.5f) << "Test setup: textPrimary vs glass bg should fail AA (ratio=" << glassRatio << ")";

    // The validator must report this specific failure
    int glassIssueCount = 0;
    for (const auto& issue : issues)
    {
        if (issue.contains("glass"))
            glassIssueCount++;
    }
    EXPECT_EQ(glassIssueCount, 1) << "Should detect exactly one glass-background contrast failure";
}

// Test: accent color on all glass themes has reasonable visibility on background.
// Accent is used for emphasis elements (glows, borders, indicators) not as primary
// readable text, so we validate it meets a minimum visibility threshold of 2:1
// rather than the WCAG text requirement of 3:1.
TEST_F(ThemeAccessibilityTest, AccentColorVisibleOnBackgroundForGlassThemes)
{
    for (const auto& name : {"Glass Dark Blue", "Glass Dark Purple", "Glass Dark Brown", "Glass Dark Black"})
    {
        auto* theme = getThemeManager().getTheme(name);
        ASSERT_NE(theme, nullptr) << name << " not found";

        GlassStyle glass;
        glass.computeFrom(*theme);

        // Accent as non-text graphical element: WCAG 1.4.11 requires 3:1 for
        // UI components, but accent glows/borders are decorative overlays that
        // always accompany text labels. We verify minimum 2:1 visibility.
        float ratio = ColorTheme::calculateContrastRatio(glass.accent, theme->backgroundPrimary);
        EXPECT_GE(ratio, 2.0f) << "Theme '" << name << "': accent on backgroundPrimary contrast ratio " << ratio
                               << " < 2.0 (minimum visibility)";
    }
}
