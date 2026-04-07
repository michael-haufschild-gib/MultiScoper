/*
    Oscil - Glass Style
    Computed color/parameter struct derived from ColorTheme glass fields
*/

#pragma once

#include <juce_graphics/juce_graphics.h>

namespace oscil
{

struct ColorTheme; // forward declare

struct GlassStyle
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
    juce::Colour accentGlow;   // same as accent (for glow painting)

    juce::Colour insetLightEdge; // white at lightEdgeAlpha

    float shadowIntensity = 0.4f;
    float shadowSpread = 12.0f;
    float accentGlowRadius = 12.0f;
    float accentGlowAlpha = 0.3f;

    /// Recompute all derived values from a ColorTheme
    void computeFrom(const ColorTheme& theme);

    /// Construct a GlassStyle from a theme in one call
    static GlassStyle fromTheme(const ColorTheme& theme)
    {
        GlassStyle glass;
        glass.computeFrom(theme);
        return glass;
    }
};

} // namespace oscil
