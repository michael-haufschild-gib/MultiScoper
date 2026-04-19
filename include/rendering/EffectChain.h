/*
    MultiScoper - Effect Chain
    Manages the sequential application of post-processing effects
*/

#pragma once

#include "Framebuffer.h"
#include "FramebufferPool.h"
#include "IEffectProvider.h"
#include "VisualConfiguration.h"
#include "effects/PostProcessEffect.h"

#include <functional>
#include <string>
#include <vector>

#if MULTISCOPER_ENABLE_OPENGL

namespace multiscoper
{

/**
 * Represents a single step in the post-processing chain.
 */
struct EffectStep
{
    juce::String effectId;
    std::function<bool(const VisualConfiguration&)> isEnabled;
    std::function<void(PostProcessEffect*, const VisualConfiguration&)> configure;
};

/**
 * Manages a chain of post-processing effects.
 * Handles ping-pong buffering and conditional execution.
 */
class EffectChain
{
public:
    EffectChain() = default;
    virtual ~EffectChain() = default;

    /**
     * Add an effect step to the end of the chain.
     * @param step The effect configuration step
     */
    void addStep(EffectStep step);

    /**
     * Clear the chain.
     */
    void clear();

    /**
     * Process the chain.
     * @param context OpenGL context
     * @param source Input framebuffer
     * @param pool Framebuffer pool for temp buffers
     * @param deltaTime Time step
     * @param config Current visual configuration
     * @param effectProvider Reference to IEffectProvider to retrieve effect instances
     * @return The final output framebuffer (might be source, or a pool buffer)
     */
    virtual Framebuffer* process(juce::OpenGLContext& context, Framebuffer* source, FramebufferPool& pool,
                                 float deltaTime, const VisualConfiguration& config, IEffectProvider& effectProvider);

protected:
    /**
     * Bind framebuffer for rendering. Virtual for testing.
     */
    virtual void bindFramebuffer(Framebuffer* fb);

    /**
     * Unbind framebuffer. Virtual for testing.
     */
    virtual void unbindFramebuffer(Framebuffer* fb);

private:
    /// Bundle of per-process() state shared across all steps — keeps applyStep
    /// parameter count within clang-tidy readability thresholds.
    struct StepContext
    {
        juce::OpenGLContext& glContext;
        FramebufferPool& pool;
        Framebuffer* ping;
        Framebuffer* pong;
        const VisualConfiguration& config;
        IEffectProvider& effectProvider;
        float deltaTime;
    };

    /// Apply a single step, returning the framebuffer to use as the next input
    /// (unchanged `current` if the step was skipped).
    static Framebuffer* applyStep(const EffectStep& step, const StepContext& ctx, Framebuffer* current);

    std::vector<EffectStep> steps_;
};

} // namespace multiscoper

#endif // MULTISCOPER_ENABLE_OPENGL
