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
 * Manages framebuffers, post-processing effects, particles,
 * 3D rendering, and per-waveform visual states.
 *
 * THREAD SAFETY:
 * This class is designed to be used EXCLUSIVELY from the OpenGL rendering thread.
 */
class RenderEngine : public IEffectProvider
{
public:
    RenderEngine();
    ~RenderEngine() override;

    // ========================================================================
    // Lifecycle
    // ========================================================================

    bool initialize(juce::OpenGLContext& context);
    void shutdown();
    void resize(int width, int height);
    [[nodiscard]] bool isInitialized() const { return initialized_; }

    // ========================================================================
    // Per-Frame Rendering
    // ========================================================================

    void beginFrame(float deltaTime);
    void renderWaveform(const WaveformRenderData& data);
    void endFrame();

    // ========================================================================
    // Waveform State Management
    // ========================================================================

    void registerWaveform(int waveformId);
    void unregisterWaveform(int waveformId);
    [[nodiscard]] std::optional<VisualConfiguration> getWaveformConfig(int waveformId);
    [[nodiscard]] bool hasWaveform(int waveformId);
    void setWaveformConfig(int waveformId, const VisualConfiguration& config);
    void clearAllWaveforms();
    void syncWaveforms(const std::unordered_set<int>& activeIds);

    // ========================================================================
    // Subsystem Access
    // ========================================================================

    ParticleSystem* getParticleSystem() { return waveformPass_->getParticleSystem(); }
    Camera3D* getCamera() { return waveformPass_->getCamera(); }
    FramebufferPool* getFramebufferPool() { return effectPipeline_->getFramebufferPool(); }
    EnvironmentMapManager* getEnvironmentMapManager() { return waveformPass_->getEnvironmentMapManager(); }
    TextureManager* getTextureManager() { return waveformPass_->getTextureManager(); }

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

    // Subsystems
    std::unique_ptr<RenderBootstrapper> bootstrapper_;
    std::unique_ptr<EffectPipeline> effectPipeline_;
    std::unique_ptr<WaveformPass> waveformPass_;
    RenderStats stats_;

    // Per-waveform states (protected by waveformStatesMutex_)
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
