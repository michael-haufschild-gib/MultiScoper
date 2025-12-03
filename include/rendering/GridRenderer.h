/*
    Oscil - Grid Renderer
    Handles rendering of background grids
*/

#pragma once

#include <juce_opengl/juce_opengl.h>
#include "rendering/WaveformGLRenderer.h" // For WaveformRenderData

namespace oscil
{

class GridRenderer
{
public:
    GridRenderer();
    ~GridRenderer();

    void initialize(juce::OpenGLContext& context);
    void release(juce::OpenGLContext& context);
    void render(juce::OpenGLContext& context, const WaveformRenderData& data);

private:
    void createBuffers(juce::OpenGLContext& context);
    void compileShaders(juce::OpenGLContext& context);

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
