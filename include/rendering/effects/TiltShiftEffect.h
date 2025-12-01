/*
    Oscil - Tilt Shift Effect
    Simulates depth of field by blurring the top and bottom of the screen.
*/

#pragma once

#include "PostProcessEffect.h"
#include "rendering/VisualConfiguration.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Tilt Shift post-processing effect.
 * Applies a variable blur based on Y-position to simulate macro photography depth of field.
 */
class TiltShiftEffect : public PostProcessEffect
{
public:
    TiltShiftEffect();
    ~TiltShiftEffect() override;

    [[nodiscard]] juce::String getId() const override { return "tilt_shift"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Tilt Shift"; }

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

    void setSettings(const TiltShiftSettings& settings) { settings_ = settings; }
    [[nodiscard]] const TiltShiftSettings& getSettings() const { return settings_; }

private:
    TiltShiftSettings settings_;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;
    bool compiled_ = false;
    GLint textureLoc_ = -1;

    GLint positionLoc_ = -1;
    GLint rangeLoc_ = -1;
    GLint blurRadiusLoc_ = -1;
    GLint iterationsLoc_ = -1;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
