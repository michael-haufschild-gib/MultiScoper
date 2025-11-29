/*
    Oscil - Glitch Effect Implementation
*/

#include "rendering/effects/GlitchEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

static const char* glitchFragmentShader = R"(
    uniform sampler2D sourceTexture;
    uniform float intensity;
    uniform float time;
    uniform vec2 resolution;
    uniform float blockSize;
    uniform float lineShift;
    uniform float colorSeparation;

    varying vec2 vTexCoord;

    // Hash functions for pseudo-randomness
    float hash(float n) { return fract(sin(n) * 43758.5453); }
    float hash2(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }

    void main()
    {
        vec2 uv = vTexCoord;

        // Time-based glitch trigger (random intervals)
        float glitchTime = floor(time * 10.0);
        float glitchRandom = hash(glitchTime);

        // Only apply glitch effect periodically
        float glitchActive = step(0.7, glitchRandom) * intensity;

        if (glitchActive > 0.0)
        {
            // Block-based displacement
            vec2 block = floor(uv / blockSize) * blockSize;
            float blockRandom = hash2(block + glitchTime);

            if (blockRandom > 0.8)
            {
                // Horizontal shift for this block
                uv.x += (blockRandom - 0.5) * lineShift * 4.0 * glitchActive;
            }

            // Scanline-based horizontal shift
            float lineRandom = hash(floor(uv.y * resolution.y) + glitchTime);
            if (lineRandom > 0.95)
            {
                uv.x += (lineRandom - 0.5) * lineShift * 2.0 * glitchActive;
            }
        }

        // Clamp UVs
        uv = clamp(uv, 0.0, 1.0);

        // Color channel separation
        float separation = colorSeparation * glitchActive;
        float r = texture2D(sourceTexture, uv + vec2(separation, 0.0)).r;
        float g = texture2D(sourceTexture, uv).g;
        float b = texture2D(sourceTexture, uv - vec2(separation, 0.0)).b;

        // Original alpha
        float a = texture2D(sourceTexture, uv).a;

        // Occasional color inversion on glitch
        vec3 color = vec3(r, g, b);
        float invertRandom = hash(glitchTime + 0.5);
        if (invertRandom > 0.95 && glitchActive > 0.0)
        {
            color = 1.0 - color;
        }

        gl_FragColor = vec4(color, a);
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
