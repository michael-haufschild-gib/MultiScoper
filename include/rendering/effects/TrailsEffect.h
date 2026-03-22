/*
    Oscil - Trails Effect
    Creates persistence/motion blur effect using history buffer
*/

#pragma once

#include "PostProcessEffect.h"
#include "rendering/VisualConfiguration.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Trails post-processing effect.
 * Blends current frame with history to create persistence/trails.
 * Requires a history FBO from WaveformRenderState.
 */
class TrailsEffect : public PostProcessEffect
{
public:
    /// Create a trails effect with default settings.
    TrailsEffect();
    ~TrailsEffect() override;

    [[nodiscard]] juce::String getId() const override { return "trails"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Trails"; }

    /// Compile the trails blending shader program.
    bool compile(juce::OpenGLContext& context) override;
    /// Release the shader program.
    void release(juce::OpenGLContext& context) override;
    [[nodiscard]] bool isCompiled() const override;

    void apply(
        juce::OpenGLContext& context,
        Framebuffer* source,
        Framebuffer* destination,
        FramebufferPool& pool,
        float deltaTime
    ) override;

    /**
     * Apply trails with external history buffer.
     * Used by RenderEngine which manages history FBOs per-waveform.
     */
    void applyWithHistory(
        juce::OpenGLContext& context,
        Framebuffer* source,
        Framebuffer* history,
        Framebuffer* destination,
        FramebufferPool& pool,
        float deltaTime
    );

    /**
     * Configure the trails effect.
     */
    void setSettings(const TrailSettings& settings) { settings_ = settings; }
    [[nodiscard]] const TrailSettings& getSettings() const { return settings_; }

private:
    TrailSettings settings_;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    GLint currentTextureLoc_ = -1;
    GLint historyTextureLoc_ = -1;
    GLint decayLoc_ = -1;
    GLint opacityLoc_ = -1;

    bool compiled_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
