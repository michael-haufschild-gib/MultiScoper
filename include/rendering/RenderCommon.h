/*
    Oscil - Render Common
    Shared definitions for the rendering subsystem.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace oscil
{

/**
 * Quality level presets for performance/quality trade-offs.
 */
enum class QualityLevel
{
    Eco,    // Performance priority - minimal effects
    Normal, // Balanced - most 2D effects
    Ultra   // Visual priority - all effects including 3D
};

/**
 * Convert sRGB color to linear color space.
 * Use this on CPU to avoid per-fragment pow() calls in shaders.
 * 
 * @param srgb The color in sRGB space
 * @return The color in linear space
 */
inline juce::Colour sRGBToLinear(juce::Colour srgb)
{
    auto toLinear = [](float c) { return std::pow(c, 2.2f); };
    return juce::Colour::fromFloatRGBA(
        toLinear(srgb.getFloatRed()),
        toLinear(srgb.getFloatGreen()),
        toLinear(srgb.getFloatBlue()),
        srgb.getFloatAlpha());
}

/**
 * Convert linear color to sRGB color space.
 * @param linear The color in linear space
 * @return The color in sRGB space
 */
inline juce::Colour linearToSRGB(juce::Colour linear)
{
    auto toSRGB = [](float c) { return std::pow(c, 1.0f / 2.2f); };
    return juce::Colour::fromFloatRGBA(
        toSRGB(linear.getFloatRed()),
        toSRGB(linear.getFloatGreen()),
        toSRGB(linear.getFloatBlue()),
        linear.getFloatAlpha());
}

} // namespace oscil
