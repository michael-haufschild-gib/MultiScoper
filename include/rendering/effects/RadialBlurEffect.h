/*
    MultiScoper - Radial Blur Effect
    Zoom-glow effect sampling from center at different distances
*/

#pragma once

#include "SingleShaderEffect.h"
#include "rendering/VisualConfiguration.h"

#if MULTISCOPER_ENABLE_OPENGL

namespace multiscoper
{

/**
 * Radial blur post-processing effect.
 * Creates a zoom-glow effect by sampling the image at multiple zoom levels
 * from the center, similar to the Electric Flower WebGL demo.
 */
class RadialBlurEffect : public SingleShaderEffect
{
public:
    /// Create a radial blur effect with default settings.
    RadialBlurEffect();
    ~RadialBlurEffect() override;

    [[nodiscard]] juce::String getId() const override { return "radial_blur"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Radial Blur"; }

    /**
     * Configure from VisualConfiguration.
     */
    void configure(const VisualConfiguration& config) override { settings_ = config.radialBlur; }

    /**
     * Configure the radial blur effect.
     */
    void setSettings(const RadialBlurSettings& settings) { settings_ = settings; }
    [[nodiscard]] const RadialBlurSettings& getSettings() const { return settings_; }

protected:
    [[nodiscard]] const char* getFragmentSource() const override;
    bool resolveUniforms() override;
    void setUniforms(const Framebuffer& source, float deltaTime) override;

private:
    RadialBlurSettings settings_;

    GLint amountLoc_ = -1;
    GLint glowLoc_ = -1;
    GLint samplesLoc_ = -1;
};

} // namespace multiscoper

#endif // MULTISCOPER_ENABLE_OPENGL
