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
    uniform sampler2D sourceTexture;
    uniform float brightness;
    uniform float contrast;
    uniform float saturation;
    uniform float temperature;
    uniform float tint;
    uniform vec3 shadows;
    uniform vec3 highlights;

    varying vec2 vTexCoord;

    vec3 rgb2hsv(vec3 c)
    {
        vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
        vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
        vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
        float d = q.x - min(q.w, q.y);
        float e = 1.0e-10;
        return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
    }

    vec3 hsv2rgb(vec3 c)
    {
        vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
        vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
        return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
    }

    void main()
    {
        vec4 color = texture2D(sourceTexture, vTexCoord);
        vec3 rgb = color.rgb;

        // Apply brightness
        rgb += vec3(brightness);

        // Apply contrast (around mid gray)
        rgb = (rgb - 0.5) * contrast + 0.5;

        // Apply saturation
        float luma = dot(rgb, vec3(0.299, 0.587, 0.114));
        rgb = mix(vec3(luma), rgb, saturation);

        // Apply temperature (warm/cool shift)
        // Warm = more red/yellow, Cool = more blue
        rgb.r += temperature * 0.1;
        rgb.b -= temperature * 0.1;

        // Apply tint (green/magenta shift)
        rgb.g += tint * 0.1;

        // Apply shadow/highlight color tinting
        float luminance = dot(rgb, vec3(0.299, 0.587, 0.114));
        float shadowWeight = 1.0 - smoothstep(0.0, 0.5, luminance);
        float highlightWeight = smoothstep(0.5, 1.0, luminance);

        rgb = mix(rgb, rgb * shadows, shadowWeight * 0.5);
        rgb = mix(rgb, rgb * highlights, highlightWeight * 0.5);

        // Clamp result
        rgb = clamp(rgb, 0.0, 1.0);

        gl_FragColor = vec4(rgb, color.a);
    }
)";

ColorGradeEffect::ColorGradeEffect()
{
}

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

bool ColorGradeEffect::isCompiled() const
{
    return compiled_;
}

void ColorGradeEffect::apply(
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

    // Apply intensity scaling
    float scale = intensity_;
    ext.glUniform1f(brightnessLoc_, settings_.brightness * scale);
    ext.glUniform1f(contrastLoc_, 1.0f + (settings_.contrast - 1.0f) * scale);
    ext.glUniform1f(saturationLoc_, 1.0f + (settings_.saturation - 1.0f) * scale);
    ext.glUniform1f(temperatureLoc_, settings_.temperature * scale);
    ext.glUniform1f(tintLoc_, settings_.tint * scale);

    // Shadow and highlight colors
    ext.glUniform3f(shadowsLoc_,
        settings_.shadows.getFloatRed(),
        settings_.shadows.getFloatGreen(),
        settings_.shadows.getFloatBlue());
    ext.glUniform3f(highlightsLoc_,
        settings_.highlights.getFloatRed(),
        settings_.highlights.getFloatGreen(),
        settings_.highlights.getFloatBlue());

    pool.renderFullscreenQuad();
    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
