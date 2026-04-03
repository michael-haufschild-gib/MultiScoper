/*
    Oscil - Effect Pipeline
    Manages framebuffers and post-processing effects.
*/

#pragma once

#include "rendering/EffectChain.h"
#include "rendering/Framebuffer.h"
#include "rendering/FramebufferPool.h"
#include "rendering/IEffectProvider.h"
#include "rendering/RenderCommon.h"
#include "rendering/WaveformRenderState.h"
#include "rendering/effects/PostProcessEffect.h"

#include <juce_opengl/juce_opengl.h>

#include <memory>
#include <unordered_map>

namespace oscil
{

class EffectPipeline : public IEffectProvider
{
public:
    /// Create an uninitialized effect pipeline.
    EffectPipeline();
    ~EffectPipeline() override;

    /// Initialize framebuffer pool, scene FBO, and all post-processing effects.
    bool initialize(juce::OpenGLContext& context, int width, int height);
    /// Release all GPU resources and effects.
    void shutdown(juce::OpenGLContext& context);
    /// Resize all framebuffers and the scene FBO to new dimensions.
    void resize(juce::OpenGLContext& context, int width, int height);

    /**
     * Apply per-waveform post-processing.
     * @return The framebuffer containing the result (or source if no effects applied).
     */
    Framebuffer* applyPostProcessing(Framebuffer* source, WaveformRenderState& state, juce::OpenGLContext& context,
                                     float deltaTime, juce::OpenGLShaderProgram* compositeShader, GLint compositeLoc);

    /**
     * Get a post-process effect by ID.
     */
    PostProcessEffect* getEffect(const juce::String& effectId) override;

    void setGlobalPostProcessingEnabled(bool enabled) { globalPostProcessEnabled_ = enabled; }
    [[nodiscard]] bool isGlobalPostProcessingEnabled() const { return globalPostProcessEnabled_; }

    FramebufferPool* getFramebufferPool() { return fbPool_.get(); }
    Framebuffer* getSceneFBO() { return sceneFBO_.get(); }

    void setQualityLevel(QualityLevel level);

    /// Copy the contents of one framebuffer to another using the composite shader.
    void copyFramebuffer(juce::OpenGLContext& context, Framebuffer* source, Framebuffer* destination,
                         juce::OpenGLShaderProgram* compositeShader, GLint compositeTextureLoc);

private:
    void initializeEffects();
    void createEffectInstances();
    void buildEffectChain();
    void releaseEffects();

    std::unique_ptr<FramebufferPool> fbPool_;
    std::unique_ptr<Framebuffer> sceneFBO_;

    std::unordered_map<juce::String, std::unique_ptr<PostProcessEffect>> effects_;
    EffectChain effectChain_;

    bool globalPostProcessEnabled_ = true;

    // Cache context for lazy compilation if needed (though methods pass context)
    // Better to pass context to getEffect if possible, but IEffectProvider interface might not allow it.
    // We'll store a pointer to context only during active use if needed, or rely on lazy compilation happening during
    // render.
    juce::OpenGLContext* context_ = nullptr;
};

} // namespace oscil
