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
    virtual ~Framebuffer() = default;

    GLuint fbo = 0;
    GLuint colorTexture = 0;
    GLuint colorRenderbuffer = 0; // For MSAA
    GLuint depthBuffer = 0;
    GLuint depthTexture = 0;
    int width = 0;
    int height = 0;
    int numSamples = 0;
    GLenum format = GL_RGBA8;
    bool hasDepth = false;
    bool hasDepthTexture = false;

    /**
     * Create the framebuffer with specified dimensions and format.
     * @param context The OpenGL context
     * @param w Width in pixels
     * @param h Height in pixels
     * @param samples Number of MSAA samples (0 = disable MSAA)
     * @param fmt Texture format (GL_RGBA8, GL_RGBA16F, etc.)
     * @param withDepth Whether to create a depth buffer (required for 3D rendering)
     * @param useDepthTexture Whether to create depth as a sampleable texture (for DoF, etc.) instead of Renderbuffer
     * @return true if creation succeeded
     */
    virtual bool create(juce::OpenGLContext& context, int w, int h, int samples = 0, GLenum fmt = GL_RGBA8, bool withDepth = false, bool useDepthTexture = false);

    /**
     * Destroy the framebuffer and release all resources.
     * @param context The OpenGL context (must be active)
     */
    virtual void destroy(juce::OpenGLContext& context);

    /**
     * Resize the framebuffer to new dimensions.
     * Destroys and recreates internal textures.
     * @param context The OpenGL context
     * @param w New width
     * @param h New height
     */
    virtual void resize(juce::OpenGLContext& context, int w, int h);

    /**
     * Blit (copy/resolve) this framebuffer to another framebuffer.
     * Used to resolve MSAA buffers to standard textures.
     * @param context The OpenGL context
     * @param dest The destination framebuffer (can be nullptr for default framebuffer)
     */
    virtual void blitTo(juce::OpenGLContext& context, Framebuffer* dest);

    /**
     * Bind this framebuffer for rendering.
     * All subsequent draw calls will render to this FBO.
     */
    virtual void bind();

    /**
     * Unbind this framebuffer, returning to the default framebuffer.
     */
    virtual void unbind();

    /**
     * Bind the color texture for sampling in a shader.
     * Fails if this is an MSAA framebuffer (must resolve first).
     * @param textureUnit The texture unit to bind to (0 = GL_TEXTURE0)
     */
    virtual void bindTexture(int textureUnit = 0);

    /**
     * Bind the depth texture for sampling in a shader (if created with useDepthTexture=true).
     * @param textureUnit The texture unit to bind to
     */
    virtual void bindDepthTexture(int textureUnit = 1);

    /**
     * Check if the framebuffer is valid and ready for use.
     */
    [[nodiscard]] virtual bool isValid() const { return fbo != 0; }

    /**
     * Clear the framebuffer with the specified color.
     * Must call bind() first.
     * @param colour The clear color
     * @param clearDepth Whether to also clear the depth buffer
     */
    virtual void clear(juce::Colour colour, bool clearDepth = true);

private:
    bool createColorTexture(juce::OpenGLContext& context);
    bool createDepthBuffer(juce::OpenGLContext& context);
    bool checkFramebufferComplete();
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
