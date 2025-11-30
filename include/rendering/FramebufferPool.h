/*
    Oscil - Framebuffer Pool
    Manages shared FBOs for efficient memory usage and post-processing pipeline
*/

#pragma once

#include "Framebuffer.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Manages a pool of framebuffers for efficient rendering.
 * Provides FBOs for waveform rendering, ping-pong post-processing,
 * and a fullscreen quad for effect rendering.
 */
class FramebufferPool
{
public:
    FramebufferPool();
    ~FramebufferPool();

    /**
     * Initialize the pool with specified dimensions.
     * Creates all required FBOs.
     * @param context The OpenGL context
     * @param width Initial width
     * @param height Initial height
     * @return true if initialization succeeded
     */
    bool initialize(juce::OpenGLContext& context, int width, int height);

    /**
     * Release all resources.
     * @param context The OpenGL context (must be active)
     */
    void shutdown(juce::OpenGLContext& context);

    /**
     * Resize all FBOs to new dimensions.
     * @param context The OpenGL context
     * @param width New width
     * @param height New height
     */
    void resize(juce::OpenGLContext& context, int width, int height);

    /**
     * Get the waveform FBO (with depth buffer for 3D rendering).
     * Used for initial waveform geometry + particle rendering.
     */
    Framebuffer* getWaveformFBO() { return waveformFBO_.get(); }

    /**
     * Get the ping FBO for effect chain processing.
     * Ping-pong buffers are used for multi-pass effects.
     */
    Framebuffer* getPingFBO() { return pingFBO_.get(); }

    /**
     * Get the pong FBO for effect chain processing.
     */
    Framebuffer* getPongFBO() { return pongFBO_.get(); }

    /**
     * Render a fullscreen quad using the currently bound shader.
     * The quad covers normalized device coordinates [-1,1].
     * Useful for post-processing effects.
     */
    void renderFullscreenQuad();

    /**
     * Get current dimensions of the FBOs.
     */
    [[nodiscard]] int getWidth() const { return width_; }
    [[nodiscard]] int getHeight() const { return height_; }

    /**
     * Check if the pool is initialized and ready for use.
     */
    [[nodiscard]] bool isInitialized() const { return initialized_; }

    // Prevent copying
    FramebufferPool(const FramebufferPool&) = delete;
    FramebufferPool& operator=(const FramebufferPool&) = delete;

private:
    bool createFullscreenQuad(juce::OpenGLContext& context);
    void destroyFullscreenQuad(juce::OpenGLContext& context);

    std::unique_ptr<Framebuffer> waveformFBO_;
    std::unique_ptr<Framebuffer> pingFBO_;
    std::unique_ptr<Framebuffer> pongFBO_;

    // Fullscreen quad for post-processing
    GLuint quadVAO_ = 0;
    GLuint quadVBO_ = 0;

    juce::OpenGLContext* context_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    bool initialized_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
