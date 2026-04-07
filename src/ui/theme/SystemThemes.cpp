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

/// Set glass-style button colors from an accent color and background.
/// Primary: accent-tinted bg. Secondary/Tertiary: transparent with white overlay on hover.
void setGlassButtons(ColorTheme& theme, juce::Colour accent, juce::Colour disabledText)
{
    theme.btnPrimaryBg = accent.withAlpha(0.15f).withAlpha(1.0f).interpolatedWith(theme.backgroundPrimary, 0.5f);
    theme.btnPrimaryBgHover = accent.withAlpha(0.25f).withAlpha(1.0f).interpolatedWith(theme.backgroundPrimary, 0.4f);
    theme.btnPrimaryBgActive = accent.withAlpha(0.10f).withAlpha(1.0f).interpolatedWith(theme.backgroundPrimary, 0.6f);
    theme.btnPrimaryBgDisabled = theme.controlBackground;
    theme.btnPrimaryText = juce::Colour(0xFFFFFFFF);
    theme.btnPrimaryTextHover = juce::Colour(0xFFFFFFFF);
    theme.btnPrimaryTextActive = juce::Colour(0xFFFFFFFF);
    theme.btnPrimaryTextDisabled = disabledText;

    theme.btnSecondaryBg = juce::Colour(0x00000000);
    theme.btnSecondaryBgHover = juce::Colour(0x14FFFFFF);
    theme.btnSecondaryBgActive = juce::Colour(0x1FFFFFFF);
    theme.btnSecondaryBgDisabled = juce::Colour(0x00000000);
    theme.btnSecondaryText = juce::Colour(0xBBFFFFFF);
    theme.btnSecondaryTextHover = juce::Colour(0xFFFFFFFF);
    theme.btnSecondaryTextActive = juce::Colour(0xFFFFFFFF);
    theme.btnSecondaryTextDisabled = disabledText;

    theme.btnTertiaryBg = juce::Colours::transparentBlack;
    theme.btnTertiaryBgHover = juce::Colour(0x14FFFFFF);
    theme.btnTertiaryBgActive = juce::Colour(0x1FFFFFFF);
    theme.btnTertiaryBgDisabled = juce::Colours::transparentBlack;
    theme.btnTertiaryText = juce::Colour(0xBBFFFFFF);
    theme.btnTertiaryTextHover = juce::Colour(0xFFFFFFFF);
    theme.btnTertiaryTextActive = juce::Colour(0xFFFFFFFF);
    theme.btnTertiaryTextDisabled = disabledText;
}

} // namespace

ColorTheme createDarkProfessional()
{
    ColorTheme theme;
    theme.name = "Dark Professional";
    theme.isSystemTheme = true;
    theme.backgroundPrimary = juce::Colour(0xFF1E1E1E);
    theme.backgroundSecondary = juce::Colour(0xFF2D2D2D);
    theme.backgroundPane = juce::Colour(0xFF252525);
    theme.gridMajor = juce::Colour(0xFF3A3A3A);
    theme.gridMinor = juce::Colour(0xFF2A2A2A);
    theme.gridZeroLine = juce::Colour(0xFF4A4A4A);
    theme.textPrimary = juce::Colour(0xFFE0E0E0);
    theme.textSecondary = juce::Colour(0xFFA0A0A0);
    theme.textHighlight = juce::Colour(0xFFFFFFFF);
    theme.controlBackground = juce::Colour(0xFF353535);
    theme.controlBorder = juce::Colour(0xFF454545);
    theme.controlHighlight = juce::Colour(0xFF505050);
    theme.controlActive = juce::Colour(0xFF007ACC);
    theme.btnPrimaryBg = juce::Colour(0xFF007ACC);
    theme.btnPrimaryBgHover = juce::Colour(0xFF008AD9);
    theme.btnPrimaryBgActive = juce::Colour(0xFF0062A3);
    theme.btnPrimaryBgDisabled = juce::Colour(0xFF353535);
    theme.btnPrimaryText = juce::Colour(0xFFFFFFFF);
    theme.btnPrimaryTextDisabled = juce::Colour(0xFFA0A0A0);

    theme.btnSecondaryBg = juce::Colour(0xFF3A3A3A);
    theme.btnSecondaryBgHover = juce::Colour(0xFF454545);
    theme.btnSecondaryBgActive = juce::Colour(0xFF303030);
    theme.btnSecondaryBgDisabled = juce::Colour(0xFF252525);
    theme.btnSecondaryText = juce::Colour(0xFFE0E0E0);
    theme.btnSecondaryTextDisabled = juce::Colour(0xFF606060);

    theme.btnTertiaryBg = juce::Colours::transparentBlack;
    theme.btnTertiaryBgHover = juce::Colour(0x1AFFFFFF);
    theme.btnTertiaryBgActive = juce::Colour(0x33FFFFFF);
    theme.btnTertiaryBgDisabled = juce::Colours::transparentBlack;
    theme.btnTertiaryText = juce::Colour(0xFFE0E0E0);
    theme.btnTertiaryTextDisabled = juce::Colour(0xFF606060);

    // Glass defaults are sensible for a flat opaque theme
    // (struct defaults: glassAlpha=0.55, panelAlpha=0.82, etc.)
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

// NOLINTNEXTLINE(readability-function-size)
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

// === Glass System Themes ===

ColorTheme createGlassDarkBlue()
{
    ColorTheme theme;
    theme.name = "Glass Dark Blue";
    theme.isSystemTheme = true;

    theme.backgroundPrimary = juce::Colour(0xFF0F1520);
    theme.backgroundSecondary = juce::Colour(0xFF141D2B);
    theme.backgroundPane = juce::Colour(0xFF131B28);
    theme.gridMajor = juce::Colour(0xFF1E2D42);
    theme.gridMinor = juce::Colour(0xFF152030);
    theme.gridZeroLine = juce::Colour(0xFF2A3F5A);
    theme.textPrimary = juce::Colour(0xFFDDE4EE);
    theme.textSecondary = juce::Colour(0xBBFFFFFF);
    theme.textHighlight = juce::Colour(0xFFFFFFFF);
    theme.controlBackground = juce::Colour(0xFF1A2538);
    theme.controlBorder = juce::Colour(0xFF2A3B52);
    theme.controlHighlight = juce::Colour(0xFF2E4060);
    theme.controlActive = juce::Colour(0xFF4D8FD6);

    theme.accentHue = 220.0f;
    theme.accentSaturation = 0.7f;
    theme.accentLightness = 0.6f;

    auto accent = juce::Colour::fromHSV(220.0f / 360.0f, 0.7f, 0.6f, 1.0f);
    setGlassButtons(theme, accent, juce::Colour(0xFF607080));
    return theme;
}

ColorTheme createGlassDarkPurple()
{
    ColorTheme theme;
    theme.name = "Glass Dark Purple";
    theme.isSystemTheme = true;

    theme.backgroundPrimary = juce::Colour(0xFF150D24);
    theme.backgroundSecondary = juce::Colour(0xFF1A1230);
    theme.backgroundPane = juce::Colour(0xFF170F28);
    theme.gridMajor = juce::Colour(0xFF251840);
    theme.gridMinor = juce::Colour(0xFF1C1030);
    theme.gridZeroLine = juce::Colour(0xFF352458);
    theme.textPrimary = juce::Colour(0xFFE0D8F0);
    theme.textSecondary = juce::Colour(0xBBFFFFFF);
    theme.textHighlight = juce::Colour(0xFFFFFFFF);
    theme.controlBackground = juce::Colour(0xFF1E1435);
    theme.controlBorder = juce::Colour(0xFF302250);
    theme.controlHighlight = juce::Colour(0xFF3A2860);
    theme.controlActive = juce::Colour(0xFF9966DD);

    theme.accentHue = 270.0f;
    theme.accentSaturation = 0.7f;
    theme.accentLightness = 0.6f;

    auto accent = juce::Colour::fromHSV(270.0f / 360.0f, 0.7f, 0.6f, 1.0f);
    setGlassButtons(theme, accent, juce::Colour(0xFF605070));
    return theme;
}

ColorTheme createGlassDarkBrown()
{
    ColorTheme theme;
    theme.name = "Glass Dark Brown";
    theme.isSystemTheme = true;

    theme.backgroundPrimary = juce::Colour(0xFF1A150D);
    theme.backgroundSecondary = juce::Colour(0xFF201A10);
    theme.backgroundPane = juce::Colour(0xFF1C1710);
    theme.gridMajor = juce::Colour(0xFF30281A);
    theme.gridMinor = juce::Colour(0xFF241E12);
    theme.gridZeroLine = juce::Colour(0xFF403525);
    theme.textPrimary = juce::Colour(0xFFEDE4D6);
    theme.textSecondary = juce::Colour(0xBBFFFFFF);
    theme.textHighlight = juce::Colour(0xFFFFFFFF);
    theme.controlBackground = juce::Colour(0xFF251E14);
    theme.controlBorder = juce::Colour(0xFF3A3020);
    theme.controlHighlight = juce::Colour(0xFF45382A);
    theme.controlActive = juce::Colour(0xFFD4952A);

    theme.accentHue = 40.0f;
    theme.accentSaturation = 0.8f;
    theme.accentLightness = 0.6f;

    auto accent = juce::Colour::fromHSV(40.0f / 360.0f, 0.8f, 0.6f, 1.0f);
    setGlassButtons(theme, accent, juce::Colour(0xFF706050));
    return theme;
}

ColorTheme createGlassDarkBlack()
{
    ColorTheme theme;
    theme.name = "Glass Dark Black";
    theme.isSystemTheme = true;

    theme.backgroundPrimary = juce::Colour(0xFF111114);
    theme.backgroundSecondary = juce::Colour(0xFF161619);
    theme.backgroundPane = juce::Colour(0xFF131316);
    theme.gridMajor = juce::Colour(0xFF222226);
    theme.gridMinor = juce::Colour(0xFF1A1A1E);
    theme.gridZeroLine = juce::Colour(0xFF2C2C32);
    theme.textPrimary = juce::Colour(0xFFE0E0E4);
    theme.textSecondary = juce::Colour(0xBBFFFFFF);
    theme.textHighlight = juce::Colour(0xFFFFFFFF);
    theme.controlBackground = juce::Colour(0xFF1A1A1E);
    theme.controlBorder = juce::Colour(0xFF2A2A30);
    theme.controlHighlight = juce::Colour(0xFF32323A);
    theme.controlActive = juce::Colour(0xFF7080AA);

    theme.accentHue = 240.0f;
    theme.accentSaturation = 0.3f;
    theme.accentLightness = 0.6f;

    auto accent = juce::Colour::fromHSV(240.0f / 360.0f, 0.3f, 0.6f, 1.0f);
    setGlassButtons(theme, accent, juce::Colour(0xFF606068));
    return theme;
}

} // namespace oscil::SystemThemes
