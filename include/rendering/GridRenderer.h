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

    bool initialized_ = false;
};

} // namespace oscil