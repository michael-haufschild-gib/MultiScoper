/*
    Oscil - Wireframe Mesh Shader
    3D waveform visualization as a retro wireframe grid
*/

#pragma once

#include "WaveformShader3D.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Renders waveform as a 3D wireframe mesh grid.
 * Creates a retro/synthwave aesthetic with glowing lines.
 */
class WireframeMeshShader : public WaveformShader3D
{
public:
    WireframeMeshShader();
    ~WireframeMeshShader() override;

    [[nodiscard]] juce::String getId() const override { return "wireframe_mesh"; }
    [[nodiscard]] juce::String getName() const override { return "Wireframe Mesh"; }

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

    // Wireframe-specific settings
    void setGridDensity(int density) { gridDensity_ = density; }
    [[nodiscard]] int getGridDensity() const { return gridDensity_; }

    void setGridDepth(float depth) { gridDepth_ = depth; }
    [[nodiscard]] float getGridDepth() const { return gridDepth_; }

    void setLineGlow(float glow) { lineGlow_ = glow; }
    [[nodiscard]] float getLineGlow() const { return lineGlow_; }

    void setPerspectiveStrength(float strength) { perspectiveStrength_ = strength; }
    [[nodiscard]] float getPerspectiveStrength() const { return perspectiveStrength_; }

    void setScrollSpeed(float speed) { scrollSpeed_ = speed; }
    [[nodiscard]] float getScrollSpeed() const { return scrollSpeed_; }

private:
    void generateWireframeMesh(const WaveformData3D& data, float xSpread);

    bool compiled_ = false;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    // VAO/VBO
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    int vertexCount_ = 0;

    // Uniform locations
    GLint modelLoc_ = -1;
    GLint viewLoc_ = -1;
    GLint projLoc_ = -1;
    GLint colorLoc_ = -1;
    GLint timeLoc_ = -1;
    GLint glowLoc_ = -1;

    // Settings
    int gridDensity_ = 32;        // Number of grid lines
    float gridDepth_ = 1.0f;      // Depth of the grid (Z extent)
    float lineGlow_ = 0.8f;       // Glow intensity
    float perspectiveStrength_ = 0.5f;  // How much depth affects size
    float scrollSpeed_ = 0.5f;    // Grid scroll animation speed

    // Animation
    

    // Cached vertex data
    std::vector<float> vertices_;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
