/*
    Oscil - Effect Chain Implementation
*/

#include "rendering/EffectChain.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

void EffectChain::addStep(EffectStep step) { steps_.push_back(std::move(step)); }

void EffectChain::clear() { steps_.clear(); }

Framebuffer* EffectChain::process(juce::OpenGLContext& context, Framebuffer* source, FramebufferPool& pool,
                                  float deltaTime, const VisualConfiguration& config, IEffectProvider& effectProvider)
{
    Framebuffer* current = source;
    Framebuffer* ping = pool.getPingFBO();
    Framebuffer* pong = pool.getPongFBO();

    for (const auto& step : steps_)
    {
        // Check if effect is enabled in config
        if (!step.isEnabled(config))
            continue;

        // Retrieve effect instance
        auto* effect = effectProvider.getEffect(step.effectId);
        if (!effect || !effect->isCompiled() || !effect->isEnabled())
            continue;

        // Configure effect
        if (step.configure)
        {
            step.configure(effect, config);
        }

        // Determine destination
        // If current is ping, dest is pong. If current is source (or pong), dest is ping.
        Framebuffer* dest = (current == ping) ? pong : ping;

        if (!dest || !dest->isValid())
        {
            jassertfalse; // Pool FBO invalid — indicates FramebufferPool setup failure
            DBG("EffectChain: dest FBO null/invalid for effect '" << step.effectId << "' — skipping");
            continue;
        }

        // Apply effect — effects own their FBO bind/unbind lifecycle
        // (multi-pass effects like Bloom need fine-grained FBO control)
        effect->apply(context, current, dest, pool, deltaTime);

        // Swap
        current = dest;
    }

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

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
