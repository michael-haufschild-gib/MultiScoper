/*
    Oscil - Lens Flare Effect Implementation
*/

#include "rendering/effects/LensFlareEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// Single-pass lens flare generation shader
// Adapted from Scion Post Process (Unity) and John Chapman's Pseudo Lens Flare
static const char* lensFlareFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform float intensity;
    uniform float threshold;
    uniform float ghostDispersal;
    uniform int ghostCount;
    uniform float haloWidth;
    uniform float chromaticDistortion;

    in vec2 vTexCoord;
    out vec4 fragColor;

    // Helper: Sample texture with chromatic aberration
    vec3 textureDistorted(sampler2D tex, vec2 uv, vec2 direction, vec3 distortion) {
        return vec3(
            texture(tex, uv + direction * distortion.r).r,
            texture(tex, uv + direction * distortion.g).g,
            texture(tex, uv + direction * distortion.b).b
        );
    }

    void main() {
        // 1. Get original image
        vec4 original = texture(sourceTexture, vTexCoord);

        // 2. Prepare for flare generation
        vec2 texelSize = 1.0 / vec2(textureSize(sourceTexture, 0));
        vec3 distortion = vec3(-texelSize.x * chromaticDistortion, 0.0, texelSize.x * chromaticDistortion);

        // Flip coordinates for ghost generation (ghosts appear opposite to the light source)
        vec2 uv = 1.0 - vTexCoord;
        
        // Vector from center to current UV
        vec2 ghostVec = (vec2(0.5) - uv) * ghostDispersal;
        vec2 direction = normalize(ghostVec);
        
        vec3 result = vec3(0.0);
        
        // 3. Generate Ghosts
        for (int i = 0; i < ghostCount; ++i) { 
            // Step along the vector
            vec2 offset = fract(uv + ghostVec * float(i));
            
            // Weighting function: bright in center, falls off
            float d = distance(offset, vec2(0.5));
            float weight = 1.0 - smoothstep(0.0, 0.75, d); 
            
            // Sample and threshold
            // We distort the sample coordinate to create chromatic aberration in the ghosts
            vec3 s = textureDistorted(sourceTexture, offset, direction, distortion);
            
            // Apply threshold to only pick up bright spots
            float lum = dot(s, vec3(0.2126, 0.7152, 0.0722));
            float threshVal = max(0.0, lum - threshold);
            
            // Normalize brightness after thresholding so we don't just get white
            s *= threshVal / max(lum, 0.001); 
            
            result += s * weight;
        }
        
        // 4. Generate Halo
        vec2 haloVec = normalize(ghostVec) * haloWidth;
        vec2 haloUV = fract(uv + haloVec);
        float haloDist = distance(haloUV, vec2(0.5));
        float haloWeight = 1.0 - smoothstep(0.0, 0.75, haloDist);
        // Sharpen halo weight for a ring effect
        haloWeight = pow(haloWeight, 5.0); 
        
        vec3 s = textureDistorted(sourceTexture, haloUV, direction, distortion);
        float lum = dot(s, vec3(0.2126, 0.7152, 0.0722));
        s *= max(0.0, lum - threshold) / max(lum, 0.001);
        
        result += s * haloWeight;
        
        // 5. Combine
        // Add flare to original image
        fragColor = vec4(original.rgb + result * intensity, original.a);
    }
)";

LensFlareEffect::LensFlareEffect()
{
}

LensFlareEffect::~LensFlareEffect() = default;

bool LensFlareEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compileEffectShader(*shader_, lensFlareFragmentShader))
    {
        DBG("LensFlareEffect: Failed to compile shader");
        shader_.reset();
        return false;
    }

    textureLoc_ = shader_->getUniformIDFromName("sourceTexture");
    intensityLoc_ = shader_->getUniformIDFromName("intensity");
    thresholdLoc_ = shader_->getUniformIDFromName("threshold");
    ghostDispersalLoc_ = shader_->getUniformIDFromName("ghostDispersal");
    ghostCountLoc_ = shader_->getUniformIDFromName("ghostCount");
    haloWidthLoc_ = shader_->getUniformIDFromName("haloWidth");
    chromaticDistortionLoc_ = shader_->getUniformIDFromName("chromaticDistortion");

    if (textureLoc_ < 0)
    {
        DBG("LensFlareEffect: Missing texture uniform");
        shader_.reset();
        return false;
    }

    compiled_ = true;
    DBG("LensFlareEffect: Compiled successfully");
    return true;
}

void LensFlareEffect::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    shader_.reset();
    compiled_ = false;
}

bool LensFlareEffect::isCompiled() const
{
    return compiled_;
}

void LensFlareEffect::apply(
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

    ext.glUniform1f(intensityLoc_, settings_.intensity * intensity_); // Use base intensity multiplier
    ext.glUniform1f(thresholdLoc_, settings_.threshold);
    ext.glUniform1f(ghostDispersalLoc_, settings_.ghostDispersal);
    ext.glUniform1i(ghostCountLoc_, settings_.ghostCount);
    ext.glUniform1f(haloWidthLoc_, settings_.haloWidth);
    ext.glUniform1f(chromaticDistortionLoc_, settings_.chromaticDistortion);

    pool.renderFullscreenQuad();
    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
