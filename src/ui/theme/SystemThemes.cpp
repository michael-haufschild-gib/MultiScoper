/*
    Oscil - System Theme Definitions
    Pre-defined immutable themes shipped with the application
*/

#include "ui/theme/ThemeManager.h"

#include <cmath>

namespace oscil::SystemThemes
{

namespace
{

/// Button palette for the Light Mode theme.
/// Extracted from createLightMode() to keep that function within statement limits.
void setLightModeButtons(ColorTheme& theme)
{
    theme.btnPrimaryBg = juce::Colour(0xFF0066CC);
    theme.btnPrimaryBgHover = juce::Colour(0xFF0077DD);
    theme.btnPrimaryBgActive = juce::Colour(0xFF005599);
    theme.btnPrimaryBgDisabled = juce::Colour(0xFFCCCCCC);
    theme.btnPrimaryText = juce::Colour(0xFFFFFFFF);
    theme.btnPrimaryTextHover = juce::Colour(0xFFFFFFFF);
    theme.btnPrimaryTextActive = juce::Colour(0xFFFFFFFF);
    theme.btnPrimaryTextDisabled = juce::Colour(0xFF888888);

    theme.btnSecondaryBg = juce::Colour(0xFFE0E0E0);
    theme.btnSecondaryBgHover = juce::Colour(0xFFD0D0D0);
    theme.btnSecondaryBgActive = juce::Colour(0xFFC0C0C0);
    theme.btnSecondaryBgDisabled = juce::Colour(0xFFF0F0F0);
    theme.btnSecondaryText = juce::Colour(0xFF303030);
    theme.btnSecondaryTextHover = juce::Colour(0xFF202020);
    theme.btnSecondaryTextActive = juce::Colour(0xFF202020);
    theme.btnSecondaryTextDisabled = juce::Colour(0xFFA0A0A0);

    theme.btnTertiaryBg = juce::Colours::transparentBlack;
    theme.btnTertiaryBgHover = juce::Colour(0x1A000000);
    theme.btnTertiaryBgActive = juce::Colour(0x33000000);
    theme.btnTertiaryBgDisabled = juce::Colours::transparentBlack;
    theme.btnTertiaryText = juce::Colour(0xFF404040);
    theme.btnTertiaryTextHover = juce::Colour(0xFF202020);
    theme.btnTertiaryTextActive = juce::Colour(0xFF202020);
    theme.btnTertiaryTextDisabled = juce::Colour(0xFFA0A0A0);
}

/// Flat button colour pack: solid accent fill for Primary, dark
/// contrast-safe text on the accent, transparent chromeless Secondary /
/// Tertiary with white overlay on hover.
void setGlassButtons(ColorTheme& theme, juce::Colour accent)
{
    // Pick the text colour that actually gives the higher WCAG contrast
    // against the accent. A fixed luminance threshold (previously 0.55)
    // straddles the switchover for several accent hues, producing AA
    // failures — compute both ratios and pick the winner instead.
    auto const kWhite = juce::Colour(0xFFFFFFFF);
    auto const kNearBlack = juce::Colour(0xFF0A0D12);
    auto const whiteRatio = ColorTheme::calculateContrastRatio(kWhite, accent);
    auto const blackRatio = ColorTheme::calculateContrastRatio(kNearBlack, accent);
    auto const primaryText = (whiteRatio >= blackRatio) ? kWhite : kNearBlack;

    theme.btnPrimaryBg = accent;
    theme.btnPrimaryBgHover = accent.brighter(0.10f);
    theme.btnPrimaryBgActive = accent.darker(0.18f);
    theme.btnPrimaryBgDisabled = theme.controlBackground;
    theme.btnPrimaryText = primaryText;
    theme.btnPrimaryTextHover = primaryText;
    theme.btnPrimaryTextActive = primaryText;
    theme.btnPrimaryTextDisabled = theme.textMuted;

    theme.btnSecondaryBg = juce::Colour(0x00000000);
    theme.btnSecondaryBgHover = juce::Colour(0x14FFFFFF);
    theme.btnSecondaryBgActive = juce::Colour(0x1FFFFFFF);
    theme.btnSecondaryBgDisabled = juce::Colour(0x00000000);
    theme.btnSecondaryText = theme.textPrimary;
    theme.btnSecondaryTextHover = juce::Colour(0xFFFFFFFF);
    theme.btnSecondaryTextActive = juce::Colour(0xFFFFFFFF);
    theme.btnSecondaryTextDisabled = theme.textMuted;

    theme.btnTertiaryBg = juce::Colours::transparentBlack;
    theme.btnTertiaryBgHover = juce::Colour(0x14FFFFFF);
    theme.btnTertiaryBgActive = juce::Colour(0x1FFFFFFF);
    theme.btnTertiaryBgDisabled = juce::Colours::transparentBlack;
    theme.btnTertiaryText = theme.textSecondary;
    theme.btnTertiaryTextHover = theme.textPrimary;
    theme.btnTertiaryTextActive = theme.textPrimary;
    theme.btnTertiaryTextDisabled = theme.textMuted;
}

void setDarkProfessionalPalette(ColorTheme& theme)
{
    theme.backgroundPrimary = juce::Colour(0xFF0A0D12);
    theme.backgroundSecondary = juce::Colour(0xFF10141B);
    theme.backgroundPane = juce::Colour(0xFF0F131A);
    theme.backgroundRaised = juce::Colour(0xFF161B24);
    theme.divider = juce::Colour(0xFF1F2630);
    theme.gridMajor = juce::Colour(0xFF242A35);
    theme.gridMinor = juce::Colour(0xFF171C25);
    theme.gridZeroLine = juce::Colour(0xFF3B4555);
    theme.textPrimary = juce::Colour(0xFFE8EAED);
    theme.textSecondary = juce::Colour(0xFF8A94A3);
    theme.textHighlight = juce::Colour(0xFFFFFFFF);
    theme.textMuted = juce::Colour(0xFF4B5563);
    theme.controlBackground = juce::Colour(0xFF161B24);
    theme.controlBorder = juce::Colour(0xFF242A35);
    theme.controlHighlight = juce::Colour(0xFF1F2630);
    theme.controlActive = juce::Colour(0xFF1FD4F3);

    theme.accentHue = 188.0f;
    theme.accentSaturation = 0.87f;
    theme.accentLightness = 0.95f;
    theme.glassAlpha = 1.0f;
    theme.panelAlpha = 1.0f;
    theme.blurRadius = 0.0f;
    theme.lightEdgeAlpha = 0.0f;
    theme.accentGlowRadius = 0.0f;
    theme.accentGlowAlpha = 0.0f;
    theme.shadowIntensity = 0.18f;
    theme.shadowSpread = 4.0f;
}

void setDarkProfessionalButtons(ColorTheme& theme)
{
    // Primary: solid accent fill with contrast-safe text (white).
    theme.btnPrimaryBg = juce::Colour(0xFF1FD4F3);
    theme.btnPrimaryBgHover = juce::Colour(0xFF4DDDF5);
    theme.btnPrimaryBgActive = juce::Colour(0xFF16ADC5);
    theme.btnPrimaryBgDisabled = juce::Colour(0xFF161B24);
    theme.btnPrimaryText = juce::Colour(0xFF0A0D12);
    theme.btnPrimaryTextHover = juce::Colour(0xFF0A0D12);
    theme.btnPrimaryTextActive = juce::Colour(0xFF0A0D12);
    theme.btnPrimaryTextDisabled = juce::Colour(0xFF4B5563);

    // Secondary: transparent with hairline border; turns raised on hover.
    theme.btnSecondaryBg = juce::Colours::transparentBlack;
    theme.btnSecondaryBgHover = juce::Colour(0x14FFFFFF);
    theme.btnSecondaryBgActive = juce::Colour(0x1FFFFFFF);
    theme.btnSecondaryBgDisabled = juce::Colours::transparentBlack;
    theme.btnSecondaryText = juce::Colour(0xFFE8EAED);
    theme.btnSecondaryTextHover = juce::Colour(0xFFFFFFFF);
    theme.btnSecondaryTextActive = juce::Colour(0xFFFFFFFF);
    theme.btnSecondaryTextDisabled = juce::Colour(0xFF4B5563);

    theme.btnTertiaryBg = juce::Colours::transparentBlack;
    theme.btnTertiaryBgHover = juce::Colour(0x14FFFFFF);
    theme.btnTertiaryBgActive = juce::Colour(0x1FFFFFFF);
    theme.btnTertiaryBgDisabled = juce::Colours::transparentBlack;
    theme.btnTertiaryText = juce::Colour(0xFF7A8293);
    theme.btnTertiaryTextHover = juce::Colour(0xFFE8EAED);
    theme.btnTertiaryTextActive = juce::Colour(0xFFE8EAED);
    theme.btnTertiaryTextDisabled = juce::Colour(0xFF4B5563);
}

} // namespace

ColorTheme createDarkProfessional()
{
    // "Dark Professional" is the flagship theme. Near-black surfaces
    // separated by 1px hairlines, a bright cyan primary accent (#1FD4F3),
    // and a terse three-tier text hierarchy.
    ColorTheme theme;
    theme.name = "Dark Professional";
    theme.isSystemTheme = true;
    setDarkProfessionalPalette(theme);
    setDarkProfessionalButtons(theme);
    return theme;
}

ColorTheme createClassicGreen()
{
    ColorTheme theme;
    theme.name = "Classic Green";
    theme.isSystemTheme = true;
    theme.backgroundPrimary = juce::Colour(0xFF0A0A0A);
    theme.backgroundSecondary = juce::Colour(0xFF151515);
    theme.backgroundPane = juce::Colour(0xFF0D0D0D);
    theme.gridMajor = juce::Colour(0xFF1A3A1A);
    theme.gridMinor = juce::Colour(0xFF0D1F0D);
    theme.gridZeroLine = juce::Colour(0xFF2A4A2A);
    theme.textPrimary = juce::Colour(0xFF00FF00);
    theme.textSecondary = juce::Colour(0xFF00AA00);
    theme.textHighlight = juce::Colour(0xFF00FF00);
    theme.controlBackground = juce::Colour(0xFF1A1A1A);
    theme.controlBorder = juce::Colour(0xFF00AA00);
    theme.controlHighlight = juce::Colour(0xFF003300);
    theme.controlActive = juce::Colour(0xFF00FF00);

    // Green-tinted buttons matching the monochrome aesthetic
    theme.btnPrimaryBg = juce::Colour(0xFF005500);
    theme.btnPrimaryBgHover = juce::Colour(0xFF006600);
    theme.btnPrimaryBgActive = juce::Colour(0xFF004400);
    theme.btnPrimaryBgDisabled = juce::Colour(0xFF1A1A1A);
    theme.btnPrimaryText = juce::Colour(0xFF00FF00);
    theme.btnPrimaryTextHover = juce::Colour(0xFF00FF00);
    theme.btnPrimaryTextActive = juce::Colour(0xFF00FF00);
    theme.btnPrimaryTextDisabled = juce::Colour(0xFF004400);

    theme.btnSecondaryBg = juce::Colour(0xFF0D1F0D);
    theme.btnSecondaryBgHover = juce::Colour(0xFF1A3A1A);
    theme.btnSecondaryBgActive = juce::Colour(0xFF0A150A);
    theme.btnSecondaryBgDisabled = juce::Colour(0xFF0A0A0A);
    theme.btnSecondaryText = juce::Colour(0xFF00AA00);
    theme.btnSecondaryTextHover = juce::Colour(0xFF00FF00);
    theme.btnSecondaryTextActive = juce::Colour(0xFF00FF00);
    theme.btnSecondaryTextDisabled = juce::Colour(0xFF003300);

    theme.btnTertiaryBg = juce::Colours::transparentBlack;
    theme.btnTertiaryBgHover = juce::Colour(0x1A00FF00);
    theme.btnTertiaryBgActive = juce::Colour(0x3300FF00);
    theme.btnTertiaryBgDisabled = juce::Colours::transparentBlack;
    theme.btnTertiaryText = juce::Colour(0xFF00AA00);
    theme.btnTertiaryTextHover = juce::Colour(0xFF00FF00);
    theme.btnTertiaryTextActive = juce::Colour(0xFF00FF00);
    theme.btnTertiaryTextDisabled = juce::Colour(0xFF003300);

    theme.waveformColors.clear();
    for (int i = 0; i < 64; ++i)
    {
        float const brightness = 0.5f + (0.5f * (static_cast<float>(i) / 64.0f));
        theme.waveformColors.push_back(juce::Colour::fromHSV(0.33f, 1.0f, brightness, 1.0f));
    }
    return theme;
}

ColorTheme createClassicAmber()
{
    ColorTheme theme;
    theme.name = "Classic Amber";
    theme.isSystemTheme = true;
    theme.backgroundPrimary = juce::Colour(0xFF0A0A00);
    theme.backgroundSecondary = juce::Colour(0xFF151508);
    theme.backgroundPane = juce::Colour(0xFF0D0D05);
    theme.gridMajor = juce::Colour(0xFF3A3A1A);
    theme.gridMinor = juce::Colour(0xFF1F1F0D);
    theme.gridZeroLine = juce::Colour(0xFF4A4A2A);
    theme.textPrimary = juce::Colour(0xFFFFAA00);
    theme.textSecondary = juce::Colour(0xFFAA7700);
    theme.textHighlight = juce::Colour(0xFFFFCC00);
    theme.controlBackground = juce::Colour(0xFF1A1A0A);
    theme.controlBorder = juce::Colour(0xFFAA7700);
    theme.controlHighlight = juce::Colour(0xFF332200);
    theme.controlActive = juce::Colour(0xFFFFAA00);

    // Amber-tinted buttons matching the monochrome aesthetic
    theme.btnPrimaryBg = juce::Colour(0xFF553300);
    theme.btnPrimaryBgHover = juce::Colour(0xFF664400);
    theme.btnPrimaryBgActive = juce::Colour(0xFF442200);
    theme.btnPrimaryBgDisabled = juce::Colour(0xFF1A1A0A);
    theme.btnPrimaryText = juce::Colour(0xFFFFAA00);
    theme.btnPrimaryTextHover = juce::Colour(0xFFFFCC00);
    theme.btnPrimaryTextActive = juce::Colour(0xFFFFAA00);
    theme.btnPrimaryTextDisabled = juce::Colour(0xFF443300);

    theme.btnSecondaryBg = juce::Colour(0xFF1F1F0D);
    theme.btnSecondaryBgHover = juce::Colour(0xFF3A3A1A);
    theme.btnSecondaryBgActive = juce::Colour(0xFF15150A);
    theme.btnSecondaryBgDisabled = juce::Colour(0xFF0A0A00);
    theme.btnSecondaryText = juce::Colour(0xFFBB8800);
    theme.btnSecondaryTextHover = juce::Colour(0xFFFFAA00);
    theme.btnSecondaryTextActive = juce::Colour(0xFFFFAA00);
    theme.btnSecondaryTextDisabled = juce::Colour(0xFF332200);

    theme.btnTertiaryBg = juce::Colours::transparentBlack;
    theme.btnTertiaryBgHover = juce::Colour(0x1AFFAA00);
    theme.btnTertiaryBgActive = juce::Colour(0x33FFAA00);
    theme.btnTertiaryBgDisabled = juce::Colours::transparentBlack;
    theme.btnTertiaryText = juce::Colour(0xFFAA7700);
    theme.btnTertiaryTextHover = juce::Colour(0xFFFFAA00);
    theme.btnTertiaryTextActive = juce::Colour(0xFFFFAA00);
    theme.btnTertiaryTextDisabled = juce::Colour(0xFF332200);

    theme.waveformColors.clear();
    for (int i = 0; i < 64; ++i)
    {
        float const hue = 0.08f + (0.04f * std::sin(static_cast<float>(i) * 0.2f));
        theme.waveformColors.push_back(juce::Colour::fromHSV(hue, 1.0f, 1.0f, 1.0f));
    }
    return theme;
}

ColorTheme createHighContrast()
{
    ColorTheme theme;
    theme.name = "High Contrast";
    theme.isSystemTheme = true;
    theme.backgroundPrimary = juce::Colour(0xFF000000);
    theme.backgroundSecondary = juce::Colour(0xFF000000);
    theme.backgroundPane = juce::Colour(0xFF000000);
    theme.gridMajor = juce::Colour(0xFF404040);
    theme.gridMinor = juce::Colour(0xFF202020);
    theme.gridZeroLine = juce::Colour(0xFF606060);
    theme.textPrimary = juce::Colour(0xFFFFFFFF);
    theme.textSecondary = juce::Colour(0xFFCCCCCC);
    theme.textHighlight = juce::Colour(0xFFFFFF00);
    theme.controlBackground = juce::Colour(0xFF000000);
    theme.controlBorder = juce::Colour(0xFFFFFFFF);
    theme.controlHighlight = juce::Colour(0xFF404040);
    theme.controlActive = juce::Colour(0xFFFFFF00);
    theme.crosshairLine = juce::Colour(0xCCFFFFFF);

    // High Contrast: full opacity glass for accessibility — no translucency
    theme.glassAlpha = 1.0f;
    theme.panelAlpha = 1.0f;
    theme.borderSubtleAlpha = 0.30f;
    theme.borderDefaultAlpha = 0.50f;
    theme.borderStrongAlpha = 0.80f;
    theme.lightEdgeAlpha = 0.0f;
    theme.accentHue = 60.0f; // yellow accent matching textHighlight
    theme.accentSaturation = 1.0f;
    theme.accentLightness = 0.5f;

    theme.waveformColors.clear();
    theme.waveformColors = {
        juce::Colour(0xFFFFFF00), juce::Colour(0xFF00FFFF), juce::Colour(0xFFFF00FF), juce::Colour(0xFFFFFFFF),
        juce::Colour(0xFF00FF00), juce::Colour(0xFFFF0000), juce::Colour(0xFF0080FF), juce::Colour(0xFFFF8000),
    };
    while (theme.waveformColors.size() < 64)
        theme.waveformColors.emplace_back(0xFFFFFFFF);
    return theme;
}

ColorTheme createLightMode()
{
    ColorTheme theme;
    theme.name = "Light Mode";
    theme.isSystemTheme = true;
    theme.backgroundPrimary = juce::Colour(0xFFF5F5F5);
    theme.backgroundSecondary = juce::Colour(0xFFE8E8E8);
    theme.backgroundPane = juce::Colour(0xFFFFFFFF);
    theme.gridMajor = juce::Colour(0xFFCCCCCC);
    theme.gridMinor = juce::Colour(0xFFE0E0E0);
    theme.gridZeroLine = juce::Colour(0xFFAAAAAA);
    theme.textPrimary = juce::Colour(0xFF202020);
    theme.textSecondary = juce::Colour(0xFF606060);
    theme.textHighlight = juce::Colour(0xFF000000);
    theme.controlBackground = juce::Colour(0xFFFFFFFF);
    theme.controlBorder = juce::Colour(0xFFCCCCCC);
    theme.controlHighlight = juce::Colour(0xFFE0E0E0);
    theme.controlActive = juce::Colour(0xFF0066CC);
    theme.crosshairLine = juce::Colour(0xCC202020);

    // Status colors darkened for contrast on light backgrounds (WCAG AA large text 3:1)
    theme.statusActive = juce::Colour(0xFF006600);  // Dark green
    theme.statusWarning = juce::Colour(0xFF886600); // Dark amber
    theme.statusError = juce::Colour(0xFFAA0000);   // Dark red

    // Light mode: higher glass alpha for readability, dark-text borders
    theme.glassAlpha = 0.75f;
    theme.panelAlpha = 0.90f;
    theme.accentHue = 215.0f;
    theme.accentSaturation = 0.8f;
    theme.accentLightness = 0.45f;

    // Light-mode buttons: dark accent on light bg
    setLightModeButtons(theme);

    theme.waveformColors.clear();
    theme.waveformColors = {
        juce::Colour(0xFF007700), juce::Colour(0xFF007777), juce::Colour(0xFF770077), juce::Colour(0xFF777700),
        juce::Colour(0xFFCC5500), juce::Colour(0xFF0000CC), juce::Colour(0xFFCC0000), juce::Colour(0xFF00CC00),
    };
    while (theme.waveformColors.size() < 64)
    {
        float const hue = static_cast<float>(theme.waveformColors.size()) / 64.0f;
        theme.waveformColors.push_back(juce::Colour::fromHSV(hue, 0.9f, 0.6f, 1.0f));
    }
    return theme;
}

// === Accent-variant System Themes ===
// All accent variants share the flat base palette and differ only in
// `accent*` and some subtle hue tinting of backgrounds. The "Glass" prefix
// is retained for saved-theme / test compatibility; the aesthetic has been
// flattened to match Dark Professional.

namespace
{
/// Apply the flat base palette to `theme`, then adjust the accent triplet.
/// Caller may further tint specific colours afterwards.
void applyFlatBase(ColorTheme& theme, float hue, float saturation, float lightness)
{
    theme.backgroundPrimary = juce::Colour(0xFF0A0D12);
    theme.backgroundSecondary = juce::Colour(0xFF10141B);
    theme.backgroundPane = juce::Colour(0xFF0F131A);
    theme.backgroundRaised = juce::Colour(0xFF161B24);
    theme.divider = juce::Colour(0xFF1F2630);
    theme.gridMajor = juce::Colour(0xFF242A35);
    theme.gridMinor = juce::Colour(0xFF171C25);
    theme.gridZeroLine = juce::Colour(0xFF3B4555);
    theme.textPrimary = juce::Colour(0xFFE8EAED);
    theme.textSecondary = juce::Colour(0xFF8A94A3);
    theme.textHighlight = juce::Colour(0xFFFFFFFF);
    theme.textMuted = juce::Colour(0xFF4B5563);
    theme.controlBackground = juce::Colour(0xFF161B24);
    theme.controlBorder = juce::Colour(0xFF242A35);
    theme.controlHighlight = juce::Colour(0xFF1F2630);

    theme.accentHue = hue;
    theme.accentSaturation = saturation;
    theme.accentLightness = lightness;
    theme.controlActive = juce::Colour::fromHSV(hue / 360.0f, saturation, lightness, 1.0f);

    theme.glassAlpha = 1.0f;
    theme.panelAlpha = 1.0f;
    theme.blurRadius = 0.0f;
    theme.lightEdgeAlpha = 0.0f;
    theme.accentGlowRadius = 0.0f;
    theme.accentGlowAlpha = 0.0f;
    theme.shadowIntensity = 0.18f;
    theme.shadowSpread = 4.0f;
    theme.borderSubtleAlpha = 0.06f;
    theme.borderDefaultAlpha = 0.10f;
    theme.borderStrongAlpha = 0.20f;
}
} // namespace

ColorTheme createGlassDarkBlue()
{
    ColorTheme theme;
    theme.name = "Glass Dark Blue";
    theme.isSystemTheme = true;

    // Cobalt accent (#5A9CFF-ish) on the flat base.
    // Lightness 0.95 keeps the accent bright enough that near-black text
    // on a Primary button clears WCAG AA (4.5:1); 0.85 lands in the dead
    // zone where neither white nor black text passes.
    applyFlatBase(theme, /*hue=*/220.0f, /*saturation=*/0.7f, /*lightness=*/0.95f);
    auto accent = juce::Colour::fromHSV(220.0f / 360.0f, 0.7f, 0.95f, 1.0f);
    setGlassButtons(theme, accent);
    return theme;
}

ColorTheme createGlassDarkPurple()
{
    ColorTheme theme;
    theme.name = "Glass Dark Purple";
    theme.isSystemTheme = true;

    // Violet accent — #9966DD-ish. Lightness 0.95 for AA contrast headroom
    // (see Glass Dark Blue comment).
    applyFlatBase(theme, /*hue=*/270.0f, /*saturation=*/0.6f, /*lightness=*/0.95f);
    auto accent = juce::Colour::fromHSV(270.0f / 360.0f, 0.6f, 0.95f, 1.0f);
    setGlassButtons(theme, accent);
    return theme;
}

ColorTheme createGlassDarkBrown()
{
    ColorTheme theme;
    theme.name = "Glass Dark Brown";
    theme.isSystemTheme = true;

    // Amber accent — #D4952A-ish.
    applyFlatBase(theme, /*hue=*/40.0f, /*saturation=*/0.8f, /*lightness=*/0.85f);
    auto accent = juce::Colour::fromHSV(40.0f / 360.0f, 0.8f, 0.85f, 1.0f);
    setGlassButtons(theme, accent);
    return theme;
}

ColorTheme createGlassDarkBlack()
{
    ColorTheme theme;
    theme.name = "Glass Dark Black";
    theme.isSystemTheme = true;

    // Neutral cool-grey accent — muted, engineering-tool aesthetic.
    applyFlatBase(theme, /*hue=*/220.0f, /*saturation=*/0.15f, /*lightness=*/0.75f);
    auto accent = juce::Colour::fromHSV(220.0f / 360.0f, 0.15f, 0.75f, 1.0f);
    setGlassButtons(theme, accent);
    return theme;
}

} // namespace oscil::SystemThemes
