/*
    Oscil - Crystalline Shader
    Renders waveform as sharp, faceted crystal material
*/

#pragma once

#include "rendering/materials/MaterialShader.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Crystalline shader - renders waveform with faceted, diamond-like geometry
 * and high refractive/dispersive properties.
 */
class CrystallineShader : public MaterialShader
{
public:
    CrystallineShader();
    ~CrystallineShader() override;

    [[nodiscard]] juce::String getId() const override { return "crystalline"; }
    [[nodiscard]] juce::String getName() const override { return "Crystalline"; }

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
    void generateCrystalMesh(const WaveformData3D& data);

    bool compiled_ = false;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ibo_ = 0;
    int indexCount_ = 0;

    GLint modelLoc_ = -1;
    GLint viewLoc_ = -1;
    GLint projLoc_ = -1;
    GLint cameraPosLoc_ = -1;
    GLint lightDirLoc_ = -1;
    
    // Material uniforms
    GLint baseColorLoc_ = -1;
    GLint roughnessLoc_ = -1;
    GLint metallicLoc_ = -1;
    GLint iorLoc_ = -1;
    GLint envMapLoc_ = -1;

    
    float crystalScale_ = 0.05f;

    std::vector<float> vertices_;
    std::vector<GLuint> indices_;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
