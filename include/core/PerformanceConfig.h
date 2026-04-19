/*
    MultiScoper - Rendering Mode and Performance Metrics
    Types used by the rendering pipeline and status bar
*/

#pragma once

#include <juce_core/juce_core.h>

#include <cstdint>

namespace multiscoper
{

/**
 * Rendering mode (OpenGL or Software)
 */
enum class RenderingMode : std::uint8_t
{
    OPENGL,  // GPU-accelerated rendering
    SOFTWARE // CPU-based fallback rendering
};

inline juce::String renderingModeToString(RenderingMode mode)
{
    return mode == RenderingMode::OPENGL ? "OPENGL" : "SOFTWARE";
}

inline RenderingMode stringToRenderingMode(const juce::String& str)
{
    return str == "OPENGL" ? RenderingMode::OPENGL : RenderingMode::SOFTWARE;
}

/**
 * Real-time performance metrics
 * PRD aligned: Entities -> StatusBar (metrics)
 */
struct PerformanceMetrics
{
    float cpuUsage = 0.0f;    // 0.0-100.0%
    float memoryUsage = 0.0f; // Current memory usage in MB
    float fps = 0.0f;         // Current frames per second
    RenderingMode renderingMode = RenderingMode::SOFTWARE;

    // Extended metrics
    float audioThreadLoad = 0.0f; // Audio thread CPU usage
    float uiThreadLoad = 0.0f;    // UI thread CPU usage
    int activeOscillators = 0;    // Number of active oscillators
    int activeSources = 0;        // Number of active sources
};

} // namespace multiscoper
