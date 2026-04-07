/*
    Oscil - Single Shader Effect Base Class
    Intermediate base for post-processing effects that use a single shader program
*/

#pragma once

#include "PostProcessEffect.h"

#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Base class for single-shader post-processing effects.
 * Handles the common compile/release/isCompiled lifecycle and provides
 * a shared sourceTexture uniform location.
 *
 * Subclasses implement:
 * - getFragmentSource(): return the GLSL fragment shader source
 * - resolveUniforms(): look up effect-specific uniform locations, return false if any missing
 * - setUniforms(): set effect-specific uniforms each frame (called from apply())
 */
class SingleShaderEffect : public PostProcessEffect
{
public:
    ~SingleShaderEffect() override = default;

    /// Compile the fragment shader and resolve all uniform locations.
    bool compile(juce::OpenGLContext& context) override;
    /// Release the shader program and reset compiled state.
    void release(juce::OpenGLContext& context) override;
    [[nodiscard]] bool isCompiled() const override;

    /**
     * Standard single-shader apply: binds destination, sets up GL state,
     * uses the shader, binds source texture, calls setUniforms(), renders
     * a fullscreen quad, and unbinds. Subclasses provide only setUniforms().
     */
    void apply(juce::OpenGLContext& context, Framebuffer* source, Framebuffer* destination, FramebufferPool& pool,
               float deltaTime) final;

protected:
    SingleShaderEffect() = default;

    /**
     * Return the fragment shader GLSL source for this effect.
     */
    [[nodiscard]] virtual const char* getFragmentSource() const = 0;

    /**
     * Look up effect-specific uniform locations from the compiled shader.
     * Called after the shader is compiled and sourceTexture is resolved.
     * @return false if any required uniforms are missing
     */
    virtual bool resolveUniforms() = 0;

    /**
     * Set effect-specific uniforms before the fullscreen quad is rendered.
     * Called after the shader is active and sourceTexture is bound to unit 0.
     * @param source The input framebuffer (for dimensions, etc.)
     * @param deltaTime Time since last frame in seconds
     */
    virtual void setUniforms(const Framebuffer& source, float deltaTime) = 0;

    std::unique_ptr<juce::OpenGLShaderProgram> shader_;
    GLint textureLoc_ = -1;
    bool compiled_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
