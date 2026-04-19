/*
    MultiScoper - Surface Style
    Computed color/parameter pack derived from a ColorTheme. Used by the
    flat-surface paint system. Note: the struct retains some *Glass* field
    names (bgGlass, glassAlpha, etc.) as stable ValueTree serialization
    tokens — ColorTheme exports them and user themes on disk reference
    them. Production paint code treats these as opaque surface tokens.
*/

#pragma once

#include <juce_graphics/juce_graphics.h>

namespace multiscoper
{

struct ColorTheme; // forward declare

struct SurfaceStyle
{
    juce::Colour bgGlass;
    juce::Colour bgPanel;
    juce::Colour bgHover;
    juce::Colour bgActive;

    juce::Colour borderSubtle;
    juce::Colour borderDefault;
    juce::Colour borderStrong;

    juce::Colour accent;
    juce::Colour accentSubtle; // accent at 15% alpha
    juce::Colour accentMuted;  // accent at 35% alpha

    // Neutral baseline: no shadow until computeFrom() inherits the theme's
    // flat-surface values (ColorTheme defaults 0.18 / 4.0). Starting above
    // that baseline can produce a heavy drop shadow on any default-constructed
    // SurfaceStyle before the theme is applied.
    float shadowIntensity = 0.0f;
    float shadowSpread = 0.0f;

    /// Recompute all derived values from a ColorTheme
    void computeFrom(const ColorTheme& theme);

    /// Construct a SurfaceStyle from a theme in one call
    static SurfaceStyle fromTheme(const ColorTheme& theme)
    {
        SurfaceStyle surface;
        surface.computeFrom(theme);
        return surface;
    }
};

} // namespace multiscoper
