/*
    Oscil - Radial Blur Effect Implementation
    Based on Electric Flower WebGL demo zoom-glow technique
*/

#include "rendering/effects/RadialBlurEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

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

RadialBlurEffect::RadialBlurEffect() = default;

RadialBlurEffect::~RadialBlurEffect() = default;

const char* RadialBlurEffect::getFragmentSource() const { return radialBlurFragmentShader; }

bool RadialBlurEffect::resolveUniforms()
{
    amountLoc_ = shader_->getUniformIDFromName("amount");
    glowLoc_ = shader_->getUniformIDFromName("glow");
    samplesLoc_ = shader_->getUniformIDFromName("samples");

    return amountLoc_ >= 0 && glowLoc_ >= 0 && samplesLoc_ >= 0;
}

void RadialBlurEffect::setUniforms(const Framebuffer& source, float deltaTime)
{
    juce::ignoreUnused(source, deltaTime);
    juce::OpenGLExtensionFunctions::glUniform1f(amountLoc_, settings_.amount * getIntensity());
    juce::OpenGLExtensionFunctions::glUniform1f(glowLoc_, settings_.glow);
    juce::OpenGLExtensionFunctions::glUniform1i(samplesLoc_, juce::jlimit(2, 8, settings_.samples));
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
