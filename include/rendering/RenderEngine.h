#pragma once

#include "IEffectProvider.h"
#include "RenderCommon.h"
#include "VisualConfiguration.h"
#include "subsystems/EffectPipeline.h"
#include "subsystems/RenderBootstrapper.h"
#include "subsystems/RenderStats.h"
#include "subsystems/WaveformPass.h"

#include <juce_core/juce_core.h>

#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#if OSCIL_ENABLE_OPENGL
    #include <juce_opengl/juce_opengl.h>

namespace oscil
{

/**
 * Central orchestrator for all rendering subsystems.
 * Manages framebuffers, post-processing effects, and per-waveform visual states.
 *
 * THREAD SAFETY:
 * - Render methods (beginFrame, renderWaveform, endFrame, resize) must be called
 *   from the OpenGL thread under WaveformGLRenderer::engineLock_ WriteLock.
 * - Management methods (registerWaveform, unregisterWaveform, setWaveformConfig,
 *   syncWaveforms, clearAllWaveforms, getWaveformConfig, hasWaveform) may be called
 *   from the message thread via WaveformGLRenderer::performRenderEngineOperation,
 *   which acquires a ReadLock. These methods use waveformStatesMutex_ internally.
 * - The SpinLock protects against concurrent management calls, NOT concurrent
 *   render + management (which is prevented by the external ReadWriteLock).
 */
class RenderEngine : public IEffectProvider
{
public:
    /// Create an uninitialized render engine.
    RenderEngine();
    ~RenderEngine() override;

    // ========================================================================
    // Lifecycle
    // ========================================================================

    /// Initialize all subsystems (framebuffers, shaders, effects) for the given context.
    bool initialize(juce::OpenGLContext& context);
    /// Shut down all subsystems and release GPU resources.
    void shutdown();
    /// Resize all framebuffers to new dimensions.
    void resize(int width, int height);
    [[nodiscard]] bool isInitialized() const { return initialized_; }

    // ========================================================================
    // Per-Frame Rendering
    // ========================================================================

    /// Begin a new render frame, clearing buffers and updating timing.
    void beginFrame(float deltaTime);
    /// Render a single waveform with its visual configuration and effects.
    void renderWaveform(const WaveformRenderData& data);
    /// Finalize the frame and blit the result to the default framebuffer.
    void endFrame();

    // ========================================================================
    // Waveform State Management
    // ========================================================================

    /// Register a waveform ID and create its persistent render state.
    void registerWaveform(int waveformId);
    /// Unregister a waveform ID and release its render state.
    void unregisterWaveform(int waveformId);
    /// Get the visual configuration for a registered waveform.
    [[nodiscard]] std::optional<VisualConfiguration> getWaveformConfig(int waveformId);
    /// Check whether a waveform ID is registered.
    [[nodiscard]] bool hasWaveform(int waveformId);
    /// Set the visual configuration for a registered waveform.
    void setWaveformConfig(int waveformId, const VisualConfiguration& config);
    /// Remove all registered waveforms and their render states.
    void clearAllWaveforms();
    /// Synchronize registered waveforms with the active set, removing stale entries.
    void syncWaveforms(const std::unordered_set<int>& activeIds);

    // ========================================================================
    // Subsystem Access
    // ========================================================================

    FramebufferPool* getFramebufferPool() { return effectPipeline_->getFramebufferPool(); }

    // ========================================================================
    // Global Settings
    // ========================================================================

    void setQualityLevel(QualityLevel level);
    [[nodiscard]] QualityLevel getQualityLevel() const { return qualityLevel_; }

    void setBackgroundColour(juce::Colour colour) { backgroundColour_ = colour; }
    [[nodiscard]] juce::Colour getBackgroundColour() const { return backgroundColour_; }

    [[nodiscard]] float getDeltaTime() const { return stats_.getDeltaTime(); }
    [[nodiscard]] float getTime() const { return stats_.getTime(); }

    // ========================================================================
    // Effect Management
    // ========================================================================

    /// Get a post-processing effect by its identifier.
    PostProcessEffect* getEffect(const juce::String& effectId) override;

    void setGlobalPostProcessingEnabled(bool enabled);
    [[nodiscard]] bool isGlobalPostProcessingEnabled() const;

    // Prevent copying
    RenderEngine(const RenderEngine&) = delete;
    RenderEngine& operator=(const RenderEngine&) = delete;

private:
    // Internal rendering methods
    void blitToScreen();
    void executeComposite(Framebuffer* source, const VisualConfiguration& config);
    WaveformRenderState* resolveWaveformState(int waveformId);
    void renderWaveformLayer(const WaveformRenderData& data, WaveformRenderState& state);

    // Subsystems
    std::unique_ptr<RenderBootstrapper> bootstrapper_;
    std::unique_ptr<EffectPipeline> effectPipeline_;
    std::unique_ptr<WaveformPass> waveformPass_;
    RenderStats stats_;

    // Per-waveform states (protected by waveformStatesMutex_ for management methods;
    // render path accesses without this lock under the external WriteLock guarantee)
    mutable juce::SpinLock waveformStatesMutex_;
    std::unordered_map<int, WaveformRenderState> waveformStates_;

    // Context and state
    juce::OpenGLContext* context_ = nullptr;
    QualityLevel qualityLevel_ = QualityLevel::Normal;
    juce::Colour backgroundColour_{juce::Colours::transparentBlack};

    // Flags
    bool initialized_ = false;

    // Current frame state
    int currentWidth_ = 0;
    int currentHeight_ = 0;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
