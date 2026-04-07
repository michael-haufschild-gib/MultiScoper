/*
    Oscil - Film Grain Effect Implementation
*/

#include "rendering/effects/FilmGrainEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

// Film grain fragment shader using procedural noise
static const char* filmGrainFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform float intensity;
    uniform float time;
    uniform vec2 resolution;

    in vec2 vTexCoord;
    out vec4 fragColor;

    // High-quality hash (Gold Noise logic or similar)
    float hash(vec2 p)
    {
        vec3 p3 = fract(vec3(p.xyx) * .1031);
        p3 += dot(p3, p3.yzx + 33.33);
        return fract((p3.x + p3.y) * p3.z);
    }

    void main()
    {
        vec4 color = texture(sourceTexture, vTexCoord);
        
        // Calculate luminance
        float luma = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));

        // Generate noise
        // Use large coordinates to avoid pattern repetition
        float noise = hash(vTexCoord * resolution + time * 100.0);

        // Film Grain logic:
        // 1. Gaussian-ish distribution (approx by summing uniform noise? or just use uniform)
        // Uniform is fine for this scale.
        
        // 2. Luminance response:
        // Grain is most visible in mid-tones, less in black (no signal) and white (saturation).
        // Parabolic curve: 4.0 * x * (1.0 - x)
        float response = luma * (1.0 - luma) * 4.0;
        
        // 3. Soft Light / Overlay Blending
        // Standard additive grain lifts blacks. We want to preserve blacks.
        // Simple "Soft Additive": color + grain * color
        // Or "Overlay":
        
        float noiseVal = (noise - 0.5) * 2.0; // -1 to 1
        vec3 grain = vec3(noiseVal) * intensity * response;
        
        // Apply using a "Soft Light" approximation that respects brightness
        // mix(color, color + grain, ...)
        // We use the luminance itself to mask the grain in dark areas further
        
        vec3 result = color.rgb + color.rgb * grain * 2.0;
        
        fragColor = vec4(result, color.a);
    }
)";

FilmGrainEffect::FilmGrainEffect() = default;

FilmGrainEffect::~FilmGrainEffect() = default;

const char* FilmGrainEffect::getFragmentSource() const { return filmGrainFragmentShader; }

bool FilmGrainEffect::resolveUniforms()
{
    intensityLoc_ = shader_->getUniformIDFromName("intensity");
    timeLoc_ = shader_->getUniformIDFromName("time");
    resolutionLoc_ = shader_->getUniformIDFromName("resolution");

    return intensityLoc_ >= 0 && timeLoc_ >= 0 && resolutionLoc_ >= 0;
}

void FilmGrainEffect::setUniforms(const Framebuffer& source, float deltaTime)
{
    // Update time based on configured speed.
    // Wrap at 10.0 to keep float precision high — the shader's hash(fract(...))
    // makes the wrap boundary invisible.
    accumulatedTime_ += deltaTime * settings_.speed;
    if (accumulatedTime_ > 10.0f)
        accumulatedTime_ -= 10.0f;

    juce::OpenGLExtensionFunctions::glUniform1f(intensityLoc_, settings_.intensity * getIntensity());
    juce::OpenGLExtensionFunctions::glUniform1f(timeLoc_, accumulatedTime_);
    juce::OpenGLExtensionFunctions::glUniform2f(resolutionLoc_, static_cast<float>(source.width),
                                                static_cast<float>(source.height));
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
