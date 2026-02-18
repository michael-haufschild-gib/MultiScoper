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
     * Configure the bloom effect from VisualConfiguration.
     */
    void configure(const VisualConfiguration& config) override { settings_ = config.bloom; }

    /**
     * Configure the bloom effect directly.
     * Parameters are clamped to valid ranges to prevent undefined behavior.
     */
    void setSettings(const BloomSettings& settings)
    {
        settings_ = settings;
        // Validate and clamp parameters to safe ranges
        settings_.intensity = juce::jlimit(0.0f, 2.0f, settings_.intensity);
        settings_.threshold = juce::jlimit(0.0f, 1.0f, settings_.threshold);
        settings_.iterations = juce::jlimit(2, 8, settings_.iterations);
        settings_.downsampleSteps = juce::jlimit(1, 6, settings_.downsampleSteps);
        settings_.spread = juce::jlimit(0.1f, 5.0f, settings_.spread);
        settings_.softKnee = juce::jlimit(0.0f, 1.0f, settings_.softKnee);
    }
    [[nodiscard]] const BloomSettings& getSettings() const { return settings_; }

private:
    BloomSettings settings_;

    // Shaders
    std::unique_ptr<juce::OpenGLShaderProgram> prefilterShader_;
    GLint prefilterThreshLoc_ = -1;
    GLint prefilterSoftKneeLoc_ = -1;
    GLint prefilterResLoc_ = -1;
    
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
    static const int kMaxMipLevels = 6;

    bool compiled_ = false;
    int lastWidth_ = 0;
    int lastHeight_ = 0;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
