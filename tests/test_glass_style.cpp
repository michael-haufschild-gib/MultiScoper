/*
    Oscil - GlassStyle Tests
    Verifies computed glass colors derived from ColorTheme parameters
*/

#include "ui/components/GlassStyle.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace oscil;

class GlassStyleTest : public ::testing::Test
{
protected:
    /// Build a known theme with explicit glass parameters for deterministic testing
    ColorTheme makeTestTheme()
    {
        ColorTheme theme;
        theme.backgroundPrimary = juce::Colour(0xFF1E1E1E);
        theme.backgroundPane = juce::Colour(0xFF2D2D2D);
        theme.textPrimary = juce::Colour(0xFFE0E0E0);

        theme.accentHue = 200.0f;
        theme.accentSaturation = 0.7f;
        theme.accentLightness = 0.65f;

        theme.glassAlpha = 0.55f;
        theme.panelAlpha = 0.82f;
        theme.borderSubtleAlpha = 0.08f;
        theme.borderDefaultAlpha = 0.12f;
        theme.borderStrongAlpha = 0.20f;
        theme.lightEdgeAlpha = 0.06f;
        theme.shadowIntensity = 0.4f;
        theme.shadowSpread = 12.0f;
        theme.accentGlowRadius = 12.0f;
        theme.accentGlowAlpha = 0.3f;

        return theme;
    }
};

// =============================================================================
// bgGlass / bgPanel alpha derivation
// =============================================================================

TEST_F(GlassStyleTest, BgGlassAlphaMatchesThemeGlassAlpha)
{
    auto theme = makeTestTheme();
    GlassStyle glass;
    glass.computeFrom(theme);

    // bgGlass = backgroundPane.withAlpha(glassAlpha)
    // The RGB channels should match backgroundPane; alpha should match glassAlpha
    EXPECT_EQ(glass.bgGlass.getRed(), theme.backgroundPane.getRed());
    EXPECT_EQ(glass.bgGlass.getGreen(), theme.backgroundPane.getGreen());
    EXPECT_EQ(glass.bgGlass.getBlue(), theme.backgroundPane.getBlue());
    EXPECT_NEAR(glass.bgGlass.getFloatAlpha(), theme.glassAlpha, 0.01f);
}

TEST_F(GlassStyleTest, BgPanelAlphaMatchesThemePanelAlpha)
{
    auto theme = makeTestTheme();
    GlassStyle glass;
    glass.computeFrom(theme);

    EXPECT_EQ(glass.bgPanel.getRed(), theme.backgroundPane.getRed());
    EXPECT_NEAR(glass.bgPanel.getFloatAlpha(), theme.panelAlpha, 0.01f);
}

TEST_F(GlassStyleTest, FullOpacityGlassAlphaProducesSolidBackground)
{
    auto theme = makeTestTheme();
    theme.glassAlpha = 1.0f;
    GlassStyle glass;
    glass.computeFrom(theme);

    EXPECT_NEAR(glass.bgGlass.getFloatAlpha(), 1.0f, 0.01f);
}

// =============================================================================
// Accent color derivation
// =============================================================================

TEST_F(GlassStyleTest, AccentColorDerivedFromHueSatLight)
{
    auto theme = makeTestTheme();
    GlassStyle glass;
    glass.computeFrom(theme);

    auto expected =
        juce::Colour::fromHSV(theme.accentHue / 360.0f, theme.accentSaturation, theme.accentLightness, 1.0f);

    EXPECT_EQ(glass.accent.getRed(), expected.getRed());
    EXPECT_EQ(glass.accent.getGreen(), expected.getGreen());
    EXPECT_EQ(glass.accent.getBlue(), expected.getBlue());
    EXPECT_NEAR(glass.accent.getFloatAlpha(), 1.0f, 0.01f);
}

TEST_F(GlassStyleTest, AccentSubtleHas15PercentAlpha)
{
    auto theme = makeTestTheme();
    GlassStyle glass;
    glass.computeFrom(theme);

    // accentSubtle = accent.withAlpha(0.15f)
    EXPECT_EQ(glass.accentSubtle.getRed(), glass.accent.getRed());
    EXPECT_EQ(glass.accentSubtle.getGreen(), glass.accent.getGreen());
    EXPECT_EQ(glass.accentSubtle.getBlue(), glass.accent.getBlue());
    EXPECT_NEAR(glass.accentSubtle.getFloatAlpha(), 0.15f, 0.01f);
}

TEST_F(GlassStyleTest, AccentMutedHas30PercentAlpha)
{
    auto theme = makeTestTheme();
    GlassStyle glass;
    glass.computeFrom(theme);

    EXPECT_EQ(glass.accentMuted.getRed(), glass.accent.getRed());
    EXPECT_NEAR(glass.accentMuted.getFloatAlpha(), 0.30f, 0.01f);
}

TEST_F(GlassStyleTest, AccentGlowMatchesAccentColor)
{
    auto theme = makeTestTheme();
    GlassStyle glass;
    glass.computeFrom(theme);

    EXPECT_EQ(glass.accentGlow.getARGB(), glass.accent.getARGB());
}

// =============================================================================
// Border alpha levels
// =============================================================================

TEST_F(GlassStyleTest, BorderSubtleAlphaMatchesTheme)
{
    auto theme = makeTestTheme();
    GlassStyle glass;
    glass.computeFrom(theme);

    // borderSubtle = textPrimary.withAlpha(borderSubtleAlpha)
    EXPECT_EQ(glass.borderSubtle.getRed(), theme.textPrimary.getRed());
    EXPECT_NEAR(glass.borderSubtle.getFloatAlpha(), theme.borderSubtleAlpha, 0.01f);
}

TEST_F(GlassStyleTest, BorderDefaultAlphaMatchesTheme)
{
    auto theme = makeTestTheme();
    GlassStyle glass;
    glass.computeFrom(theme);

    EXPECT_EQ(glass.borderDefault.getRed(), theme.textPrimary.getRed());
    EXPECT_NEAR(glass.borderDefault.getFloatAlpha(), theme.borderDefaultAlpha, 0.01f);
}

TEST_F(GlassStyleTest, BorderStrongAlphaMatchesTheme)
{
    auto theme = makeTestTheme();
    GlassStyle glass;
    glass.computeFrom(theme);

    EXPECT_EQ(glass.borderStrong.getRed(), theme.textPrimary.getRed());
    EXPECT_NEAR(glass.borderStrong.getFloatAlpha(), theme.borderStrongAlpha, 0.01f);
}

TEST_F(GlassStyleTest, BorderAlphaOrderingSubtleLessThanDefaultLessThanStrong)
{
    auto theme = makeTestTheme();
    GlassStyle glass;
    glass.computeFrom(theme);

    EXPECT_LT(glass.borderSubtle.getFloatAlpha(), glass.borderDefault.getFloatAlpha());
    EXPECT_LT(glass.borderDefault.getFloatAlpha(), glass.borderStrong.getFloatAlpha());
}

// =============================================================================
// Hover / active states
// =============================================================================

TEST_F(GlassStyleTest, BgHoverUsesTextPrimaryAtLowAlpha)
{
    auto theme = makeTestTheme();
    GlassStyle glass;
    glass.computeFrom(theme);

    // bgHover = textPrimary.withAlpha(0.08f)
    EXPECT_EQ(glass.bgHover.getRed(), theme.textPrimary.getRed());
    EXPECT_NEAR(glass.bgHover.getFloatAlpha(), 0.08f, 0.01f);
}

TEST_F(GlassStyleTest, BgActiveUsesTextPrimaryAtHigherAlpha)
{
    auto theme = makeTestTheme();
    GlassStyle glass;
    glass.computeFrom(theme);

    // bgActive = textPrimary.withAlpha(0.12f)
    EXPECT_EQ(glass.bgActive.getRed(), theme.textPrimary.getRed());
    EXPECT_NEAR(glass.bgActive.getFloatAlpha(), 0.12f, 0.01f);
}

// =============================================================================
// Inset light edge
// =============================================================================

TEST_F(GlassStyleTest, InsetLightEdgeIsWhiteAtThemeAlpha)
{
    auto theme = makeTestTheme();
    GlassStyle glass;
    glass.computeFrom(theme);

    // insetLightEdge = white.withAlpha(lightEdgeAlpha)
    EXPECT_EQ(glass.insetLightEdge.getRed(), 255);
    EXPECT_EQ(glass.insetLightEdge.getGreen(), 255);
    EXPECT_EQ(glass.insetLightEdge.getBlue(), 255);
    EXPECT_NEAR(glass.insetLightEdge.getFloatAlpha(), theme.lightEdgeAlpha, 0.01f);
}

// =============================================================================
// Shadow and glow parameters
// =============================================================================

TEST_F(GlassStyleTest, ShadowParametersPassedThrough)
{
    auto theme = makeTestTheme();
    theme.shadowIntensity = 0.6f;
    theme.shadowSpread = 18.0f;
    GlassStyle glass;
    glass.computeFrom(theme);

    EXPECT_NEAR(glass.shadowIntensity, 0.6f, 0.001f);
    EXPECT_NEAR(glass.shadowSpread, 18.0f, 0.001f);
}

TEST_F(GlassStyleTest, AccentGlowParametersPassedThrough)
{
    auto theme = makeTestTheme();
    theme.accentGlowRadius = 20.0f;
    theme.accentGlowAlpha = 0.5f;
    GlassStyle glass;
    glass.computeFrom(theme);

    EXPECT_NEAR(glass.accentGlowRadius, 20.0f, 0.001f);
    EXPECT_NEAR(glass.accentGlowAlpha, 0.5f, 0.001f);
}

// =============================================================================
// Different hue values produce visually distinct accent colors
// =============================================================================

TEST_F(GlassStyleTest, DifferentHuesProduceDifferentAccentColors)
{
    auto theme1 = makeTestTheme();
    theme1.accentHue = 0.0f; // red
    GlassStyle glass1;
    glass1.computeFrom(theme1);

    auto theme2 = makeTestTheme();
    theme2.accentHue = 120.0f; // green
    GlassStyle glass2;
    glass2.computeFrom(theme2);

    auto theme3 = makeTestTheme();
    theme3.accentHue = 240.0f; // blue
    GlassStyle glass3;
    glass3.computeFrom(theme3);

    // Each should produce a different accent RGB
    EXPECT_NE(glass1.accent.getARGB(), glass2.accent.getARGB());
    EXPECT_NE(glass2.accent.getARGB(), glass3.accent.getARGB());
    EXPECT_NE(glass1.accent.getARGB(), glass3.accent.getARGB());
}

// =============================================================================
// Recompute correctness: changing theme fields changes glass output
// =============================================================================

TEST_F(GlassStyleTest, RecomputeReflectsChangedGlassAlpha)
{
    auto theme = makeTestTheme();
    GlassStyle glass;

    theme.glassAlpha = 0.3f;
    glass.computeFrom(theme);
    float alpha1 = glass.bgGlass.getFloatAlpha();

    theme.glassAlpha = 0.9f;
    glass.computeFrom(theme);
    float alpha2 = glass.bgGlass.getFloatAlpha();

    EXPECT_NEAR(alpha1, 0.3f, 0.01f);
    EXPECT_NEAR(alpha2, 0.9f, 0.01f);
}

// =============================================================================
// System theme glass style: every system theme produces valid glass output
// =============================================================================

TEST_F(GlassStyleTest, AllSystemThemesProduceValidGlassStyle)
{
    auto testTheme = [](const ColorTheme& theme) {
        GlassStyle glass;
        glass.computeFrom(theme);

        // bgGlass must not be fully transparent (would be invisible)
        EXPECT_GT(glass.bgGlass.getFloatAlpha(), 0.0f) << "Theme '" << theme.name << "' has zero glass alpha";

        // Accent must be opaque
        EXPECT_NEAR(glass.accent.getFloatAlpha(), 1.0f, 0.01f) << "Theme '" << theme.name << "' accent is not opaque";

        // Shadow intensity must be non-negative
        EXPECT_GE(glass.shadowIntensity, 0.0f);

        // Border ordering must hold
        EXPECT_LE(glass.borderSubtle.getFloatAlpha(), glass.borderDefault.getFloatAlpha())
            << "Theme '" << theme.name << "' violates border ordering";
        EXPECT_LE(glass.borderDefault.getFloatAlpha(), glass.borderStrong.getFloatAlpha())
            << "Theme '" << theme.name << "' violates border ordering";
    };

    testTheme(SystemThemes::createDarkProfessional());
    testTheme(SystemThemes::createClassicGreen());
    testTheme(SystemThemes::createClassicAmber());
    testTheme(SystemThemes::createHighContrast());
    testTheme(SystemThemes::createLightMode());
    testTheme(SystemThemes::createGlassDarkBlue());
    testTheme(SystemThemes::createGlassDarkPurple());
    testTheme(SystemThemes::createGlassDarkBrown());
    testTheme(SystemThemes::createGlassDarkBlack());
}
