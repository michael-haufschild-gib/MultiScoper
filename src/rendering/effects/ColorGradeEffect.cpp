/*
    Oscil - Color Grade Effect Implementation
*/

#include "rendering/effects/ColorGradeEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

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

    // Narkowicz ACES Tone Mapping
    // Standard for "filmic" look in games
    vec3 ACESFilm(vec3 x)
    {
        float a = 2.51;
        float b = 0.03;
        float c = 2.43;
        float d = 0.59;
        float e = 0.14;
        return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
    }

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

ColorGradeEffect::ColorGradeEffect() {}

ColorGradeEffect::~ColorGradeEffect() = default;

bool ColorGradeEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compileEffectShader(*shader_, colorGradeFragmentShader))
    {
        DBG("ColorGradeEffect: Failed to compile shader");
        shader_.reset();
        return false;
    }

    textureLoc_ = shader_->getUniformIDFromName("sourceTexture");
    brightnessLoc_ = shader_->getUniformIDFromName("brightness");
    contrastLoc_ = shader_->getUniformIDFromName("contrast");
    saturationLoc_ = shader_->getUniformIDFromName("saturation");
    temperatureLoc_ = shader_->getUniformIDFromName("temperature");
    tintLoc_ = shader_->getUniformIDFromName("tint");
    shadowsLoc_ = shader_->getUniformIDFromName("shadows");
    highlightsLoc_ = shader_->getUniformIDFromName("highlights");

    if (textureLoc_ < 0)
    {
        DBG("ColorGradeEffect: Missing texture uniform");
        shader_.reset();
        return false;
    }

    compiled_ = true;
    DBG("ColorGradeEffect: Compiled successfully");
    return true;
}

void ColorGradeEffect::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    shader_.reset();
    compiled_ = false;
}

bool ColorGradeEffect::isCompiled() const { return compiled_; }

void ColorGradeEffect::apply(juce::OpenGLContext& context, Framebuffer* source, Framebuffer* destination,
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

    // Apply intensity scaling
    float scale = intensity_;
    ext.glUniform1f(brightnessLoc_, settings_.brightness * scale);
    ext.glUniform1f(contrastLoc_, 1.0f + (settings_.contrast - 1.0f) * scale);
    ext.glUniform1f(saturationLoc_, 1.0f + (settings_.saturation - 1.0f) * scale);
    ext.glUniform1f(temperatureLoc_, settings_.temperature * scale);
    ext.glUniform1f(tintLoc_, settings_.tint * scale);

    // Shadow and highlight colors
    ext.glUniform3f(shadowsLoc_, settings_.shadows.getFloatRed(), settings_.shadows.getFloatGreen(),
                    settings_.shadows.getFloatBlue());
    ext.glUniform3f(highlightsLoc_, settings_.highlights.getFloatRed(), settings_.highlights.getFloatGreen(),
                    settings_.highlights.getFloatBlue());

    pool.renderFullscreenQuad();
    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
