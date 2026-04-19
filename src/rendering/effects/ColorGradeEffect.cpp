/*
    MultiScoper - Color Grade Effect Implementation
*/

#include "rendering/effects/ColorGradeEffect.h"

#if MULTISCOPER_ENABLE_OPENGL

namespace multiscoper
{

// Color grading fragment shader
static const char* colorGradeFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    
    // Settings
    uniform float brightness;
    uniform float contrast;
    uniform float saturation;
    uniform float temperature;
    uniform float tint;
    uniform vec3 shadows;
    uniform vec3 highlights;

    in vec2 vTexCoord;
    out vec4 fragColor;

    vec3 adjustContrast(vec3 color, float contrast)
    {
        return (color - 0.5) * contrast + 0.5;
    }

    vec3 adjustSaturation(vec3 color, float saturation)
    {
        // Better luminance weights (Rec. 709)
        const vec3 lumaWeights = vec3(0.2126, 0.7152, 0.0722);
        float luminance = dot(color, lumaWeights);
        return mix(vec3(luminance), color, saturation);
    }

    void main()
    {
        vec4 texColor = texture(sourceTexture, vTexCoord);
        vec3 color = texColor.rgb;

        // 1. Exposure / Brightness (Pre-tonemap)
        color *= pow(2.0, brightness); // Photographic exposure

        // 2. Contrast
        color = adjustContrast(color, contrast);

        // 3. Color Balance (Temperature/Tint)
        // Warmer = add R, sub B. Cooler = sub R, add B.
        // Tint = Green/Magenta axis.
        vec3 balance = vec3(temperature, tint, -temperature);
        color += color * balance * 0.2; // Gentle shift

        // 4. Saturation
        color = adjustSaturation(color, saturation);

        // 5. Shadows/Highlights (Split Toning)
        float lum = dot(color, vec3(0.2126, 0.7152, 0.0722));
        
        // Smooth blending using overlay-style logic
        vec3 shadowTint = mix(vec3(1.0), shadows, 1.0 - lum);
        vec3 highlightTint = mix(vec3(1.0), highlights, lum);
        
        color *= shadowTint * highlightTint;

        // NOTE: We output LINEAR HDR color here.
        // Tone Mapping (ACES) and Gamma Correction are handled in the final Blit shader
        // in RenderEngine.cpp. This prevents double-tone-mapping.

        fragColor = vec4(color, texColor.a);
    }
)";

ColorGradeEffect::ColorGradeEffect() = default;

ColorGradeEffect::~ColorGradeEffect() = default;

const char* ColorGradeEffect::getFragmentSource() const { return colorGradeFragmentShader; }

bool ColorGradeEffect::resolveUniforms()
{
    brightnessLoc_ = shader_->getUniformIDFromName("brightness");
    contrastLoc_ = shader_->getUniformIDFromName("contrast");
    saturationLoc_ = shader_->getUniformIDFromName("saturation");
    temperatureLoc_ = shader_->getUniformIDFromName("temperature");
    tintLoc_ = shader_->getUniformIDFromName("tint");
    shadowsLoc_ = shader_->getUniformIDFromName("shadows");
    highlightsLoc_ = shader_->getUniformIDFromName("highlights");

    return brightnessLoc_ >= 0 && contrastLoc_ >= 0 && saturationLoc_ >= 0 && temperatureLoc_ >= 0 && tintLoc_ >= 0 &&
           shadowsLoc_ >= 0 && highlightsLoc_ >= 0;
}

void ColorGradeEffect::setUniforms(const Framebuffer& source, float deltaTime)
{
    juce::ignoreUnused(source, deltaTime);

    // Apply intensity scaling
    float const scale = getIntensity();
    juce::OpenGLExtensionFunctions::glUniform1f(brightnessLoc_, settings_.brightness * scale);
    juce::OpenGLExtensionFunctions::glUniform1f(contrastLoc_, 1.0f + ((settings_.contrast - 1.0f) * scale));
    juce::OpenGLExtensionFunctions::glUniform1f(saturationLoc_, 1.0f + ((settings_.saturation - 1.0f) * scale));
    juce::OpenGLExtensionFunctions::glUniform1f(temperatureLoc_, settings_.temperature * scale);
    juce::OpenGLExtensionFunctions::glUniform1f(tintLoc_, settings_.tint * scale);

    // Shadow and highlight colors
    juce::OpenGLExtensionFunctions::glUniform3f(shadowsLoc_, settings_.shadows.getFloatRed(),
                                                settings_.shadows.getFloatGreen(), settings_.shadows.getFloatBlue());
    juce::OpenGLExtensionFunctions::glUniform3f(highlightsLoc_, settings_.highlights.getFloatRed(),
                                                settings_.highlights.getFloatGreen(),
                                                settings_.highlights.getFloatBlue());
}

} // namespace multiscoper

#endif // MULTISCOPER_ENABLE_OPENGL
