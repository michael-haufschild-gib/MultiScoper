/*
    Oscil - Grid Renderer
    Handles rendering of background grids
*/

#pragma once

#include "rendering/WaveformGLRenderer.h" // For WaveformRenderData

#include <juce_opengl/juce_opengl.h>

namespace oscil
{

class GridRenderer
{
public:
    /// Create an uninitialized grid renderer.
    GridRenderer();
    ~GridRenderer();

    /// Compile grid shaders and allocate GPU buffers.
    void initialize(juce::OpenGLContext& context);
    /// Release all GPU resources.
    void release(juce::OpenGLContext& context);
    /// Render the background grid for the given waveform data.
    void render(juce::OpenGLContext& context, const WaveformRenderData& data);

private:
    void createBuffers(juce::OpenGLContext& context);
    void compileShaders(juce::OpenGLContext& context);

    void drawLines(juce::OpenGLContext& context, const std::vector<float>& verts, juce::Colour col) const;
    void generateChannelGrid(const GridConfiguration& config, float yTop, float yBottom);
    void generateHorizontalGrid(float yTop, float yBottom);
    void generateTimeGrid(const GridConfiguration& config, float yTop, float yBottom);
    void generateMusicalGrid(const GridConfiguration& config, float yTop, float yBottom);

    static void addLine(std::vector<float>& dest, float x1, float y1, float x2, float y2)
    {
        dest.push_back(x1);
        dest.push_back(y1);
        dest.push_back(x2);
        dest.push_back(y2);
    }

    std::unique_ptr<juce::OpenGLShaderProgram> colorShader_;
    GLint colorUniformLoc_ = -1;

    GLuint gridVAO_ = 0;
    GLuint gridVBO_ = 0;
    static constexpr size_t gridBufferCapacity_ = 4096; // Max vertices for grid lines

    // Reusable buffers to avoid per-frame allocation
    std::vector<float> minorLines_;
    std::vector<float> majorLines_;
    std::vector<float> zeroLines_;
    std::vector<float> timeMajorLines_;
    std::vector<float> timeMinorLines_;

    bool initialized_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GridRenderer)
};

} // namespace oscil
