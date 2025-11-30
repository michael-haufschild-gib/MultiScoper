/*
    Oscil - Digital Glitch Shader
    Renders waveform with digital artifacts and geometry displacement
*/

#pragma once

#include "rendering/WaveformShader.h"
#include <memory>

namespace oscil
{

/**
 * Digital Glitch shader - renders waveform with randomized geometric displacement.
 */
class DigitalGlitchShader : public WaveformShader
{
public:
    DigitalGlitchShader();
    ~DigitalGlitchShader() override;

    [[nodiscard]] juce::String getId() const override { return "digital_glitch"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Digital Glitch"; }
    [[nodiscard]] juce::String getDescription() const override
    {
        return "Waveform with digital geometry glitch artifacts";
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
