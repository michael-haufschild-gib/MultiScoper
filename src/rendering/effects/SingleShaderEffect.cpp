/*
    Oscil - Single Shader Effect Base Class Implementation
*/

#include "rendering/effects/SingleShaderEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

bool SingleShaderEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    // Bail out of permanent-failure state to prevent per-frame retry loops;
    // release() is the reset point if a new GL context is initialized later.
    if (hasCompileFailedPermanently())
        return false;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compileEffectShader(*shader_, getFragmentSource()))
    {
        DBG(getId() + ": Failed to compile shader");
        shader_.reset();
        markCompileFailed();
        return false;
    }

    textureLoc_ = shader_->getUniformIDFromName("sourceTexture");

    if (textureLoc_ < 0 || !resolveUniforms())
    {
        DBG(getId() + ": Missing uniforms");
        shader_.reset();
        markCompileFailed();
        return false;
    }

    compiled_ = true;
    return true;
}

void SingleShaderEffect::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    shader_.reset();
    compiled_ = false;
    resetCompileFailed();
}

bool SingleShaderEffect::isCompiled() const { return compiled_; }

void SingleShaderEffect::apply(juce::OpenGLContext& context, Framebuffer* source, Framebuffer* destination,
                               FramebufferPool& pool, float deltaTime)
{
    juce::ignoreUnused(context);

    if (!compiled_ || source == nullptr || destination == nullptr)
        return;

    destination->bind();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    shader_->use();
    source->bindTexture(0);
    juce::OpenGLExtensionFunctions::glUniform1i(textureLoc_, 0);

    setUniforms(*source, deltaTime);

    pool.renderFullscreenQuad();
    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
