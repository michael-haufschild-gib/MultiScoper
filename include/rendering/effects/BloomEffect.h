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

    // Shaders
    std::unique_ptr<juce::OpenGLShaderProgram> prefilterShader_;
    GLint prefilterThreshLoc_ = -1;
    GLint prefilterSoftKneeLoc_ = -1;
    
    std::unique_ptr<juce::OpenGLShaderProgram> downsampleShader_;
    GLint downsampleResLoc_ = -1;

    std::unique_ptr<juce::OpenGLShaderProgram> upsampleShader_;
    GLint upsampleFilterRadiusLoc_ = -1;
    GLint upsampleTexelSizeLoc_ = -1;

    std::unique_ptr<juce::OpenGLShaderProgram> combineShader_;
    GLint combineOriginalLoc_ = -1;
    GLint combineBloomLoc_ = -1;
    GLint combineIntensityLoc_ = -1;

    // Internal framebuffers for mip chain (pyramid)
    // mipChain_[0] is half res, [1] is quarter res, etc.
    std::vector<std::unique_ptr<Framebuffer>> mipChain_;
    static const int kMaxMipLevels = 5;

    bool compiled_ = false;
    int lastWidth_ = 0;
    int lastHeight_ = 0;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
