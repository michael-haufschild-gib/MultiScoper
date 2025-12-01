/*
    Oscil - Effect Provider Interface
    Abstract interface for retrieving effects, allowing decoupling of EffectChain from RenderEngine.
*/

#pragma once

#include <juce_core/juce_core.h>

namespace oscil
{

class PostProcessEffect;

class IEffectProvider
{
public:
    virtual ~IEffectProvider() = default;

    /**
     * Get a post-process effect by ID.
     * @param effectId The identifier of the effect.
     * @return Pointer to the effect, or nullptr if not found.
     */
    virtual PostProcessEffect* getEffect(const juce::String& effectId) = 0;
};

} // namespace oscil
