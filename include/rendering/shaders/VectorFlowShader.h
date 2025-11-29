/*
    Oscil - Vector Flow Shader
    Animated vector flow effect with dashed line segments
*/

#pragma once

#include "rendering/WaveformShader.h"
#include <memory>

namespace oscil
{

/**
 * Vector Flow shader - renders waveforms with animated dashed line segments.
 * Creates a flowing effect where dashed segments move along the waveform.
 */
class VectorFlowShader : public WaveformShader
{
public:
    VectorFlowShader();
    ~VectorFlowShader() override;

    // WaveformShader interface
    [[nodiscard]] juce::String getId() const override { return "vector_flow"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Vector Flow"; }
    [[nodiscard]] juce::String getDescription() const override
    {
        return "Animated flowing dashed line effect";
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

    /**
     * Update animation state
     * @param deltaTime Time elapsed since last update in seconds
     */
    void update(float deltaTime);

private:
#if OSCIL_ENABLE_OPENGL
    struct GLResources;
    std::unique_ptr<GLResources> gl_;
#endif

    // Flow parameters
    float flowSpeed_ = 1.5f;
    float segmentLength_ = 0.2f;
    float gapLength_ = 0.1f;
    float time_ = 0.0f;
};

} // namespace oscil
