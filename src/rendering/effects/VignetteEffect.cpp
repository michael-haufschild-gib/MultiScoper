/*
    Oscil - Vignette Effect Implementation
*/

#include "rendering/effects/VignetteEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// Vignette fragment shader
static const char* vignetteFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform float intensity;
    uniform float softness;
    uniform vec4 colour;

    in vec2 vTexCoord;
    out vec4 fragColor;

    void main()
    {
        vec4 color = texture(sourceTexture, vTexCoord);
        
        // Calculate distance from center
        vec2 position = (vTexCoord - 0.5);
        
        // Use squared distance for smooth optical falloff
        // Multiply by 2.0 to hit corners
        float len2 = dot(position, position) * 2.0;
        
        // Vignette curve
        // Intensity controls how "dark" the darkest part can get (indirectly via curve steepness)
        // Softness controls the transition
        
        float vignette = 1.0 - smoothstep(1.0 - softness, 1.0 + softness * 0.5, len2);
        
        // Mix original color with vignette color
        // Apply intensity strength to the final mix
        color.rgb = mix(color.rgb, colour.rgb, (1.0 - vignette) * intensity);
        
        fragColor = color;
    }
)";

VignetteEffect::VignetteEffect()
{
}

VignetteEffect::~VignetteEffect() = default;

bool VignetteEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compileEffectShader(*shader_, vignetteFragmentShader))
    {
        DBG("VignetteEffect: Failed to compile shader");
        shader_.reset();
        return false;
    }

    // Get uniform locations
    textureLoc_ = shader_->getUniformIDFromName("sourceTexture");
    intensityLoc_ = shader_->getUniformIDFromName("intensity");
    softnessLoc_ = shader_->getUniformIDFromName("softness");
    colorLoc_ = shader_->getUniformIDFromName("vignetteColor");

    if (textureLoc_ < 0 || intensityLoc_ < 0 || softnessLoc_ < 0 || colorLoc_ < 0)
    {
        DBG("VignetteEffect: Missing uniforms");
        shader_.reset();
        return false;
    }

    compiled_ = true;
    DBG("VignetteEffect: Compiled successfully");
    return true;
}

void VignetteEffect::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    shader_.reset();
    compiled_ = false;
}

bool VignetteEffect::isCompiled() const
{
    return compiled_;
}

void VignetteEffect::apply(
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

    // Bind destination and clear
    destination->bind();

    // Disable depth test for fullscreen pass
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // Use shader
    shader_->use();

    // Bind source texture
    source->bindTexture(0);
    ext.glUniform1i(textureLoc_, 0);

    // Set uniforms
    ext.glUniform1f(intensityLoc_, settings_.intensity * intensity_);
    ext.glUniform1f(softnessLoc_, settings_.softness);
    ext.glUniform4f(colorLoc_,
        settings_.colour.getFloatRed(),
        settings_.colour.getFloatGreen(),
        settings_.colour.getFloatBlue(),
        settings_.colour.getFloatAlpha());

    // Render fullscreen quad
    pool.renderFullscreenQuad();

    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
