/*
    Oscil - Scanline Effect Implementation
*/

#include "rendering/effects/ScanlineEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

static const char* scanlineFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform float intensity;
    uniform float density;
    uniform float width;
    uniform float height;
    uniform bool phosphorGlow;

    in vec2 vTexCoord;
    out vec4 fragColor;

    void main()
    {
        vec4 srcColor = texture(sourceTexture, vTexCoord);
        vec3 color = srcColor.rgb;

        // 1. Vertical Scanlines (The electron beam trace)
        // Use sine wave aligned to pixel rows
        float count = height * density;
        float scanline = sin((vTexCoord.y) * count * 6.2831853);
        
        // Sharpen the scanline (make it less sine-like, more beam-like)
        scanline = pow(scanline * 0.5 + 0.5, 1.5);
        
        // 2. RGB Aperture Grille (Trinitron style)
        // Aligned to horizontal pixels
        float xPos = vTexCoord.x * width * density;
        float mask = 1.0;
        
        // Create RGB mask pattern
        // We mix between 1.0 (white) and the Mask Color based on intensity
        vec3 maskColor = vec3(1.0);
        
        // Simple modulo for RGB stripes
        float m = mod(xPos, 3.0);
        
        // Smooth mask to prevent harsh aliasing
        vec3 rMask = vec3(1.0, 0.0, 0.0);
        vec3 gMask = vec3(0.0, 1.0, 0.0);
        vec3 bMask = vec3(0.0, 0.0, 1.0);
        
        float strip = sin(xPos * 6.2831853); // Pattern
        
        // Apply scanline darkness
        color *= mix(vec3(1.0), vec3(scanline), intensity);
        
        // Apply RGB Mask (if density is high enough to warrant it)
        if (density > 0.5) {
            // Calculate mask weights based on position
            // This is a simplified approximation of a shadow mask
            float maskWeight = intensity * 0.5;
            
            vec3 activeMask = vec3(1.0);
            if (m < 1.0) activeMask = mix(vec3(1.0), rMask, maskWeight);
            else if (m < 2.0) activeMask = mix(vec3(1.0), gMask, maskWeight);
            else activeMask = mix(vec3(1.0), bMask, maskWeight);
            
            color *= activeMask;
        }

        // Phosphor Glow (Bloom/Bleed)
        if (phosphorGlow) {
            // Sample neighbors to simulate phosphor persistence/bleed
            // Guard against division by zero for very small dimensions
            vec2 onePixel = vec2(1.0/max(width, 1.0), 1.0/max(height, 1.0));
            vec3 blur = texture(sourceTexture, vTexCoord + vec2(onePixel.x, 0.0)).rgb;
            blur += texture(sourceTexture, vTexCoord - vec2(onePixel.x, 0.0)).rgb;
            color += blur * 0.15 * intensity; 
        }

        // Brightness compensation (scanlines darken image)
        color *= 1.1 + (intensity * 0.2);

        fragColor = vec4(color, srcColor.a);
    }
)";

ScanlineEffect::ScanlineEffect()
{
}

ScanlineEffect::~ScanlineEffect() = default;

bool ScanlineEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compileEffectShader(*shader_, scanlineFragmentShader))
    {
        DBG("ScanlineEffect: Failed to compile shader");
        shader_.reset();
        return false;
    }

    textureLoc_ = shader_->getUniformIDFromName("sourceTexture");
    intensityLoc_ = shader_->getUniformIDFromName("intensity");
    densityLoc_ = shader_->getUniformIDFromName("density");
    widthLoc_ = shader_->getUniformIDFromName("width");
    heightLoc_ = shader_->getUniformIDFromName("height");
    phosphorGlowLoc_ = shader_->getUniformIDFromName("phosphorGlow");

    compiled_ = true;
    DBG("ScanlineEffect: Compiled successfully");
    return true;
}

void ScanlineEffect::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    shader_.reset();
    compiled_ = false;
}

bool ScanlineEffect::isCompiled() const
{
    return compiled_;
}

void ScanlineEffect::apply(
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

    // Set uniforms with validation to prevent GL errors on invalid locations
    if (textureLoc_ >= 0)
        ext.glUniform1i(textureLoc_, 0);
    if (intensityLoc_ >= 0)
        ext.glUniform1f(intensityLoc_, settings_.intensity * intensity_);
    if (densityLoc_ >= 0)
        ext.glUniform1f(densityLoc_, settings_.density);
    if (widthLoc_ >= 0)
        ext.glUniform1f(widthLoc_, static_cast<float>(source->width));
    if (heightLoc_ >= 0)
        ext.glUniform1f(heightLoc_, static_cast<float>(source->height));
    if (phosphorGlowLoc_ >= 0)
        ext.glUniform1i(phosphorGlowLoc_, settings_.phosphorGlow ? 1 : 0);

    pool.renderFullscreenQuad();

    // Cleanup texture unit - explicitly set active unit before unbinding
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

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
