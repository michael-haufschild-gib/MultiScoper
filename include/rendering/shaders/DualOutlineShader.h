/*
    Oscil - Dual Outline Shader
    Renders waveform with double outlines (inner and outer)
*/

#pragma once

#include "rendering/WaveformShader.h"
#include <memory>

namespace oscil
{

/**
 * Dual Outline shader - renders two distinct concentric lines for the waveform.
 */
class DualOutlineShader : public WaveformShader
{
public:
    /// Create a dual outline shader instance.
    DualOutlineShader();
    ~DualOutlineShader() override;

    [[nodiscard]] juce::String getId() const override { return "dual_outline"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Dual Outline"; }
    [[nodiscard]] juce::String getDescription() const override
    {
        return "Waveform with double outline effect";
    }

#if OSCIL_ENABLE_OPENGL
    /// Compile the dual outline shader program.
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

    void drawChannel(juce::OpenGLExtensionFunctions& ext,
                     const std::vector<float>& samples,
                     float centerY, float amplitude,
                     float boundsX, float boundsWidth,
                     float lineWidth, GLint posLoc, GLint distLoc);
#endif
};

} // namespace oscil
