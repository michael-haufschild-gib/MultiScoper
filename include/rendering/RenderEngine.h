/*
    Oscil - Render Engine
    Central orchestrator for advanced rendering with post-processing,
    particles, 3D visualization, and material shaders
*/

#pragma once

#include "Framebuffer.h"
#include "FramebufferPool.h"
#include "WaveformRenderState.h"
#include "VisualConfiguration.h"
#include "WaveformGLRenderer.h"
#include "shaders3d/WaveformShader3D.h" // For LightingConfig
#include <juce_core/juce_core.h>
#include <unordered_map>
#include <memory>

#if OSCIL_ENABLE_OPENGL
#include <juce_opengl/juce_opengl.h>

namespace oscil
{

// Forward declarations
class PostProcessEffect;
class ParticleSystem;
class EnvironmentMapManager;
// Camera3D is included via WaveformShader3D.h

/**
 * Quality level presets for performance/quality trade-offs.
 */
enum class QualityLevel
{
    Eco,      // Performance priority - minimal effects
    Normal,   // Balanced - most 2D effects, limited particles
    Ultra     // Visual priority - all effects including 3D
};

/**
 * Central orchestrator for all rendering subsystems.
 * Manages framebuffers, post-processing effects, particles,
 * 3D rendering, and per-waveform visual states.
 */
class RenderEngine
{
public:
    RenderEngine();
    ~RenderEngine();

    // ========================================================================
    // Lifecycle
    // ========================================================================

    /**
     * Initialize the render engine.
     * Creates all framebuffers and compiles effect shaders.
     * @param context The OpenGL context
     * @return true if initialization succeeded
     */
    bool initialize(juce::OpenGLContext& context);

    /**
     * Shutdown the render engine and release all resources.
     * Must be called while OpenGL context is active.
     */
    void shutdown();

    /**
     * Resize all framebuffers to new dimensions.
     * @param width New width
     * @param height New height
     */
    void resize(int width, int height);

    /**
     * Check if the engine is initialized and ready.
     */
    [[nodiscard]] bool isInitialized() const { return initialized_; }

    // ========================================================================
    // Per-Frame Rendering
    // ========================================================================

    /**
     * Begin a new frame. Clears the scene FBO.
     * @param deltaTime Time since last frame in seconds
     */
    void beginFrame(float deltaTime);

    /**
     * Render a single waveform with full pipeline.
     * Includes geometry, particles, post-processing, and compositing.
     * @param data The waveform render data
     */
    void renderWaveform(const WaveformRenderData& data);

    /**
     * End the frame and blit to screen.
     * Applies global effects and presents to the default framebuffer.
     */
    void endFrame();

    // ========================================================================
    // Waveform State Management
    // ========================================================================

    /**
     * Register a waveform with the render engine.
     * Creates persistent state for the waveform.
     * @param waveformId Unique identifier
     */
    void registerWaveform(int waveformId);

    /**
     * Unregister a waveform and clean up its state.
     * @param waveformId The waveform to unregister
     */
    void unregisterWaveform(int waveformId);

    /**
     * Get the visual configuration for a waveform.
     * @param waveformId The waveform identifier
     * @return Pointer to config, or nullptr if not found
     */
    VisualConfiguration* getWaveformConfig(int waveformId);

    /**
     * Update the visual configuration for a waveform.
     * @param waveformId The waveform identifier
     * @param config The new configuration
     */
    void setWaveformConfig(int waveformId, const VisualConfiguration& config);

    /**
     * Clear all registered waveforms.
     */
    void clearAllWaveforms();

    // ========================================================================
    // Subsystem Access
    // ========================================================================

    /**
     * Get the particle system.
     */
    ParticleSystem* getParticleSystem() { return particleSystem_.get(); }

    /**
     * Get the 3D camera.
     */
    Camera3D* getCamera() { return camera_.get(); }

    /**
     * Get the framebuffer pool.
     */
    FramebufferPool* getFramebufferPool() { return fbPool_.get(); }

    /**
     * Get the environment map manager.
     */
    EnvironmentMapManager* getEnvironmentMapManager() { return envMapManager_.get(); }

    // ========================================================================
    // Global Settings
    // ========================================================================

    /**
     * Set the quality level preset.
     * Affects available effects and particle limits.
     * @param level The quality level
     */
    void setQualityLevel(QualityLevel level);
    [[nodiscard]] QualityLevel getQualityLevel() const { return qualityLevel_; }

    /**
     * Set the background color for clearing.
     */
    void setBackgroundColour(juce::Colour colour) { backgroundColour_ = colour; }
    [[nodiscard]] juce::Colour getBackgroundColour() const { return backgroundColour_; }

    /**
     * Get current frame delta time.
     */
    [[nodiscard]] float getDeltaTime() const { return deltaTime_; }

    /**
     * Get accumulated time since engine start.
     */
    [[nodiscard]] float getTime() const { return accumulatedTime_; }

    // ========================================================================
    // Effect Management
    // ========================================================================

    /**
     * Get a post-process effect by ID.
     */
    PostProcessEffect* getEffect(const juce::String& effectId);

    /**
     * Enable or disable global post-processing.
     */
    void setGlobalPostProcessingEnabled(bool enabled) { globalPostProcessEnabled_ = enabled; }
    [[nodiscard]] bool isGlobalPostProcessingEnabled() const { return globalPostProcessEnabled_; }

    // Prevent copying
    RenderEngine(const RenderEngine&) = delete;
    RenderEngine& operator=(const RenderEngine&) = delete;

private:
    // Internal rendering methods
    void renderWaveformComplete(const WaveformRenderData& data, WaveformRenderState& state);
    void renderWaveformGeometry(const WaveformRenderData& data, const VisualConfiguration& config);
    void renderWaveformParticles(const WaveformRenderData& data, WaveformRenderState& state);
    Framebuffer* applyPostProcessing(Framebuffer* source, WaveformRenderState& state);
    void compositeToScene(Framebuffer* source, const VisualConfiguration& config);
    void applyGlobalEffects();
    void blitToScreen();

    // Effect initialization
    void initializeEffects();
    void releaseEffects();

    // Environment map initialization
    void createDefaultEnvironmentMaps();

    // Setup helpers
    void setupCamera2D();
    void setupCamera3D(const Settings3D& settings);

    // Framebuffer management
    std::unique_ptr<FramebufferPool> fbPool_;
    std::unique_ptr<Framebuffer> sceneFBO_;

    // Per-waveform states
    std::unordered_map<int, WaveformRenderState> waveformStates_;

    // Subsystems
    std::unique_ptr<ParticleSystem> particleSystem_;
    std::unique_ptr<Camera3D> camera_;
    std::unique_ptr<EnvironmentMapManager> envMapManager_;

    // Post-processing effects
    std::unordered_map<juce::String, std::unique_ptr<PostProcessEffect>> effects_;

    // Blit shader for final output
    std::unique_ptr<juce::OpenGLShaderProgram> blitShader_;
    GLint blitTextureLoc_ = -1;

    // Context and state
    juce::OpenGLContext* context_ = nullptr;
    QualityLevel qualityLevel_ = QualityLevel::Normal;
    juce::Colour backgroundColour_{0xFF1A1A1A};

    // Timing
    float deltaTime_ = 0.0f;
    float accumulatedTime_ = 0.0f;

    // Lighting
    LightingConfig lightingConfig_;

    // Flags
    bool initialized_ = false;
    bool globalPostProcessEnabled_ = true;

    // Current frame state
    int currentWidth_ = 0;
    int currentHeight_ = 0;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
