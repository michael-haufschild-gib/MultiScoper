/*
    Oscil - Waveform OpenGL Renderer
    Implements juce::OpenGLRenderer for GPU-accelerated waveform rendering
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

#if OSCIL_ENABLE_OPENGL
    #include "core/dsp/TimingConfig.h"

    #include "rendering/VisualConfiguration.h"
    #include "rendering/WaveformShader.h"

    #include <juce_opengl/juce_opengl.h>

    #include <atomic>
    #include <memory>
    #include <unordered_map>
    #include <vector>

namespace oscil
{

// Forward declarations
class ShaderRegistry;
class RenderEngine;

/**
 * Grid colors copied from theme on message thread for thread-safe GL access.
 */
struct GridColors
{
    juce::Colour gridMinor{0x33FFFFFF};
    juce::Colour gridMajor{0x66FFFFFF};
    juce::Colour gridZeroLine{0x99FFFFFF};
};

/**
 * Data required to render a single waveform on the GL thread.
 * This struct is copied to ensure thread-safety between UI and GL threads.
 */
struct WaveformRenderData
{
    int id = 0;                       // Unique identifier
    juce::Rectangle<float> bounds;    // Screen position and size
    std::vector<float> channel1;      // First channel samples
    std::vector<float> channel2;      // Second channel (stereo only)
    juce::Colour colour{0xFF00FFFF};  // Waveform color (default, overwritten by oscillator)
    float opacity = 1.0f;             // Opacity (0-1)
    float lineWidth = 1.5f;           // Line thickness
    bool isStereo = false;            // Whether to render channel2
    bool visible = true;              // Whether to render this waveform
    float verticalScale = 1.0f;       // Vertical scale factor (includes auto-scale)
    VisualConfiguration visualConfig; // Full visual configuration for render engine
    GridConfiguration gridConfig;     // Grid configuration
    GridColors gridColors;            // Grid colors (copied from theme on message thread)
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
    /// Create an uninitialized waveform GL renderer.
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
     * When enabled, post-processing effects are available.
     * @param enabled Whether to use the render engine
     */
    void setRenderEngineEnabled(bool enabled) { useRenderEngine_ = enabled; }
    [[nodiscard]] bool isRenderEngineEnabled() const { return useRenderEngine_; }

    /**
     * Execute an operation on the RenderEngine safely.
     * The operation is executed under a lock to ensure thread safety between
     * the OpenGL thread (rendering) and the Message thread (configuration).
     *
     * @param func A function/lambda that takes RenderEngine& as argument.
     *             If the engine is not initialized, the function is not called.
     */
    template <typename Func>
    void performRenderEngineOperation(Func&& func)
    {
        const juce::ScopedReadLock lock(engineLock_);
        if (renderEngine_)
            func(*renderEngine_);
    }

private:
    void compileDebugShader();
    void renderDebugRect(const juce::Rectangle<float>& bounds, juce::Colour colour);
    std::vector<WaveformRenderData> collectWaveformsToRender();
    void renderWithEngine(const std::vector<WaveformRenderData>& waveformsToRender, float deltaTime);
    void setupDebugProjection(juce::OpenGLExtensionFunctions& ext, GLint projLoc, GLint colorLoc, float viewportWidth,
                              float viewportHeight, juce::Colour colour);

    juce::OpenGLContext* context_ = nullptr;
    std::atomic<bool> contextReady_{false};
    std::atomic<bool> cleanupPerformed_{false}; // Guard for idempotent cleanup

    // Thread-safe waveform data storage
    // Using SpinLock for fast synchronization (short critical sections)
    juce::SpinLock dataLock_;
    std::unordered_map<int, WaveformRenderData> waveforms_;

    // Render state
    juce::Colour backgroundColour_{juce::Colours::transparentBlack};
    bool shadersCompiled_ = false;

    // Debug rendering resources
    std::unique_ptr<juce::OpenGLShaderProgram> debugShader_;
    GLuint debugVAO_ = 0;
    GLuint debugVBO_ = 0;
    GLint debugProjectionLoc_ = -1;
    GLint debugColorLoc_ = -1;
    bool debugShaderCompiled_ = false;

    // Advanced render engine for post-processing effects
    // Protected by engineLock_ for thread-safe access/lifecycle management
    juce::ReadWriteLock engineLock_;
    std::unique_ptr<RenderEngine> renderEngine_;
    bool useRenderEngine_ = true; // Enable by default
    std::chrono::steady_clock::time_point lastFrameTime_;

    // Per-instance resize tracking (not static - each instance needs its own)
    int lastResizeWidth_ = 0;
    int lastResizeHeight_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformGLRenderer)
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
