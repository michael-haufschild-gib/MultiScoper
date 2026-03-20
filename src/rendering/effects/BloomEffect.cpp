/*
    Oscil - Bloom Effect Implementation (Next-Gen Dual Filter)
*/

#include "rendering/effects/BloomEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// =============================================================================
// Shaders
// =============================================================================

// Prefilter: Extracts bright pixels and performs the first downsample (13-tap Dual Filter)
// Includes Karis Average to reduce fireflies (temporal instability)
static const char* prefilterFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform vec2 srcResolution;
    uniform float threshold;
    uniform float softKnee;

    in vec2 vTexCoord;
    out vec4 fragColor;

    // Partial Karis Average: weighting by 1/(1+luma)
    float karisWeight(vec3 color) {
        float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
        return 1.0 / (1.0 + luma);
    }

    vec3 applyThreshold(vec3 color) {
        float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
        float knee = threshold * softKnee;
        
        float contribution = max(0.0, brightness - threshold);
        
        if (softKnee > 0.001) {
            float soft = brightness - threshold + knee;
            soft = clamp(soft, 0.0, 2.0 * knee);
            contribution = soft * soft / (4.0 * knee + 0.00001);
            contribution += max(0.0, brightness - threshold - knee);
        }
        
        contribution /= max(brightness, 0.00001);
        return color * contribution;
    }

    void main()
    {
        vec2 texelSize = 1.0 / srcResolution;
        float x = texelSize.x;
        float y = texelSize.y;

        // 13-tap Jimenez Filter with Karis Average
        // A B C
        // D E F
        // G H I
        // J K L
        // M N O
        
        // Groups:
        // Center: [F, G, J, K] (Wait, standard dual filter uses 5 groups)
        // Let's use the standard Jimenez 13-tap locations:
        // (0,0), (-2,-2), (-2,2), (2,-2), (2,2) ... 
        
        // Sample locations from "Next Generation Post Processing"
        vec3 a = texture(sourceTexture, vTexCoord + vec2(-2*x, 2*y)).rgb;
        vec3 b = texture(sourceTexture, vTexCoord + vec2( 0,   2*y)).rgb;
        vec3 c = texture(sourceTexture, vTexCoord + vec2( 2*x, 2*y)).rgb;

        vec3 d = texture(sourceTexture, vTexCoord + vec2(-1*x, 1*y)).rgb;
        vec3 e = texture(sourceTexture, vTexCoord + vec2( 1*x, 1*y)).rgb;

        vec3 f = texture(sourceTexture, vTexCoord + vec2(-2*x, 0)).rgb;
        vec3 g = texture(sourceTexture, vTexCoord).rgb;
        vec3 h = texture(sourceTexture, vTexCoord + vec2( 2*x, 0)).rgb;

        vec3 i = texture(sourceTexture, vTexCoord + vec2(-1*x, -1*y)).rgb;
        vec3 j = texture(sourceTexture, vTexCoord + vec2( 1*x, -1*y)).rgb;

        vec3 k = texture(sourceTexture, vTexCoord + vec2(-2*x, -2*y)).rgb;
        vec3 l = texture(sourceTexture, vTexCoord + vec2( 0,   -2*y)).rgb;
        vec3 m = texture(sourceTexture, vTexCoord + vec2( 2*x, -2*y)).rgb;

        // Weighted average groups with Karis weight
        // Group 0 (Center)
        vec3 g0 = (d + e + i + j) * 0.25;
        // Group 1 (Top Left)
        vec3 g1 = (a + b + g + f) * 0.25;
        // Group 2 (Top Right)
        vec3 g2 = (b + c + h + g) * 0.25;
        // Group 3 (Bottom Left)
        vec3 g3 = (f + g + l + k) * 0.25;
        // Group 4 (Bottom Right)
        vec3 g4 = (g + h + m + l) * 0.25;

        // Apply Karis weights to groups to suppress fireflies
        // Note: We apply thresholding *after* downsampling to preserve energy of thin lines,
        // but we might want to threshold *before* to prevent dimming?
        // Standard approach: Downsample -> Threshold.
        // Karis approach: Weighted Average -> Threshold.
        
        float w0 = karisWeight(g0);
        float w1 = karisWeight(g1);
        float w2 = karisWeight(g2);
        float w3 = karisWeight(g3);
        float w4 = karisWeight(g4);

        vec3 sum = g0 * w0 * 0.5;
        sum += g1 * w1 * 0.125;
        sum += g2 * w2 * 0.125;
        sum += g3 * w3 * 0.125;
        sum += g4 * w4 * 0.125;
        
        float totalWeight = w0 * 0.5 + (w1 + w2 + w3 + w4) * 0.125;
        
        vec3 avgColor = sum / max(totalWeight, 0.0001);

        // Finally apply threshold
        vec3 result = applyThreshold(avgColor);

        fragColor = vec4(result, 1.0);
    }
)";

// Downsample: Dual Filter (13-tap) - Preserves shape better than Gaussian
// Based on "Next Generation Post Processing in Call of Duty: Advanced Warfare" (Jimenez 2014)
static const char* downsampleFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform vec2 srcResolution;

    in vec2 vTexCoord;
    out vec4 fragColor;

    void main()
    {
        vec2 texelSize = 1.0 / srcResolution;
        float x = texelSize.x;
        float y = texelSize.y;

        // Take 13 samples
        // a - b - c
        // - d - e -
        // f - g - h
        // - i - j -
        // k - l - m

        vec3 a = texture(sourceTexture, vTexCoord + vec2(-2*x, 2*y)).rgb;
        vec3 b = texture(sourceTexture, vTexCoord + vec2( 0,   2*y)).rgb;
        vec3 c = texture(sourceTexture, vTexCoord + vec2( 2*x, 2*y)).rgb;

        vec3 d = texture(sourceTexture, vTexCoord + vec2(-1*x, 1*y)).rgb;
        vec3 e = texture(sourceTexture, vTexCoord + vec2( 1*x, 1*y)).rgb;

        vec3 f = texture(sourceTexture, vTexCoord + vec2(-2*x, 0)).rgb;
        vec3 g = texture(sourceTexture, vTexCoord).rgb;
        vec3 h = texture(sourceTexture, vTexCoord + vec2( 2*x, 0)).rgb;

        vec3 i = texture(sourceTexture, vTexCoord + vec2(-1*x, -1*y)).rgb;
        vec3 j = texture(sourceTexture, vTexCoord + vec2( 1*x, -1*y)).rgb;

        vec3 k = texture(sourceTexture, vTexCoord + vec2(-2*x, -2*y)).rgb;
        vec3 l = texture(sourceTexture, vTexCoord + vec2( 0,   -2*y)).rgb;
        vec3 m = texture(sourceTexture, vTexCoord + vec2( 2*x, -2*y)).rgb;

        // Weighted average
        vec3 result = vec3(0.0);
        result += (d + e + i + j) * 0.5;      // Inner box
        result += (a + b + g + f) * 0.125;    // Top-left box
        result += (b + c + h + g) * 0.125;    // Top-right box
        result += (f + g + l + k) * 0.125;    // Bottom-left box
        result += (g + h + m + l) * 0.125;    // Bottom-right box

        fragColor = vec4(result * 0.25, 1.0); // Normalize (sum of weights = 4, so divide by 4)
    }
)";

// Upsample: Tent Filter (3x3) - Smooth reconstruction
static const char* upsampleFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform float filterRadius;
    uniform vec2 texelSize;

    in vec2 vTexCoord;
    out vec4 fragColor;

    void main()
    {
        // Scale offset by filterRadius (user setting) and actual texel size
        // This ensures we sample neighboring pixels correctly at any resolution
        vec2 offset = texelSize * filterRadius;

        vec3 result = vec3(0.0);

        // 9-tap tent filter
        // Center
        result += texture(sourceTexture, vTexCoord).rgb * 4.0;

        // Direct neighbors (Cross)
        result += texture(sourceTexture, vTexCoord + vec2(-offset.x, 0.0)).rgb * 2.0;
        result += texture(sourceTexture, vTexCoord + vec2( offset.x, 0.0)).rgb * 2.0;
        result += texture(sourceTexture, vTexCoord + vec2( 0.0, -offset.y)).rgb * 2.0;
        result += texture(sourceTexture, vTexCoord + vec2( 0.0,  offset.y)).rgb * 2.0;

        // Diagonals (Corners)
        result += texture(sourceTexture, vTexCoord + vec2(-offset.x, -offset.y)).rgb * 1.0;
        result += texture(sourceTexture, vTexCoord + vec2( offset.x, -offset.y)).rgb * 1.0;
        result += texture(sourceTexture, vTexCoord + vec2(-offset.x,  offset.y)).rgb * 1.0;
        result += texture(sourceTexture, vTexCoord + vec2( offset.x,  offset.y)).rgb * 1.0;

        fragColor = vec4(result * 0.0625, 1.0); // Divide by 16
    }
)";

// Final Combine
static const char* combineFragmentShader = R"(
    #version 330 core
    uniform sampler2D originalTexture;
    uniform sampler2D bloomTexture;
    uniform float intensity;

    in vec2 vTexCoord;
    out vec4 fragColor;

    // Simple hash for dithering
    float random(vec2 p) {
        return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
    }

    void main()
    {
        vec4 original = texture(originalTexture, vTexCoord);
        vec3 bloom = texture(bloomTexture, vTexCoord).rgb;

        // Additive combine of Scene + Bloom in LINEAR space
        // Tone mapping and gamma correction will happen in the final blit
        vec3 hdrColor = original.rgb + bloom * intensity;

        // Calculate alpha for compositing over transparent background
        float bloomAlpha = length(bloom * intensity);
        float finalAlpha = clamp(original.a + bloomAlpha, 0.0, 1.0);

        fragColor = vec4(hdrColor, finalAlpha);
    }
)";

// =============================================================================
// Implementation
// =============================================================================

BloomEffect::BloomEffect()
{
    // Reserve space for chain
    mipChain_.reserve(kMaxMipLevels);
    for (int i = 0; i < kMaxMipLevels; ++i)
        mipChain_.push_back(std::make_unique<Framebuffer>());
}

BloomEffect::~BloomEffect() = default;

bool BloomEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_) return true;

    // 1. Prefilter
    prefilterShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileEffectShader(*prefilterShader_, prefilterFragmentShader)) return false;
    prefilterThreshLoc_ = prefilterShader_->getUniformIDFromName("threshold");
    prefilterSoftKneeLoc_ = prefilterShader_->getUniformIDFromName("softKnee");
    prefilterResLoc_ = prefilterShader_->getUniformIDFromName("srcResolution");

    // 2. Downsample
    downsampleShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileEffectShader(*downsampleShader_, downsampleFragmentShader)) return false;
    downsampleResLoc_ = downsampleShader_->getUniformIDFromName("srcResolution");

    // 3. Upsample
    upsampleShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileEffectShader(*upsampleShader_, upsampleFragmentShader)) return false;
    upsampleFilterRadiusLoc_ = upsampleShader_->getUniformIDFromName("filterRadius");
    upsampleTexelSizeLoc_ = upsampleShader_->getUniformIDFromName("texelSize");

    // 4. Combine
    combineShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileEffectShader(*combineShader_, combineFragmentShader)) return false;
    combineOriginalLoc_ = combineShader_->getUniformIDFromName("originalTexture");
    combineBloomLoc_ = combineShader_->getUniformIDFromName("bloomTexture");
    combineIntensityLoc_ = combineShader_->getUniformIDFromName("intensity");

    compiled_ = true;
    return true;
}

void BloomEffect::release(juce::OpenGLContext& context)
{
    for (auto& fbo : mipChain_)
        fbo->destroy(context);

    prefilterShader_.reset();
    downsampleShader_.reset();
    upsampleShader_.reset();
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
    if (!compiled_ || !source || !destination) return;

    // 1. Check for resize
    int w = source->width;
    int h = source->height;

    if (w != lastWidth_ || h != lastHeight_)
    {
        for (int i = 0; i < kMaxMipLevels; ++i)
        {
            // Mip 0 is half res, Mip 1 is quarter, etc.
            // Use shift to divide by 2^(i+1)
            int mipW = std::max(1, w >> (i + 1));
            int mipH = std::max(1, h >> (i + 1));
            
            if (mipChain_[static_cast<size_t>(i)]->isValid())
                mipChain_[static_cast<size_t>(i)]->destroy(context);
            
            // Use Linear/Clamp for all mips (default in Framebuffer::create)
            mipChain_[static_cast<size_t>(i)]->create(context, mipW, mipH, 0, GL_RGBA16F, false);
        }
        lastWidth_ = w;
        lastHeight_ = h;
    }

    auto& ext = context.extensions;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // =========================================================================
    // Pass 1: Prefilter + Downsample to Mip 0
    // =========================================================================
    mipChain_[0]->bind();
    prefilterShader_->use();
    source->bindTexture(0);
    ext.glUniform1f(prefilterThreshLoc_, settings_.threshold);
    ext.glUniform1f(prefilterSoftKneeLoc_, settings_.softKnee);
    ext.glUniform2f(prefilterResLoc_, (float)source->width, (float)source->height);
    pool.renderFullscreenQuad();
    mipChain_[0]->unbind();

    // =========================================================================
    // Pass 2: Downsample Chain (Mip 0 -> 1 -> 2 -> 3 -> 4 -> 5)
    // =========================================================================
    downsampleShader_->use();
    for (int i = 0; i < kMaxMipLevels - 1; ++i)
    {
        Framebuffer* src = mipChain_[static_cast<size_t>(i)].get();
        Framebuffer* dst = mipChain_[static_cast<size_t>(i)+1].get();

        dst->bind();
        src->bindTexture(0);
        
        // Pass resolution of SOURCE for the dual filter offset calculations
        ext.glUniform2f(downsampleResLoc_, (float)src->width, (float)src->height);
        
        pool.renderFullscreenQuad();
        dst->unbind();
    }

    // =========================================================================
    // Pass 3: Upsample Chain (Mip 5 -> 4 -> 3 -> 2 -> 1 -> 0)
    // =========================================================================
    // Enable additive blending to accumulate the glow
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    upsampleShader_->use();
    ext.glUniform1f(upsampleFilterRadiusLoc_, settings_.spread); // Controls tent radius

    for (int i = kMaxMipLevels - 1; i > 0; --i)
    {
        Framebuffer* src = mipChain_[static_cast<size_t>(i)].get();
        Framebuffer* dst = mipChain_[static_cast<size_t>(i)-1].get();

        dst->bind();
        src->bindTexture(0);
        
        // Pass texel size of the SOURCE (the smaller mip we are upsampling from)
        // This ensures the filter kernel size is relative to the source pixels
        ext.glUniform2f(upsampleTexelSizeLoc_, 1.0f / (float)src->width, 1.0f / (float)src->height);

        pool.renderFullscreenQuad();
        dst->unbind();
    }

    // =========================================================================
    // Pass 4: Combine
    // =========================================================================
    // Switch to standard blending for the final composition
    glDisable(GL_BLEND); 

    destination->bind();
    combineShader_->use();

    source->bindTexture(0);
    ext.glUniform1i(combineOriginalLoc_, 0);

    // Mip 0 now contains the accumulated bloom from all levels
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mipChain_[0]->colorTexture);
    ext.glUniform1i(combineBloomLoc_, 1);

    ext.glUniform1f(combineIntensityLoc_, settings_.intensity);

    pool.renderFullscreenQuad();

    // Cleanup
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    destination->unbind();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
