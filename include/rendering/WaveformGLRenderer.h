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
#include "core/dsp/TimingConfig.h"
#include "core/dsp/SignalProcessor.h"  // For LODTier enum
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
 * Grid colors copied from theme on message thread for thread-safe GL access.
 */
struct GridColors
{
    juce::Colour gridMinor{ 0x33FFFFFF };
    juce::Colour gridMajor{ 0x66FFFFFF };
    juce::Colour gridZeroLine{ 0x99FFFFFF };
};

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
    juce::Colour colour{ 0xFF00FFFF };   // Waveform color (default, overwritten by oscillator)
    float opacity = 1.0f;                // Opacity (0-1)
    float lineWidth = 1.5f;              // Line thickness
    bool isStereo = false;               // Whether to render channel2
    bool visible = true;                 // Whether to render this waveform
    float verticalScale = 1.0f;          // Vertical scale factor (includes auto-scale)
    VisualConfiguration visualConfig;    // Full visual configuration for render engine
    GridConfiguration gridConfig;        // Grid configuration
    GridColors gridColors;               // Grid colors (copied from theme on message thread)
    
    // LOD (Level-of-Detail) metadata for adaptive rendering
    LODTier lodTier = LODTier::Full;     // Current LOD quality tier
    bool hasMinMaxEnvelope = false;      // True if min/max envelopes are available
    
    // Min/max envelopes for peak-accurate rendering at low LOD
    // When hasMinMaxEnvelope is true, shaders can use these to draw accurate peaks
    // even when the main sample arrays are heavily decimated
    std::vector<float> minEnvelope1;     // Min values for channel1
    std::vector<float> maxEnvelope1;     // Max values for channel1  
    std::vector<float> minEnvelope2;     // Min values for channel2 (stereo)
    std::vector<float> maxEnvelope2;     // Max values for channel2 (stereo)
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
     * @param data The render data to update (copy version)
     */
    void updateWaveform(const WaveformRenderData& data);

    /**
     * Update waveform data for rendering (thread-safe, move version)
     * Prefer this when the caller doesn't need the data after the call
     * @param data The render data to update (moved)
     */
    void updateWaveform(WaveformRenderData&& data);

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
     * When enabled, post-processing and 3D effects are available.
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

    using FrameCaptureCallback = std::function<void(juce::Image)>;

    /**
     * Request a capture of the current frame.
     * The callback will be executed on the Message Thread with the captured image.
     * @param callback Function to call with the captured image.
     */
    void requestFrameCapture(FrameCaptureCallback callback);

    /**
     * Check if context recovery is pending.
     * Returns true if the renderer detected a context loss and is waiting to reinitialize.
     */
    [[nodiscard]] bool isRecoveryPending() const { return needsReinitialization_.load(std::memory_order_acquire); }

    /**
     * Force a reinitialization of GPU resources on the next frame.
     * Useful when the host signals graphics changes.
     */
    void requestReinitialization() { needsReinitialization_.store(true, std::memory_order_release); }

private:
    /**
     * Handle detected context loss.
     * Marks resources as invalid and schedules reinitialization.
     */
    void handleContextLoss();

    /**
     * Attempt to reinitialize GPU resources after context loss.
     * @return true if reinitialization succeeded
     */
    bool attemptReinitialization();
    void compileDebugShader();
    void renderDebugRect(const juce::Rectangle<float>& bounds, juce::Colour colour);
    void processPendingCaptures(int width, int height);

    std::atomic<juce::OpenGLContext*> context_{ nullptr };
    juce::OpenGLContext* lastKnownContext_ = nullptr;  // For context change detection
    std::atomic<bool> contextReady_{ false };
    std::atomic<bool> cleanupPerformed_{ false };  // Guard for idempotent cleanup
    std::atomic<bool> needsReinitialization_{ false };  // Context recovery flag
    std::atomic<int> consecutiveRecoveryFailures_{ 0 };  // Track failed recovery attempts

    // Thread-safe waveform data storage using double-buffering
    // Message thread writes to waveformsWrite_, GL thread reads from waveformsRead_
    // Swap occurs atomically at the start of each frame to avoid deep copies
    juce::SpinLock dataLock_;
    std::unordered_map<int, WaveformRenderData> waveformsWrite_;  // Message thread writes
    std::unordered_map<int, WaveformRenderData> waveformsRead_;   // GL thread reads
    std::atomic<bool> swapPending_{false};

    // C12 FIX: Atomic flag for thread-safe deferred clear
    // Set by UI thread, consumed by render thread at start of frame
    std::atomic<bool> clearRequested_{false};

    // Frame capture
    juce::SpinLock captureLock_;
    std::vector<FrameCaptureCallback> pendingCaptures_;

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

    // Advanced render engine for post-processing and 3D effects
    // Protected by engineLock_ for thread-safe access/lifecycle management
    juce::ReadWriteLock engineLock_;
    std::unique_ptr<RenderEngine> renderEngine_;
    bool useRenderEngine_ = true;  // Enable by default
    std::chrono::steady_clock::time_point lastFrameTime_;

    // Per-instance resize tracking (not static - each instance needs its own)
    int lastResizeWidth_ = 0;
    int lastResizeHeight_ = 0;

    // Per-instance debug counters (not static - prevents data races with multiple instances)
    int debugFrameCounter_ = 0;
    int debugEntryLogCounter_ = 0;

    // Pooled capture buffer to avoid per-frame allocation
    std::vector<uint8_t> captureBuffer_;

    // Reused vector for waveforms to render (avoids per-frame allocation)
    std::vector<const WaveformRenderData*> waveformsToRender_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformGLRenderer)
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
