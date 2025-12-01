/*
    Oscil - Electric Filigree Shader
    Renders electric/lightning effects using ridged multifractal noise
*/

#pragma once

#include "WaveformShader3D.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

class ElectricFiligreeShader : public WaveformShader3D
{
public:
    ElectricFiligreeShader();
    ~ElectricFiligreeShader() override;

    [[nodiscard]] juce::String getId() const override { return "electric_filigree"; }
    [[nodiscard]] juce::String getName() const override { return "Electric Filigree"; }
    [[nodiscard]] juce::String getDescription() const override { return "Electric lightning effect using ridged noise."; }

    bool compile(juce::OpenGLContext& context) override;
    void release(juce::OpenGLContext& context) override;
    [[nodiscard]] bool isCompiled() const override { return compiled_; }

    void render(juce::OpenGLContext& context,
                const WaveformData3D& data,
                const Camera3D& camera,
                const LightingConfig& lighting) override;

    void update(float deltaTime) override;

    void setParameter(const juce::String& name, float value) override;
    [[nodiscard]] float getParameter(const juce::String& name) const override;

private:
    void updateMesh(const WaveformData3D& data, float xSpread);

    bool compiled_ = false;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    // VAO/VBO
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ibo_ = 0;
    int indexCount_ = 0;

    // Uniform locations
    GLint modelLoc_ = -1;
    GLint viewLoc_ = -1;
    GLint projLoc_ = -1;
    GLint colorLoc_ = -1;
    GLint timeLoc_ = -1;
    GLint noiseScaleLoc_ = -1;
    GLint noiseSpeedLoc_ = -1;
    GLint branchIntensityLoc_ = -1;

    // Settings
    float noiseScale_ = 2.0f;
    float noiseSpeed_ = 1.0f;
    float branchIntensity_ = 0.8f;

    // Cached mesh data
    std::vector<float> vertices_;
    std::vector<GLuint> indices_;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
