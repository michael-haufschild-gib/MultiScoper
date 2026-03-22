/*
    Oscil - Neon Glow Shader
    Renders waveforms with an intense neon tube look
*/

#pragma once

#include "rendering/WaveformShader.h"
#include <memory>

namespace oscil
{

/**
 * Neon Glow shader - renders a bright white core with intense colored glow.
 */
class NeonGlowShader : public WaveformShader
{
public:
    /// Create a neon glow shader instance.
    NeonGlowShader();
    ~NeonGlowShader() override;

    [[nodiscard]] juce::String getId() const override { return "neon_glow"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Neon Glow"; }
    [[nodiscard]] juce::String getDescription() const override
    {
        return "Intense neon tube effect with white core";
    }

#if OSCIL_ENABLE_OPENGL
    /// Compile the neon glow shader program.
    bool compile(juce::OpenGLContext& context) override;
    /// Release the shader program and GPU resources.
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
