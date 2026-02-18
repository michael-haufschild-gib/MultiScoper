/*
    Oscil - Framebuffer Pool Implementation
*/

#include "rendering/FramebufferPool.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// ============================================================================
// VertexBufferPool Implementation
// ============================================================================

VertexBufferPool::VertexBufferPool()
{
}

VertexBufferPool::~VertexBufferPool()
{
    if (initialized_)
    {
        juce::Logger::writeToLog("[VertexBufferPool] LEAK: Destructor called without shutdown(). GPU resources may have leaked.");
        jassertfalse;
    }
}

bool VertexBufferPool::initialize(juce::OpenGLContext& context)
{
    if (initialized_)
        shutdown(context);

    context_ = &context;
    auto& ext = context.extensions;

    // Create VAO
    ext.glGenVertexArrays(1, &vao_);
    if (vao_ == 0)
    {
        juce::Logger::writeToLog("VertexBufferPool: Failed to create VAO");
        return false;
    }

    // Create VBO with pre-allocated capacity
    ext.glGenBuffers(1, &vbo_);
    if (vbo_ == 0)
    {
        ext.glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
        juce::Logger::writeToLog("VertexBufferPool: Failed to create VBO");
        return false;
    }

    // Pre-allocate buffer storage
    ext.glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(MAX_BUFFER_SIZE), nullptr, GL_DYNAMIC_DRAW);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);

    currentOffset_ = 0;
    initialized_ = true;
    juce::Logger::writeToLog("VertexBufferPool: Initialized with " + juce::String(MAX_BUFFER_SIZE) + " bytes capacity");
    return true;
}

void VertexBufferPool::shutdown(juce::OpenGLContext& context)
{
    if (!initialized_)
        return;

    auto& ext = context.extensions;

    if (vbo_ != 0)
    {
        ext.glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }

    if (vao_ != 0)
    {
        ext.glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    context_ = nullptr;
    currentOffset_ = 0;
    initialized_ = false;
    juce::Logger::writeToLog("VertexBufferPool: Shutdown complete");
}

void VertexBufferPool::reset()
{
    currentOffset_ = 0;
}

ptrdiff_t VertexBufferPool::allocate(const float* data, size_t vertexCount)
{
    if (!initialized_ || context_ == nullptr || data == nullptr || vertexCount == 0)
        return -1;

    size_t requiredBytes = vertexCount * BYTES_PER_VERTEX;
    if (currentOffset_ + requiredBytes > MAX_BUFFER_SIZE)
    {
        // Buffer full, cannot allocate
        juce::Logger::writeToLog("VertexBufferPool: Allocation failed, buffer full");
        return -1;
    }

    auto& ext = context_->extensions;
    ptrdiff_t offset = static_cast<ptrdiff_t>(currentOffset_);

    // Upload data to the buffer at the current offset
    // Note: Assumes VBO is already bound via bind() - for batch efficiency
    ext.glBufferSubData(GL_ARRAY_BUFFER, 
                        static_cast<GLintptr>(currentOffset_),
                        static_cast<GLsizeiptr>(requiredBytes),
                        data);

    currentOffset_ += requiredBytes;
    return offset;
}

void VertexBufferPool::bind()
{
    if (!initialized_ || context_ == nullptr)
        return;

    auto& ext = context_->extensions;
    ext.glBindVertexArray(vao_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, vbo_);
}

void VertexBufferPool::unbind()
{
    if (!initialized_ || context_ == nullptr)
        return;

    auto& ext = context_->extensions;
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    ext.glBindVertexArray(0);
}

// ============================================================================
// FramebufferPool Implementation
// ============================================================================

FramebufferPool::FramebufferPool()
{
}

FramebufferPool::~FramebufferPool()
{
    if (initialized_)
    {
        juce::Logger::writeToLog("[FramebufferPool] LEAK: Destructor called without shutdown(). GPU resources may have leaked.");
        jassertfalse;
    }
}

std::unique_ptr<Framebuffer> FramebufferPool::createFramebuffer()
{
    return std::make_unique<Framebuffer>();
}

bool FramebufferPool::initialize(juce::OpenGLContext& context, int width, int height)
{
    if (initialized_)
        shutdown(context);

    context_ = &context;
    width_ = width;
    height_ = height;

    // Ensure FBOs are created
    if (!waveformFBO_) waveformFBO_ = createFramebuffer();
    if (!pingFBO_) pingFBO_ = createFramebuffer();
    if (!pongFBO_) pongFBO_ = createFramebuffer();

    // Create waveform FBO with depth buffer (for 3D rendering) and HDR format
    // Enable depth texture for post-processing effects like Depth of Field
    if (!waveformFBO_->create(context, width, height, 0, GL_RGBA16F, true, true))
    {
        juce::Logger::writeToLog("FramebufferPool: Failed to create waveform FBO");
        return false;
    }

    // Create ping FBO for effect chain (HDR, no depth)
    if (!pingFBO_->create(context, width, height, 0, GL_RGBA16F, false))
    {
        juce::Logger::writeToLog("FramebufferPool: Failed to create ping FBO");
        waveformFBO_->destroy(context);
        return false;
    }

    // Create pong FBO for effect chain (HDR, no depth)
    if (!pongFBO_->create(context, width, height, 0, GL_RGBA16F, false))
    {
        juce::Logger::writeToLog("FramebufferPool: Failed to create pong FBO");
        waveformFBO_->destroy(context);
        pingFBO_->destroy(context);
        return false;
    }

    // Create fullscreen quad for effect rendering
    if (!createFullscreenQuad(context))
    {
        juce::Logger::writeToLog("FramebufferPool: Failed to create fullscreen quad");
        shutdown(context);
        return false;
    }

    // Create vertex buffer pool for batched geometry
    if (!createVertexBufferPool(context))
    {
        juce::Logger::writeToLog("FramebufferPool: Failed to create vertex buffer pool");
        shutdown(context);
        return false;
    }

    initialized_ = true;
    juce::Logger::writeToLog("FramebufferPool: Initialized " + juce::String(width) + "x" + juce::String(height));
    return true;
}

void FramebufferPool::shutdown(juce::OpenGLContext& context)
{
    if (!initialized_)
        return;

    // Shutdown vertex buffer pool first
    destroyVertexBufferPool(context);

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

    juce::Logger::writeToLog("FramebufferPool: Shutdown complete");
}

void FramebufferPool::resize(juce::OpenGLContext& context, int width, int height)
{
    if (width == width_ && height == height_)
        return;

    width_ = width;
    height_ = height;

    // Resize all FBOs - if any fails, the FBO will be invalid (caller should check isValid())
    bool resizeFailed = false;
    if (waveformFBO_)
    {
        if (!waveformFBO_->resize(context, width, height))
        {
            juce::Logger::writeToLog("FramebufferPool: waveformFBO resize failed");
            resizeFailed = true;
        }
    }
    if (pingFBO_)
    {
        if (!pingFBO_->resize(context, width, height))
        {
            juce::Logger::writeToLog("FramebufferPool: pingFBO resize failed");
            resizeFailed = true;
        }
    }
    if (pongFBO_)
    {
        if (!pongFBO_->resize(context, width, height))
        {
            juce::Logger::writeToLog("FramebufferPool: pongFBO resize failed");
            resizeFailed = true;
        }
    }

    if (resizeFailed)
    {
        juce::Logger::writeToLog("FramebufferPool: One or more FBOs failed to resize to " + juce::String(width) + "x" + juce::String(height));
    }

    juce::Logger::writeToLog("FramebufferPool: Resized to " + juce::String(width) + "x" + juce::String(height));
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
        juce::Logger::writeToLog("FramebufferPool: Failed to create quad VAO");
        return false;
    }

    // Create VBO
    ext.glGenBuffers(1, &quadVBO_);
    if (quadVBO_ == 0)
    {
        ext.glDeleteVertexArrays(1, &quadVAO_);
        quadVAO_ = 0;
        juce::Logger::writeToLog("FramebufferPool: Failed to create quad VBO");
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

bool FramebufferPool::createVertexBufferPool(juce::OpenGLContext& context)
{
    vertexBufferPool_ = std::make_unique<VertexBufferPool>();
    if (!vertexBufferPool_->initialize(context))
    {
        vertexBufferPool_.reset();
        return false;
    }
    return true;
}

void FramebufferPool::destroyVertexBufferPool(juce::OpenGLContext& context)
{
    if (vertexBufferPool_)
    {
        vertexBufferPool_->shutdown(context);
        vertexBufferPool_.reset();
    }
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
