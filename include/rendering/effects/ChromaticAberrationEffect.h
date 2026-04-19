/*
    MultiScoper - Chromatic Aberration Effect
    Simulates lens color fringing
*/

#pragma once

#include "SingleShaderEffect.h"
#include "rendering/VisualConfiguration.h"

#if MULTISCOPER_ENABLE_OPENGL

namespace multiscoper
{

/**
 * Chromatic aberration post-processing effect.
 * Separates RGB channels slightly for a lens-like color fringing effect.
 */
class ChromaticAberrationEffect : public SingleShaderEffect
{
public:
    /// Create a chromatic aberration effect with default settings.
    ChromaticAberrationEffect();
    ~ChromaticAberrationEffect() override;

    [[nodiscard]] juce::String getId() const override { return "chromatic_aberration"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Chromatic Aberration"; }

    /**
     * Configure from VisualConfiguration.
     */
    void configure(const VisualConfiguration& config) override { settings_ = config.chromaticAberration; }

    /**
     * Configure the chromatic aberration effect.
     */
    void setSettings(const ChromaticAberrationSettings& settings) { settings_ = settings; }
    [[nodiscard]] const ChromaticAberrationSettings& getSettings() const { return settings_; }

protected:
    [[nodiscard]] const char* getFragmentSource() const override;
    bool resolveUniforms() override;
    void setUniforms(const Framebuffer& source, float deltaTime) override;

private:
    ChromaticAberrationSettings settings_;

    GLint intensityLoc_ = -1;
};

} // namespace multiscoper

#endif // MULTISCOPER_ENABLE_OPENGL
