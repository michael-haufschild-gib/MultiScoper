/*
    Oscil - Vignette Effect Implementation
*/

#include "rendering/effects/VignetteEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

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

VignetteEffect::VignetteEffect() = default;

VignetteEffect::~VignetteEffect() = default;

const char* VignetteEffect::getFragmentSource() const { return vignetteFragmentShader; }

bool VignetteEffect::resolveUniforms()
{
    intensityLoc_ = shader_->getUniformIDFromName("intensity");
    softnessLoc_ = shader_->getUniformIDFromName("softness");
    colorLoc_ = shader_->getUniformIDFromName("colour");

    return intensityLoc_ >= 0 && softnessLoc_ >= 0 && colorLoc_ >= 0;
}

void VignetteEffect::setUniforms(const Framebuffer& source, float deltaTime)
{
    juce::ignoreUnused(source, deltaTime);
    juce::OpenGLExtensionFunctions::glUniform1f(intensityLoc_, settings_.intensity * getIntensity());
    float const safeSoftness = std::max(settings_.softness, 1.0e-4f);
    juce::OpenGLExtensionFunctions::glUniform1f(softnessLoc_, safeSoftness);
    juce::OpenGLExtensionFunctions::glUniform4f(colorLoc_, settings_.colour.getFloatRed(),
                                                settings_.colour.getFloatGreen(), settings_.colour.getFloatBlue(),
                                                settings_.colour.getFloatAlpha());
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
