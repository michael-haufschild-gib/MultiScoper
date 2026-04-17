/*
    Oscil - Surface Style (flat) — Implementation

    Historical name: "GlassStyle" / "glassmorphism". The 2026 Oscil uplift
    removed that aesthetic entirely. The struct was renamed to SurfaceStyle
    but field names (`bgGlass`, `glassAlpha`, `lightEdgeAlpha`, `accentGlow*`)
    are retained as stable ValueTree serialization tokens — changing them
    would break user theme files on disk.

    Token semantics under the flat surface system:

      * `bgGlass` and `bgPanel` are fully opaque surfaces.
      * `bgHover` / `bgActive` are faint white overlays that tint a surface
        on interaction without changing its hue.
      * `accentSubtle` / `accentMuted` are tints of the accent used for
        selection/hover tints, NOT button-background colours.
      * `insetLightEdge` is transparent (no glass highlight).
      * Shadow/glow parameters are zero by default; paint code should treat
        them as optional.
*/

#include "ui/components/SurfaceStyle.h"

#include "ui/theme/ThemeManager.h"

namespace oscil
{

void SurfaceStyle::computeFrom(const ColorTheme& theme)
{
    // Opaque surfaces — no translucency. `glassAlpha` / `panelAlpha` are
    // retained in ColorTheme for serialization back-compat; ignored here.
    bgGlass = theme.backgroundPane;
    bgPanel = theme.backgroundPane;

    // Interaction tints: uniformly light overlay so hover/active look the
    // same on any hue. Alpha is deliberately modest.
    bgHover = juce::Colour(0xFFFFFFFF).withAlpha(0.06f);
    bgActive = juce::Colour(0xFFFFFFFF).withAlpha(0.10f);

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
    accentGlow = accent; // legacy alias; glow painter is a no-op now.

    // Flat aesthetic: no inset light edge.
    insetLightEdge = juce::Colours::transparentBlack;

    shadowIntensity = theme.shadowIntensity;
    shadowSpread = theme.shadowSpread;
    accentGlowRadius = theme.accentGlowRadius; // typically 0 on new themes
    accentGlowAlpha = theme.accentGlowAlpha;   // typically 0 on new themes
}

} // namespace oscil
