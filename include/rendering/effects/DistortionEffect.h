/*
    Oscil - Distortion Effect
    Wave/ripple distortion effect
*/

#pragma once

#include "PostProcessEffect.h"
#include "rendering/VisualConfiguration.h"

#include <cmath>
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
        float deltaTime) override;

    void configure(const VisualConfiguration& config) override { settings_ = config.distortion; }

    /**
     * Configure the distortion effect.
     * Parameters are clamped to valid ranges to prevent undefined behavior.
     * NaN/Inf values are replaced with safe defaults.
     */
    void setSettings(const DistortionSettings& settings)
    {
        settings_ = settings;
        // Guard against NaN/Inf values
        if (!std::isfinite(settings_.intensity))
            settings_.intensity = 0.0f;
        if (!std::isfinite(settings_.frequency))
            settings_.frequency = 4.0f;
        if (!std::isfinite(settings_.speed))
            settings_.speed = 1.0f;
        // Validate and clamp parameters to safe ranges
        settings_.intensity = juce::jlimit(0.0f, 1.0f, settings_.intensity);
        settings_.frequency = juce::jlimit(0.5f, 20.0f, settings_.frequency);
        settings_.speed = juce::jlimit(0.1f, 10.0f, settings_.speed);
    }
    [[nodiscard]] const DistortionSettings& getSettings() const { return settings_; }

private:
    DistortionSettings settings_;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    GLint textureLoc_ = -1;
    GLint intensityLoc_ = -1;
    GLint frequencyLoc_ = -1;
    GLint speedLoc_ = -1;
    GLint timeLoc_ = -1;

    float accumulatedTime_ = 0.0f;
    bool compiled_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
