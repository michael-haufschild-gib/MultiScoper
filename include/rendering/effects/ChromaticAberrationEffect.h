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
    /// Create a chromatic aberration effect with default settings.
    ChromaticAberrationEffect();
    ~ChromaticAberrationEffect() override;

    [[nodiscard]] juce::String getId() const override { return "chromatic_aberration"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Chromatic Aberration"; }

    /// Compile the chromatic aberration shader program.
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
    void configure(const VisualConfiguration& config) override { settings_ = config.chromaticAberration; }

    /**
     * Configure the chromatic aberration effect.
     */
    void setSettings(const ChromaticAberrationSettings& settings) { settings_ = settings; }
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
