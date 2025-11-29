/*
    Oscil - Neon Glow Shader
    Default shader with neon glow effect around waveforms
*/

#pragma once

#include "rendering/WaveformShader.h"
#include <memory>

namespace oscil
{

/**
 * Neon Glow shader - renders waveforms with a glowing neon effect.
 * The glow color matches the waveform color and fades outward.
 */
class NeonGlowShader : public WaveformShader
{
public:
    NeonGlowShader();
    ~NeonGlowShader() override;

    // WaveformShader interface
    [[nodiscard]] juce::String getId() const override { return "neon_glow"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Neon Glow"; }
    [[nodiscard]] juce::String getDescription() const override
    {
        return "Waveform with glowing neon effect";
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

    void renderSoftware(
        juce::Graphics& g,
        const std::vector<float>& channel1,
        const std::vector<float>* channel2,
        const ShaderRenderParams& params
    ) override;

private:
#if OSCIL_ENABLE_OPENGL
    struct GLResources;
    std::unique_ptr<GLResources> gl_;
#endif

    // Glow parameters
    static constexpr float GLOW_INTENSITY = 0.8f;
    static constexpr int GLOW_PASSES = 3;
};

} // namespace oscil
