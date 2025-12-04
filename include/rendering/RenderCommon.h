/*
    Oscil - Render Common
    Shared definitions for the rendering subsystem.
*/

#pragma once

namespace oscil
{

/**
 * Quality level presets for performance/quality trade-offs.
 */
enum class QualityLevel
{
    Eco,    // Performance priority - minimal effects
    Normal, // Balanced - most 2D effects, limited particles
    Ultra   // Visual priority - all effects including 3D
};

} // namespace oscil
