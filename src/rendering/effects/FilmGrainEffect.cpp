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
        // Parabolic curve: 1.0 - (luma - 0.5) * 2.0 squared? 
        // Or simplified: x * (1 - x) * 4.0
        float response = luma * (1.0 - luma) * 4.0;
        // Boost shadows slightly as digital noise often appears there too
        response = mix(response, 1.0, 0.2);

        // Apply grain
        // "Overlay" blend mode approximation or Soft Light is best, 
        // but simple additive/subtractive modulated by intensity works well for speed.
        
        float grain = (noise - 0.5) * 2.0; // -1 to 1
        vec3 grainColor = vec3(grain) * intensity * response;
        
        fragColor = vec4(color.rgb + grainColor, color.a);
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
