/*
    MultiScoper - State Management Implementation
*/

#include "core/MultiScoperState.h"

#include "core/MultiScoperLog.h"
#include "core/SchemaMigration.h"

#include <algorithm>
#include <utility>
namespace multiscoper
{

MultiScoperState::MultiScoperState() { initializeDefaultState(); }

MultiScoperState::MultiScoperState(const juce::String& xmlString)
{
    initializeDefaultState();
    (void) fromXmlString(xmlString);
}

void MultiScoperState::initializeDefaultState()
{
    state_ = juce::ValueTree(StateIds::MultiScoperState);
    state_.setProperty(StateIds::Version, CURRENT_SCHEMA_VERSION, nullptr);

    // Create sub-trees
    state_.appendChild(juce::ValueTree(StateIds::Oscillators), nullptr);
    state_.appendChild(juce::ValueTree(StateIds::Panes), nullptr);

    auto layout = juce::ValueTree(StateIds::Layout);
    layout.setProperty(StateIds::Columns, 1, nullptr);
    state_.appendChild(layout, nullptr);

    auto theme = juce::ValueTree(StateIds::Theme);
    theme.setProperty(StateIds::ThemeName, "Dark Professional", nullptr);
    state_.appendChild(theme, nullptr);

    // Create empty Timing node (will be populated by TimingEngine on save)
    state_.appendChild(juce::ValueTree(StateIds::Timing), nullptr);
}

juce::String MultiScoperState::toXmlString()
{
    // Sync layoutManager_ back to state_ before serialization
    // This ensures pane changes made at runtime are persisted
    syncLayoutManagerToState();

    if (auto xml = state_.createXml())
    {
        auto xmlStr = xml->toString();
        MULTISCOPER_LOG(STATE, "toXmlString: " << xmlStr.length() << "ch " << getOscillatorsNode().getNumChildren()
                                               << "osc " << layoutManager_.getPaneCount() << "panes");
        return xmlStr;
    }
    MULTISCOPER_LOG(STATE, "toXmlString: FAILED to create XML");
    return {};
}

void MultiScoperState::syncLayoutManagerToState()
{
    // Remove existing Panes node and replace with current layoutManager state
    auto existingPanes = state_.getChildWithName(StateIds::Panes);
    if (existingPanes.isValid())
    {
        state_.removeChild(existingPanes, nullptr);
    }

    // Add the current pane layout from layoutManager_
    state_.appendChild(layoutManager_.toValueTree(), nullptr);
}

bool MultiScoperState::fromXmlString(const juce::String& xmlString)
{
    if (xmlString.isEmpty())
    {
        juce::Logger::writeToLog("MultiScoperState::fromXmlString: empty input");
        return false;
    }

    auto xml = juce::XmlDocument::parse(xmlString);
    if (!xml)
    {
        juce::Logger::writeToLog("MultiScoperState::fromXmlString: XML parse failed (" +
                                 juce::String(xmlString.length()) + " chars)");
        return false;
    }

    auto loadedState = juce::ValueTree::fromXml(*xml);
    if (!loadedState.isValid() || !loadedState.hasType(StateIds::MultiScoperState))
    {
        juce::Logger::writeToLog("MultiScoperState::fromXmlString: invalid root node type '" +
                                 loadedState.getType().toString() + "' (expected '" +
                                 StateIds::MultiScoperState.toString() + "')");
        return false;
    }

    // Fail closed on unsupported schema versions. Downgrades, unknown-future
    // versions, and gaps in the migration chain all reject the load and
    // preserve the current in-memory state.
    int const loadedVersion = loadedState.getProperty(StateIds::Version, 0);
    auto const migrationResult = migration::migrateMultiScoperState(loadedState, loadedVersion, CURRENT_SCHEMA_VERSION);
    if (migrationResult != migration::MigrationResult::Success)
    {
        juce::Logger::writeToLog(juce::String("MultiScoperState::fromXmlString: migration rejected (from v") +
                                 juce::String(loadedVersion) + " to v" + juce::String(CURRENT_SCHEMA_VERSION) +
                                 "): " + migration::migrationResultToString(migrationResult));
        return false;
    }
    state_ = loadedState;

    // Load layout manager state
    auto panesNode = getPanesNode();
    if (panesNode.isValid())
    {
        layoutManager_.fromValueTree(panesNode);
    }

    auto layoutNode = getLayoutNode();
    if (layoutNode.isValid())
    {
        int const cols = layoutNode.getProperty(StateIds::Columns, 1);
        layoutManager_.setColumnLayout(static_cast<ColumnLayout>(cols));
    }

    MULTISCOPER_LOG(STATE, "fromXmlString: " << getOscillatorCount() << "osc " << layoutManager_.getPaneCount()
                                             << "panes v" << getSchemaVersion());
    return true;
}

std::vector<Oscillator> MultiScoperState::getOscillators() const
{
    std::vector<Oscillator> result;

    auto oscillatorsNode = getOscillatorsNode();
    for (int i = 0; i < oscillatorsNode.getNumChildren(); ++i)
    {
        auto child = oscillatorsNode.getChild(i);
        if (child.hasType(StateIds::Oscillator))
        {
            result.emplace_back(child);
        }
    }

    return result;
}

int MultiScoperState::getOscillatorCount() const { return getOscillatorsNode().getNumChildren(); }

void MultiScoperState::addOscillator(const Oscillator& oscillator)
{
    auto oscillatorsNode = getOrCreateOscillatorsNode();
    oscillatorsNode.appendChild(oscillator.toValueTree(), nullptr);
    MULTISCOPER_LOG(STATE, "addOscillator: id=" << oscillator.getId().id << " name=" << oscillator.getName() << " src="
                                                << oscillator.getSourceId().id << " pane=" << oscillator.getPaneId().id
                                                << " total=" << oscillatorsNode.getNumChildren());
}

void MultiScoperState::removeOscillator(const OscillatorId& oscillatorId)
{
    auto oscillatorsNode = getOrCreateOscillatorsNode();
    int removedIndex = -1;

    // Find and remove the oscillator
    for (int i = 0; i < oscillatorsNode.getNumChildren(); ++i)
    {
        auto child = oscillatorsNode.getChild(i);
        if (child.getProperty(StateIds::Id).toString() == oscillatorId.id)
        {
            removedIndex = child.getProperty(StateIds::Order, i);
            MULTISCOPER_LOG(STATE, "removeOscillator: id=" << oscillatorId.id
                                                           << " name=" << child.getProperty(StateIds::Name).toString()
                                                           << " remaining=" << (oscillatorsNode.getNumChildren() - 1));
            oscillatorsNode.removeChild(i, nullptr);
            break;
        }
    }

    // Renumber remaining oscillators to maintain contiguous indices
    if (removedIndex >= 0)
    {
        for (int i = 0; i < oscillatorsNode.getNumChildren(); ++i)
        {
            auto child = oscillatorsNode.getChild(i);
            int const currentIndex = child.getProperty(StateIds::Order, i);
            if (currentIndex > removedIndex)
            {
                child.setProperty(StateIds::Order, currentIndex - 1, nullptr);
            }
        }
    }
}

void MultiScoperState::updateOscillator(const Oscillator& oscillator)
{
    auto oscillatorsNode = getOrCreateOscillatorsNode();

    for (int i = 0; i < oscillatorsNode.getNumChildren(); ++i)
    {
        auto child = oscillatorsNode.getChild(i);
        if (child.getProperty(StateIds::Id).toString() == oscillator.getId().id)
        {
            MULTISCOPER_LOG(STATE, "updateOscillator: id=" << oscillator.getId().id << " name=" << oscillator.getName()
                                                           << " src=" << oscillator.getSourceId().id
                                                           << " pane=" << oscillator.getPaneId().id
                                                           << " vis=" << (int) oscillator.isVisible());
            // Update properties and children from the oscillator's ValueTree
            auto srcTree = oscillator.toValueTree();
            child.copyPropertiesFrom(srcTree, nullptr);

            // copyPropertiesFrom does NOT copy children, so we must do it manually
            child.removeAllChildren(nullptr);
            for (int j = 0; j < srcTree.getNumChildren(); ++j)
            {
                child.appendChild(srcTree.getChild(j).createCopy(), nullptr);
            }
            return;
        }
    }
    MULTISCOPER_LOG(STATE, "updateOscillator: id=" << oscillator.getId().id << " NOT FOUND");
}

void MultiScoperState::reorderOscillators(int oldIndex, int newIndex)
{
    if (oldIndex == newIndex)
        return;
    MULTISCOPER_LOG(STATE, "reorderOscillators: oldIndex=" << oldIndex << " newIndex=" << newIndex);

    auto oscillators = getOscillators();
    if (oldIndex < 0 || !std::cmp_less(oldIndex, oscillators.size()) || newIndex < 0 ||
        !std::cmp_less(newIndex, oscillators.size()))
    {
        return;
    }

    // Sort oscillators by current orderIndex
    std::ranges::sort(oscillators,
                      [](const Oscillator& a, const Oscillator& b) { return a.getOrderIndex() < b.getOrderIndex(); });

    // Move the item from oldIndex to newIndex
    auto movedOsc = oscillators[static_cast<size_t>(oldIndex)];
    oscillators.erase(oscillators.begin() + oldIndex);
    oscillators.insert(oscillators.begin() + newIndex, movedOsc);

    // Update orderIndex for all oscillators
    for (size_t i = 0; i < oscillators.size(); ++i)
    {
        oscillators[i].setOrderIndex(static_cast<int>(i));
        updateOscillator(oscillators[i]);
    }
}

std::optional<Oscillator> MultiScoperState::getOscillator(const OscillatorId& oscillatorId) const
{
    auto oscillatorsNode = getOscillatorsNode();

    for (int i = 0; i < oscillatorsNode.getNumChildren(); ++i)
    {
        auto child = oscillatorsNode.getChild(i);
        if (child.getProperty(StateIds::Id).toString() == oscillatorId.id)
        {
            return Oscillator(child);
        }
    }

    return std::nullopt;
}

// Theme name, layout preferences, capture-quality config, node accessors,
// listener plumbing, and schema-version getter are in MultiScoperStateLayout.cpp.
// GlobalPreferences implementation is in GlobalPreferences.cpp.

} // namespace multiscoper
