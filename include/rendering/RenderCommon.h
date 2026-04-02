/*
    Oscil - Render Common
    Shared definitions for the rendering subsystem.
*/

#pragma once

#include <cstdint>

namespace oscil
{

/**
 * Quality level for post-processing effects.
 * Controls which GPU effects (bloom, blur, etc.) are active.
 *
 * Related but distinct from:
 * - QualityMode (PerformanceConfig.h): controls rendering FPS and resolution scaling
 * - QualityPreset (QualityPreset.h): controls audio capture sample rate
 */
enum class QualityLevel : std::uint8_t
{
    Eco,    // Performance priority - minimal effects
    Normal, // Balanced - most 2D effects, limited particles
    Ultra   // Visual priority - all effects including 3D
};

} // namespace oscil
