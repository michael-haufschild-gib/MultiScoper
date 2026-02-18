/*
    Oscil - Framebuffer Pool
    Manages shared FBOs for efficient memory usage and post-processing pipeline
*/

#pragma once

#include "Framebuffer.h"
#include <memory>
#include <vector>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

// ============================================================================
// Vertex Buffer Pool for Batched Geometry
// ============================================================================

/**
 * Pool of pre-allocated vertex buffer storage for batched waveform rendering.
 * Reduces VBO allocations and bind calls by sub-allocating from a large buffer.
 *
 * THREAD SAFETY (H12):
 * This class is designed to be used EXCLUSIVELY from the OpenGL rendering thread.
 * It holds a raw pointer to the OpenGL context and makes GL calls directly.
 * Do not access from UI thread or audio thread.
 *
 * Usage:
 * 1. Call reset() at the start of each frame
 * 2. Call allocate() for each waveform's geometry
 * 3. Call bind() once before rendering all waveforms
 * 4. Use getOffset() to set glVertexAttribPointer offsets
 */
class VertexBufferPool
{
public:
    // Maximum vertices per frame (256KB at 4 floats per vertex * 4 bytes)
    static constexpr size_t MAX_VERTICES = 65536;
    static constexpr size_t FLOATS_PER_VERTEX = 4;  // x, y, dist, padding
    static constexpr size_t BYTES_PER_VERTEX = FLOATS_PER_VERTEX * sizeof(float);
    static constexpr size_t MAX_BUFFER_SIZE = MAX_VERTICES * BYTES_PER_VERTEX;

    VertexBufferPool();
    ~VertexBufferPool();

    /**
     * Initialize the VBO pool.
     * @param context The OpenGL context
     * @return true if initialization succeeded
     */
    bool initialize(juce::OpenGLContext& context);

    /**
     * Release GPU resources.
     * @param context The OpenGL context (must be active)
     */
    void shutdown(juce::OpenGLContext& context);

    /**
     * Reset the pool for a new frame.
     * Call at the start of each batch.
     */
    void reset();

    /**
     * Allocate space for vertices and return the offset.
     * Data is uploaded immediately.
     * @param data Pointer to vertex data
     * @param vertexCount Number of vertices to allocate
     * @return Offset in bytes from buffer start, or -1 if allocation failed
     */
    ptrdiff_t allocate(const float* data, size_t vertexCount);

    /**
     * Bind the pooled VBO for rendering.
     */
    void bind();

    /**
     * Unbind the pooled VBO.
     */
    void unbind();

    /**
     * Get the VAO for the pool.
     */
    [[nodiscard]] GLuint getVAO() const { return vao_; }

    /**
     * Get the VBO for the pool.
     */
    [[nodiscard]] GLuint getVBO() const { return vbo_; }

    /**
     * Check if the pool is initialized.
     */
    [[nodiscard]] bool isInitialized() const { return initialized_; }

    /**
     * Get remaining capacity in vertices.
     */
    [[nodiscard]] size_t getRemainingCapacity() const 
    {
        return MAX_VERTICES - currentOffset_ / BYTES_PER_VERTEX;
    }

    // Prevent copying
    VertexBufferPool(const VertexBufferPool&) = delete;
    VertexBufferPool& operator=(const VertexBufferPool&) = delete;

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    juce::OpenGLContext* context_ = nullptr;
    size_t currentOffset_ = 0;  // Current offset in bytes
    bool initialized_ = false;
};

// ============================================================================
// Framebuffer Pool
// ============================================================================

/**
 * Manages a pool of framebuffers for efficient rendering.
 * Provides FBOs for waveform rendering, ping-pong post-processing,
 * and a fullscreen quad for effect rendering.
 *
 * THREAD SAFETY (H12):
 * This class is designed to be used EXCLUSIVELY from the OpenGL rendering thread.
 * It holds a raw pointer to the OpenGL context and makes GL calls directly.
 * - initialize(), shutdown(), resize() must be called from the OpenGL thread
 * - All FBO access (getWaveformFBO, getPingFBO, getPongFBO) must be from OpenGL thread
 * - renderFullscreenQuad() makes GL draw calls and must be from OpenGL thread
 * Do not access from UI thread or audio thread.
 */
class FramebufferPool
{
public:
    FramebufferPool();
    virtual ~FramebufferPool();

    /**
     * Initialize the pool with specified dimensions.
     * Creates all required FBOs.
     * @param context The OpenGL context
     * @param width Initial width
     * @param height Initial height
     * @return true if initialization succeeded
     */
    virtual bool initialize(juce::OpenGLContext& context, int width, int height);

    /**
     * Release all resources.
     * @param context The OpenGL context (must be active)
     */
    virtual void shutdown(juce::OpenGLContext& context);

    /**
     * Resize all FBOs to new dimensions.
     * @param context The OpenGL context
     * @param width New width
     * @param height New height
     */
    virtual void resize(juce::OpenGLContext& context, int width, int height);

    /**
     * Get the waveform FBO (with depth buffer for 3D rendering).
     * Used for initial waveform geometry rendering.
     */
    virtual Framebuffer* getWaveformFBO() const { return waveformFBO_.get(); }

    /**
     * Get the ping FBO for effect chain processing.
     * Ping-pong buffers are used for multi-pass effects.
     */
    virtual Framebuffer* getPingFBO() const { return pingFBO_.get(); }

    /**
     * Get the pong FBO for effect chain processing.
     */
    virtual Framebuffer* getPongFBO() const { return pongFBO_.get(); }

    /**
     * Render a fullscreen quad using the currently bound shader.
     * The quad covers normalized device coordinates [-1,1].
     * Useful for post-processing effects.
     */
    virtual void renderFullscreenQuad();

    /**
     * Get current dimensions of the FBOs.
     */
    [[nodiscard]] int getWidth() const { return width_; }
    [[nodiscard]] int getHeight() const { return height_; }

    /**
     * Check if the pool is initialized and ready for use.
     */
    [[nodiscard]] bool isInitialized() const { return initialized_; }

    // ========================================================================
    // Vertex Buffer Pool Access
    // ========================================================================

    /**
     * Get the vertex buffer pool for batched geometry.
     */
    VertexBufferPool* getVertexBufferPool() const { return vertexBufferPool_.get(); }

    // Prevent copying
    FramebufferPool(const FramebufferPool&) = delete;
    FramebufferPool& operator=(const FramebufferPool&) = delete;

protected:
    /**
     * Factory method to create a new Framebuffer instance.
     * Override for testing.
     */
    virtual std::unique_ptr<Framebuffer> createFramebuffer();

    /**
     * Create the fullscreen quad resources.
     * Virtual for testing.
     */
    virtual bool createFullscreenQuad(juce::OpenGLContext& context);

    /**
     * Destroy the fullscreen quad resources.
     * Virtual for testing.
     */
    virtual void destroyFullscreenQuad(juce::OpenGLContext& context);

    /**
     * Create the vertex buffer pool.
     * Virtual for testing - override to skip in unit tests without GL context.
     */
    virtual bool createVertexBufferPool(juce::OpenGLContext& context);

    /**
     * Destroy the vertex buffer pool.
     * Virtual for testing.
     */
    virtual void destroyVertexBufferPool(juce::OpenGLContext& context);

private:
    std::unique_ptr<Framebuffer> waveformFBO_;
    std::unique_ptr<Framebuffer> pingFBO_;
    std::unique_ptr<Framebuffer> pongFBO_;

    // Fullscreen quad for post-processing
    GLuint quadVAO_ = 0;
    GLuint quadVBO_ = 0;

    // Vertex buffer pool for batched geometry
    std::unique_ptr<VertexBufferPool> vertexBufferPool_;

    juce::OpenGLContext* context_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    bool initialized_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
