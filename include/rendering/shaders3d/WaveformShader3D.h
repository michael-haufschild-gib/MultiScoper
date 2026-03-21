/*
    Oscil - 3D Waveform Shader Base Class
    Abstract base for all 3D waveform visualization shaders
*/

#pragma once

#include "rendering/Camera3D.h"
#include "rendering/WaveformShader.h"
#include "rendering/LightingConfig.h"
#include <juce_opengl/juce_opengl.h>
#include <vector>
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Waveform data for 3D rendering.
 */
struct WaveformData3D
{
    const float* samples = nullptr;  // Sample data (-1 to 1)
    int sampleCount = 0;             // Number of samples
    juce::Colour color;              // Waveform color
    float lineThickness = 2.0f;      // Line thickness
    float zOffset = 0.0f;            // Z-axis offset for multi-waveform
    float yOffset = 0.0f;            // Y-axis offset for stereo separation
    float amplitude = 1.0f;          // Amplitude scaling
    float time = 0.0f;               // Animation time
};

/**
 * Abstract base class for 3D waveform shaders.
 * Provides common functionality for view/projection transforms,
 * lighting, and mesh generation.
 */
class WaveformShader3D : public WaveformShader
{
public:
    ~WaveformShader3D() override = default;

    // WaveformShader overrides
    [[nodiscard]] juce::String getDisplayName() const override { return getName(); }
    [[nodiscard]] juce::String getDescription() const override { return getName(); } // Default description

    /**
     * Get human-readable name.
     */
    [[nodiscard]] virtual juce::String getName() const = 0;

    // 3D Configuration Setters
    void setCamera(const Camera3D& camera) { camera_ = camera; }
    void setLighting(const LightingConfig& lighting) { lighting_ = lighting; }

    /**
     * Render 2D bridge implementation (from WaveformShader).
     * Converts to 3D data and calls the 3D render method.
     */
    void render(juce::OpenGLContext& context,
                const std::vector<float>& channel1,
                const std::vector<float>* channel2,
                const ShaderRenderParams& params) override;

    /**
     * Render waveform (3D specific).
     * @param context OpenGL context
     * @param data Waveform data to render
     * @param camera Camera for view/projection matrices
     * @param lighting Lighting configuration
     */
    virtual void render(juce::OpenGLContext& context,
                        const WaveformData3D& data,
                        const Camera3D& camera,
                        const LightingConfig& lighting) = 0;

    /**
     * Update animation/time-based effects.
     * @param deltaTime Time step in seconds
     */
    virtual void update(float deltaTime) { juce::ignoreUnused(deltaTime); }

    /**
     * Set custom parameter.
     * @param name Parameter name
     * @param value Parameter value
     */
    virtual void setParameter(const juce::String& name, float value)
    {
        juce::ignoreUnused(name, value);
    }

    /**
     * Get custom parameter.
     */
    [[nodiscard]] virtual float getParameter(const juce::String& name) const
    {
        juce::ignoreUnused(name);
        return 0.0f;
    }

private:
    void renderStereoChannels(juce::OpenGLContext& context,
                              WaveformData3D& data,
                              const std::vector<float>& channel2,
                              const ShaderRenderParams& params);

protected:
    // Stored context for 3D rendering
    Camera3D camera_;
    LightingConfig lighting_;
    float time_ = 0.0f;

    /**
     * Helper to compile vertex + fragment shader.
     * @param shader Shader program to compile into
     * @param vertexSource Vertex shader source
     * @param fragmentSource Fragment shader source
     * @return true if successful
     */
    static bool compileShaderProgram(juce::OpenGLShaderProgram& shader,
                                     const char* vertexSource,
                                     const char* fragmentSource);

    /**
     * Set standard matrix uniforms.
     * @param ext OpenGL extensions
     * @param modelLoc Model matrix uniform location
     * @param viewLoc View matrix uniform location
     * @param projLoc Projection matrix uniform location
     * @param camera Camera to get matrices from
     * @param model Model matrix (identity if nullptr)
     */
    static void setMatrixUniforms(juce::OpenGLExtensionFunctions& ext,
                                  GLint modelLoc,
                                  GLint viewLoc,
                                  GLint projLoc,
                                  const Camera3D& camera,
                                  const Matrix4* model = nullptr);

    /**
     * Set lighting uniforms.
     */
    static void setLightingUniforms(juce::OpenGLExtensionFunctions& ext,
                                    GLint lightDirLoc,
                                    GLint ambientLoc,
                                    GLint diffuseLoc,
                                    GLint specularLoc,
                                    GLint specPowerLoc,
                                    const LightingConfig& lighting);

public:
    /**
     * Generate tube mesh vertices from waveform samples.
     * Creates a tubular mesh around the waveform path.
     * @param samples Sample data
     * @param sampleCount Number of samples
     * @param xSpread Horizontal spread multiplier (1.0 = -1 to 1)
     * @param radius Tube radius
     * @param segments Segments around tube circumference
     * @param vertices Output vertex buffer (position + normal + uv)
     * @param indices Output index buffer
     */
    static void generateTubeMesh(const float* samples, int sampleCount,
                                 float xSpread,
                                 float radius, int segments,
                                 std::vector<float>& vertices,
                                 std::vector<GLuint>& indices);

    /**
     * Generate ribbon mesh vertices from waveform samples.
     * Creates a flat ribbon that can have width variation.
     */
    static void generateRibbonMesh(const float* samples, int sampleCount,
                                   float xSpread,
                                   float width,
                                   std::vector<float>& vertices,
                                   std::vector<GLuint>& indices);

    /**
     * Calculate xSpread and halfHeight from camera projection.
     * Used by all 3D shaders to map waveform to screen-filling coordinates.
     */
    static void calculateCameraSpread(const Camera3D& camera, float& xSpread, float& halfHeight);

    /**
     * Setup standard position(3) + normal(3) + texcoord(2) vertex attributes.
     * Stride = 8 floats. Expects VAO already bound.
     */
    static void setupPosNormTexAttribs(juce::OpenGLExtensionFunctions& ext, GLuint programID);
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
