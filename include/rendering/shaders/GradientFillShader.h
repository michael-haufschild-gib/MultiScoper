/*
    Oscil - Gradient Fill Shader
    Renders waveform with a gradient fill under the curve
*/

#pragma once

#include "rendering/WaveformShader.h"
#include <memory>

namespace oscil
{

/**
 * Gradient Fill shader - renders a solid fill under the waveform
 * fading from the waveform color at the top to transparent at the bottom.
 */
class GradientFillShader : public WaveformShader
{
public:
    GradientFillShader();
    ~GradientFillShader() override;

    [[nodiscard]] juce::String getId() const override { return "gradient_fill"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Gradient Fill"; }
    [[nodiscard]] juce::String getDescription() const override
    {
        return "Waveform with gradient fill under the curve";
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

    /**
     * Software fallback: Draws gradient-filled waveform using JUCE Graphics
     */
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
};

} // namespace oscil
