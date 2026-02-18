/*
    Oscil - State Management Implementation
*/

#include "core/OscilState.h"
#include <algorithm>
#include <cmath>

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

juce::String OscilState::toXmlString()
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

void OscilState::syncLayoutManagerToState()
{
    // Sync layoutManager_ back to state_ before serialization.
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
        juce::Logger::writeToLog("OscilState::fromXmlString: Empty XML string provided");
        return false;
    }

    juce::XmlDocument doc(xmlString);
    auto xml = doc.getDocumentElement();
    if (!xml)
    {
        juce::Logger::writeToLog("OscilState::fromXmlString: XML parse error: " + doc.getLastParseError());
        return false;
    }

    auto loadedState = juce::ValueTree::fromXml(*xml);
    if (!loadedState.isValid())
    {
        juce::Logger::writeToLog("OscilState::fromXmlString: Invalid ValueTree from XML");
        return false;
    }

    if (!loadedState.hasType(StateIds::OscilState))
    {
        juce::Logger::writeToLog("OscilState::fromXmlString: Wrong root type, expected 'OscilState' but got '" + loadedState.getType().toString() + "'");
        return false;
    }

    // Validate schema version before accepting state
    int loadedVersion = loadedState.getProperty(StateIds::Version, 0);
    if (loadedVersion < 0 || loadedVersion > CURRENT_SCHEMA_VERSION)
    {
        juce::Logger::writeToLog("OscilState::fromXmlString: Invalid schema version " +
                                 juce::String(loadedVersion) + " (expected 0-" +
                                 juce::String(CURRENT_SCHEMA_VERSION) + ")");
        // Don't fail - we can still try to load with defaults for missing fields
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
        int cols = layoutNode.getProperty(StateIds::Columns, 1);
        // Validate column layout value and log if out of range
        if (cols < 1 || cols > 3)
        {
            juce::Logger::writeToLog("OscilState::fromXmlString: Column layout " +
                                     juce::String(cols) + " out of range [1,3], clamping");
            cols = std::clamp(cols, 1, 3);
        }
        layoutManager_.setColumnLayout(static_cast<ColumnLayout>(cols));
    }

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
    if (themeNode.isValid())
    {
        themeNode.setProperty(StateIds::ThemeName, themeName, nullptr);
    }
}

ColumnLayout OscilState::getColumnLayout() const
{
    auto layoutNode = getLayoutNode();
    int cols = layoutNode.getProperty(StateIds::Columns, 1);
    return static_cast<ColumnLayout>(std::clamp(cols, 1, 3));
}

void OscilState::setColumnLayout(ColumnLayout layout)
{
    auto layoutNode = state_.getChildWithName(StateIds::Layout);
    if (layoutNode.isValid())
    {
        layoutNode.setProperty(StateIds::Columns, static_cast<int>(layout), nullptr);
    }
    layoutManager_.setColumnLayout(layout);
}

int OscilState::getSidebarWidth() const
{
    auto layoutNode = getLayoutNode();
    int width = layoutNode.getProperty(StateIds::SidebarWidth, 300);
    // Validate sidebar width is within reasonable bounds
    constexpr int kMinSidebarWidth = 50;
    constexpr int kMaxSidebarWidth = 500;
    if (width < kMinSidebarWidth || width > kMaxSidebarWidth)
    {
        juce::Logger::writeToLog("OscilState::getSidebarWidth: Width " +
                                 juce::String(width) + " out of range [" +
                                 juce::String(kMinSidebarWidth) + "," +
                                 juce::String(kMaxSidebarWidth) + "], returning default");
        return 300;
    }
    return width;
}

void OscilState::setSidebarWidth(int width)
{
    // Validate and clamp sidebar width before storing
    constexpr int kMinSidebarWidth = 50;
    constexpr int kMaxSidebarWidth = 500;
    if (width < kMinSidebarWidth || width > kMaxSidebarWidth)
    {
        juce::Logger::writeToLog("OscilState::setSidebarWidth: Width " +
                                 juce::String(width) + " out of range, clamping to [" +
                                 juce::String(kMinSidebarWidth) + "," +
                                 juce::String(kMaxSidebarWidth) + "]");
        width = std::clamp(width, kMinSidebarWidth, kMaxSidebarWidth);
    }
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
    float dB = static_cast<float>(layoutNode.getProperty(StateIds::GainDb, 0.0));
    // Validate gain is within reasonable bounds (-60dB to +12dB)
    constexpr float kMinGainDb = -60.0f;
    constexpr float kMaxGainDb = 12.0f;
    if (dB < kMinGainDb || dB > kMaxGainDb || std::isnan(dB) || std::isinf(dB))
    {
        juce::Logger::writeToLog("OscilState::getGainDb: Value " +
                                 juce::String(dB) + " out of range [" +
                                 juce::String(kMinGainDb) + "," +
                                 juce::String(kMaxGainDb) + "], returning default 0.0");
        return 0.0f;
    }
    return dB;
}

void OscilState::setGainDb(float dB)
{
    // Validate and clamp gain before storing
    constexpr float kMinGainDb = -60.0f;
    constexpr float kMaxGainDb = 12.0f;
    if (std::isnan(dB) || std::isinf(dB))
    {
        juce::Logger::writeToLog("OscilState::setGainDb: Invalid value (nan/inf), using 0.0");
        dB = 0.0f;
    }
    else if (dB < kMinGainDb || dB > kMaxGainDb)
    {
        juce::Logger::writeToLog("OscilState::setGainDb: Value " +
                                 juce::String(dB) + " out of range, clamping to [" +
                                 juce::String(kMinGainDb) + "," +
                                 juce::String(kMaxGainDb) + "]");
        dB = std::clamp(dB, kMinGainDb, kMaxGainDb);
    }
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

    // Load quality preset with validation
    int presetInt = qualityNode.getProperty(StateIds::QualityPreset,
                                             static_cast<int>(QualityPreset::Standard));
    constexpr int kMaxQualityPreset = static_cast<int>(QualityPreset::Ultra);
    if (presetInt < 0 || presetInt > kMaxQualityPreset)
    {
        juce::Logger::writeToLog("OscilState::getCaptureQualityConfig: QualityPreset " +
                                 juce::String(presetInt) + " out of range [0," +
                                 juce::String(kMaxQualityPreset) + "], using Standard");
        presetInt = static_cast<int>(QualityPreset::Standard);
    }
    config.qualityPreset = static_cast<QualityPreset>(presetInt);

    // Load buffer duration with validation
    int durationInt = qualityNode.getProperty(StateIds::BufferDuration,
                                               static_cast<int>(BufferDuration::Medium));
    constexpr int kMaxBufferDuration = static_cast<int>(BufferDuration::Long);
    if (durationInt < 0 || durationInt > kMaxBufferDuration)
    {
        juce::Logger::writeToLog("OscilState::getCaptureQualityConfig: BufferDuration " +
                                 juce::String(durationInt) + " out of range [0," +
                                 juce::String(kMaxBufferDuration) + "], using Medium");
        durationInt = static_cast<int>(BufferDuration::Medium);
    }
    config.bufferDuration = static_cast<BufferDuration>(durationInt);

    // Load auto-adjust setting
    config.autoAdjustQuality = qualityNode.getProperty(StateIds::AutoAdjustQuality, true);

    // Load memory budget if present (stored as int64 to avoid overflow for large budgets)
    juce::int64 budgetBytes = qualityNode.getProperty(StateIds::MemoryBudgetBytes, static_cast<juce::int64>(0));
    if (budgetBytes < 0)
    {
        juce::Logger::writeToLog("OscilState::getCaptureQualityConfig: Negative memory budget " +
                                 juce::String(budgetBytes) + ", ignoring");
        budgetBytes = 0;
    }
    else if (budgetBytes > 0)
    {
        // Validate budget is within reasonable limits (1MB to 1GB)
        constexpr juce::int64 kMaxBudget = 1024LL * 1024LL * 1024LL;  // 1GB
        if (budgetBytes > kMaxBudget)
        {
            juce::Logger::writeToLog("OscilState::getCaptureQualityConfig: Memory budget " +
                                     juce::String(budgetBytes) + " exceeds max, clamping to " +
                                     juce::String(kMaxBudget));
            budgetBytes = kMaxBudget;
        }
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

} // namespace oscil
