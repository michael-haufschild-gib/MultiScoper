/*
    Oscil - Waveform Render State
    Per-waveform persistent state for the render engine
*/

#pragma once

#include "Framebuffer.h"
#include "VisualConfiguration.h"
#include <deque>
#include <memory>
#include <vector>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

// Forward declaration
using ParticleEmitterId = int;

/**
 * Persistent per-waveform state for the render engine.
 * Includes history FBOs for trails, particle emitter IDs, and visual configuration.
 */
struct WaveformRenderState
{
    int waveformId = 0;

    // Trails (stateful post-processing)
    bool trailsEnabled = false;
    std::unique_ptr<Framebuffer> historyFBO;

    // Particle emitters for this waveform
    std::vector<ParticleEmitterId> emitterIds;

    // Full visual configuration
    VisualConfiguration visualConfig;

    // Sample history for 3D temporal effects (waterfall displays)
    std::deque<std::vector<float>> sampleHistory;
    int maxHistoryDepth = 64;

    // Frame timing for effects
    float lastFrameTime = 0.0f;
    float accumulatedTime = 0.0f;

    /**
     * Enable trails effect and create history FBO.
     * @param context The OpenGL context
     * @param width FBO width
     * @param height FBO height
     */
    void enableTrails(juce::OpenGLContext& context, int width, int height);

    /**
     * Disable trails effect and destroy history FBO.
     * @param context The OpenGL context
     */
    void disableTrails(juce::OpenGLContext& context);

    /**
     * Resize history FBO if needed.
     * @param context The OpenGL context
     * @param width New width
     * @param height New height
     */
    void resizeHistoryFBO(juce::OpenGLContext& context, int width, int height);

    /**
     * Add a particle emitter to this waveform.
     * @param id The emitter ID
     */
    void addParticleEmitter(ParticleEmitterId id);

    /**
     * Remove a particle emitter from this waveform.
     * @param id The emitter ID
     */
    void removeParticleEmitter(ParticleEmitterId id);

    /**
     * Clear all particle emitters.
     */
    void clearParticleEmitters();

    /**
     * Push new samples to the history buffer.
     * Used for 3D temporal effects like waterfall displays.
     * @param samples The new sample data
     */
    void pushSamples(const std::vector<float>& samples);

    /**
     * Get the sample history as a 2D vector.
     * Newest samples are at the front (index 0).
     */
    [[nodiscard]] const std::deque<std::vector<float>>& getSampleHistory() const
    {
        return sampleHistory;
    }

    /**
     * Update frame timing.
     * @param deltaTime Time since last frame in seconds
     */
    void updateTiming(float deltaTime);

    /**
     * Release all GPU resources.
     * @param context The OpenGL context
     */
    void release(juce::OpenGLContext& context);

    /**
     * Check if this state needs the history FBO based on current config.
     */
    [[nodiscard]] bool needsHistoryFBO() const
    {
        return visualConfig.trails.enabled;
    }

    /**
     * Check if this state needs sample history for 3D effects.
     */
    [[nodiscard]] bool needsSampleHistory() const
    {
        return visualConfig.requires3D() &&
               (visualConfig.shaderType == ShaderType::WireframeMesh ||
                visualConfig.shaderType == ShaderType::VectorFlow);
    }

    /**
     * Set the maximum history depth for 3D effects.
     * @param depth Number of sample buffers to keep
     */
    void setMaxHistoryDepth(int depth)
    {
        maxHistoryDepth = depth;
        while (static_cast<int>(sampleHistory.size()) > maxHistoryDepth)
            sampleHistory.pop_back();
    }
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
