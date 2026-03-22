/*
    Oscil - Film Grain Effect
    Adds animated film grain noise for vintage/analog aesthetic
*/

#pragma once

#include "PostProcessEffect.h"
#include "rendering/VisualConfiguration.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Film grain post-processing effect.
 * Adds animated noise for a vintage/analog look.
 */
class FilmGrainEffect : public PostProcessEffect
{
public:
    /// Create a film grain effect with default settings.
    FilmGrainEffect();
    ~FilmGrainEffect() override;

    [[nodiscard]] juce::String getId() const override { return "film_grain"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Film Grain"; }

    /// Compile the film grain shader program.
    bool compile(juce::OpenGLContext& context) override;
    /// Release the shader program.
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
     * Configure from VisualConfiguration.
     */
    void configure(const VisualConfiguration& config) override { settings_ = config.filmGrain; }

    /**
     * Configure the film grain effect.
     */
    void setSettings(const FilmGrainSettings& settings) { settings_ = settings; }
    [[nodiscard]] const FilmGrainSettings& getSettings() const { return settings_; }

private:
    FilmGrainSettings settings_;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    GLint textureLoc_ = -1;
    GLint intensityLoc_ = -1;
    GLint timeLoc_ = -1;
    GLint resolutionLoc_ = -1;

    float accumulatedTime_ = 0.0f;
    bool compiled_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
