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
    /// Create a gradient fill shader instance.
    GradientFillShader();
    ~GradientFillShader() override;

    [[nodiscard]] juce::String getId() const override { return "gradient_fill"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Gradient Fill"; }
    [[nodiscard]] juce::String getDescription() const override { return "Waveform with gradient fill under the curve"; }

#if OSCIL_ENABLE_OPENGL
    /// Compile the gradient fill shader program.
    bool compile(juce::OpenGLContext& context) override;
    /// Release the shader program and GPU resources.
    void release(juce::OpenGLContext& context) override;
    [[nodiscard]] bool isCompiled() const override;

    void render(juce::OpenGLContext& context, const std::vector<float>& channel1, const std::vector<float>* channel2,
                const ShaderRenderParams& params) override;
#endif

private:
#if OSCIL_ENABLE_OPENGL
    struct GLResources;
    std::unique_ptr<GLResources> gl_;

    void drawFillChannel(juce::OpenGLExtensionFunctions& ext, const std::vector<float>& samples, float centerY,
                         float amplitude, float boundsX, float boundsWidth, GLint posLoc, GLint vLoc);
#endif
};

} // namespace oscil
