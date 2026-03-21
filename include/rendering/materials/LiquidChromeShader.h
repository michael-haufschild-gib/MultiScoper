/*
    Oscil - Liquid Chrome Shader
    Renders waveform as flowing liquid metal with reflections
*/

#pragma once

#include "MaterialShader.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Renders waveform as liquid chrome/mercury with animated surface.
 * Uses environment mapping for metallic reflections.
 */
class LiquidChromeShader : public MaterialShader
{
public:
    LiquidChromeShader();
    ~LiquidChromeShader() override;

    [[nodiscard]] juce::String getId() const override { return "liquid_chrome"; }
    [[nodiscard]] juce::String getName() const override { return "Liquid Chrome"; }

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

    // Chrome-specific settings
    void setRippleFrequency(float freq) { rippleFrequency_ = freq; }
    [[nodiscard]] float getRippleFrequency() const { return rippleFrequency_; }

    void setRippleAmplitude(float amp) { rippleAmplitude_ = amp; }
    [[nodiscard]] float getRippleAmplitude() const { return rippleAmplitude_; }

    void setRippleSpeed(float speed) { rippleSpeed_ = speed; }
    [[nodiscard]] float getRippleSpeed() const { return rippleSpeed_; }

    void setFlowSpeed(float speed) { flowSpeed_ = speed; }
    [[nodiscard]] float getFlowSpeed() const { return flowSpeed_; }

    void setTubeRadius(float radius) { tubeRadius_ = radius; }
    [[nodiscard]] float getTubeRadius() const { return tubeRadius_; }

private:
    void updateMesh(const WaveformData3D& data, float xSpread);
    void setChromeUniforms(juce::OpenGLExtensionFunctions& ext,
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
    GLint metallicLoc_ = -1;
    GLint roughnessLoc_ = -1;
    GLint fresnelPowerLoc_ = -1;
    GLint timeLoc_ = -1;
    GLint rippleFreqLoc_ = -1;
    GLint rippleAmpLoc_ = -1;
    GLint envMapLoc_ = -1;
    GLint lightDirLoc_ = -1;
    GLint specularLoc_ = -1;

    // Chrome-specific parameters
    float rippleFrequency_ = 8.0f;
    float rippleAmplitude_ = 0.02f;
    float rippleSpeed_ = 2.0f;
    float flowSpeed_ = 0.5f;
    float tubeRadius_ = 0.04f;

    // Animation
    

    // Cached mesh data
    std::vector<float> vertices_;
    std::vector<GLuint> indices_;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
