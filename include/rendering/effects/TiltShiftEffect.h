/*
    Oscil - Tilt Shift Effect
    Simulates depth of field by blurring the top and bottom of the screen.
*/

#pragma once

#include "SingleShaderEffect.h"
#include "rendering/VisualConfiguration.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Tilt Shift post-processing effect.
 * Applies a variable blur based on Y-position to simulate macro photography depth of field.
 */
class TiltShiftEffect : public SingleShaderEffect
{
public:
    /// Create a tilt shift effect with default settings.
    TiltShiftEffect();
    ~TiltShiftEffect() override;

    [[nodiscard]] juce::String getId() const override { return "tilt_shift"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Tilt Shift"; }

    void configure(const VisualConfiguration& config) override { settings_ = config.tiltShift; }
    void setSettings(const TiltShiftSettings& settings) { settings_ = settings; }
    [[nodiscard]] const TiltShiftSettings& getSettings() const { return settings_; }

protected:
    [[nodiscard]] const char* getFragmentSource() const override;
    bool resolveUniforms() override;
    void setUniforms(const Framebuffer& source, float deltaTime) override;

private:
    TiltShiftSettings settings_;

    GLint positionLoc_ = -1;
    GLint rangeLoc_ = -1;
    GLint blurRadiusLoc_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TiltShiftEffect)
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
