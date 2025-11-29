/*
    Oscil - Film Grain Effect Implementation
*/

#include "rendering/effects/FilmGrainEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// Film grain fragment shader using procedural noise
static const char* filmGrainFragmentShader = R"(
    uniform sampler2D sourceTexture;
    uniform float intensity;
    uniform float time;
    uniform vec2 resolution;

    varying vec2 vTexCoord;

    // Simple hash function for pseudo-random noise
    float hash(vec2 p)
    {
        vec3 p3 = fract(vec3(p.xyx) * 0.1031);
        p3 += dot(p3, p3.yzx + 33.33);
        return fract((p3.x + p3.y) * p3.z);
    }

    // Multi-octave noise for more natural grain
    float noise(vec2 uv, float t)
    {
        // Add time-varying component for animation
        vec2 p = uv * resolution + vec2(t * 543.21, t * 987.65);

        // Get base noise
        float n = hash(floor(p));

        // Add higher frequency detail
        float n2 = hash(floor(p * 2.0));
        float n3 = hash(floor(p * 4.0));

        return (n * 0.5 + n2 * 0.3 + n3 * 0.2);
    }

    void main()
    {
        vec4 color = texture2D(sourceTexture, vTexCoord);

        // Generate animated grain
        float grain = noise(vTexCoord, time);

        // Center noise around 0.5 and scale
        grain = (grain - 0.5) * 2.0 * intensity;

        // Apply grain additively (preserves color balance)
        vec3 result = color.rgb + vec3(grain);

        // Subtle luminance-based grain (more visible in midtones)
        float luminance = dot(color.rgb, vec3(0.299, 0.587, 0.114));
        float midtoneMask = 1.0 - abs(luminance - 0.5) * 2.0;
        result = mix(color.rgb, result, 0.5 + midtoneMask * 0.5);

        gl_FragColor = vec4(clamp(result, 0.0, 1.0), color.a);
    }
)";

FilmGrainEffect::FilmGrainEffect()
{
}

FilmGrainEffect::~FilmGrainEffect() = default;

bool FilmGrainEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compileEffectShader(*shader_, filmGrainFragmentShader))
    {
        DBG("FilmGrainEffect: Failed to compile shader");
        shader_.reset();
        return false;
    }

    // Get uniform locations
    textureLoc_ = shader_->getUniformIDFromName("sourceTexture");
    intensityLoc_ = shader_->getUniformIDFromName("intensity");
    timeLoc_ = shader_->getUniformIDFromName("time");
    resolutionLoc_ = shader_->getUniformIDFromName("resolution");

    if (textureLoc_ < 0 || intensityLoc_ < 0 || timeLoc_ < 0 || resolutionLoc_ < 0)
    {
        DBG("FilmGrainEffect: Missing uniforms");
        shader_.reset();
        return false;
    }

    compiled_ = true;
    DBG("FilmGrainEffect: Compiled successfully");
    return true;
}

void FilmGrainEffect::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    shader_.reset();
    compiled_ = false;
}

bool FilmGrainEffect::isCompiled() const
{
    return compiled_;
}

void FilmGrainEffect::apply(
    juce::OpenGLContext& context,
    Framebuffer* source,
    Framebuffer* destination,
    FramebufferPool& pool,
    float deltaTime)
{
    if (!compiled_ || !source || !destination)
        return;

    // Update time based on configured speed
    accumulatedTime_ += deltaTime * settings_.speed;
    if (accumulatedTime_ > 1000.0f)
        accumulatedTime_ = std::fmod(accumulatedTime_, 1000.0f);

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
    ext.glUniform1f(intensityLoc_, settings_.intensity * intensity_);
    ext.glUniform1f(timeLoc_, accumulatedTime_);
    ext.glUniform2f(resolutionLoc_,
        static_cast<float>(source->width),
        static_cast<float>(source->height));

    // Render fullscreen quad
    pool.renderFullscreenQuad();

    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
