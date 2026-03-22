/*
    Oscil - Waveform Render State
    Per-waveform persistent state for the render engine
*/

#pragma once

#include "Framebuffer.h"
#include "VisualConfiguration.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Persistent per-waveform state for the render engine.
 * Includes history FBOs for trails and visual configuration.
 */
struct WaveformRenderState
{
    int waveformId = 0;

    // Trails (stateful post-processing)
    bool trailsEnabled = false;
    std::unique_ptr<Framebuffer> historyFBO;

    // Full visual configuration
    VisualConfiguration visualConfig;

    // Frame timing for effects
    float lastFrameTime = 0.0f;
    float accumulatedTime = 0.0f;

    /// Allocate a history framebuffer for trail persistence.
    void enableTrails(juce::OpenGLContext& context, int width, int height);
    /// Release the history framebuffer and disable trails.
    void disableTrails(juce::OpenGLContext& context);
    /// Resize the history framebuffer to match new dimensions.
    void resizeHistoryFBO(juce::OpenGLContext& context, int width, int height);

    /// Advance accumulated time by the given delta.
    void updateTiming(float deltaTime);
    /// Release all GPU resources owned by this state.
    void release(juce::OpenGLContext& context);

    [[nodiscard]] bool needsHistoryFBO() const
    {
        return visualConfig.trails.enabled;
    }
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
