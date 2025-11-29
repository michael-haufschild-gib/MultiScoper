/*
    Oscil - Chromatic Aberration Effect Implementation
*/

#include "rendering/effects/ChromaticAberrationEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// Chromatic aberration fragment shader
static const char* chromaticAberrationFragmentShader = R"(
    uniform sampler2D sourceTexture;
    uniform float intensity;

    varying vec2 vTexCoord;

    void main()
    {
        // Calculate direction from center
        vec2 center = vec2(0.5, 0.5);
        vec2 dir = vTexCoord - center;
        float dist = length(dir);

        // Scale offset based on distance from center (radial aberration)
        vec2 offset = dir * intensity * dist;

        // Sample each channel with different offsets
        float r = texture2D(sourceTexture, vTexCoord + offset).r;
        float g = texture2D(sourceTexture, vTexCoord).g;
        float b = texture2D(sourceTexture, vTexCoord - offset).b;
        float a = texture2D(sourceTexture, vTexCoord).a;

        gl_FragColor = vec4(r, g, b, a);
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

    if (!compileEffectShader(*shader_, chromaticAberrationFragmentShader))
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

    auto& ext = context.extensions;

    destination->bind();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    shader_->use();

    source->bindTexture(0);
    ext.glUniform1i(textureLoc_, 0);
    ext.glUniform1f(intensityLoc_, settings_.intensity * intensity_);

    pool.renderFullscreenQuad();
    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
