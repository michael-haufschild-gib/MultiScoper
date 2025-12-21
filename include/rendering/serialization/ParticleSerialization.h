/*
    Oscil - Particle Serialization
    Helper for serializing particle settings
*/

#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "rendering/VisualConfiguration.h"

namespace oscil
{

class ParticleSerialization
{
public:
    static juce::ValueTree toValueTree(const ParticleSettings& settings);
    static ParticleSettings fromValueTree(const juce::ValueTree& tree);
    
    static juce::var toJson(const ParticleSettings& settings);
    static ParticleSettings fromJson(const juce::var& json);
};

} // namespace oscil
