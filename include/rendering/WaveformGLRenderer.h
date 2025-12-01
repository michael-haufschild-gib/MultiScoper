/*
    Oscil - Waveform OpenGL Renderer
    Implements juce::OpenGLRenderer for GPU-accelerated waveform rendering
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

#if OSCIL_ENABLE_OPENGL
#include <juce_opengl/juce_opengl.h>
#include "rendering/WaveformShader.h"
#include "rendering/VisualConfiguration.h"
#include <vector>
#include <unordered_map>
#include <atomic>
#include <memory>

namespace oscil
{

// Forward declarations
class ShaderRegistry;
class RenderEngine;

/**
 * Data required to render a single waveform on the GL thread.
 * This struct is copied to ensure thread-safety between UI and GL threads.
 */
struct WaveformRenderData
{
    int id = 0;                          // Unique identifier
    juce::Rectangle<float> bounds;       // Screen position and size
    std::vector<float> channel1;         // First channel samples
    std::vector<float> channel2;         // Second channel (stereo only)
    juce::Colour colour{ 0xFF00FF00 };   // Waveform color
    float opacity = 1.0f;                // Opacity (0-1)
    float lineWidth = 1.5f;              // Line thickness
    bool isStereo = false;               // Whether to render channel2
    juce::String shaderId = "basic";     // Shader to use (legacy, use visualConfig)
    bool visible = true;                 // Whether to render this waveform
    float verticalScale = 1.0f;          // Vertical scale factor (includes auto-scale)
    VisualConfiguration visualConfig;    // Full visual configuration for render engine
};

/**
 * OpenGL renderer for waveform visualizations.
 * Implements juce::OpenGLRenderer to handle GPU rendering on a background thread.
 *
 * Thread Safety:
 * - updateWaveform() is called from the message thread
 * - renderOpenGL() is called from the OpenGL thread
 * - Uses SpinLock for fast, non-blocking synchronization
 */
class WaveformGLRenderer : public juce::OpenGLRenderer
{
public:
    WaveformGLRenderer();
    ~WaveformGLRenderer() override;

    // juce::OpenGLRenderer interface
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    /**
     * Set the OpenGL context (must be called before rendering)
     */
    void setContext(juce::OpenGLContext* context);

    /**
     * Register a waveform for rendering
     * @param id Unique identifier for this waveform
     */
    void registerWaveform(int id);

    /**
     * Unregister a waveform
     * @param id The waveform identifier
     */
    void unregisterWaveform(int id);

    /**
     * Update waveform data for rendering (thread-safe)
     * Call this from the message thread when waveform data changes
     * @param data The render data to update
     */
    void updateWaveform(const WaveformRenderData& data);

    /**
     * Set the background color for clearing
     */
    void setBackgroundColour(juce::Colour colour);

    /**
     * Check if renderer is ready for GPU rendering
     */
    bool isReady() const { return contextReady_.load(); }

    /**
     * Get number of registered waveforms
     */
    int getWaveformCount() const;

    /**
     * Clear all registered waveforms (thread-safe)
     * Call this when refreshing panels to prevent stale waveform data from rendering
     */
    void clearAllWaveforms();

    /**
     * Enable or disable the advanced render engine.
     * When enabled, post-processing, particles, and 3D effects are available.
     * @param enabled Whether to use the render engine
     */
    void setRenderEngineEnabled(bool enabled) { useRenderEngine_ = enabled; }
    [[nodiscard]] bool isRenderEngineEnabled() const { return useRenderEngine_; }

    /**
     * Get the render engine (for configuration access).
     * @return Pointer to render engine, or nullptr if not initialized
     * 
     * CRITICAL THREAD SAFETY WARNING:
     * This returns a pointer to the engine which is primarily owned/used by the OpenGL thread.
     * Accessing this from the message thread while rendering is active is unsafe unless
     * strictly synchronized.
     * Ideally, use message-thread friendly configuration methods instead of accessing this directly.
     */
    RenderEngine* getRenderEngine() { return renderEngine_.get(); }

private:
    void compileShaders();
    void compileDebugShader();
    void renderWaveform(const WaveformRenderData& data);
    void renderDebugRect(const juce::Rectangle<float>& bounds, juce::Colour colour);

    juce::OpenGLContext* context_ = nullptr;
    std::atomic<bool> contextReady_{ false };

    // Thread-safe waveform data storage
    // Using SpinLock for fast synchronization (short critical sections)
    juce::SpinLock dataLock_;
    std::unordered_map<int, WaveformRenderData> waveforms_;

    // Render state
    juce::Colour backgroundColour_{ juce::Colours::transparentBlack };
    bool shadersCompiled_ = false;

    // Debug rendering resources
    std::unique_ptr<juce::OpenGLShaderProgram> debugShader_;
    GLuint debugVAO_ = 0;
    GLuint debugVBO_ = 0;
    GLint debugProjectionLoc_ = -1;
    GLint debugColorLoc_ = -1;
    bool debugShaderCompiled_ = false;

    // Advanced render engine for post-processing, particles, 3D
    std::unique_ptr<RenderEngine> renderEngine_;
    bool useRenderEngine_ = true;  // Enable by default
    std::chrono::steady_clock::time_point lastFrameTime_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformGLRenderer)
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
