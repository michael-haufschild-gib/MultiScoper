/*
    Oscil - Test Server Utilities
    Shared helper functions for test server endpoint handlers
*/

#pragma once

#include "core/Oscillator.h"

#include <string>
#include <vector>

namespace oscil
{

/**
 * Resolve an OscillatorId from either a string ID or a positional index.
 * If idStr is non-empty, uses it directly. Otherwise looks up by index.
 * Returns an invalid OscillatorId if neither resolves.
 */
inline OscillatorId resolveOscillatorId(const std::string& idStr, int index, const std::vector<Oscillator>& oscillators)
{
    OscillatorId targetId;
    if (!idStr.empty())
        targetId.id = juce::String(idStr);
    else if (index >= 0 && std::cmp_less(index, oscillators.size()))
        targetId = oscillators[static_cast<size_t>(index)].getId();
    return targetId;
}

} // namespace oscil
