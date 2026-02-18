/*
    Oscil - State Management
    ValueTree-based state persistence for plugin settings
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "core/Oscillator.h"
#include "core/dsp/CaptureQualityConfig.h"
#include "core/Pane.h"
#include "core/GlobalPreferences.h"
#include <atomic>
#include <vector>

namespace oscil
{

// ValueTree type identifiers
namespace StateIds
{
    static const juce::Identifier OscilState{ "OscilState" };
    static const juce::Identifier Oscillators{ "Oscillators" };
    static const juce::Identifier Oscillator{ "Oscillator" };
    static const juce::Identifier Panes{ "Panes" };
    static const juce::Identifier Pane{ "Pane" };
    static const juce::Identifier Layout{ "Layout" };
    static const juce::Identifier Theme{ "Theme" };
    static const juce::Identifier Timing{ "Timing" };
    static const juce::Identifier Version{ "version" };

    // Plugin instance identifier (persisted across sessions)
    static const juce::Identifier InstanceUUID{ "instanceUUID" };

    // Oscillator properties
    static const juce::Identifier Id{ "id" };
    static const juce::Identifier SourceId{ "sourceId" };
    static const juce::Identifier SourceName{ "sourceName" };  // Fallback source identification (pre-UUID migration)
    static const juce::Identifier SourceInstanceUUID{ "sourceInstanceUUID" };  // Stable reference to source plugin instance
    static const juce::Identifier ProcessingMode{ "processingMode" };
    static const juce::Identifier Colour{ "colour" };
    static const juce::Identifier Opacity{ "opacity" };
    static const juce::Identifier PaneId{ "paneId" };
    static const juce::Identifier Order{ "order" };
    static const juce::Identifier Visible{ "visible" };
    static const juce::Identifier Name{ "name" };
    static const juce::Identifier SchemaVersion{ "schemaVersion" };

    // Extended oscillator properties (PRD aligned)
    static const juce::Identifier OscillatorState{ "oscillatorState" };
    static const juce::Identifier LineWidth{ "lineWidth" };
    static const juce::Identifier TimeWindow{ "timeWindow" };
    static const juce::Identifier Persistence{ "persistence" };
    static const juce::Identifier ShaderId{ "shaderId" };
    static const juce::Identifier VisualPresetId{ "visualPresetId" };

    // Layout properties
    static const juce::Identifier Columns{ "columns" };
    static const juce::Identifier Collapsed{ "collapsed" };

    // Theme properties
    static const juce::Identifier ThemeName{ "themeName" };

    // Timing properties (PRD aligned)
    static const juce::Identifier TimeBase{ "timeBase" };
    static const juce::Identifier WindowSize{ "windowSize" };
    static const juce::Identifier TriggerMode{ "triggerMode" };
    static const juce::Identifier TriggerThreshold{ "triggerThreshold" };
    static const juce::Identifier TimingMode{ "timingMode" };
    static const juce::Identifier TimeIntervalMs{ "timeIntervalMs" };
    static const juce::Identifier NoteInterval{ "noteInterval" };
    static const juce::Identifier HostSyncEnabled{ "hostSyncEnabled" };

    // Window layout properties (PRD aligned)
    static const juce::Identifier SidebarWidth{ "sidebarWidth" };
    static const juce::Identifier SidebarCollapsed{ "sidebarCollapsed" };
    static const juce::Identifier StatusBarVisible{ "statusBarVisible" };

    // Display options
    static const juce::Identifier ShowGrid{ "showGrid" };
    static const juce::Identifier AutoScale{ "autoScale" };

    // Value Tree structure
    static const juce::Identifier ConfigNode{ "config" };
    static const juce::Identifier LayoutNode{ "layout" };

    static const juce::Identifier GainDb{ "gainDb" };

    // Rendering mode
    static const juce::Identifier GpuRenderingEnabled{ "gpuRenderingEnabled" };

    // Capture quality configuration
    static const juce::Identifier CaptureQuality{ "CaptureQuality" };
    static const juce::Identifier QualityPreset{ "qualityPreset" };
    static const juce::Identifier BufferDuration{ "bufferDuration" };
    static const juce::Identifier AutoAdjustQuality{ "autoAdjustQuality" };
    static const juce::Identifier MemoryBudgetBytes{ "memoryBudgetBytes" };
}

/**
 * Central state manager for Oscil plugin.
 * Handles serialization, deserialization, and migration of plugin state.
 *
 * THREAD SAFETY: This class is NOT thread-safe. All access must occur
 * on the message thread. The underlying juce::ValueTree is designed for
 * single-threaded access with listener notifications.
 *
 * For cross-thread communication, use the PluginProcessor's async
 * mechanisms which properly defer state operations to the message thread.
 */
class OscilState
{
public:
    OscilState();

    /**
     * Create state from XML string (from DAW)
     */
    explicit OscilState(const juce::String& xmlString);

    /**
     * Get the root ValueTree
     * WARNING: Mutable access - caller must ensure message thread ownership.
     * Prefer const version or specific accessors when possible.
     */
    [[nodiscard]] juce::ValueTree& getState()
    {
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
        return state_;
    }
    [[nodiscard]] const juce::ValueTree& getState() const { return state_; }

    /**
     * Serialize to XML for DAW persistence
     * NOTE: Non-const because it syncs layoutManager_ to state_ before serialization
     */
    [[nodiscard]] juce::String toXmlString();

    /**
     * Load from XML string
     */
    [[nodiscard]] bool fromXmlString(const juce::String& xmlString);

    /**
     * Get all oscillators
     */
    [[nodiscard]] std::vector<Oscillator> getOscillators() const;

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
     * Updates orderIndex for all affected oscillators
     */
    void reorderOscillators(int oldIndex, int newIndex);

    /**
     * Get oscillator by ID
     */
    [[nodiscard]] std::optional<Oscillator> getOscillator(const OscillatorId& oscillatorId) const;

    /**
     * Get the pane layout manager
     */
    [[nodiscard]] PaneLayoutManager& getLayoutManager() { return layoutManager_; }
    [[nodiscard]] const PaneLayoutManager& getLayoutManager() const { return layoutManager_; }

    /**
     * Get/set current theme name
     */
    [[nodiscard]] juce::String getThemeName() const;
    void setThemeName(const juce::String& themeName);

    /**
     * Get/set column layout
     */
    [[nodiscard]] ColumnLayout getColumnLayout() const;
    void setColumnLayout(ColumnLayout layout);

    /**
     * Get/set sidebar configuration
     */
    [[nodiscard]] int getSidebarWidth() const;
    void setSidebarWidth(int width);

    [[nodiscard]] bool isSidebarCollapsed() const;
    void setSidebarCollapsed(bool collapsed);

    /**
     * Get/set status bar visibility
     */
    [[nodiscard]] bool isStatusBarVisible() const;
    void setStatusBarVisible(bool visible);

    /**
     * Display options
     */
    [[nodiscard]] bool isShowGridEnabled() const;
    void setShowGridEnabled(bool enabled);

    [[nodiscard]] bool isAutoScaleEnabled() const;
    void setAutoScaleEnabled(bool enabled);

    [[nodiscard]] float getGainDb() const;
    void setGainDb(float dB);

    /**
     * Performance settings
     */
    [[nodiscard]] float getCpuWarningThreshold() const;

    /**
     * Get/set GPU rendering mode
     * When enabled, OpenGL context is attached for hardware acceleration
     */
    [[nodiscard]] bool isGpuRenderingEnabled() const;
    void setGpuRenderingEnabled(bool enabled);

    /**
     * Get/set capture quality configuration
     * Controls waveform buffer resolution and memory usage
     */
    [[nodiscard]] CaptureQualityConfig getCaptureQualityConfig() const;
    void setCaptureQualityConfig(const CaptureQualityConfig& config);

    /**
     * Add a listener for state changes
     */
    void addListener(juce::ValueTree::Listener* listener);
    void removeListener(juce::ValueTree::Listener* listener);

    /**
     * Get the current schema version
     */
    [[nodiscard]] int getSchemaVersion() const;

    // Current schema version
    static constexpr int CURRENT_SCHEMA_VERSION = 2;

private:
    juce::ValueTree state_;
    PaneLayoutManager layoutManager_;

    void initializeDefaultState();

    /**
     * Sync layoutManager_ back to state_ before serialization.
     * This ensures pane changes made at runtime are persisted.
     */
    void syncLayoutManagerToState();

    // Non-const versions for modification - ensure nodes exist
    juce::ValueTree getOrCreateOscillatorsNode();
    juce::ValueTree getOrCreatePanesNode();
    juce::ValueTree getOrCreateLayoutNode();
    juce::ValueTree getOrCreateCaptureQualityNode();

    // Const versions for reading - return empty if not found
    juce::ValueTree getOscillatorsNode() const;
    juce::ValueTree getPanesNode() const;
    juce::ValueTree getLayoutNode() const;
    juce::ValueTree getCaptureQualityNode() const;
};

} // namespace oscil
