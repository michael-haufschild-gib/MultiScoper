/*
    Oscil - Vector Flow Shader
    Renders waveform as a field of flowing vector lines
*/

#pragma once

#include "rendering/shaders3d/WaveformShader3D.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Vector Flow shader - renders the waveform as a series of flowing segments/vectors.
 * giving the appearance of data streaming along the path.
 */
class VectorFlowShader : public WaveformShader3D
{
public:
    VectorFlowShader();
    ~VectorFlowShader() override;

    [[nodiscard]] juce::String getId() const override { return "vector_flow"; }
    [[nodiscard]] juce::String getName() const override { return "Vector Flow"; }

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

    void setFlowSpeed(float speed) { flowSpeed_ = speed; }
    void setSegmentLength(float length) { segmentLength_ = length; }
    void setGapLength(float length) { gapLength_ = length; }

private:
    void generateVectorMesh(const WaveformData3D& data);

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
    GLint flowParamsLoc_ = -1; // x=speed, y=segLen, z=gapLen

    float flowSpeed_ = 1.5f;
    float segmentLength_ = 0.2f;
    float gapLength_ = 0.1f;
    

    std::vector<float> vertices_;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
