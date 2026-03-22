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
    /// Create a basic shader instance.
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
    /// Compile the basic glow shader program.
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

    void renderSoftware(
        juce::Graphics& g,
        const std::vector<float>& channel1,
        const std::vector<float>* channel2,
        const ShaderRenderParams& params
    ) override;

    static constexpr float GLOW_INTENSITY = 0.8f;
    static constexpr int GLOW_PASSES = 3;

private:
#if OSCIL_ENABLE_OPENGL
    struct GLResources;
    std::unique_ptr<GLResources> gl_;

    void drawGlowPasses(juce::OpenGLExtensionFunctions& ext, int vertexCount);
    void resolveUniforms(juce::OpenGLContext& context);
    bool validateUniforms() const;
    void renderChannel(juce::OpenGLExtensionFunctions& ext,
                       const std::vector<float>& samples,
                       float centerY, float amplitude, float glowWidth,
                       float boundsX, float boundsWidth);
    std::vector<float> vertexBuffer_;
#endif
};

} // namespace oscil
