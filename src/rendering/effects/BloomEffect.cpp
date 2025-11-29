/*
    Oscil - Bloom Effect Implementation
*/

#include "rendering/effects/BloomEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// Threshold shader - extracts bright areas
static const char* thresholdFragmentShader = R"(
    uniform sampler2D sourceTexture;
    uniform float threshold;

    varying vec2 vTexCoord;

    void main()
    {
        vec4 color = texture2D(sourceTexture, vTexCoord);

        // Calculate luminance
        float luminance = dot(color.rgb, vec3(0.299, 0.587, 0.114));

        // Extract only pixels above threshold
        float brightness = max(0.0, luminance - threshold);
        float contribution = brightness / max(luminance, 0.001);

        gl_FragColor = vec4(color.rgb * contribution, 1.0);
    }
)";

// Gaussian blur shader - single direction
static const char* blurFragmentShader = R"(
    uniform sampler2D sourceTexture;
    uniform vec2 direction;
    uniform vec2 resolution;

    varying vec2 vTexCoord;

    void main()
    {
        vec2 texelSize = 1.0 / resolution;
        vec2 offset = texelSize * direction;

        // 9-tap Gaussian blur weights
        // sigma = 2.0, calculated weights
        const float weights[5] = float[5](
            0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216
        );

        vec3 result = texture2D(sourceTexture, vTexCoord).rgb * weights[0];

        for (int i = 1; i < 5; i++)
        {
            vec2 sampleOffset = offset * float(i);
            result += texture2D(sourceTexture, vTexCoord + sampleOffset).rgb * weights[i];
            result += texture2D(sourceTexture, vTexCoord - sampleOffset).rgb * weights[i];
        }

        gl_FragColor = vec4(result, 1.0);
    }
)";

// Combine shader - adds bloom to original
static const char* combineFragmentShader = R"(
    uniform sampler2D originalTexture;
    uniform sampler2D bloomTexture;
    uniform float intensity;

    varying vec2 vTexCoord;

    void main()
    {
        vec4 original = texture2D(originalTexture, vTexCoord);
        vec3 bloom = texture2D(bloomTexture, vTexCoord).rgb;

        // Additive blend
        vec3 result = original.rgb + bloom * intensity;

        gl_FragColor = vec4(result, original.a);
    }
)";

BloomEffect::BloomEffect()
    : brightFBO_(std::make_unique<Framebuffer>())
    , blurTempFBO_(std::make_unique<Framebuffer>())
{
}

BloomEffect::~BloomEffect() = default;

bool BloomEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    // Compile threshold shader
    thresholdShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileEffectShader(*thresholdShader_, thresholdFragmentShader))
    {
        DBG("BloomEffect: Failed to compile threshold shader");
        return false;
    }
    thresholdTextureLoc_ = thresholdShader_->getUniformIDFromName("sourceTexture");
    thresholdValueLoc_ = thresholdShader_->getUniformIDFromName("threshold");

    // Compile blur shader
    blurShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileEffectShader(*blurShader_, blurFragmentShader))
    {
        DBG("BloomEffect: Failed to compile blur shader");
        return false;
    }
    blurTextureLoc_ = blurShader_->getUniformIDFromName("sourceTexture");
    blurDirectionLoc_ = blurShader_->getUniformIDFromName("direction");
    blurResolutionLoc_ = blurShader_->getUniformIDFromName("resolution");

    // Compile combine shader
    combineShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileEffectShader(*combineShader_, combineFragmentShader))
    {
        DBG("BloomEffect: Failed to compile combine shader");
        return false;
    }
    combineOriginalLoc_ = combineShader_->getUniformIDFromName("originalTexture");
    combineBloomLoc_ = combineShader_->getUniformIDFromName("bloomTexture");
    combineIntensityLoc_ = combineShader_->getUniformIDFromName("intensity");

    compiled_ = true;
    DBG("BloomEffect: Compiled successfully");
    return true;
}

void BloomEffect::release(juce::OpenGLContext& context)
{
    if (brightFBO_->isValid())
        brightFBO_->destroy(context);
    if (blurTempFBO_->isValid())
        blurTempFBO_->destroy(context);

    thresholdShader_.reset();
    blurShader_.reset();
    combineShader_.reset();

    compiled_ = false;
    lastWidth_ = 0;
    lastHeight_ = 0;
}

bool BloomEffect::isCompiled() const
{
    return compiled_;
}

void BloomEffect::apply(
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

    // Create or resize internal FBOs if needed
    // Use half resolution for bloom for performance
    int bloomWidth = source->width / 2;
    int bloomHeight = source->height / 2;

    if (lastWidth_ != bloomWidth || lastHeight_ != bloomHeight)
    {
        if (brightFBO_->isValid())
            brightFBO_->destroy(context);
        if (blurTempFBO_->isValid())
            blurTempFBO_->destroy(context);

        brightFBO_->create(context, bloomWidth, bloomHeight, GL_RGBA16F, false);
        blurTempFBO_->create(context, bloomWidth, bloomHeight, GL_RGBA16F, false);

        lastWidth_ = bloomWidth;
        lastHeight_ = bloomHeight;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // Pass 1: Extract bright areas
    brightFBO_->bind();
    brightFBO_->clear(juce::Colours::black, false);
    thresholdShader_->use();
    source->bindTexture(0);
    ext.glUniform1i(thresholdTextureLoc_, 0);
    ext.glUniform1f(thresholdValueLoc_, settings_.threshold);
    pool.renderFullscreenQuad();
    brightFBO_->unbind();

    // Iterative blur passes
    int iterations = juce::jlimit(1, 8, settings_.iterations);
    float spread = settings_.spread;

    for (int i = 0; i < iterations; i++)
    {
        // Horizontal blur
        blurTempFBO_->bind();
        blurShader_->use();
        brightFBO_->bindTexture(0);
        ext.glUniform1i(blurTextureLoc_, 0);
        ext.glUniform2f(blurDirectionLoc_, spread, 0.0f);
        ext.glUniform2f(blurResolutionLoc_,
            static_cast<float>(bloomWidth),
            static_cast<float>(bloomHeight));
        pool.renderFullscreenQuad();
        blurTempFBO_->unbind();

        // Vertical blur
        brightFBO_->bind();
        blurTempFBO_->bindTexture(0);
        ext.glUniform2f(blurDirectionLoc_, 0.0f, spread);
        pool.renderFullscreenQuad();
        brightFBO_->unbind();
    }

    // Pass 3: Combine bloom with original
    destination->bind();
    combineShader_->use();

    // Bind original to texture unit 0
    source->bindTexture(0);
    ext.glUniform1i(combineOriginalLoc_, 0);

    // Bind bloom to texture unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, brightFBO_->colorTexture);
    ext.glUniform1i(combineBloomLoc_, 1);

    ext.glUniform1f(combineIntensityLoc_, settings_.intensity * intensity_);

    pool.renderFullscreenQuad();

    // Reset texture unit
    glActiveTexture(GL_TEXTURE0);
    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
