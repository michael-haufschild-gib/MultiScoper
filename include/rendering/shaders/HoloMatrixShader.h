/*
    Oscil - Holo Matrix Shader
    Holographic matrix-style effect with animated crystalline patterns
*/

#pragma once

#include "rendering/WaveformShader.h"
#include <memory>
#include <vector>

namespace oscil
{

/**
 * Holo Matrix shader - renders waveforms with holographic matrix effect.
 * Features animated crystalline patterns and time-based visual effects.
 */
class HoloMatrixShader : public WaveformShader
{
public:
    HoloMatrixShader();
    ~HoloMatrixShader() override;

    // WaveformShader interface
    [[nodiscard]] juce::String getId() const override { return "holo_matrix"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Holo Matrix"; }
    [[nodiscard]] juce::String getDescription() const override
    {
        return "Holographic matrix effect with animated crystalline patterns";
    }

    /**
     * Update time-based animation
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime);

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

    // Animation parameters
    float crystalScale_ = 0.05f;
    float time_ = 0.0f;

    std::vector<float> vertices_;

    // Effect parameters
    static constexpr float MATRIX_INTENSITY = 0.7f;
    static constexpr float CRYSTAL_SCALE_DEFAULT = 0.05f;
};

} // namespace oscil
