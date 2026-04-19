/*
    MultiScoper - Bloom Effect Implementation (Next-Gen Dual Filter)
    Compile + bind + pass orchestration. GLSL shader source strings live
    in BloomEffectShaders.cpp to keep this translation unit focused on
    OpenGL object lifecycle and the bloom chain.
*/

#include "rendering/effects/BloomEffect.h"

#if MULTISCOPER_ENABLE_OPENGL

namespace multiscoper
{

using namespace juce::gl;

// Shader sources (extern from BloomEffectShaders.cpp)
namespace detail
{
extern const char* kBloomPrefilterFragmentShader;
extern const char* kBloomDownsampleFragmentShader;
extern const char* kBloomUpsampleFragmentShader;
extern const char* kBloomCombineFragmentShader;
} // namespace detail

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

bool BloomEffect::compilePrefilterStage(juce::OpenGLContext& context)
{
    prefilterShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileEffectShader(*prefilterShader_, detail::kBloomPrefilterFragmentShader))
        return false;
    prefilterThreshLoc_ = prefilterShader_->getUniformIDFromName("threshold");
    prefilterSoftKneeLoc_ = prefilterShader_->getUniformIDFromName("softKnee");
    prefilterResLoc_ = prefilterShader_->getUniformIDFromName("srcResolution");
    if (prefilterThreshLoc_ < 0 || prefilterSoftKneeLoc_ < 0 || prefilterResLoc_ < 0)
    {
        DBG("BloomEffect: Missing prefilter shader uniforms");
        return false;
    }
    return true;
}

bool BloomEffect::compileDownsampleStage(juce::OpenGLContext& context)
{
    downsampleShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileEffectShader(*downsampleShader_, detail::kBloomDownsampleFragmentShader))
        return false;
    downsampleResLoc_ = downsampleShader_->getUniformIDFromName("srcResolution");
    if (downsampleResLoc_ < 0)
    {
        DBG("BloomEffect: Missing downsample shader uniforms");
        return false;
    }
    return true;
}

bool BloomEffect::compileUpsampleStage(juce::OpenGLContext& context)
{
    upsampleShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileEffectShader(*upsampleShader_, detail::kBloomUpsampleFragmentShader))
        return false;
    upsampleFilterRadiusLoc_ = upsampleShader_->getUniformIDFromName("filterRadius");
    upsampleTexelSizeLoc_ = upsampleShader_->getUniformIDFromName("texelSize");
    if (upsampleFilterRadiusLoc_ < 0 || upsampleTexelSizeLoc_ < 0)
    {
        DBG("BloomEffect: Missing upsample shader uniforms");
        return false;
    }
    return true;
}

bool BloomEffect::compileCombineStage(juce::OpenGLContext& context)
{
    combineShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileEffectShader(*combineShader_, detail::kBloomCombineFragmentShader))
        return false;
    combineOriginalLoc_ = combineShader_->getUniformIDFromName("originalTexture");
    combineBloomLoc_ = combineShader_->getUniformIDFromName("bloomTexture");
    combineIntensityLoc_ = combineShader_->getUniformIDFromName("intensity");
    if (combineOriginalLoc_ < 0 || combineBloomLoc_ < 0 || combineIntensityLoc_ < 0)
    {
        DBG("BloomEffect: Missing combine shader uniforms");
        return false;
    }
    return true;
}

bool BloomEffect::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    if (!compilePrefilterStage(context) || !compileDownsampleStage(context) || !compileUpsampleStage(context) ||
        !compileCombineStage(context))
    {
        release(context);
        return false;
    }

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

bool BloomEffect::isCompiled() const { return compiled_; }

void BloomEffect::resizeMipChain(juce::OpenGLContext& context, int w, int h)
{
    for (int i = 0; i < kMaxMipLevels; ++i)
    {
        int const mipW = std::max(1, w >> (i + 1));
        int const mipH = std::max(1, h >> (i + 1));

        if (mipChain_[static_cast<size_t>(i)]->isValid())
            mipChain_[static_cast<size_t>(i)]->destroy(context);

        if (!mipChain_[static_cast<size_t>(i)]->create(context, mipW, mipH, 0, GL_RGBA16F, false))
        {
            DBG("BloomEffect: Failed to create mip level " << i << " (" << mipW << "x" << mipH << ")");
            // Destroy all mips (including any from a previous resize) and mark as failed
            for (auto& mip : mipChain_)
            {
                if (mip->isValid())
                    mip->destroy(context);
            }
            lastWidth_ = 0;
            lastHeight_ = 0;
            return;
        }
    }
    lastWidth_ = w;
    lastHeight_ = h;
}

void BloomEffect::passPrefilter(Framebuffer* source, FramebufferPool& pool)
{
    mipChain_[0]->bind();
    prefilterShader_->use();
    source->bindTexture(0);
    juce::OpenGLExtensionFunctions::glUniform1f(prefilterThreshLoc_, settings_.threshold);
    juce::OpenGLExtensionFunctions::glUniform1f(prefilterSoftKneeLoc_, settings_.softKnee);
    juce::OpenGLExtensionFunctions::glUniform2f(prefilterResLoc_, static_cast<float>(source->width),
                                                static_cast<float>(source->height));
    pool.renderFullscreenQuad();
    mipChain_[0]->unbind();
}

void BloomEffect::passDownsample(FramebufferPool& pool)
{
    downsampleShader_->use();
    for (int i = 0; i < kMaxMipLevels - 1; ++i)
    {
        Framebuffer* src = mipChain_[static_cast<size_t>(i)].get();
        Framebuffer* dst = mipChain_[static_cast<size_t>(i) + 1].get();
        dst->bind();
        src->bindTexture(0);
        juce::OpenGLExtensionFunctions::glUniform2f(downsampleResLoc_, static_cast<float>(src->width),
                                                    static_cast<float>(src->height));
        pool.renderFullscreenQuad();
        dst->unbind();
    }
}

void BloomEffect::passUpsample(FramebufferPool& pool)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    upsampleShader_->use();
    juce::OpenGLExtensionFunctions::glUniform1f(upsampleFilterRadiusLoc_, settings_.spread);

    for (int i = kMaxMipLevels - 1; i > 0; --i)
    {
        Framebuffer* src = mipChain_[static_cast<size_t>(i)].get();
        Framebuffer* dst = mipChain_[static_cast<size_t>(i) - 1].get();
        dst->bind();
        src->bindTexture(0);
        juce::OpenGLExtensionFunctions::glUniform2f(upsampleTexelSizeLoc_, 1.0f / static_cast<float>(src->width),
                                                    1.0f / static_cast<float>(src->height));
        pool.renderFullscreenQuad();
        dst->unbind();
    }

    glDisable(GL_BLEND);
}

void BloomEffect::passCombine(Framebuffer* source, Framebuffer* destination, FramebufferPool& pool)
{
    destination->bind();
    combineShader_->use();

    source->bindTexture(0);
    juce::OpenGLExtensionFunctions::glUniform1i(combineOriginalLoc_, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mipChain_[0]->colorTexture);
    juce::OpenGLExtensionFunctions::glUniform1i(combineBloomLoc_, 1);
    juce::OpenGLExtensionFunctions::glUniform1f(combineIntensityLoc_, settings_.intensity);

    pool.renderFullscreenQuad();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    destination->unbind();
}

void BloomEffect::apply(juce::OpenGLContext& context, Framebuffer* source, Framebuffer* destination,
                        FramebufferPool& pool, float deltaTime)
{
    juce::ignoreUnused(deltaTime);
    if (!compiled_ || !source || !destination)
        return;

    if (source->width != lastWidth_ || source->height != lastHeight_)
        resizeMipChain(context, source->width, source->height);

    // If mip chain allocation failed, pass through source unmodified
    if (lastWidth_ == 0 || lastHeight_ == 0)
    {
        juce::gl::glBindFramebuffer(juce::gl::GL_READ_FRAMEBUFFER, source->fbo);
        juce::gl::glBindFramebuffer(juce::gl::GL_DRAW_FRAMEBUFFER, destination->fbo);
        juce::gl::glBlitFramebuffer(0, 0, source->width, source->height, 0, 0, destination->width, destination->height,
                                    GL_COLOR_BUFFER_BIT, GL_LINEAR);
        juce::gl::glBindFramebuffer(juce::gl::GL_FRAMEBUFFER, 0);
        return;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    passPrefilter(source, pool);
    passDownsample(pool);
    passUpsample(pool);
    passCombine(source, destination, pool);
}

} // namespace multiscoper

#endif // MULTISCOPER_ENABLE_OPENGL
