/*
    Oscil - Lens Flare Effect
    Generates dynamic ghost and halo lens flares from bright image content.
*/

#pragma once

#include "PostProcessEffect.h"
#include "rendering/VisualConfiguration.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Lens Flare post-processing effect.
 * Simulates lens artifacts (ghosts, halos) caused by bright light sources.
 */
class LensFlareEffect : public PostProcessEffect
{
public:
    LensFlareEffect();
    ~LensFlareEffect() override;

    [[nodiscard]] juce::String getId() const override { return "lens_flare"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Lens Flare"; }

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
     * Configure the lens flare effect.
     */
    void setSettings(const LensFlareSettings& settings) { settings_ = settings; }
    [[nodiscard]] const LensFlareSettings& getSettings() const { return settings_; }

private:
    LensFlareSettings settings_;

    // Shader
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;
    GLint textureLoc_ = -1;
    GLint intensityLoc_ = -1;
    GLint thresholdLoc_ = -1;
    GLint ghostDispersalLoc_ = -1;
    GLint ghostCountLoc_ = -1;
    GLint haloWidthLoc_ = -1;
    GLint chromaticDistortionLoc_ = -1;

    bool compiled_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
