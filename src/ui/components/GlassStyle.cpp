/*
    Oscil - Glass Style Implementation
    Derives computed glass colors from ColorTheme parameters
*/

#include "ui/components/GlassStyle.h"

#include "ui/theme/ThemeManager.h"

namespace oscil
{

void GlassStyle::computeFrom(const ColorTheme& theme)
{
    bgGlass = theme.backgroundPane.withAlpha(theme.glassAlpha);
    bgPanel = theme.backgroundPane.withAlpha(theme.panelAlpha);
    bgHover = theme.textPrimary.withAlpha(0.08f);
    bgActive = theme.textPrimary.withAlpha(0.12f);

    borderSubtle = theme.textPrimary.withAlpha(theme.borderSubtleAlpha);
    borderDefault = theme.textPrimary.withAlpha(theme.borderDefaultAlpha);
    borderStrong = theme.textPrimary.withAlpha(theme.borderStrongAlpha);

    accent = juce::Colour::fromHSV(theme.accentHue / 360.0f, theme.accentSaturation, theme.accentLightness, 1.0f);
    accentSubtle = accent.withAlpha(0.15f);
    accentMuted = accent.withAlpha(0.30f);
    accentGlow = accent;

    insetLightEdge = juce::Colours::white.withAlpha(theme.lightEdgeAlpha);

    shadowIntensity = theme.shadowIntensity;
    shadowSpread = theme.shadowSpread;
    accentGlowRadius = theme.accentGlowRadius;
    accentGlowAlpha = theme.accentGlowAlpha;
}

} // namespace oscil
