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
    uniform sampler2D currentTexture;
    uniform sampler2D historyTexture;
    uniform float decay;
    uniform float opacity;

    varying vec2 vTexCoord;

    void main()
    {
        vec4 current = texture2D(currentTexture, vTexCoord);
        vec4 history = texture2D(historyTexture, vTexCoord);

        // Fade history based on decay rate
        vec4 fadedHistory = history * (1.0 - decay);

        // Blend current over faded history
        // Using over compositing: C = Ca * Aa + Cb * (1 - Aa)
        vec3 blended = current.rgb * current.a + fadedHistory.rgb * (1.0 - current.a);
        float alpha = current.a + fadedHistory.a * (1.0 - current.a);

        // Apply opacity
        gl_FragColor = vec4(blended, alpha * opacity);
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

    auto& ext = context.extensions;

    destination->bind();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // Simple passthrough since we don't have history
    shader_->use();
    source->bindTexture(0);
    ext.glUniform1i(currentTextureLoc_, 0);
    source->bindTexture(1); // Use source as history too (no effect)
    ext.glUniform1i(historyTextureLoc_, 1);
    ext.glUniform1f(decayLoc_, 1.0f); // Full decay = no trails
    ext.glUniform1f(opacityLoc_, 1.0f);

    pool.renderFullscreenQuad();
    destination->unbind();
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

    auto& ext = context.extensions;

    destination->bind();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    shader_->use();

    // Bind current frame
    source->bindTexture(0);
    ext.glUniform1i(currentTextureLoc_, 0);

    // Bind history
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, history->colorTexture);
    ext.glUniform1i(historyTextureLoc_, 1);

    ext.glUniform1f(decayLoc_, settings_.decay * intensity_);
    ext.glUniform1f(opacityLoc_, settings_.opacity);

    pool.renderFullscreenQuad();

    glActiveTexture(GL_TEXTURE0);
    destination->unbind();

    // Copy result back to history for next frame
    // This is done by the RenderEngine after all processing
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
