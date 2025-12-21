/*
    Oscil - Oscillator State Manager Implementation
*/

#include "core/OscillatorStateManager.h"
#include "core/OscilState.h" // For StateIds

namespace oscil
{

OscillatorStateManager::OscillatorStateManager()
{
}

void OscillatorStateManager::setStatsNode(juce::ValueTree oscillatorsNode)
{
    state_ = oscillatorsNode;
}

std::vector<Oscillator> OscillatorStateManager::getOscillators() const
{
    std::vector<Oscillator> result;
    if (!state_.isValid())
        return result;

    for (int i = 0; i < state_.getNumChildren(); ++i)
    {
        auto child = state_.getChild(i);
        if (child.hasType(StateIds::Oscillator))
        {
            result.emplace_back(child);
        }
    }

    return result;
}

size_t OscillatorStateManager::getOscillatorCount() const
{
    if (!state_.isValid())
        return 0;
    return static_cast<size_t>(state_.getNumChildren());
}

void OscillatorStateManager::addOscillator(const Oscillator& oscillator)
{
    if (state_.isValid())
    {
        state_.appendChild(oscillator.toValueTree(), nullptr);
    }
}

void OscillatorStateManager::removeOscillator(const OscillatorId& oscillatorId)
{
    if (!state_.isValid())
        return;

    int removedIndex = -1;

    // Find and remove the oscillator
    for (int i = 0; i < state_.getNumChildren(); ++i)
    {
        auto child = state_.getChild(i);
        if (child.getProperty(StateIds::Id).toString() == oscillatorId.id)
        {
            removedIndex = child.getProperty(StateIds::Order, i);
            state_.removeChild(i, nullptr);
            break;
        }
    }

    // Renumber remaining oscillators to maintain contiguous indices
    if (removedIndex >= 0)
    {
        for (int i = 0; i < state_.getNumChildren(); ++i)
        {
            auto child = state_.getChild(i);
            int currentIndex = child.getProperty(StateIds::Order, i);
            if (currentIndex > removedIndex)
            {
                child.setProperty(StateIds::Order, currentIndex - 1, nullptr);
            }
        }
    }
}

void OscillatorStateManager::updateOscillator(const Oscillator& oscillator)
{
    if (!state_.isValid())
        return;

    for (int i = 0; i < state_.getNumChildren(); ++i)
    {
        auto child = state_.getChild(i);
        if (child.getProperty(StateIds::Id).toString() == oscillator.getId().id)
        {
            // Update properties
            child.copyPropertiesFrom(oscillator.toValueTree(), nullptr);
            
            // Update children (e.g. VisualOverrides)
            child.removeAllChildren(nullptr);
            auto srcTree = oscillator.toValueTree();
            for (int j = 0; j < srcTree.getNumChildren(); ++j)
            {
                child.appendChild(srcTree.getChild(j).createCopy(), nullptr);
            }
            return;
        }
    }
}

void OscillatorStateManager::reorderOscillators(int oldIndex, int newIndex)
{
    if (!state_.isValid())
        return;

    if (oldIndex == newIndex) return;

    int numChildren = state_.getNumChildren();

    if (oldIndex < 0 || oldIndex >= numChildren ||
        newIndex < 0 || newIndex >= numChildren)
    {
        return;
    }

    for (int i = 0; i < numChildren; ++i)
    {
        auto child = state_.getChild(i);
        int currentOrder = child.getProperty(StateIds::Order, i);

        if (currentOrder == oldIndex)
        {
            child.setProperty(StateIds::Order, newIndex, nullptr);
        }
        else if (oldIndex < newIndex)
        {
            if (currentOrder > oldIndex && currentOrder <= newIndex)
            {
                child.setProperty(StateIds::Order, currentOrder - 1, nullptr);
            }
        }
        else // oldIndex > newIndex
        {
            if (currentOrder >= newIndex && currentOrder < oldIndex)
            {
                child.setProperty(StateIds::Order, currentOrder + 1, nullptr);
            }
        }
    }
}

std::optional<Oscillator> OscillatorStateManager::getOscillator(const OscillatorId& oscillatorId) const
{
    if (!state_.isValid())
        return std::nullopt;

    for (int i = 0; i < state_.getNumChildren(); ++i)
    {
        auto child = state_.getChild(i);
        if (child.getProperty(StateIds::Id).toString() == oscillatorId.id)
        {
            return Oscillator(child);
        }
    }

    return std::nullopt;
}

void OscillatorStateManager::rebindOscillatorsToSources(IInstanceRegistry& registry)
{
    if (!state_.isValid())
        return;

    // Use passed instanceTrackId? 
    // Wait, the logic in OscilState used oscillatorTree.getProperty(StateIds::TrackIdentifier).
    // Ah, `OscilState::rebindOscillatorsToSources` logic:
    /*
        juce::String trackIdentifier = oscillatorTree.getProperty(StateIds::TrackIdentifier, "").toString();
    */
    // The `instanceTrackId` parameter I added might be redundant or for a different purpose?
    // In `OscilState.cpp`, `rebindOscillatorsToSources` logic uses `registry.getSourceByTrackIdentifier(trackIdentifier)`.
    // It DOES NOT use `OscilState::getInstanceTrackId()`.
    
    // So I can remove `instanceTrackId` from the signature if it's unused.
    // However, I declared it in the header. I should check if I need it.
    // Logic:
    /*
        auto sourceInfo = registry.getSourceByTrackIdentifier(trackIdentifier);
    */
    // Yeah, it depends on the oscillator's stored trackIdentifier.
    
    for (int i = 0; i < state_.getNumChildren(); ++i)
    {
        auto oscillatorTree = state_.getChild(i);
        if (!oscillatorTree.hasType(StateIds::Oscillator))
            continue;

        juce::String trackIdentifier = oscillatorTree.getProperty(StateIds::TrackIdentifier, "").toString();
        if (trackIdentifier.isEmpty())
            continue;

        auto sourceInfo = registry.getSourceByTrackIdentifier(trackIdentifier);
        if (sourceInfo.has_value())
        {
            oscillatorTree.setProperty(StateIds::SourceId, sourceInfo->sourceId.id, nullptr);
            oscillatorTree.setProperty(StateIds::OscillatorState,
                                        static_cast<int>(OscillatorState::ACTIVE), nullptr);
        }
        else
        {
            oscillatorTree.setProperty(StateIds::OscillatorState,
                                        static_cast<int>(OscillatorState::NO_SOURCE), nullptr);
        }
    }
}

void OscillatorStateManager::handleSourceRemoved(const SourceId& sourceId)
{
    if (!state_.isValid())
        return;

    for (int i = 0; i < state_.getNumChildren(); ++i)
    {
        auto oscillatorTree = state_.getChild(i);
        if (!oscillatorTree.hasType(StateIds::Oscillator))
            continue;

        juce::String oscSourceId = oscillatorTree.getProperty(StateIds::SourceId, "").toString();
        if (oscSourceId == sourceId.id)
        {
            oscillatorTree.setProperty(StateIds::OscillatorState,
                                        static_cast<int>(OscillatorState::NO_SOURCE), nullptr);
            oscillatorTree.setProperty(StateIds::SourceId, "", nullptr);
        }
    }
}

juce::ValueTree OscillatorStateManager::findOscillatorNode(const OscillatorId& id) const
{
    if (!state_.isValid())
        return {};

    for (int i = 0; i < state_.getNumChildren(); ++i)
    {
        auto child = state_.getChild(i);
        if (child.getProperty(StateIds::Id).toString() == id.id)
            return child;
    }
    return {};
}

} // namespace oscil
