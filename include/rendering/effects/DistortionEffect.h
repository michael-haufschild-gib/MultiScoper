/*
    Oscil - Distortion Effect
    Wave/ripple distortion effect
*/

#pragma once

#include "PostProcessEffect.h"
#include "rendering/VisualConfiguration.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Distortion post-processing effect.
 * Creates animated wave/ripple distortion.
 */
class DistortionEffect : public PostProcessEffect
{
public:
    DistortionEffect();
    ~DistortionEffect() override;

    [[nodiscard]] juce::String getId() const override { return "distortion"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Distortion"; }

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

    void setSettings(const DistortionSettings& settings) { settings_ = settings; }
    [[nodiscard]] const DistortionSettings& getSettings() const { return settings_; }

private:
    DistortionSettings settings_;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    GLint textureLoc_ = -1;
    GLint intensityLoc_ = -1;
    GLint frequencyLoc_ = -1;
    GLint timeLoc_ = -1;

    float accumulatedTime_ = 0.0f;
    bool compiled_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
