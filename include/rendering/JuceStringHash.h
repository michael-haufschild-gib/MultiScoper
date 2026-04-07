/*
    Oscil - JuceStringHash
    Hash functor for juce::String in std::unordered_map
*/

#pragma once

#include <juce_core/juce_core.h>

#include <cstddef>
#include <cstdint>

namespace oscil
{

struct JuceStringHash
{
    std::size_t operator()(const juce::String& s) const
    {
        return static_cast<std::size_t>(static_cast<uint32_t>(s.hashCode()));
    }
};

} // namespace oscil
