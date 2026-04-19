/*
    MultiScoper - Vignette Effect
    Darkens the edges of the image for cinematic look
*/

#pragma once

#include "SingleShaderEffect.h"
#include "rendering/VisualConfiguration.h"

#if MULTISCOPER_ENABLE_OPENGL

namespace multiscoper
{

/**
 * Vignette post-processing effect.
 * Darkens the edges of the image to draw focus to the center.
 */
class VignetteEffect : public SingleShaderEffect
{
public:
    /// Create a vignette effect with default settings.
    VignetteEffect();
    ~VignetteEffect() override;

    [[nodiscard]] juce::String getId() const override { return "vignette"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Vignette"; }

    /**
     * Configure from VisualConfiguration.
     */
    void configure(const VisualConfiguration& config) override { settings_ = config.vignette; }

    /**
     * Configure the vignette effect.
     */
    void setSettings(const VignetteSettings& settings) { settings_ = settings; }
    [[nodiscard]] const VignetteSettings& getSettings() const { return settings_; }

protected:
    [[nodiscard]] const char* getFragmentSource() const override;
    bool resolveUniforms() override;
    void setUniforms(const Framebuffer& source, float deltaTime) override;

private:
    VignetteSettings settings_;

    GLint intensityLoc_ = -1;
    GLint softnessLoc_ = -1;
    GLint colorLoc_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VignetteEffect)
};

} // namespace multiscoper

#endif // MULTISCOPER_ENABLE_OPENGL
