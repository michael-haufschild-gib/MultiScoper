/*
    Oscil - Post-Process Effect Base Class
    Abstract base for all post-processing effects
*/

#pragma once

#include "rendering/Framebuffer.h"
#include "rendering/FramebufferPool.h"

#include <juce_core/juce_core.h>

#include <atomic>

#if OSCIL_ENABLE_OPENGL
    #include <juce_opengl/juce_opengl.h>

namespace oscil
{

/**
 * Abstract base class for post-processing effects.
 * Effects operate on framebuffer textures using ping-pong rendering.
 */
class PostProcessEffect
{
public:
    virtual ~PostProcessEffect() = default;

    // Prevent copying and moving (effects manage GPU resources)
    PostProcessEffect(const PostProcessEffect&) = delete;
    PostProcessEffect& operator=(const PostProcessEffect&) = delete;
    PostProcessEffect(PostProcessEffect&&) = delete;
    PostProcessEffect& operator=(PostProcessEffect&&) = delete;

    /**
     * Get the unique identifier for this effect.
     */
    [[nodiscard]] virtual juce::String getId() const = 0;

    /**
     * Get human-readable name for UI display.
     */
    [[nodiscard]] virtual juce::String getDisplayName() const = 0;

    /**
     * Compile the effect's shader program.
     * @param context The OpenGL context
     * @return true if compilation succeeded
     */
    virtual bool compile(juce::OpenGLContext& context) = 0;

    /**
     * Release all GPU resources.
     * @param context The OpenGL context (must be active)
     */
    virtual void release(juce::OpenGLContext& context) = 0;

    /**
     * Check if the effect is compiled and ready.
     */
    [[nodiscard]] virtual bool isCompiled() const = 0;

    /**
     * Apply the effect to a source framebuffer, outputting to destination.
     *
     * @param context The OpenGL context
     * @param source The input framebuffer texture
     * @param destination The output framebuffer (bind before calling)
     * @param pool The framebuffer pool (for fullscreen quad rendering)
     * @param deltaTime Time since last frame in seconds (for animated effects)
     */
    virtual void apply(juce::OpenGLContext& context, Framebuffer* source, Framebuffer* destination,
                       FramebufferPool& pool, float deltaTime = 0.0f) = 0;

    /**
     * Set effect intensity/strength (0.0 = disabled, 1.0 = full effect).
     * Not all effects use this.
     */
    void setIntensity(float intensity)
    {
        intensity_.store(juce::jlimit(0.0f, 1.0f, intensity), std::memory_order_relaxed);
    }
    [[nodiscard]] float getIntensity() const { return intensity_.load(std::memory_order_relaxed); }

    /**
     * Enable or disable the effect.
     */
    void setEnabled(bool enabled) { enabled_.store(enabled, std::memory_order_relaxed); }
    [[nodiscard]] bool isEnabled() const { return enabled_.load(std::memory_order_relaxed); }

    /**
     * Configure the effect from a VisualConfiguration.
     * Override in derived classes to extract relevant settings.
     * Default implementation does nothing (for effects that configure via other means).
     * @param config The full visual configuration
     */
    virtual void configure(const struct VisualConfiguration& config) { juce::ignoreUnused(config); }

protected:
    PostProcessEffect() = default;

    /**
     * Helper to compile a shader program with fullscreen quad vertex shader.
     * @param program The shader program to compile into
     * @param fragmentSource The fragment shader source (legacy GLSL syntax)
     * @return true if compilation succeeded
     */
    static bool compileEffectShader(juce::OpenGLShaderProgram& program, const char* fragmentSource);

    /**
     * Standard fullscreen quad vertex shader source (legacy GLSL).
     * Provides position attribute (0) and texCoord attribute (1).
     * Outputs vTexCoord varying.
     */
    static const char* getFullscreenVertexShader();

    std::atomic<float> intensity_{1.0f};
    std::atomic<bool> enabled_{true};
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
