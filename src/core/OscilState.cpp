/*
    Oscil - State Management Implementation
*/

#include "core/OscilState.h"

#include "core/OscilLog.h"

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
        OSCIL_LOG(STATE, "toXmlString: " << xmlStr.length() << "ch " << getOscillators().size() << "osc "
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

    if (int const v = loadedState.getProperty(StateIds::Version, 0); v != CURRENT_SCHEMA_VERSION)
        juce::Logger::writeToLog("OscilState: loaded schema v" + juce::String(v) + ", current v" +
                                 juce::String(CURRENT_SCHEMA_VERSION));
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

    OSCIL_LOG(STATE, "fromXmlString: " << getOscillators().size() << "osc " << layoutManager_.getPaneCount()
                                       << "panes v" << getSchemaVersion());
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

juce::String OscilState::getThemeName() const
{
    auto themeNode = state_.getChildWithName(StateIds::Theme);
    return themeNode.getProperty(StateIds::ThemeName, "Dark Professional");
}

void OscilState::setThemeName(const juce::String& themeName)
{
    auto themeNode = state_.getChildWithName(StateIds::Theme);
    if (!themeNode.isValid())
    {
        themeNode = juce::ValueTree(StateIds::Theme);
        state_.appendChild(themeNode, nullptr);
    }
    themeNode.setProperty(StateIds::ThemeName, themeName, nullptr);
}

ColumnLayout OscilState::getColumnLayout() const
{
    auto layoutNode = getLayoutNode();
    int const cols = layoutNode.getProperty(StateIds::Columns, 1);
    return static_cast<ColumnLayout>(std::clamp(cols, 1, 3));
}

void OscilState::setColumnLayout(ColumnLayout layout)
{
    OSCIL_LOG(STATE, "setColumnLayout: columns=" << static_cast<int>(layout));
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::Columns, static_cast<int>(layout), nullptr);
    layoutManager_.setColumnLayout(layout);
}

int OscilState::getSidebarWidth() const
{
    auto layoutNode = getLayoutNode();
    return layoutNode.getProperty(StateIds::SidebarWidth, 300);
}

void OscilState::setSidebarWidth(int width)
{
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::SidebarWidth, width, nullptr);
}

bool OscilState::isSidebarCollapsed() const
{
    auto layoutNode = getLayoutNode();
    return layoutNode.getProperty(StateIds::SidebarCollapsed, false);
}

void OscilState::setSidebarCollapsed(bool collapsed)
{
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::SidebarCollapsed, collapsed, nullptr);
}

bool OscilState::isStatusBarVisible() const
{
    auto layoutNode = getLayoutNode();
    return layoutNode.getProperty(StateIds::StatusBarVisible, true);
}

void OscilState::setStatusBarVisible(bool visible)
{
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::StatusBarVisible, visible, nullptr);
}

bool OscilState::isShowGridEnabled() const
{
    auto layoutNode = getLayoutNode();
    return layoutNode.getProperty(StateIds::ShowGrid, true);
}

void OscilState::setShowGridEnabled(bool enabled)
{
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::ShowGrid, enabled, nullptr);
}

bool OscilState::isAutoScaleEnabled() const
{
    auto layoutNode = getLayoutNode();
    return layoutNode.getProperty(StateIds::AutoScale, true);
}

void OscilState::setAutoScaleEnabled(bool enabled)
{
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::AutoScale, enabled, nullptr);
}

float OscilState::getGainDb() const
{
    auto layoutNode = getLayoutNode();
    return static_cast<float>(layoutNode.getProperty(StateIds::GainDb, 0.0));
}

void OscilState::setGainDb(float dB)
{
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::GainDb, dB, nullptr);
}

bool OscilState::isGpuRenderingEnabled() const
{
    auto layoutNode = getLayoutNode();
// Default to true if OpenGL is available at compile time
#if OSCIL_ENABLE_OPENGL
    return layoutNode.getProperty(StateIds::GpuRenderingEnabled, true);
#else
    return false;
#endif
}

void OscilState::setGpuRenderingEnabled(bool enabled)
{
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::GpuRenderingEnabled, enabled, nullptr);
}

CaptureQualityConfig OscilState::getCaptureQualityConfig() const
{
    auto qualityNode = getCaptureQualityNode();
    if (!qualityNode.isValid())
    {
        return {}; // Return defaults
    }

    CaptureQualityConfig config;

    int const presetInt = qualityNode.getProperty(StateIds::QualityPreset, static_cast<int>(QualityPreset::Standard));
    config.qualityPreset = static_cast<QualityPreset>(std::clamp(presetInt, 0, static_cast<int>(QualityPreset::Ultra)));

    int const durationInt = qualityNode.getProperty(StateIds::BufferDuration, static_cast<int>(BufferDuration::Medium));
    config.bufferDuration =
        static_cast<BufferDuration>(std::clamp(durationInt, 0, static_cast<int>(BufferDuration::VeryLong)));

    config.autoAdjustQuality = qualityNode.getProperty(StateIds::AutoAdjustQuality, true);

    juce::int64 const budgetBytes = qualityNode.getProperty(StateIds::MemoryBudgetBytes, static_cast<juce::int64>(0));
    if (budgetBytes > 0)
    {
        config.memoryBudget.totalBudgetBytes = static_cast<size_t>(budgetBytes);
    }

    return config;
}

void OscilState::setCaptureQualityConfig(const CaptureQualityConfig& config)
{
    auto qualityNode = getOrCreateCaptureQualityNode();

    qualityNode.setProperty(StateIds::QualityPreset, static_cast<int>(config.qualityPreset), nullptr);
    qualityNode.setProperty(StateIds::BufferDuration, static_cast<int>(config.bufferDuration), nullptr);
    qualityNode.setProperty(StateIds::AutoAdjustQuality, config.autoAdjustQuality, nullptr);
    qualityNode.setProperty(StateIds::MemoryBudgetBytes, static_cast<juce::int64>(config.memoryBudget.totalBudgetBytes),
                            nullptr);
}

void OscilState::addListener(juce::ValueTree::Listener* listener) { state_.addListener(listener); }

void OscilState::removeListener(juce::ValueTree::Listener* listener) { state_.removeListener(listener); }

int OscilState::getSchemaVersion() const { return state_.getProperty(StateIds::Version, 1); }

// Const versions - just return what exists (may be invalid)
juce::ValueTree OscilState::getOscillatorsNode() const { return state_.getChildWithName(StateIds::Oscillators); }

juce::ValueTree OscilState::getPanesNode() const { return state_.getChildWithName(StateIds::Panes); }

juce::ValueTree OscilState::getLayoutNode() const { return state_.getChildWithName(StateIds::Layout); }

// Non-const versions - create if missing
juce::ValueTree OscilState::getOrCreateOscillatorsNode()
{
    auto node = state_.getChildWithName(StateIds::Oscillators);
    if (!node.isValid())
    {
        node = juce::ValueTree(StateIds::Oscillators);
        state_.appendChild(node, nullptr);
    }
    return node;
}

juce::ValueTree OscilState::getOrCreatePanesNode()
{
    auto node = state_.getChildWithName(StateIds::Panes);
    if (!node.isValid())
    {
        node = juce::ValueTree(StateIds::Panes);
        state_.appendChild(node, nullptr);
    }
    return node;
}

juce::ValueTree OscilState::getOrCreateLayoutNode()
{
    auto node = state_.getChildWithName(StateIds::Layout);
    if (!node.isValid())
    {
        node = juce::ValueTree(StateIds::Layout);
        node.setProperty(StateIds::Columns, 1, nullptr);
        state_.appendChild(node, nullptr);
    }
    return node;
}

juce::ValueTree OscilState::getOrCreateCaptureQualityNode()
{
    auto node = state_.getChildWithName(StateIds::CaptureQuality);
    if (!node.isValid())
    {
        node = juce::ValueTree(StateIds::CaptureQuality);
        node.setProperty(StateIds::QualityPreset, static_cast<int>(QualityPreset::Standard), nullptr);
        node.setProperty(StateIds::BufferDuration, static_cast<int>(BufferDuration::Medium), nullptr);
        node.setProperty(StateIds::AutoAdjustQuality, true, nullptr);
        state_.appendChild(node, nullptr);
    }
    return node;
}

juce::ValueTree OscilState::getCaptureQualityNode() const { return state_.getChildWithName(StateIds::CaptureQuality); }

// GlobalPreferences implementation is in GlobalPreferences.cpp

} // namespace oscil
