/*
    Oscil - Chromatic Aberration Effect
    Simulates lens color fringing
*/

#pragma once

#include "PostProcessEffect.h"
#include "rendering/VisualConfiguration.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Chromatic aberration post-processing effect.
 * Separates RGB channels slightly for a lens-like color fringing effect.
 */
class ChromaticAberrationEffect : public PostProcessEffect
{
public:
    ChromaticAberrationEffect();
    ~ChromaticAberrationEffect() override;

    [[nodiscard]] juce::String getId() const override { return "chromatic_aberration"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Chromatic Aberration"; }

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
     * Configure from VisualConfiguration.
     */
    void configure(const VisualConfiguration& config) override { settings_ = config.chromaticAberration; }

    /**
     * Configure the chromatic aberration effect.
     * Parameters are clamped to valid ranges to prevent undefined behavior.
     */
    void setSettings(const ChromaticAberrationSettings& settings)
    {
        settings_ = settings;
        // Validate and clamp parameters to safe ranges
        settings_.intensity = juce::jlimit(0.0f, 0.1f, settings_.intensity);
    }
    [[nodiscard]] const ChromaticAberrationSettings& getSettings() const { return settings_; }

private:
    ChromaticAberrationSettings settings_;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    GLint textureLoc_ = -1;
    GLint intensityLoc_ = -1;

    bool compiled_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
