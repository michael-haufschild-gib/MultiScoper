/*
    Oscil - Framebuffer
    OpenGL framebuffer wrapper for off-screen rendering
*/

#pragma once

#include <juce_core/juce_core.h>

#if OSCIL_ENABLE_OPENGL
#include <juce_opengl/juce_opengl.h>

namespace oscil
{

using namespace juce::gl;

/**
 * OpenGL framebuffer wrapper for off-screen rendering.
 * Supports color attachment and optional depth buffer for 3D rendering.
 */
struct Framebuffer
{
    GLuint fbo = 0;
    GLuint colorTexture = 0;
    GLuint depthBuffer = 0;
    int width = 0;
    int height = 0;
    GLenum format = GL_RGBA8;
    bool hasDepth = false;

    /**
     * Create the framebuffer with specified dimensions and format.
     * @param context The OpenGL context
     * @param w Width in pixels
     * @param h Height in pixels
     * @param fmt Texture format (GL_RGBA8, GL_RGBA16F, etc.)
     * @param withDepth Whether to create a depth buffer (required for 3D rendering)
     * @return true if creation succeeded
     */
    bool create(juce::OpenGLContext& context, int w, int h, GLenum fmt = GL_RGBA8, bool withDepth = false);

    /**
     * Destroy the framebuffer and release all resources.
     * @param context The OpenGL context (must be active)
     */
    void destroy(juce::OpenGLContext& context);

    /**
     * Resize the framebuffer to new dimensions.
     * Destroys and recreates internal textures.
     * @param context The OpenGL context
     * @param w New width
     * @param h New height
     */
    void resize(juce::OpenGLContext& context, int w, int h);

    /**
     * Bind this framebuffer for rendering.
     * All subsequent draw calls will render to this FBO.
     */
    void bind();

    /**
     * Unbind this framebuffer, returning to the default framebuffer.
     */
    void unbind();

    /**
     * Bind the color texture for sampling in a shader.
     * @param textureUnit The texture unit to bind to (0 = GL_TEXTURE0)
     */
    void bindTexture(int textureUnit = 0);

    /**
     * Check if the framebuffer is valid and ready for use.
     */
    [[nodiscard]] bool isValid() const { return fbo != 0 && colorTexture != 0; }

    /**
     * Clear the framebuffer with the specified color.
     * Must call bind() first.
     * @param colour The clear color
     * @param clearDepth Whether to also clear the depth buffer
     */
    void clear(juce::Colour colour, bool clearDepth = true);

private:
    bool createColorTexture(juce::OpenGLContext& context);
    bool createDepthBuffer(juce::OpenGLContext& context);
    bool checkFramebufferComplete();
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
