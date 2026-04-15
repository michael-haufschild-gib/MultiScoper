/*
    Oscil - State Management Implementation
*/

#include "core/OscilState.h"

#include "core/OscilLog.h"
#include "core/SchemaMigration.h"

#include <algorithm>
#include <utility>
namespace oscil
{

OscilState::OscilState() { initializeDefaultState(); }

OscilState::OscilState(const juce::String& xmlString)
{
    initializeDefaultState();
    (void) fromXmlString(xmlString);
}

void OscilState::initializeDefaultState()
{
    state_ = juce::ValueTree(StateIds::OscilState);
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

juce::String OscilState::toXmlString()
{
    // Sync layoutManager_ back to state_ before serialization
    // This ensures pane changes made at runtime are persisted
    syncLayoutManagerToState();

    if (auto xml = state_.createXml())
    {
        auto xmlStr = xml->toString();
        OSCIL_LOG(STATE, "toXmlString: " << xmlStr.length() << "ch " << getOscillatorsNode().getNumChildren() << "osc "
                                         << layoutManager_.getPaneCount() << "panes");
        return xmlStr;
    }
    OSCIL_LOG(STATE, "toXmlString: FAILED to create XML");
    return {};
}

void OscilState::syncLayoutManagerToState()
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

bool OscilState::fromXmlString(const juce::String& xmlString)
{
    if (xmlString.isEmpty())
    {
        juce::Logger::writeToLog("OscilState::fromXmlString: empty input");
        return false;
    }

    auto xml = juce::XmlDocument::parse(xmlString);
    if (!xml)
    {
        juce::Logger::writeToLog("OscilState::fromXmlString: XML parse failed (" + juce::String(xmlString.length()) +
                                 " chars)");
        return false;
    }

    auto loadedState = juce::ValueTree::fromXml(*xml);
    if (!loadedState.isValid() || !loadedState.hasType(StateIds::OscilState))
    {
        juce::Logger::writeToLog("OscilState::fromXmlString: invalid root node type '" +
                                 loadedState.getType().toString() + "' (expected '" + StateIds::OscilState.toString() +
                                 "')");
        return false;
    }

    // Fail closed on unsupported schema versions. Downgrades, unknown-future
    // versions, and gaps in the migration chain all reject the load and
    // preserve the current in-memory state.
    int const loadedVersion = loadedState.getProperty(StateIds::Version, 0);
    auto const migrationResult = migration::migrateOscilState(loadedState, loadedVersion, CURRENT_SCHEMA_VERSION);
    if (migrationResult != migration::MigrationResult::Success)
    {
        juce::Logger::writeToLog(juce::String("OscilState::fromXmlString: migration rejected (from v") +
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

    OSCIL_LOG(STATE, "fromXmlString: " << getOscillatorCount() << "osc " << layoutManager_.getPaneCount() << "panes v"
                                       << getSchemaVersion());
    return true;
}

std::vector<Oscillator> OscilState::getOscillators() const
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

int OscilState::getOscillatorCount() const { return getOscillatorsNode().getNumChildren(); }

void OscilState::addOscillator(const Oscillator& oscillator)
{
    auto oscillatorsNode = getOrCreateOscillatorsNode();
    oscillatorsNode.appendChild(oscillator.toValueTree(), nullptr);
    OSCIL_LOG(STATE, "addOscillator: id=" << oscillator.getId().id << " name=" << oscillator.getName() << " src="
                                          << oscillator.getSourceId().id << " pane=" << oscillator.getPaneId().id
                                          << " total=" << oscillatorsNode.getNumChildren());
}

void OscilState::removeOscillator(const OscillatorId& oscillatorId)
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
            OSCIL_LOG(STATE, "removeOscillator: id=" << oscillatorId.id
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

void OscilState::updateOscillator(const Oscillator& oscillator)
{
    auto oscillatorsNode = getOrCreateOscillatorsNode();

    for (int i = 0; i < oscillatorsNode.getNumChildren(); ++i)
    {
        auto child = oscillatorsNode.getChild(i);
        if (child.getProperty(StateIds::Id).toString() == oscillator.getId().id)
        {
            OSCIL_LOG(STATE, "updateOscillator: id=" << oscillator.getId().id << " name=" << oscillator.getName()
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
    OSCIL_LOG(STATE, "updateOscillator: id=" << oscillator.getId().id << " NOT FOUND");
}

void OscilState::reorderOscillators(int oldIndex, int newIndex)
{
    if (oldIndex == newIndex)
        return;
    OSCIL_LOG(STATE, "reorderOscillators: oldIndex=" << oldIndex << " newIndex=" << newIndex);

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

std::optional<Oscillator> OscilState::getOscillator(const OscillatorId& oscillatorId) const
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
// listener plumbing, and schema-version getter are in OscilStateLayout.cpp.
// GlobalPreferences implementation is in GlobalPreferences.cpp.

} // namespace oscil
