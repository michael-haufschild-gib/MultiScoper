/*
    Oscil - Render Bootstrapper
    Handles OpenGL context initialization and global shader compilation.
*/

#pragma once

#include <juce_opengl/juce_opengl.h>

#include <memory>

namespace oscil
{

class RenderBootstrapper
{
public:
    /// Create an uninitialized render bootstrapper.
    RenderBootstrapper();
    ~RenderBootstrapper();

    /**
     * Initialize global resources (blit/composite shaders).
     */
    bool initialize(juce::OpenGLContext& context);

    /**
     * Release resources.
     */
    void shutdown(juce::OpenGLContext& context);

    juce::OpenGLShaderProgram* getBlitShader() const { return blitShader_.get(); }
    juce::OpenGLShaderProgram* getCompositeShader() const { return compositeShader_.get(); }

    GLint getBlitTextureLoc() const { return blitTextureLoc_; }
    GLint getCompositeTextureLoc() const { return compositeTextureLoc_; }

private:
    std::unique_ptr<juce::OpenGLShaderProgram> blitShader_;
    std::unique_ptr<juce::OpenGLShaderProgram> compositeShader_;

    GLint blitTextureLoc_ = -1;
    GLint compositeTextureLoc_ = -1;
};

} // namespace oscil
