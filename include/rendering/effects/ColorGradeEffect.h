/*
    Oscil - Color Grade Effect
    Professional color grading with brightness, contrast, saturation, and temperature
*/

#pragma once

#include "SingleShaderEffect.h"
#include "rendering/VisualConfiguration.h"

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
class ColorGradeEffect : public SingleShaderEffect
{
public:
    /// Create a color grade effect with default settings.
    ColorGradeEffect();
    ~ColorGradeEffect() override;

    [[nodiscard]] juce::String getId() const override { return "color_grade"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Color Grade"; }

    /**
     * Configure from VisualConfiguration.
     */
    void configure(const VisualConfiguration& config) override { settings_ = config.colorGrade; }

    /**
     * Configure the color grading effect.
     */
    void setSettings(const ColorGradeSettings& settings) { settings_ = settings; }
    [[nodiscard]] const ColorGradeSettings& getSettings() const { return settings_; }

protected:
    [[nodiscard]] const char* getFragmentSource() const override;
    bool resolveUniforms() override;
    void setUniforms(const Framebuffer& source, float deltaTime) override;

private:
    ColorGradeSettings settings_;

    GLint brightnessLoc_ = -1;
    GLint contrastLoc_ = -1;
    GLint saturationLoc_ = -1;
    GLint temperatureLoc_ = -1;
    GLint tintLoc_ = -1;
    GLint shadowsLoc_ = -1;
    GLint highlightsLoc_ = -1;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
