/*
    Oscil - Chromatic Aberration Effect Implementation
*/

#include "rendering/effects/ChromaticAberrationEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// Chromatic aberration fragment shader
static const char* chromaticFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform float intensity;

    in vec2 vTexCoord;
    out vec4 fragColor;

    void main()
    {
        // Calculate vector from center
        vec2 dist = vTexCoord - 0.5;
        
        // Radial offset: increases with distance from center
        // Squared distance gives a nice lens curve
        float r2 = dot(dist, dist);
        vec2 offset = dist * (r2 * intensity * 2.0);

        // Spectral dispersion
        // R - G - B separation along the radial vector
        float r = texture(sourceTexture, vTexCoord - offset).r;
        float g = texture(sourceTexture, vTexCoord).g;
        float b = texture(sourceTexture, vTexCoord + offset).b;
        
        // Use center alpha
        float a = texture(sourceTexture, vTexCoord).a;

        fragColor = vec4(r, g, b, a);
    }
)";

ChromaticAberrationEffect::ChromaticAberrationEffect()
{
}

ChromaticAberrationEffect::~ChromaticAberrationEffect() = default;

bool ChromaticAberrationEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compileEffectShader(*shader_, chromaticFragmentShader))
    {
        DBG("ChromaticAberrationEffect: Failed to compile shader");
        shader_.reset();
        return false;
    }

    textureLoc_ = shader_->getUniformIDFromName("sourceTexture");
    intensityLoc_ = shader_->getUniformIDFromName("intensity");

    if (textureLoc_ < 0 || intensityLoc_ < 0)
    {
        DBG("ChromaticAberrationEffect: Missing uniforms");
        shader_.reset();
        return false;
    }

    compiled_ = true;
    DBG("ChromaticAberrationEffect: Compiled successfully");
    return true;
}

void ChromaticAberrationEffect::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    shader_.reset();
    compiled_ = false;
}

bool ChromaticAberrationEffect::isCompiled() const
{
    return compiled_;
}

void ChromaticAberrationEffect::apply(
    juce::OpenGLContext& context,
    Framebuffer* source,
    Framebuffer* destination,
    FramebufferPool& pool,
    float deltaTime)
{
    juce::ignoreUnused(deltaTime);

    if (!compiled_ || !source || !destination)
        return;

    // Save GL state for restoration (H24 FIX: also save blend function)
    GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLint blendSrcRGB, blendDstRGB, blendSrcAlpha, blendDstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);

    auto& ext = context.extensions;

    destination->bind();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    shader_->use();

    source->bindTexture(0);

    // H26 FIX: Add uniform location validation (even though compile() checks,
    // locations could become invalid after context changes)
    if (textureLoc_ >= 0)
        ext.glUniform1i(textureLoc_, 0);
    if (intensityLoc_ >= 0)
        ext.glUniform1f(intensityLoc_, settings_.intensity * intensity_);

    pool.renderFullscreenQuad();

    // Cleanup texture unit
    glBindTexture(GL_TEXTURE_2D, 0);
    destination->unbind();

    // Restore GL state (H24 FIX: also restore blend function)
    if (depthTestWasEnabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if (blendWasEnabled)
    {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(static_cast<GLenum>(blendSrcRGB),
                           static_cast<GLenum>(blendDstRGB),
                           static_cast<GLenum>(blendSrcAlpha),
                           static_cast<GLenum>(blendDstAlpha));
    }
    else
    {
        glDisable(GL_BLEND);
    }
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
