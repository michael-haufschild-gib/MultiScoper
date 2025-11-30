/*
    Oscil - Glitch Effect
    Digital glitch/corruption effect
*/

#pragma once

#include "PostProcessEffect.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Glitch settings for the effect.
 */
struct GlitchSettings
{
    bool enabled = false;
    float intensity = 0.5f;          // Overall glitch strength
    float blockSize = 0.05f;         // Size of glitch blocks
    float lineShift = 0.02f;         // Horizontal line displacement
    float colorSeparation = 0.01f;   // RGB shift amount
    float flickerRate = 10.0f;       // Flicker frequency
};

/**
 * Glitch post-processing effect.
 * Creates digital corruption aesthetics.
 */
class GlitchEffect : public PostProcessEffect
{
public:
    GlitchEffect();
    ~GlitchEffect() override;

    [[nodiscard]] juce::String getId() const override { return "glitch"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Glitch"; }

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

    void setSettings(const GlitchSettings& settings) { settings_ = settings; }
    [[nodiscard]] const GlitchSettings& getSettings() const { return settings_; }

private:
    GlitchSettings settings_;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    GLint textureLoc_ = -1;
    GLint intensityLoc_ = -1;
    GLint timeLoc_ = -1;
    GLint resolutionLoc_ = -1;
    GLint blockSizeLoc_ = -1;
    GLint lineShiftLoc_ = -1;
    GLint colorSeparationLoc_ = -1;

    float accumulatedTime_ = 0.0f;
    bool compiled_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
