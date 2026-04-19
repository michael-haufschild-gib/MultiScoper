/*
    MultiScoper - MultiScoperState Layout & Preferences
    Theme, column/sidebar/status-bar/grid/auto-scale/gain/GPU preferences,
    capture-quality config, ValueTree node accessors, and listener plumbing.
    Split from MultiScoperState.cpp so the top-level file is focused on lifecycle
    and XML (de)serialization.
*/

#include "core/MultiScoperLog.h"
#include "core/MultiScoperState.h"

#include <algorithm>

namespace multiscoper
{

juce::String MultiScoperState::getTrackIdentifier() const
{
    return state_.getProperty(StateIds::TrackIdentifier, juce::String{}).toString();
}

void MultiScoperState::setTrackIdentifier(const juce::String& trackIdentifier)
{
    state_.setProperty(StateIds::TrackIdentifier, trackIdentifier, nullptr);
}

juce::String MultiScoperState::getThemeName() const
{
    auto themeNode = state_.getChildWithName(StateIds::Theme);
    return themeNode.getProperty(StateIds::ThemeName, "Dark Professional");
}

void MultiScoperState::setThemeName(const juce::String& themeName)
{
    auto themeNode = state_.getChildWithName(StateIds::Theme);
    if (!themeNode.isValid())
    {
        themeNode = juce::ValueTree(StateIds::Theme);
        state_.appendChild(themeNode, nullptr);
    }
    themeNode.setProperty(StateIds::ThemeName, themeName, nullptr);
}

ColumnLayout MultiScoperState::getColumnLayout() const
{
    auto layoutNode = getLayoutNode();
    int const cols = layoutNode.getProperty(StateIds::Columns, 1);
    return static_cast<ColumnLayout>(std::clamp(cols, 1, 3));
}

void MultiScoperState::setColumnLayout(ColumnLayout layout)
{
    MULTISCOPER_LOG(STATE, "setColumnLayout: columns=" << static_cast<int>(layout));
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::Columns, static_cast<int>(layout), nullptr);
    layoutManager_.setColumnLayout(layout);
}

int MultiScoperState::getSidebarWidth() const
{
    auto layoutNode = getLayoutNode();
    return layoutNode.getProperty(StateIds::SidebarWidth, 300);
}

void MultiScoperState::setSidebarWidth(int width)
{
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::SidebarWidth, width, nullptr);
}

bool MultiScoperState::isSidebarCollapsed() const
{
    auto layoutNode = getLayoutNode();
    return layoutNode.getProperty(StateIds::SidebarCollapsed, false);
}

void MultiScoperState::setSidebarCollapsed(bool collapsed)
{
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::SidebarCollapsed, collapsed, nullptr);
}

bool MultiScoperState::isStatusBarVisible() const
{
    auto layoutNode = getLayoutNode();
    return layoutNode.getProperty(StateIds::StatusBarVisible, true);
}

void MultiScoperState::setStatusBarVisible(bool visible)
{
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::StatusBarVisible, visible, nullptr);
}

bool MultiScoperState::isShowGridEnabled() const
{
    auto layoutNode = getLayoutNode();
    return layoutNode.getProperty(StateIds::ShowGrid, true);
}

void MultiScoperState::setShowGridEnabled(bool enabled)
{
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::ShowGrid, enabled, nullptr);
}

bool MultiScoperState::isAutoScaleEnabled() const
{
    auto layoutNode = getLayoutNode();
    return layoutNode.getProperty(StateIds::AutoScale, true);
}

void MultiScoperState::setAutoScaleEnabled(bool enabled)
{
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::AutoScale, enabled, nullptr);
}

float MultiScoperState::getGainDb() const
{
    auto layoutNode = getLayoutNode();
    return static_cast<float>(layoutNode.getProperty(StateIds::GainDb, 0.0));
}

void MultiScoperState::setGainDb(float dB)
{
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::GainDb, dB, nullptr);
}

bool MultiScoperState::isGpuRenderingEnabled() const
{
    auto layoutNode = getLayoutNode();
// Default to true if OpenGL is available at compile time
#if MULTISCOPER_ENABLE_OPENGL
    return layoutNode.getProperty(StateIds::GpuRenderingEnabled, true);
#else
    return false;
#endif
}

void MultiScoperState::setGpuRenderingEnabled(bool enabled)
{
    auto layoutNode = getOrCreateLayoutNode();
    layoutNode.setProperty(StateIds::GpuRenderingEnabled, enabled, nullptr);
}

CaptureQualityConfig MultiScoperState::getCaptureQualityConfig() const
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

void MultiScoperState::setCaptureQualityConfig(const CaptureQualityConfig& config)
{
    auto qualityNode = getOrCreateCaptureQualityNode();

    qualityNode.setProperty(StateIds::QualityPreset, static_cast<int>(config.qualityPreset), nullptr);
    qualityNode.setProperty(StateIds::BufferDuration, static_cast<int>(config.bufferDuration), nullptr);
    qualityNode.setProperty(StateIds::AutoAdjustQuality, config.autoAdjustQuality, nullptr);
    qualityNode.setProperty(StateIds::MemoryBudgetBytes, static_cast<juce::int64>(config.memoryBudget.totalBudgetBytes),
                            nullptr);
}

void MultiScoperState::addListener(juce::ValueTree::Listener* listener) { state_.addListener(listener); }

void MultiScoperState::removeListener(juce::ValueTree::Listener* listener) { state_.removeListener(listener); }

int MultiScoperState::getSchemaVersion() const { return state_.getProperty(StateIds::Version, 1); }

// Const versions - just return what exists (may be invalid)
juce::ValueTree MultiScoperState::getOscillatorsNode() const { return state_.getChildWithName(StateIds::Oscillators); }

juce::ValueTree MultiScoperState::getPanesNode() const { return state_.getChildWithName(StateIds::Panes); }

juce::ValueTree MultiScoperState::getLayoutNode() const { return state_.getChildWithName(StateIds::Layout); }

// Non-const versions - create if missing
juce::ValueTree MultiScoperState::getOrCreateOscillatorsNode()
{
    auto node = state_.getChildWithName(StateIds::Oscillators);
    if (!node.isValid())
    {
        node = juce::ValueTree(StateIds::Oscillators);
        state_.appendChild(node, nullptr);
    }
    return node;
}

juce::ValueTree MultiScoperState::getOrCreatePanesNode()
{
    auto node = state_.getChildWithName(StateIds::Panes);
    if (!node.isValid())
    {
        node = juce::ValueTree(StateIds::Panes);
        state_.appendChild(node, nullptr);
    }
    return node;
}

juce::ValueTree MultiScoperState::getOrCreateLayoutNode()
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

juce::ValueTree MultiScoperState::getOrCreateCaptureQualityNode()
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

juce::ValueTree MultiScoperState::getCaptureQualityNode() const
{
    return state_.getChildWithName(StateIds::CaptureQuality);
}

} // namespace multiscoper
