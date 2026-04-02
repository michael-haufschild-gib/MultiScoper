/*
    Oscil - Radial Blur Effect Implementation
    Based on Electric Flower WebGL demo zoom-glow technique
*/

#include "rendering/effects/RadialBlurEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// Radial blur fragment shader
// Samples at multiple zoom levels from center to create glow effect
static const char* radialBlurFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform float amount;      // Zoom amount per sample (0.0 - 0.5)
    uniform float glow;        // Glow intensity multiplier
    uniform int samples;       // Number of zoom samples (2-8)

    in vec2 vTexCoord;
    out vec4 fragColor;

    void main()
    {
        vec2 center = vec2(0.5, 0.5);
        vec2 direction = vTexCoord - center;

        vec4 result = vec4(0.0);
        float totalWeight = 0.0;

        // Sample at progressively zoomed positions
        // Each sample is weighted to create a glow falloff
        for (int i = 0; i < samples; i++)
        {
            // Calculate zoom factor for this sample
            // Start at 1.0 (original) and increase by amount per step
            // Guard against division by zero when samples <= 1
            float zoomStep = samples > 1 ? float(i) / float(samples - 1) : 0.0;
            float zoom = 1.0 + amount * zoomStep;

            // Calculate sample position (zooming from center)
            vec2 samplePos = center + direction * zoom;

            // Weight decreases for outer samples (linear falloff)
            float weight = 1.0 - zoomStep * 0.5;

            // Accumulate sample
            result += texture(sourceTexture, samplePos) * weight;
            totalWeight += weight;
        }

        // Normalize by total weight
        result /= totalWeight;

        // Apply glow intensity
        // Glow > 1.0 brightens the blur, creating a bloom-like effect
        result.rgb *= glow;

        fragColor = result;
    }
)";

RadialBlurEffect::RadialBlurEffect() {}

RadialBlurEffect::~RadialBlurEffect() = default;

bool RadialBlurEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compileEffectShader(*shader_, radialBlurFragmentShader))
    {
        DBG("RadialBlurEffect: Failed to compile shader");
        shader_.reset();
        return false;
    }

    // Get uniform locations
    textureLoc_ = shader_->getUniformIDFromName("sourceTexture");
    amountLoc_ = shader_->getUniformIDFromName("amount");
    glowLoc_ = shader_->getUniformIDFromName("glow");
    samplesLoc_ = shader_->getUniformIDFromName("samples");

    if (textureLoc_ < 0 || amountLoc_ < 0 || glowLoc_ < 0 || samplesLoc_ < 0)
    {
        DBG("RadialBlurEffect: Missing uniforms");
        shader_.reset();
        return false;
    }

    compiled_ = true;
    DBG("RadialBlurEffect: Compiled successfully");
    return true;
}

void RadialBlurEffect::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    shader_.reset();
    compiled_ = false;
}

bool RadialBlurEffect::isCompiled() const { return compiled_; }

void RadialBlurEffect::apply(juce::OpenGLContext& context, Framebuffer* source, Framebuffer* destination,
                             FramebufferPool& pool, float deltaTime)
{
    juce::ignoreUnused(deltaTime);

    if (!compiled_ || !source || !destination)
        return;

    auto& ext = context.extensions;

    // Bind destination
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
    ext.glUniform1f(amountLoc_, settings_.amount * intensity_);
    ext.glUniform1f(glowLoc_, settings_.glow);
    ext.glUniform1i(samplesLoc_, juce::jlimit(2, 8, settings_.samples));

    // Render fullscreen quad
    pool.renderFullscreenQuad();

    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
