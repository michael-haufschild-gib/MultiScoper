/*
    Oscil - Chromatic Aberration Effect Implementation
*/

#include "rendering/effects/ChromaticAberrationEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

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

ChromaticAberrationEffect::ChromaticAberrationEffect() = default;

ChromaticAberrationEffect::~ChromaticAberrationEffect() = default;

const char* ChromaticAberrationEffect::getFragmentSource() const { return chromaticFragmentShader; }

bool ChromaticAberrationEffect::resolveUniforms()
{
    intensityLoc_ = shader_->getUniformIDFromName("intensity");

    return intensityLoc_ >= 0;
}

void ChromaticAberrationEffect::setUniforms(const Framebuffer& source, float deltaTime)
{
    juce::ignoreUnused(source, deltaTime);
    juce::OpenGLExtensionFunctions::glUniform1f(intensityLoc_, settings_.intensity * getIntensity());
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
