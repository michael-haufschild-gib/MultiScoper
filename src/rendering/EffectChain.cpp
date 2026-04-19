/*
    MultiScoper - Effect Chain Implementation
*/

#include "rendering/EffectChain.h"

#if MULTISCOPER_ENABLE_OPENGL

namespace multiscoper
{

using namespace juce::gl;

void EffectChain::addStep(EffectStep step) { steps_.push_back(std::move(step)); }

void EffectChain::clear() { steps_.clear(); }

// static
Framebuffer* EffectChain::applyStep(const EffectStep& step, const StepContext& ctx, Framebuffer* current)
{
    if (!step.isEnabled(ctx.config))
        return current;

    auto* effect = ctx.effectProvider.getEffect(step.effectId);
    if (!effect || !effect->isCompiled() || !effect->isEnabled())
        return current;

    if (step.configure)
        step.configure(effect, ctx.config);

    Framebuffer* dest = (current == ctx.ping) ? ctx.pong : ctx.ping;
    if (!dest || !dest->isValid())
    {
        jassertfalse; // Pool FBO invalid — indicates FramebufferPool setup failure
        DBG("EffectChain: dest FBO null/invalid for effect '" << step.effectId << "' — skipping");
        return current;
    }

    // Effects own their FBO bind/unbind lifecycle (multi-pass effects like Bloom need fine-grained control)
    effect->apply(ctx.glContext, current, dest, ctx.pool, ctx.deltaTime);
    return dest;
}

Framebuffer* EffectChain::process(juce::OpenGLContext& context, Framebuffer* source, FramebufferPool& pool,
                                  float deltaTime, const VisualConfiguration& config, IEffectProvider& effectProvider)
{
    const StepContext ctx{
        .glContext = context,
        .pool = pool,
        .ping = pool.getPingFBO(),
        .pong = pool.getPongFBO(),
        .config = config,
        .effectProvider = effectProvider,
        .deltaTime = deltaTime,
    };

    Framebuffer* current = source;
    for (const auto& step : steps_)
        current = applyStep(step, ctx, current);

    return current;
}

void EffectChain::bindFramebuffer(Framebuffer* fb)
{
    if (fb)
        fb->bind();
}

void EffectChain::unbindFramebuffer(Framebuffer* fb)
{
    if (fb)
        fb->unbind();
}

} // namespace multiscoper

#endif // MULTISCOPER_ENABLE_OPENGL
