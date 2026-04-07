/*
    Oscil - Tilt Shift Effect Implementation
*/

#include "rendering/effects/TiltShiftEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

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

TiltShiftEffect::TiltShiftEffect() = default;

TiltShiftEffect::~TiltShiftEffect() = default;

const char* TiltShiftEffect::getFragmentSource() const { return tiltShiftFragmentShader; }

bool TiltShiftEffect::resolveUniforms()
{
    blurRadiusLoc_ = shader_->getUniformIDFromName("blurRadius");
    positionLoc_ = shader_->getUniformIDFromName("position");
    rangeLoc_ = shader_->getUniformIDFromName("range");

    return positionLoc_ >= 0 && rangeLoc_ >= 0 && blurRadiusLoc_ >= 0;
}

void TiltShiftEffect::setUniforms(const Framebuffer& source, float deltaTime)
{
    juce::ignoreUnused(source, deltaTime);
    juce::OpenGLExtensionFunctions::glUniform1f(blurRadiusLoc_, settings_.blurRadius * getIntensity());
    juce::OpenGLExtensionFunctions::glUniform1f(positionLoc_, settings_.position);
    juce::OpenGLExtensionFunctions::glUniform1f(rangeLoc_, settings_.range);
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
