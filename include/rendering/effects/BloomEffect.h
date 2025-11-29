/*
    Oscil - Bloom Effect
    Multi-pass bloom/glow effect for bright areas
*/

#pragma once

#include "PostProcessEffect.h"
#include "rendering/VisualConfiguration.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Bloom post-processing effect.
 * Creates a glow effect around bright areas using multi-pass Gaussian blur.
 */
class BloomEffect : public PostProcessEffect
{
public:
    BloomEffect();
    ~BloomEffect() override;

    [[nodiscard]] juce::String getId() const override { return "bloom"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Bloom"; }

    bool compile(juce::OpenGLContext& context) override;
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
     * Configure the bloom effect.
     */
    void setSettings(const BloomSettings& settings) { settings_ = settings; }
    [[nodiscard]] const BloomSettings& getSettings() const { return settings_; }

private:
    BloomSettings settings_;

    // Threshold extraction shader
    std::unique_ptr<juce::OpenGLShaderProgram> thresholdShader_;
    GLint thresholdTextureLoc_ = -1;
    GLint thresholdValueLoc_ = -1;

    // Gaussian blur shader (used for both horizontal and vertical passes)
    std::unique_ptr<juce::OpenGLShaderProgram> blurShader_;
    GLint blurTextureLoc_ = -1;
    GLint blurDirectionLoc_ = -1;
    GLint blurResolutionLoc_ = -1;

    // Combine shader (adds bloom to original)
    std::unique_ptr<juce::OpenGLShaderProgram> combineShader_;
    GLint combineOriginalLoc_ = -1;
    GLint combineBloomLoc_ = -1;
    GLint combineIntensityLoc_ = -1;

    // Internal framebuffers for blur passes
    std::unique_ptr<Framebuffer> brightFBO_;
    std::unique_ptr<Framebuffer> blurTempFBO_;

    bool compiled_ = false;
    int lastWidth_ = 0;
    int lastHeight_ = 0;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
