/*
    Oscil - SurfaceStyle Tests

    Validates the flat surface token pack computed from a ColorTheme. The
    previous glassmorphism aesthetic (translucent fills, inset highlight,
    multi-layer shadows) was removed; these tests pin the new flat
    semantics so a future accidental reintroduction of glass-era behaviour
    fails loud.
*/

#include "ui/components/SurfaceStyle.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace oscil;

class SurfaceStyleTest : public ::testing::Test
{
protected:
    ColorTheme makeTestTheme()
    {
        ColorTheme theme;
        theme.backgroundPrimary = juce::Colour(0xFF1E1E1E);
        theme.backgroundPane = juce::Colour(0xFF2D2D2D);
        theme.textPrimary = juce::Colour(0xFFE0E0E0);

        theme.accentHue = 200.0f;
        theme.accentSaturation = 0.7f;
        theme.accentLightness = 0.65f;

        theme.glassAlpha = 1.0f;
        theme.panelAlpha = 1.0f;
        theme.borderSubtleAlpha = 0.06f;
        theme.borderDefaultAlpha = 0.10f;
        theme.borderStrongAlpha = 0.20f;
        theme.shadowIntensity = 0.18f;
        theme.shadowSpread = 4.0f;

        return theme;
    }
};

// =============================================================================
// Flat surfaces: bgGlass and bgPanel are opaque copies of backgroundPane
// =============================================================================

TEST_F(SurfaceStyleTest, BgGlassIsOpaqueBackgroundPane)
{
    auto theme = makeTestTheme();
    SurfaceStyle surface;
    surface.computeFrom(theme);

    EXPECT_EQ(surface.bgGlass.getRed(), theme.backgroundPane.getRed());
    EXPECT_EQ(surface.bgGlass.getGreen(), theme.backgroundPane.getGreen());
    EXPECT_EQ(surface.bgGlass.getBlue(), theme.backgroundPane.getBlue());
    EXPECT_NEAR(surface.bgGlass.getFloatAlpha(), 1.0f, 0.01f);
}

TEST_F(SurfaceStyleTest, BgPanelIsOpaqueBackgroundPane)
{
    auto theme = makeTestTheme();
    SurfaceStyle surface;
    surface.computeFrom(theme);

    EXPECT_EQ(surface.bgPanel.getRed(), theme.backgroundPane.getRed());
    EXPECT_NEAR(surface.bgPanel.getFloatAlpha(), 1.0f, 0.01f);
}

// Regression guard: glass-era behaviour baked theme.glassAlpha into bgGlass,
// so a translucent panel could appear over the window chrome. The flat
// replacement ignores theme.glassAlpha entirely — surface colours are
// always opaque. If a future change re-couples them, this fails.
TEST_F(SurfaceStyleTest, GlassAlphaHasNoEffectOnBgGlass)
{
    auto theme = makeTestTheme();
    SurfaceStyle surface;

    theme.glassAlpha = 0.3f;
    surface.computeFrom(theme);
    float const alphaAtLowSetting = surface.bgGlass.getFloatAlpha();

    theme.glassAlpha = 0.9f;
    surface.computeFrom(theme);
    float const alphaAtHighSetting = surface.bgGlass.getFloatAlpha();

    EXPECT_NEAR(alphaAtLowSetting, 1.0f, 0.01f);
    EXPECT_NEAR(alphaAtHighSetting, 1.0f, 0.01f);
}

// =============================================================================
// Accent color derivation (unchanged from glass era)
// =============================================================================

TEST_F(SurfaceStyleTest, AccentColorDerivedFromHueSatLight)
{
    auto theme = makeTestTheme();
    SurfaceStyle surface;
    surface.computeFrom(theme);

    auto expected =
        juce::Colour::fromHSV(theme.accentHue / 360.0f, theme.accentSaturation, theme.accentLightness, 1.0f);

    EXPECT_EQ(surface.accent.getRed(), expected.getRed());
    EXPECT_EQ(surface.accent.getGreen(), expected.getGreen());
    EXPECT_EQ(surface.accent.getBlue(), expected.getBlue());
    EXPECT_NEAR(surface.accent.getFloatAlpha(), 1.0f, 0.01f);
}

TEST_F(SurfaceStyleTest, AccentSubtleHas15PercentAlpha)
{
    auto theme = makeTestTheme();
    SurfaceStyle surface;
    surface.computeFrom(theme);

    EXPECT_EQ(surface.accentSubtle.getRed(), surface.accent.getRed());
    EXPECT_EQ(surface.accentSubtle.getGreen(), surface.accent.getGreen());
    EXPECT_EQ(surface.accentSubtle.getBlue(), surface.accent.getBlue());
    EXPECT_NEAR(surface.accentSubtle.getFloatAlpha(), 0.15f, 0.01f);
}

TEST_F(SurfaceStyleTest, AccentMutedHas35PercentAlpha)
{
    auto theme = makeTestTheme();
    SurfaceStyle surface;
    surface.computeFrom(theme);

    EXPECT_EQ(surface.accentMuted.getRed(), surface.accent.getRed());
    EXPECT_NEAR(surface.accentMuted.getFloatAlpha(), 0.35f, 0.01f);
}

// =============================================================================
// Border alpha levels — still derived from textPrimary at theme-configured alphas
// =============================================================================

TEST_F(SurfaceStyleTest, BorderSubtleAlphaMatchesTheme)
{
    auto theme = makeTestTheme();
    SurfaceStyle surface;
    surface.computeFrom(theme);

    EXPECT_EQ(surface.borderSubtle.getRed(), theme.textPrimary.getRed());
    EXPECT_NEAR(surface.borderSubtle.getFloatAlpha(), theme.borderSubtleAlpha, 0.01f);
}

TEST_F(SurfaceStyleTest, BorderDefaultAlphaMatchesTheme)
{
    auto theme = makeTestTheme();
    SurfaceStyle surface;
    surface.computeFrom(theme);

    EXPECT_EQ(surface.borderDefault.getRed(), theme.textPrimary.getRed());
    EXPECT_NEAR(surface.borderDefault.getFloatAlpha(), theme.borderDefaultAlpha, 0.01f);
}

TEST_F(SurfaceStyleTest, BorderStrongAlphaMatchesTheme)
{
    auto theme = makeTestTheme();
    SurfaceStyle surface;
    surface.computeFrom(theme);

    EXPECT_EQ(surface.borderStrong.getRed(), theme.textPrimary.getRed());
    EXPECT_NEAR(surface.borderStrong.getFloatAlpha(), theme.borderStrongAlpha, 0.01f);
}

TEST_F(SurfaceStyleTest, BorderAlphaOrderingSubtleLessThanDefaultLessThanStrong)
{
    auto theme = makeTestTheme();
    SurfaceStyle surface;
    surface.computeFrom(theme);

    EXPECT_LT(surface.borderSubtle.getFloatAlpha(), surface.borderDefault.getFloatAlpha());
    EXPECT_LT(surface.borderDefault.getFloatAlpha(), surface.borderStrong.getFloatAlpha());
}

// =============================================================================
// Hover / active: neutral white overlay that reads the same on any hue
// (previously: textPrimary-tinted overlay)
// =============================================================================

TEST_F(SurfaceStyleTest, BgHoverIsWhiteAtLowAlpha)
{
    auto theme = makeTestTheme();
    SurfaceStyle surface;
    surface.computeFrom(theme);

    EXPECT_EQ(surface.bgHover.getRed(), 255);
    EXPECT_EQ(surface.bgHover.getGreen(), 255);
    EXPECT_EQ(surface.bgHover.getBlue(), 255);
    EXPECT_NEAR(surface.bgHover.getFloatAlpha(), 0.06f, 0.01f);
}

TEST_F(SurfaceStyleTest, BgActiveIsWhiteAtHigherAlpha)
{
    auto theme = makeTestTheme();
    SurfaceStyle surface;
    surface.computeFrom(theme);

    EXPECT_EQ(surface.bgActive.getRed(), 255);
    EXPECT_NEAR(surface.bgActive.getFloatAlpha(), 0.10f, 0.01f);
}

TEST_F(SurfaceStyleTest, BgActiveIsStrongerThanBgHover)
{
    auto theme = makeTestTheme();
    SurfaceStyle surface;
    surface.computeFrom(theme);

    EXPECT_GT(surface.bgActive.getFloatAlpha(), surface.bgHover.getFloatAlpha());
}

// =============================================================================
// Inset light edge: transparent in the flat aesthetic
// =============================================================================

// =============================================================================
// Shadow parameters: passed through for the one-layer drop shadow
// =============================================================================

TEST_F(SurfaceStyleTest, ShadowParametersPassedThrough)
{
    auto theme = makeTestTheme();
    theme.shadowIntensity = 0.6f;
    theme.shadowSpread = 18.0f;
    SurfaceStyle surface;
    surface.computeFrom(theme);

    EXPECT_NEAR(surface.shadowIntensity, 0.6f, 0.001f);
    EXPECT_NEAR(surface.shadowSpread, 18.0f, 0.001f);
}

// =============================================================================
// Different hue values produce visually distinct accent colors
// =============================================================================

TEST_F(SurfaceStyleTest, DifferentHuesProduceDifferentAccentColors)
{
    auto theme1 = makeTestTheme();
    theme1.accentHue = 0.0f;
    SurfaceStyle s1;
    s1.computeFrom(theme1);

    auto theme2 = makeTestTheme();
    theme2.accentHue = 120.0f;
    SurfaceStyle s2;
    s2.computeFrom(theme2);

    auto theme3 = makeTestTheme();
    theme3.accentHue = 240.0f;
    SurfaceStyle s3;
    s3.computeFrom(theme3);

    EXPECT_NE(s1.accent.getARGB(), s2.accent.getARGB());
    EXPECT_NE(s2.accent.getARGB(), s3.accent.getARGB());
    EXPECT_NE(s1.accent.getARGB(), s3.accent.getARGB());
}

// =============================================================================
// System theme surface style: every system theme produces valid output
// =============================================================================

TEST_F(SurfaceStyleTest, AllSystemThemesProduceValidSurfaceStyle)
{
    auto testTheme = [](const ColorTheme& theme) {
        SurfaceStyle surface;
        surface.computeFrom(theme);

        // Flat surfaces must be opaque — regardless of any legacy glassAlpha.
        EXPECT_NEAR(surface.bgGlass.getFloatAlpha(), 1.0f, 0.01f)
            << "Theme '" << theme.name << "' bgGlass is not opaque";
        EXPECT_NEAR(surface.bgPanel.getFloatAlpha(), 1.0f, 0.01f)
            << "Theme '" << theme.name << "' bgPanel is not opaque";

        // Accent must be fully opaque (tints are expressed via accentSubtle/Muted).
        EXPECT_NEAR(surface.accent.getFloatAlpha(), 1.0f, 0.01f) << "Theme '" << theme.name << "' accent is not opaque";

        EXPECT_GE(surface.shadowIntensity, 0.0f);

        EXPECT_LE(surface.borderSubtle.getFloatAlpha(), surface.borderDefault.getFloatAlpha())
            << "Theme '" << theme.name << "' violates border ordering";
        EXPECT_LE(surface.borderDefault.getFloatAlpha(), surface.borderStrong.getFloatAlpha())
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
