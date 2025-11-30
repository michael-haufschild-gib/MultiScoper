/*
    Oscil - String Theory Shader
    Renders waveform as multiple vibrating strings in parallel planes
*/

#pragma once

#include "rendering/shaders3d/WaveformShader3D.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * String Theory shader - renders the waveform as multiple parallel strings
 * with slight phase/amplitude variations, simulating a vibrating string ensemble.
 */
class StringTheoryShader : public WaveformShader3D
{
public:
    StringTheoryShader();
    ~StringTheoryShader() override;

    [[nodiscard]] juce::String getId() const override { return "string_theory"; }
    [[nodiscard]] juce::String getName() const override { return "String Theory"; }

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

    void setStringCount(int count) { stringCount_ = count; }
    void setStringSpacing(float spacing) { stringSpacing_ = spacing; }
    void setPhaseVariance(float var) { phaseVariance_ = var; }

private:
    void generateStringMesh(const WaveformData3D& data);

    bool compiled_ = false;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    size_t vertexCount_ = 0;

    GLint modelLoc_ = -1;
    GLint viewLoc_ = -1;
    GLint projLoc_ = -1;
    GLint colorLoc_ = -1;
    GLint timeLoc_ = -1;
    
    int stringCount_ = 5;
    float stringSpacing_ = 0.1f;
    float phaseVariance_ = 0.5f;
    

    std::vector<float> vertices_;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
