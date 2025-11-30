/*
    Oscil - Radial Blur Effect
    Zoom-glow effect sampling from center at different distances
*/

#pragma once

#include "PostProcessEffect.h"
#include "rendering/VisualConfiguration.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Radial blur post-processing effect.
 * Creates a zoom-glow effect by sampling the image at multiple zoom levels
 * from the center, similar to the Electric Flower WebGL demo.
 */
class RadialBlurEffect : public PostProcessEffect
{
public:
    RadialBlurEffect();
    ~RadialBlurEffect() override;

    [[nodiscard]] juce::String getId() const override { return "radialBlur"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Radial Blur"; }

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
     * Configure the radial blur effect.
     */
    void setSettings(const RadialBlurSettings& settings) { settings_ = settings; }
    [[nodiscard]] const RadialBlurSettings& getSettings() const { return settings_; }

private:
    RadialBlurSettings settings_;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    GLint textureLoc_ = -1;
    GLint amountLoc_ = -1;
    GLint glowLoc_ = -1;
    GLint samplesLoc_ = -1;

    bool compiled_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
