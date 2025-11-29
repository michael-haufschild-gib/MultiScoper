/*
    Oscil - Color Grade Effect
    Professional color grading with brightness, contrast, saturation, and temperature
*/

#pragma once

#include "PostProcessEffect.h"
#include "rendering/VisualConfiguration.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Color grading post-processing effect.
 * Provides professional color correction with:
 * - Brightness, contrast, saturation
 * - Temperature (cool to warm)
 * - Tint (green to magenta)
 * - Shadow/highlight color tinting
 */
class ColorGradeEffect : public PostProcessEffect
{
public:
    ColorGradeEffect();
    ~ColorGradeEffect() override;

    [[nodiscard]] juce::String getId() const override { return "color_grade"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Color Grade"; }

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
     * Configure the color grading effect.
     */
    void setSettings(const ColorGradeSettings& settings) { settings_ = settings; }
    [[nodiscard]] const ColorGradeSettings& getSettings() const { return settings_; }

private:
    ColorGradeSettings settings_;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    GLint textureLoc_ = -1;
    GLint brightnessLoc_ = -1;
    GLint contrastLoc_ = -1;
    GLint saturationLoc_ = -1;
    GLint temperatureLoc_ = -1;
    GLint tintLoc_ = -1;
    GLint shadowsLoc_ = -1;
    GLint highlightsLoc_ = -1;

    bool compiled_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
