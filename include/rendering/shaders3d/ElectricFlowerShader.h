/*
    Oscil - Electric Flower Shader
    3D waveform visualization using cascaded rotations inspired by
    webglsamples.org/electricflower demo
*/

#pragma once

#include "WaveformShader3D.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Electric Flower shader - inspired by webglsamples.org/electricflower
 * Preserves waveform geometry while applying organic visual style.
 *
 * Key features:
 * - Waveform geometry preserved (no 3D rotations distorting shape)
 * - Irrational multipliers (phi, sqrt(2), e) for organic pulsing animation
 * - Amplitude-driven effects: brightness, point size, color shifts
 * - Color interpolation between base and complementary colors
 * - Additive blending for glow effect
 * - Point rendering with soft falloff + line strip connection
 */
class ElectricFlowerShader : public WaveformShader3D
{
public:
    ElectricFlowerShader();
    ~ElectricFlowerShader() override;

    [[nodiscard]] juce::String getId() const override { return "electric_flower"; }
    [[nodiscard]] juce::String getName() const override { return "Electric Flower"; }

    bool compile(juce::OpenGLContext& context) override;
    void release(juce::OpenGLContext& context) override;
    [[nodiscard]] bool isCompiled() const override;

    void render(juce::OpenGLContext& context,
                const WaveformData3D& data,
                const Camera3D& camera,
                const LightingConfig& lighting) override;

    void update(float deltaTime) override;

    void setParameter(const juce::String& name, float value) override;
    [[nodiscard]] float getParameter(const juce::String& name) const override;

private:
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    // Uniform locations
    GLint modelLoc_ = -1;
    GLint viewLoc_ = -1;
    GLint projLoc_ = -1;
    GLint colorLoc_ = -1;
    GLint color2Loc_ = -1;
    GLint timeLoc_ = -1;
    GLint pointSizeLoc_ = -1;
    GLint glowIntensityLoc_ = -1;

    // VBO for vertex data
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    int vertexCount_ = 0;

    // Parameters
    float rotationSpeed_ = 1.0f;
    float pointSize_ = 4.0f;
    float glowIntensity_ = 1.0f;
    float colorShift_ = 0.5f;  // How much color2 differs from base color

    // Vertex data buffer
    std::vector<float> vertices_;

    bool compiled_ = false;

    void updateVertices(const WaveformData3D& data, float xSpread);
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
