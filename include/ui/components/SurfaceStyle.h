/*
    Oscil - Surface Style
    Computed color/parameter pack derived from a ColorTheme. Used by the
    flat-surface paint system. Note: the struct retains some *Glass* field
    names (bgGlass, glassAlpha, etc.) as stable ValueTree serialization
    tokens — ColorTheme exports them and user themes on disk reference
    them. Production paint code treats these as opaque surface tokens.
*/

#pragma once

#include <juce_graphics/juce_graphics.h>

namespace oscil
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
    juce::Colour accentMuted;  // accent at 30% alpha

    float shadowIntensity = 0.4f;
    float shadowSpread = 12.0f;

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

} // namespace oscil
