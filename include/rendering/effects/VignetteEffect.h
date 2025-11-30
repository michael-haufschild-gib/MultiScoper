/*
    Oscil - Vignette Effect
    Darkens the edges of the image for cinematic look
*/

#pragma once

#include "PostProcessEffect.h"
#include "rendering/VisualConfiguration.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Vignette post-processing effect.
 * Darkens the edges of the image to draw focus to the center.
 */
class VignetteEffect : public PostProcessEffect
{
public:
    VignetteEffect();
    ~VignetteEffect() override;

    [[nodiscard]] juce::String getId() const override { return "vignette"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Vignette"; }

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
     * Configure the vignette effect.
     */
    void setSettings(const VignetteSettings& settings) { settings_ = settings; }
    [[nodiscard]] const VignetteSettings& getSettings() const { return settings_; }

private:
    VignetteSettings settings_;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    GLint textureLoc_ = -1;
    GLint intensityLoc_ = -1;
    GLint softnessLoc_ = -1;
    GLint colorLoc_ = -1;

    bool compiled_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
