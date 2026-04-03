/*
    Oscil - Tilt Shift Effect Implementation
*/

#include "rendering/effects/TiltShiftEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// Simple single-pass tilt shift blur
// Not a perfect Gaussian, but sufficient for the effect
static const char* tiltShiftFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform float blurRadius;
    uniform float position;
    uniform float range;
    
    in vec2 vTexCoord;
    out vec4 fragColor;

    void main()
    {
        vec4 color = vec4(0.0);
        float totalWeight = 0.0;
        
        // Calculate blur amount based on distance from focus center
        float dist = abs(vTexCoord.y - position);
        float blurAmount = smoothstep(range * 0.5, range, dist);
        
        // If perfectly in focus, skip texture lookups
        if (blurAmount < 0.01) {
            fragColor = texture(sourceTexture, vTexCoord);
            return;
        }

        float radius = blurRadius * blurAmount * 0.005; // Scale to UV space

        // 9-tap blur (or more if needed)
        // Poisson-disc-like sampling or just simple box/cross
        for (float x = -1.0; x <= 1.0; x += 1.0) {
            for (float y = -1.0; y <= 1.0; y += 1.0) {
                vec2 offset = vec2(x, y) * radius;
                float weight = 1.0 - length(vec2(x,y)) * 0.5; // Center weighted
                color += texture(sourceTexture, vTexCoord + offset) * weight;
                totalWeight += weight;
            }
        }
        
        fragColor = color / totalWeight;
    }
)";

TiltShiftEffect::TiltShiftEffect() {}

TiltShiftEffect::~TiltShiftEffect() = default;

bool TiltShiftEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compileEffectShader(*shader_, tiltShiftFragmentShader))
    {
        DBG("TiltShiftEffect: Failed to compile shader");
        shader_.reset();
        return false;
    }

    textureLoc_ = shader_->getUniformIDFromName("sourceTexture");
    blurRadiusLoc_ = shader_->getUniformIDFromName("blurRadius");
    positionLoc_ = shader_->getUniformIDFromName("position");
    rangeLoc_ = shader_->getUniformIDFromName("range");

    if (textureLoc_ < 0 || positionLoc_ < 0 || rangeLoc_ < 0 || blurRadiusLoc_ < 0)
    {
        DBG("TiltShiftEffect: Missing uniforms");
        shader_.reset();
        return false;
    }

    compiled_ = true;
    return true;
}

void TiltShiftEffect::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    shader_.reset();
    compiled_ = false;
}

bool TiltShiftEffect::isCompiled() const { return compiled_; }

void TiltShiftEffect::apply(juce::OpenGLContext& context, Framebuffer* source, Framebuffer* destination,
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
    ext.glUniform1f(blurRadiusLoc_, settings_.blurRadius);
    ext.glUniform1f(positionLoc_, settings_.position);
    ext.glUniform1f(rangeLoc_, settings_.range);

    pool.renderFullscreenQuad();
    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
