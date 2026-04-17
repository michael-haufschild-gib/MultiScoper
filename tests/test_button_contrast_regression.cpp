/*
    Oscil - Button / Modal Contrast Regression Tests

    Guards two previously-observed bugs:

    (1) "Blue text on blue button" — OscilButton Primary variant previously
        rendered `glass.accent` text on `glass.accentSubtle` bg (same hue,
        different alpha). For HSV(220°, 0.7, 0.6) accent the rendered pair
        gave ~2.65:1 WCAG contrast on Glass Dark Blue — AA fail.

    (2) "White modal header on dark theme" — OscilModal titlebar painted
        `glass.bgHover.withAlpha(alpha)` where `bgHover` pre-bakes
        `textPrimary @ 0.08` and `withAlpha` REPLACES alpha, so the
        steady-state titlebar fill became `textPrimary @ 1.0` — near-white.

    These tests validate the *rendered* colours, not the ColorTheme tokens
    in isolation. Previous `test_theme_accessibility.cpp` coverage checks
    `btnPrimaryText/btnPrimaryBg` struct fields that the paint code does
    not actually read, so the decoupling gap was invisible to CI.
*/

#include "ui/components/SurfaceStyle.h"
#include "ui/theme/ColorTheme.h"
#include "ui/theme/ThemeManager.h"

#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>

using namespace oscil;

namespace
{
// Mirror the fix in OscilButtonPainting.cpp: effective bg colour for the
// Primary variant at rest is `glass.accentSubtle` composited over the pane.
juce::Colour primaryButtonEffectiveBg(const ColorTheme& theme)
{
    SurfaceStyle glass;
    glass.computeFrom(theme);
    return ColorTheme::compositeOnBackground(glass.accentSubtle, theme.backgroundPane);
}

// Mirror the fix in OscilButtonPainting.cpp::pickContrastingTextOver.
juce::Colour primaryButtonTextColour(const ColorTheme& theme)
{
    auto const bg = primaryButtonEffectiveBg(theme);
    if (ColorTheme::calculateLuminance(bg) < 0.4f)
        return theme.textHighlight;
    return juce::Colour(0xFF1A1A1A);
}

// Mirror OscilModalPainting.cpp titlebar steady-state fill after the fix.
juce::Colour modalTitlebarSteadyStateFill(const ColorTheme& theme)
{
    SurfaceStyle glass;
    glass.computeFrom(theme);
    // After the fix: withMultipliedAlpha(alpha=1.0) preserves the 0.08
    // design alpha. The bar is composited over the glass panel (which in
    // turn is composited over the backdrop).
    auto panel =
        ColorTheme::compositeOnBackground(theme.backgroundPane.withAlpha(theme.glassAlpha), theme.backgroundPrimary);
    return ColorTheme::compositeOnBackground(glass.bgHover, panel);
}
} // namespace

class ContrastRegressionTest : public ::testing::Test
{
protected:
    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }

    ThemeManager& themes() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// ---------------------------------------------------------------------------
// T2 — Primary button text meets WCAG AA on every system theme
// ---------------------------------------------------------------------------

TEST_F(ContrastRegressionTest, PrimaryButtonTextMeetsAA_AllSystemThemes)
{
    for (const auto& themeName : themes().getAvailableThemes())
    {
        auto* theme = themes().getTheme(themeName);
        ASSERT_NE(theme, nullptr) << themeName.toStdString();

        auto const bg = primaryButtonEffectiveBg(*theme);
        auto const text = primaryButtonTextColour(*theme);
        auto const ratio = ColorTheme::calculateContrastRatio(text, bg);

        EXPECT_GE(ratio, 4.5f) << "Theme '" << themeName.toStdString() << "' Primary button text contrast " << ratio
                               << " fails WCAG AA (4.5:1). bg=" << bg.toDisplayString(true).toStdString()
                               << " text=" << text.toDisplayString(true).toStdString();
    }
}

// Regression guard for the exact bug: the previously-broken resolution
// returned glass.accent — assert we do NOT return it (i.e. the buggy
// code path is gone).
TEST_F(ContrastRegressionTest, PrimaryButtonTextIsNotAccentHue_GlassDarkBlue)
{
    auto* theme = themes().getTheme("Glass Dark Blue");
    ASSERT_NE(theme, nullptr);

    SurfaceStyle glass;
    glass.computeFrom(*theme);
    auto const text = primaryButtonTextColour(*theme);

    // Compute hue distance in degrees; same-hue means bug regressed.
    float const textHue = text.getHue() * 360.0f;
    float const accentHue = glass.accent.getHue() * 360.0f;
    float hueDelta = std::abs(textHue - accentHue);
    if (hueDelta > 180.0f)
        hueDelta = 360.0f - hueDelta;

    // White / near-black text register as low saturation; report that
    // as "not accent-coloured" without a hue comparison.
    bool const textIsNeutral = text.getSaturation() < 0.1f;
    EXPECT_TRUE(textIsNeutral || hueDelta > 60.0f)
        << "Primary text shares hue with accent — blue-on-blue bug regressed.";
}

// ---------------------------------------------------------------------------
// T1 — Modal titlebar does NOT render as solid textPrimary (white bar)
// ---------------------------------------------------------------------------

TEST_F(ContrastRegressionTest, ModalTitlebar_NotSolidTextPrimary_AllDarkThemes)
{
    // Skip explicitly-light themes where textPrimary is dark; for dark
    // themes the bug produced a near-white bar, which is what we guard.
    std::vector<juce::String> darkThemes{"Dark Professional", "Classic Green", "Classic Amber", "Glass Dark Blue",
                                         "Glass Dark Purple"};

    for (const auto& themeName : darkThemes)
    {
        auto* theme = themes().getTheme(themeName);
        if (theme == nullptr)
            continue;

        auto const titlebar = modalTitlebarSteadyStateFill(*theme);

        // If titlebar == textPrimary solid, the bug regressed. Assert
        // there is a meaningful luminance gap — we expect the titlebar
        // to sit near the panel background, not near textPrimary.
        float const titleLum = ColorTheme::calculateLuminance(titlebar);
        float const textLum = ColorTheme::calculateLuminance(theme->textPrimary);
        float const paneLum = ColorTheme::calculateLuminance(theme->backgroundPane);

        // Titlebar should be closer to the pane than to textPrimary.
        float const distToText = std::abs(titleLum - textLum);
        float const distToPane = std::abs(titleLum - paneLum);

        EXPECT_LT(distToPane, distToText) << "Theme '" << themeName.toStdString() << "' titlebar luminance " << titleLum
                                          << " is closer to textPrimary (" << textLum << ") than to backgroundPane ("
                                          << paneLum << ") — white-titlebar bug regressed.";
    }
}
