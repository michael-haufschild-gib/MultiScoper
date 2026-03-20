/*
    Oscil - State Management Implementation
*/

#include "core/OscilState.h"
#include <algorithm>

namespace oscil
{

OscilState::OscilState()
{
    initializeDefaultState();
}

OscilState::OscilState(const juce::String& xmlString)
{
    initializeDefaultState();
    (void)fromXmlString(xmlString);
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

juce::String OscilState::toXmlString() const
{
    // Sync layoutManager_ back to state_ before serialization
    // This ensures pane changes made at runtime are persisted
    syncLayoutManagerToState();

    if (auto xml = state_.createXml())
    {
        return xml->toString();
    }
    return {};
}

void OscilState::syncLayoutManagerToState() const
{
    // Sync layoutManager_ back to state_ before serialization.
    // Although this method modifies the underlying ValueTree data of state_,
    // it is conceptually preserving "logical constness" by ensuring the 
    // serialized representation matches the current runtime state (layoutManager_).
    // juce::ValueTree is a reference-counted wrapper, so modifying its content
    // does not require the wrapper itself to be mutable.

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
        return false;

    if (auto xml = juce::XmlDocument::parse(xmlString))
    {
        auto loadedState = juce::ValueTree::fromXml(*xml);
        if (loadedState.isValid() && loadedState.hasType(StateIds::OscilState))
        {
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
                int cols = layoutNode.getProperty(StateIds::Columns, 1);
                layoutManager_.setColumnLayout(static_cast<ColumnLayout>(cols));
            }

            return true;
        }
    }
    return false;
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
            int currentIndex = child.getProperty(StateIds::Order, i);
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
            // Update properties
            child.copyPropertiesFrom(oscillator.toValueTree(), nullptr);
            
            // Update children (e.g. VisualOverrides)
            // copyPropertiesFrom does NOT copy children, so we must do it manually
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

void OscilState::reorderOscillators(int oldIndex, int newIndex)
{
    if (oldIndex == newIndex) return;

    auto oscillators = getOscillators();
    if (oldIndex < 0 || oldIndex >= static_cast<int>(oscillators.size()) ||
        newIndex < 0 || newIndex >= static_cast<int>(oscillators.size()))
    {
        return;
    }

    // Sort oscillators by current orderIndex
    std::sort(oscillators.begin(), oscillators.end(),
              [](const Oscillator& a, const Oscillator& b) {
                  return a.getOrderIndex() < b.getOrderIndex();
              });

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
    int cols = layoutNode.getProperty(StateIds::Columns, 1);
    return static_cast<ColumnLayout>(std::clamp(cols, 1, 3));
}

void OscilState::setColumnLayout(ColumnLayout layout)
{
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
        return CaptureQualityConfig();  // Return defaults
    }

    CaptureQualityConfig config;

    // Load quality preset
    int presetInt = qualityNode.getProperty(StateIds::QualityPreset,
                                             static_cast<int>(QualityPreset::Standard));
    config.qualityPreset = static_cast<QualityPreset>(
        std::clamp(presetInt, 0, static_cast<int>(QualityPreset::Ultra)));

    // Load buffer duration
    int durationInt = qualityNode.getProperty(StateIds::BufferDuration,
                                               static_cast<int>(BufferDuration::Medium));
    config.bufferDuration = static_cast<BufferDuration>(
        std::clamp(durationInt, 0, static_cast<int>(BufferDuration::Long)));

    // Load auto-adjust setting
    config.autoAdjustQuality = qualityNode.getProperty(StateIds::AutoAdjustQuality, true);

    // Load memory budget if present (stored as int64 to avoid overflow for large budgets)
    juce::int64 budgetBytes = qualityNode.getProperty(StateIds::MemoryBudgetBytes, static_cast<juce::int64>(0));
    if (budgetBytes > 0)
    {
        config.memoryBudget.totalBudgetBytes = static_cast<size_t>(budgetBytes);
    }

    return config;
}

void OscilState::setCaptureQualityConfig(const CaptureQualityConfig& config)
{
    auto qualityNode = getOrCreateCaptureQualityNode();

    qualityNode.setProperty(StateIds::QualityPreset,
                            static_cast<int>(config.qualityPreset), nullptr);
    qualityNode.setProperty(StateIds::BufferDuration,
                            static_cast<int>(config.bufferDuration), nullptr);
    qualityNode.setProperty(StateIds::AutoAdjustQuality,
                            config.autoAdjustQuality, nullptr);
    qualityNode.setProperty(StateIds::MemoryBudgetBytes,
                            static_cast<juce::int64>(config.memoryBudget.totalBudgetBytes), nullptr);
}

void OscilState::addListener(juce::ValueTree::Listener* listener)
{
    state_.addListener(listener);
}

void OscilState::removeListener(juce::ValueTree::Listener* listener)
{
    state_.removeListener(listener);
}

int OscilState::getSchemaVersion() const
{
    return state_.getProperty(StateIds::Version, 1);
}


// Const versions - just return what exists (may be invalid)
juce::ValueTree OscilState::getOscillatorsNode() const
{
    return state_.getChildWithName(StateIds::Oscillators);
}

juce::ValueTree OscilState::getPanesNode() const
{
    return state_.getChildWithName(StateIds::Panes);
}

juce::ValueTree OscilState::getLayoutNode() const
{
    return state_.getChildWithName(StateIds::Layout);
}

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
        // Set defaults
        node.setProperty(StateIds::QualityPreset,
                         static_cast<int>(QualityPreset::Standard), nullptr);
        node.setProperty(StateIds::BufferDuration,
                         static_cast<int>(BufferDuration::Medium), nullptr);
        node.setProperty(StateIds::AutoAdjustQuality, true, nullptr);
        state_.appendChild(node, nullptr);
    }
    return node;
}

juce::ValueTree OscilState::getCaptureQualityNode() const
{
    return state_.getChildWithName(StateIds::CaptureQuality);
}

// GlobalPreferences implementation

GlobalPreferences::GlobalPreferences()
{
    preferences_ = juce::ValueTree("GlobalPreferences");
    load();
}

GlobalPreferences::~GlobalPreferences()
{
}

juce::File GlobalPreferences::getPreferencesFile() const
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    return appDataDir.getChildFile("Oscil").getChildFile("preferences.xml");
}

void GlobalPreferences::load()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto file = getPreferencesFile();
    if (file.existsAsFile())
    {
        if (auto xml = juce::XmlDocument::parse(file))
        {
            preferences_ = juce::ValueTree::fromXml(*xml);
        }
    }
}

void GlobalPreferences::save()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto file = getPreferencesFile();

    // Create parent directory if it doesn't exist
    auto parentDir = file.getParentDirectory();
    if (!parentDir.exists())
    {
        auto result = parentDir.createDirectory();
        if (result.failed())
        {
            DBG("GlobalPreferences::save() - Failed to create directory: " << result.getErrorMessage());
            return;
        }
    }

    if (auto xml = preferences_.createXml())
    {
        if (!xml->writeTo(file))
        {
            DBG("GlobalPreferences::save() - Failed to write preferences to: " << file.getFullPathName());
        }
    }
    else
    {
        DBG("GlobalPreferences::save() - Failed to create XML from preferences");
    }
}

juce::String GlobalPreferences::getDefaultTheme() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return preferences_.getProperty("defaultTheme", "Dark Professional");
}

void GlobalPreferences::setDefaultTheme(const juce::String& themeName)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    preferences_.setProperty("defaultTheme", themeName, nullptr);
    save();
}

int GlobalPreferences::getDefaultColumnLayout() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return preferences_.getProperty("defaultColumns", 1);
}

void GlobalPreferences::setDefaultColumnLayout(int columns)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    preferences_.setProperty("defaultColumns", columns, nullptr);
    save();
}

bool GlobalPreferences::getShowStatusBar() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return preferences_.getProperty("showStatusBar", true);
}

void GlobalPreferences::setShowStatusBar(bool show)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    preferences_.setProperty("showStatusBar", show, nullptr);
    save();
}

bool GlobalPreferences::getReducedMotion() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return preferences_.getProperty("reducedMotion", false);
}

void GlobalPreferences::setReducedMotion(bool reduced)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    preferences_.setProperty("reducedMotion", reduced, nullptr);
    save();
}

bool GlobalPreferences::getUIAudioFeedback() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return preferences_.getProperty("uiAudioFeedback", false);
}

void GlobalPreferences::setUIAudioFeedback(bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    preferences_.setProperty("uiAudioFeedback", enabled, nullptr);
    save();
}

bool GlobalPreferences::getTooltipsEnabled() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return preferences_.getProperty("tooltipsEnabled", true);
}

void GlobalPreferences::setTooltipsEnabled(bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    preferences_.setProperty("tooltipsEnabled", enabled, nullptr);
    save();
}

int GlobalPreferences::getDefaultSidebarWidth() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return preferences_.getProperty("defaultSidebarWidth", 280);
}

void GlobalPreferences::setDefaultSidebarWidth(int width)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    preferences_.setProperty("defaultSidebarWidth", width, nullptr);
    save();
}

} // namespace oscil
