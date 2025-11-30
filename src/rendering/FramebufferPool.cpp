/*
    Oscil - Framebuffer Pool Implementation
*/

#include "rendering/FramebufferPool.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

FramebufferPool::FramebufferPool()
    : waveformFBO_(std::make_unique<Framebuffer>())
    , pingFBO_(std::make_unique<Framebuffer>())
    , pongFBO_(std::make_unique<Framebuffer>())
{
}

FramebufferPool::~FramebufferPool()
{
    // Note: shutdown() should be called explicitly before destruction
    // while OpenGL context is still active
}

bool FramebufferPool::initialize(juce::OpenGLContext& context, int width, int height)
{
    if (initialized_)
        shutdown(context);

    context_ = &context;
    width_ = width;
    height_ = height;

    // Create waveform FBO with depth buffer (for 3D rendering) and HDR format
    // Enable depth texture for post-processing effects like Depth of Field
    if (!waveformFBO_->create(context, width, height, GL_RGBA16F, true, true))
    {
        DBG("FramebufferPool: Failed to create waveform FBO");
        return false;
    }

    // Create ping FBO for effect chain (HDR, no depth)
    if (!pingFBO_->create(context, width, height, GL_RGBA16F, false))
    {
        DBG("FramebufferPool: Failed to create ping FBO");
        waveformFBO_->destroy(context);
        return false;
    }

    // Create pong FBO for effect chain (HDR, no depth)
    if (!pongFBO_->create(context, width, height, GL_RGBA16F, false))
    {
        DBG("FramebufferPool: Failed to create pong FBO");
        waveformFBO_->destroy(context);
        pingFBO_->destroy(context);
        return false;
    }

    // Create fullscreen quad for effect rendering
    if (!createFullscreenQuad(context))
    {
        DBG("FramebufferPool: Failed to create fullscreen quad");
        shutdown(context);
        return false;
    }

    initialized_ = true;
    DBG("FramebufferPool: Initialized " << width << "x" << height);
    return true;
}

void FramebufferPool::shutdown(juce::OpenGLContext& context)
{
    if (!initialized_)
        return;

    destroyFullscreenQuad(context);

    if (waveformFBO_)
        waveformFBO_->destroy(context);
    if (pingFBO_)
        pingFBO_->destroy(context);
    if (pongFBO_)
        pongFBO_->destroy(context);

    context_ = nullptr;
    width_ = 0;
    height_ = 0;
    initialized_ = false;

    DBG("FramebufferPool: Shutdown complete");
}

void FramebufferPool::resize(juce::OpenGLContext& context, int width, int height)
{
    if (width == width_ && height == height_)
        return;

    width_ = width;
    height_ = height;

    if (waveformFBO_)
        waveformFBO_->resize(context, width, height);
    if (pingFBO_)
        pingFBO_->resize(context, width, height);
    if (pongFBO_)
        pongFBO_->resize(context, width, height);

    DBG("FramebufferPool: Resized to " << width << "x" << height);
}

bool FramebufferPool::createFullscreenQuad(juce::OpenGLContext& context)
{
    auto& ext = context.extensions;

    // Fullscreen quad vertices: position (x,y) and texcoord (u,v)
    // Covers NDC space [-1, 1] for position, [0, 1] for texcoords
    static const float quadVertices[] = {
        // Position    TexCoord
        -1.0f,  1.0f,  0.0f, 1.0f,   // Top-left
        -1.0f, -1.0f,  0.0f, 0.0f,   // Bottom-left
         1.0f, -1.0f,  1.0f, 0.0f,   // Bottom-right

        -1.0f,  1.0f,  0.0f, 1.0f,   // Top-left
         1.0f, -1.0f,  1.0f, 0.0f,   // Bottom-right
         1.0f,  1.0f,  1.0f, 1.0f    // Top-right
    };

    // Create VAO
    ext.glGenVertexArrays(1, &quadVAO_);
    if (quadVAO_ == 0)
    {
        DBG("FramebufferPool: Failed to create quad VAO");
        return false;
    }

    // Create VBO
    ext.glGenBuffers(1, &quadVBO_);
    if (quadVBO_ == 0)
    {
        ext.glDeleteVertexArrays(1, &quadVAO_);
        quadVAO_ = 0;
        DBG("FramebufferPool: Failed to create quad VBO");
        return false;
    }

    // Bind and upload data
    ext.glBindVertexArray(quadVAO_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    ext.glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Set up vertex attributes
    // Attribute 0: position (vec2)
    ext.glEnableVertexAttribArray(0);
    ext.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

    // Attribute 1: texcoord (vec2)
    ext.glEnableVertexAttribArray(1);
    ext.glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                              reinterpret_cast<void*>(2 * sizeof(float)));

    // Unbind
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    ext.glBindVertexArray(0);

    return true;
}

void FramebufferPool::destroyFullscreenQuad(juce::OpenGLContext& context)
{
    auto& ext = context.extensions;

    if (quadVBO_ != 0)
    {
        ext.glDeleteBuffers(1, &quadVBO_);
        quadVBO_ = 0;
    }

    if (quadVAO_ != 0)
    {
        ext.glDeleteVertexArrays(1, &quadVAO_);
        quadVAO_ = 0;
    }
}

void FramebufferPool::renderFullscreenQuad()
{
    if (quadVAO_ == 0 || context_ == nullptr)
        return;

    auto& ext = context_->extensions;

    ext.glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ext.glBindVertexArray(0);
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
