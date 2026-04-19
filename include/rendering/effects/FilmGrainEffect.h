/*
    MultiScoper - Film Grain Effect
    Adds animated film grain noise for vintage/analog aesthetic
*/

#pragma once

#include "SingleShaderEffect.h"
#include "rendering/VisualConfiguration.h"

#if MULTISCOPER_ENABLE_OPENGL

namespace multiscoper
{

/**
 * Film grain post-processing effect.
 * Adds animated noise for a vintage/analog look.
 */
class FilmGrainEffect : public SingleShaderEffect
{
public:
    /// Create a film grain effect with default settings.
    FilmGrainEffect();
    ~FilmGrainEffect() override;

    [[nodiscard]] juce::String getId() const override { return "film_grain"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Film Grain"; }

    /**
     * Configure from VisualConfiguration.
     */
    void configure(const VisualConfiguration& config) override { settings_ = config.filmGrain; }

    /**
     * Configure the film grain effect.
     */
    void setSettings(const FilmGrainSettings& settings) { settings_ = settings; }
    [[nodiscard]] const FilmGrainSettings& getSettings() const { return settings_; }

protected:
    [[nodiscard]] const char* getFragmentSource() const override;
    bool resolveUniforms() override;
    void setUniforms(const Framebuffer& source, float deltaTime) override;

private:
    FilmGrainSettings settings_;

    GLint intensityLoc_ = -1;
    GLint timeLoc_ = -1;
    GLint resolutionLoc_ = -1;

    float accumulatedTime_ = 0.0f;
};

} // namespace multiscoper

#endif // MULTISCOPER_ENABLE_OPENGL
