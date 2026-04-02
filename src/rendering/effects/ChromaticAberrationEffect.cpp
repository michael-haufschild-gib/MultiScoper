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

ChromaticAberrationEffect::ChromaticAberrationEffect() {}

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

bool ChromaticAberrationEffect::isCompiled() const { return compiled_; }

void ChromaticAberrationEffect::apply(juce::OpenGLContext& context, Framebuffer* source, Framebuffer* destination,
                                      FramebufferPool& pool, float deltaTime)
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
    ext.glUniform1f(intensityLoc_, settings_.intensity * getIntensity());

    pool.renderFullscreenQuad();
    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
