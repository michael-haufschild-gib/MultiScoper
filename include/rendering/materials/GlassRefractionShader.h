/*
    Oscil - Glass Refraction Shader
    Renders waveform as a glass-like refractive surface
*/

#pragma once

#include "MaterialShader.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Renders waveform as glass/crystal with refraction effects.
 * Uses environment mapping for reflections and fake refraction.
 */
class GlassRefractionShader : public MaterialShader
{
public:
    GlassRefractionShader();
    ~GlassRefractionShader() override;

    [[nodiscard]] juce::String getId() const override { return "glass_refraction"; }
    [[nodiscard]] juce::String getName() const override { return "Glass Refraction"; }

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

    // Glass-specific settings
    void setRefractionIndex(float ior) { material_.ior = ior; }
    [[nodiscard]] float getRefractionIndex() const { return material_.ior; }

    void setRefractionStrength(float strength) { material_.refractionStrength = strength; }
    [[nodiscard]] float getRefractionStrength() const { return material_.refractionStrength; }

    void setDispersion(float dispersion) { dispersion_ = dispersion; }
    [[nodiscard]] float getDispersion() const { return dispersion_; }

    void setThickness(float thickness) { thickness_ = thickness; }
    [[nodiscard]] float getThickness() const { return thickness_; }

private:
    void updateMesh(const WaveformData3D& data, float xSpread);
    void setGlassUniforms(juce::OpenGLExtensionFunctions& ext,
                          const WaveformData3D& data,
                          const Camera3D& camera,
                          const LightingConfig& lighting,
                          float halfHeight);

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
    GLint cameraPosLoc_ = -1;
    GLint colorLoc_ = -1;
    GLint iorLoc_ = -1;
    GLint refractionStrengthLoc_ = -1;
    GLint fresnelPowerLoc_ = -1;
    GLint dispersionLoc_ = -1;
    GLint timeLoc_ = -1;
    GLint envMapLoc_ = -1;
    GLint lightDirLoc_ = -1;
    GLint specularLoc_ = -1;

    // Glass-specific parameters
    float dispersion_ = 0.02f;  // Chromatic dispersion amount
    float thickness_ = 0.05f;   // Glass tube thickness

    // Animation
    

    // Cached mesh data
    std::vector<float> vertices_;
    std::vector<GLuint> indices_;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
