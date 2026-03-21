/*
    Oscil - Basic Shader
    Default shader
*/

#pragma once

#include "rendering/WaveformShader.h"
#include <memory>

namespace oscil
{

/**
 * Basic shader - renders waveforms with a basic color glow effect.
 * The glow color matches the waveform color and fades outward.
 */
class BasicShader : public WaveformShader
{
public:
    BasicShader();
    ~BasicShader() override;

    // WaveformShader interface
    [[nodiscard]] juce::String getId() const override { return "basic"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Basic"; }
    [[nodiscard]] juce::String getDescription() const override
    {
        return "Waveform with simple glowing effect";
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

#if OSCIL_ENABLE_OPENGL
    void drawGlowPasses(juce::OpenGLExtensionFunctions& ext, int vertexCount);
#endif

    static constexpr float GLOW_INTENSITY = 0.8f;
    static constexpr int GLOW_PASSES = 3;
};

} // namespace oscil
