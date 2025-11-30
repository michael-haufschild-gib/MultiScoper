/*
    Oscil - Glitch Effect Implementation
*/

#include "rendering/effects/GlitchEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

static const char* glitchFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform float time;
    uniform float intensity;

    in vec2 vTexCoord;
    out vec4 fragColor;

    float rand(vec2 co) {
        return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
    }

    void main()
    {
        vec2 uv = vTexCoord;
        
        // Create blocky noise for displacement
        // Quantize UV for blocks
        float blockY = floor(uv.y * 20.0);
        float noise = rand(vec2(blockY, floor(time * 10.0)));
        
        // Displace X only occasionally
        float shift = 0.0;
        if (noise > (1.0 - intensity)) {
            shift = (rand(vec2(time, blockY)) - 0.5) * 0.2;
        }
        
        // RGB Split during glitch
        float r = texture(sourceTexture, uv + vec2(shift, 0.0)).r;
        float g = texture(sourceTexture, uv + vec2(-shift, 0.0)).g;
        float b = texture(sourceTexture, uv).b;
        
        fragColor = vec4(r, g, b, 1.0);
    }
)";

GlitchEffect::GlitchEffect()
{
}

GlitchEffect::~GlitchEffect() = default;

bool GlitchEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compileEffectShader(*shader_, glitchFragmentShader))
    {
        DBG("GlitchEffect: Failed to compile shader");
        shader_.reset();
        return false;
    }

    textureLoc_ = shader_->getUniformIDFromName("sourceTexture");
    intensityLoc_ = shader_->getUniformIDFromName("intensity");
    timeLoc_ = shader_->getUniformIDFromName("time");
    resolutionLoc_ = shader_->getUniformIDFromName("resolution");
    blockSizeLoc_ = shader_->getUniformIDFromName("blockSize");
    lineShiftLoc_ = shader_->getUniformIDFromName("lineShift");
    colorSeparationLoc_ = shader_->getUniformIDFromName("colorSeparation");

    compiled_ = true;
    DBG("GlitchEffect: Compiled successfully");
    return true;
}

void GlitchEffect::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    shader_.reset();
    compiled_ = false;
}

bool GlitchEffect::isCompiled() const
{
    return compiled_;
}

void GlitchEffect::apply(
    juce::OpenGLContext& context,
    Framebuffer* source,
    Framebuffer* destination,
    FramebufferPool& pool,
    float deltaTime)
{
    if (!compiled_ || !source || !destination)
        return;

    // Update time
    accumulatedTime_ += deltaTime * settings_.flickerRate;
    if (accumulatedTime_ > 1000.0f)
        accumulatedTime_ = std::fmod(accumulatedTime_, 1000.0f);

    auto& ext = context.extensions;

    destination->bind();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    shader_->use();

    source->bindTexture(0);
    ext.glUniform1i(textureLoc_, 0);
    ext.glUniform1f(intensityLoc_, settings_.intensity * intensity_);
    ext.glUniform1f(timeLoc_, accumulatedTime_);
    ext.glUniform2f(resolutionLoc_,
        static_cast<float>(source->width),
        static_cast<float>(source->height));
    ext.glUniform1f(blockSizeLoc_, settings_.blockSize);
    ext.glUniform1f(lineShiftLoc_, settings_.lineShift);
    ext.glUniform1f(colorSeparationLoc_, settings_.colorSeparation);

    pool.renderFullscreenQuad();
    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
