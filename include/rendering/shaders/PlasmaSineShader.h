/*
    Oscil - Plasma Sine Shader
    Renders waveform with a procedural plasma fill
*/

#pragma once

#include "rendering/WaveformShader.h"
#include <memory>

namespace oscil
{

/**
 * Plasma Sine shader - renders a procedural plasma effect within the waveform area.
 */
class PlasmaSineShader : public WaveformShader
{
public:
    PlasmaSineShader();
    ~PlasmaSineShader() override;

    [[nodiscard]] juce::String getId() const override { return "plasma_sine"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Plasma Sine"; }
    [[nodiscard]] juce::String getDescription() const override
    {
        return "Procedural plasma fill effect";
    }

#if OSCIL_ENABLE_OPENGL
    bool compile(juce::OpenGLContext& context) override;
    void release(juce::OpenGLContext& context) override;
    [[nodiscard]] bool isCompiled() const override;

    void render(
        juce::OpenGLContext& context,
        const std::vector<float>& channel1,
        const std::vector<float>* channel2,
        const ShaderRenderParams& params
    ) override;
#endif

private:
#if OSCIL_ENABLE_OPENGL
    struct GLResources;
    std::unique_ptr<GLResources> gl_;
#endif
};

} // namespace oscil
