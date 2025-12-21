/*
    Oscil - Oscillator State Manager
    Handles persistence and logic for oscillator state within the ValueTree
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "core/Oscillator.h"
#include "core/interfaces/IInstanceRegistry.h"
#include <vector>
#include <optional>

namespace oscil
{

/**
 * Manages the "Oscillators" subtree of the application state.
 * Encapsulates logic for adding, removing, updating, and querying oscillators.
 */
class OscillatorStateManager
{
public:
    OscillatorStateManager();

    /**
     * Update the underlying ValueTree node managed by this class.
     * Call this when the root state is replaced (e.g. loading from XML).
     */
    void setStatsNode(juce::ValueTree oscillatorsNode);

    /**
     * Get all oscillators
     */
    [[nodiscard]] std::vector<Oscillator> getOscillators() const;

    /**
     * Get the number of oscillators
     */
    [[nodiscard]] size_t getOscillatorCount() const;

    /**
     * Add an oscillator
     */
    void addOscillator(const Oscillator& oscillator);

    /**
     * Remove an oscillator
     */
    void removeOscillator(const OscillatorId& oscillatorId);

    /**
     * Update an oscillator
     */
    void updateOscillator(const Oscillator& oscillator);

    /**
     * Reorder oscillators - moves oscillator from oldIndex to newIndex
     */
    void reorderOscillators(int oldIndex, int newIndex);

    /**
     * Get oscillator by ID
     */
    [[nodiscard]] std::optional<Oscillator> getOscillator(const OscillatorId& oscillatorId) const;

    /**
     * Rebind oscillators to sources after state load.
     */
    void rebindOscillatorsToSources(IInstanceRegistry& registry);

    /**
     * Handle source removal
     */
    void handleSourceRemoved(const SourceId& sourceId);

private:
    juce::ValueTree state_;

    // Helpers
    juce::ValueTree findOscillatorNode(const OscillatorId& id) const;
};

} // namespace oscil
