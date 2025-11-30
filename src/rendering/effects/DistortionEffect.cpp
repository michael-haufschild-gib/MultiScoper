/*
    Oscil - Distortion Effect Implementation
*/

#include "rendering/effects/DistortionEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

static const char* distortionFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform float intensity;
    uniform float frequency;
    uniform float speed;
    uniform float time;

    in vec2 vTexCoord;
    out vec4 fragColor;

    void main()
    {
        vec2 uv = vTexCoord;
        
        // Calculate sine wave displacement
        // Y displacement based on X coordinate + time
        float offsetY = sin(uv.x * frequency + time * speed) * intensity;
        
        // X displacement based on Y coordinate + time (optional, adds more chaos)
        float offsetX = cos(uv.y * frequency + time * speed * 1.1) * intensity * 0.5;
        
        uv.y += offsetY;
        uv.x += offsetX;
        
        // Wrap or Clamp? Clamp to avoid artifacts
        uv = clamp(uv, 0.0, 1.0);

        fragColor = texture(sourceTexture, uv);
    }
)";

DistortionEffect::DistortionEffect()
{
}

DistortionEffect::~DistortionEffect() = default;

bool DistortionEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compileEffectShader(*shader_, distortionFragmentShader))
    {
        DBG("DistortionEffect: Failed to compile shader");
        shader_.reset();
        return false;
    }

    textureLoc_ = shader_->getUniformIDFromName("sourceTexture");
    intensityLoc_ = shader_->getUniformIDFromName("intensity");
    frequencyLoc_ = shader_->getUniformIDFromName("frequency");
    timeLoc_ = shader_->getUniformIDFromName("time");

    compiled_ = true;
    DBG("DistortionEffect: Compiled successfully");
    return true;
}

void DistortionEffect::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    shader_.reset();
    compiled_ = false;
}

bool DistortionEffect::isCompiled() const
{
    return compiled_;
}

void DistortionEffect::apply(
    juce::OpenGLContext& context,
    Framebuffer* source,
    Framebuffer* destination,
    FramebufferPool& pool,
    float deltaTime)
{
    if (!compiled_ || !source || !destination)
        return;

    // Update time
    accumulatedTime_ += deltaTime * settings_.speed;
    if (accumulatedTime_ > 1000.0f)
        accumulatedTime_ = std::fmod(accumulatedTime_, 1000.0f);

    auto& ext = context.extensions;

    destination->bind();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    shader_->use();

    source->bindTexture(0);
    ext.glUniform1i(textureLoc_, 0);
    ext.glUniform1f(intensityLoc_, settings_.intensity * intensity_ * 0.05f);
    ext.glUniform1f(frequencyLoc_, settings_.frequency);
    ext.glUniform1f(timeLoc_, accumulatedTime_);

    pool.renderFullscreenQuad();
    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
