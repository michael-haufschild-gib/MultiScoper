/*
    Oscil - Volumetric Ribbon Shader
    3D waveform visualization as a lit, volumetric ribbon
*/

#pragma once

#include "WaveformShader3D.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Renders waveform as a 3D volumetric ribbon with lighting.
 * Creates a tube-like mesh around the waveform path.
 */
class VolumetricRibbonShader : public WaveformShader3D
{
public:
    VolumetricRibbonShader();
    ~VolumetricRibbonShader() override;

    [[nodiscard]] juce::String getId() const override { return "volumetric_ribbon"; }
    [[nodiscard]] juce::String getName() const override { return "Volumetric Ribbon"; }
    [[nodiscard]] juce::String getDescription() const override { return "3D volumetric ribbon with internal lighting and pulse effects."; }

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

    // Ribbon-specific settings
    void setTubeRadius(float radius) { tubeRadius_ = radius; }
    [[nodiscard]] float getTubeRadius() const { return tubeRadius_; }

    void setTubeSegments(int segments) { tubeSegments_ = segments; }
    [[nodiscard]] int getTubeSegments() const { return tubeSegments_; }

    void setGlowIntensity(float intensity) { glowIntensity_ = intensity; }
    [[nodiscard]] float getGlowIntensity() const { return glowIntensity_; }

    void setPulseSpeed(float speed) { pulseSpeed_ = speed; }
    [[nodiscard]] float getPulseSpeed() const { return pulseSpeed_; }

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
    GLint lightDirLoc_ = -1;
    GLint ambientLoc_ = -1;
    GLint diffuseLoc_ = -1;
    GLint specularLoc_ = -1;
    GLint specPowerLoc_ = -1;
    GLint timeLoc_ = -1;
    GLint glowIntensityLoc_ = -1;
    GLint cameraPosLoc_ = -1;

    // Settings
    float tubeRadius_ = 0.03f;
    int tubeSegments_ = 8;
    float glowIntensity_ = 0.5f;
    float pulseSpeed_ = 1.0f;

    // Animation
    

    // Cached mesh data
    std::vector<float> vertices_;
    std::vector<GLuint> indices_;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
