/*
    Oscil - Trails Effect Implementation
*/

#include "rendering/effects/TrailsEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// Trails shader - blends current frame with history
static const char* trailsFragmentShader = R"(
    #version 330 core
    uniform sampler2D currentTexture;
    uniform sampler2D historyTexture;
    uniform float decay;
    uniform float opacity;

    in vec2 vTexCoord;
    out vec4 fragColor;

    void main()
    {
        vec4 current = texture(currentTexture, vTexCoord);
        vec4 history = texture(historyTexture, vTexCoord);

        // Apply decay to history
        // We fade alpha and rgb
        vec4 fadedHistory = history * (1.0 - decay);

        // Combine: current on top of history
        // Standard alpha blending: src + dst * (1 - src.a)
        // But here we want max brightness or additive accumulation?
        // Let's use max for trails to preserve structure
        
        vec4 result = max(current, fadedHistory);
        
        // Apply master opacity
        result.a *= opacity;

        fragColor = result;
    }
)";

TrailsEffect::TrailsEffect()
{
}

TrailsEffect::~TrailsEffect() = default;

bool TrailsEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compileEffectShader(*shader_, trailsFragmentShader))
    {
        DBG("TrailsEffect: Failed to compile shader");
        shader_.reset();
        return false;
    }

    currentTextureLoc_ = shader_->getUniformIDFromName("currentTexture");
    historyTextureLoc_ = shader_->getUniformIDFromName("historyTexture");
    decayLoc_ = shader_->getUniformIDFromName("decay");
    opacityLoc_ = shader_->getUniformIDFromName("opacity");

    if (currentTextureLoc_ < 0 || historyTextureLoc_ < 0 ||
        decayLoc_ < 0 || opacityLoc_ < 0)
    {
        DBG("TrailsEffect: Missing uniforms");
        shader_.reset();
        return false;
    }

    compiled_ = true;
    DBG("TrailsEffect: Compiled successfully");
    return true;
}

void TrailsEffect::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    shader_.reset();
    compiled_ = false;
}

bool TrailsEffect::isCompiled() const
{
    return compiled_;
}

void TrailsEffect::apply(
    juce::OpenGLContext& context,
    Framebuffer* source,
    Framebuffer* destination,
    FramebufferPool& pool,
    float deltaTime)
{
    // This version doesn't have access to history - requires applyWithHistory
    // Just copy source to destination as fallback
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

    // Simple passthrough since we don't have history
    // NOTE: We bind source texture to unit 0 only and use the same unit for both
    // samplers. Binding the same texture to multiple units simultaneously can
    // cause undefined behavior on some drivers. Since this is a fallback path
    // with full decay (no trails), we use the same texture unit for both.
    shader_->use();
    source->bindTexture(0);
    ext.glUniform1i(currentTextureLoc_, 0);
    ext.glUniform1i(historyTextureLoc_, 0);  // Same unit - both samplers read from source
    ext.glUniform1f(decayLoc_, 1.0f); // Full decay = no trails
    ext.glUniform1f(opacityLoc_, 1.0f);

    pool.renderFullscreenQuad();

    // Cleanup texture unit (only unit 0 used in fallback path)
    glActiveTexture(GL_TEXTURE0);
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

void TrailsEffect::applyWithHistory(
    juce::OpenGLContext& context,
    Framebuffer* source,
    Framebuffer* history,
    Framebuffer* destination,
    FramebufferPool& pool,
    float deltaTime)
{
    juce::ignoreUnused(deltaTime);

    if (!compiled_ || !source || !history || !destination)
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

    // Bind current frame
    source->bindTexture(0);
    if (currentTextureLoc_ >= 0)
        ext.glUniform1i(currentTextureLoc_, 0);

    // Bind history
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, history->colorTexture);
    if (historyTextureLoc_ >= 0)
        ext.glUniform1i(historyTextureLoc_, 1);

    if (decayLoc_ >= 0)
        ext.glUniform1f(decayLoc_, settings_.decay * intensity_);
    if (opacityLoc_ >= 0)
        ext.glUniform1f(opacityLoc_, settings_.opacity);

    pool.renderFullscreenQuad();

    // Cleanup texture units
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
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

    // Copy result back to history for next frame
    // This is done by the RenderEngine after all processing
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
