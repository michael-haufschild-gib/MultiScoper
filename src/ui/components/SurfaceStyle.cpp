/*
    MultiScoper - Surface Style (flat) — Implementation

    Token semantics:
      * `bgGlass` and `bgPanel` are fully opaque surfaces.
      * `bgHover` / `bgActive` are faint tints of the surface on interaction.
      * `accentSubtle` / `accentMuted` are tints of the accent used for
        selection/hover, NOT button-background colours.
      * Shadow parameters default to zero; paint code treats them as optional.
*/

#include "ui/components/SurfaceStyle.h"

#include "ui/theme/ThemeManager.h"

namespace multiscoper
{

void SurfaceStyle::computeFrom(const ColorTheme& theme)
{
    // Opaque surfaces — no translucency.
    bgGlass = theme.backgroundPane;
    bgPanel = theme.backgroundPane;

    // Interaction tints: derived from textPrimary so a dark theme gets a
    // pale overlay (textPrimary ~ white) and a light theme gets a darker
    // overlay (textPrimary ~ near-black). A hardcoded white tint is
    // invisible on light surfaces and was the cause of "no hover feedback"
    // / "hover paints near-black" bugs in mixed-theme paint code.
    bgHover = theme.textPrimary.withAlpha(0.06f);
    bgActive = theme.textPrimary.withAlpha(0.10f);

    // Hairline-class borders derived from textPrimary so they look crisp
    // on every palette (dark or light).
    borderSubtle = theme.textPrimary.withAlpha(theme.borderSubtleAlpha);
    borderDefault = theme.textPrimary.withAlpha(theme.borderDefaultAlpha);
    borderStrong = theme.textPrimary.withAlpha(theme.borderStrongAlpha);

    // Accent tokens. `accent` is the saturated brand colour (solid, no alpha
    // baked in). Subtle/muted are alpha variants for selection tints.
    accent = juce::Colour::fromHSV(theme.accentHue / 360.0f, theme.accentSaturation, theme.accentLightness, 1.0f);
    accentSubtle = accent.withAlpha(0.15f);
    accentMuted = accent.withAlpha(0.35f);

    shadowIntensity = theme.shadowIntensity;
    shadowSpread = theme.shadowSpread;
}

} // namespace multiscoper
