/*
    Oscil - 3D Settings Serialization
    Helper for serializing 3D settings, lighting, and material
*/

#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "rendering/VisualConfiguration.h"

namespace oscil
{

class Settings3DSerialization
{
public:
    static juce::ValueTree toValueTree(const Settings3D& settings);
    static Settings3D fromValueTree(const juce::ValueTree& tree);
    static juce::var toJson(const Settings3D& settings);
    static Settings3D fromJson(const juce::var& json);

    static juce::ValueTree toValueTree(const LightingConfig& settings);
    static LightingConfig lightingFromValueTree(const juce::ValueTree& tree);
    static juce::var toJson(const LightingConfig& settings);
    static LightingConfig lightingFromJson(const juce::var& json);

    static juce::ValueTree toValueTree(const MaterialSettings& settings);
    static MaterialSettings materialFromValueTree(const juce::ValueTree& tree);
    static juce::var toJson(const MaterialSettings& settings);
    static MaterialSettings materialFromJson(const juce::var& json);
};

} // namespace oscil
