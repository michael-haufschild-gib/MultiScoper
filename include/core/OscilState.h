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
#include <vector>

namespace oscil
{

// ValueTree type identifiers
namespace StateIds
{
    inline const juce::Identifier OscilState{ "OscilState" };
    inline const juce::Identifier Oscillators{ "Oscillators" };
    inline const juce::Identifier Oscillator{ "Oscillator" };
    inline const juce::Identifier Panes{ "Panes" };
    inline const juce::Identifier Pane{ "Pane" };
    inline const juce::Identifier Layout{ "Layout" };
    inline const juce::Identifier Theme{ "Theme" };
    inline const juce::Identifier Timing{ "Timing" };
    inline const juce::Identifier Version{ "version" };

    // Oscillator properties
    inline const juce::Identifier Id{ "id" };
    inline const juce::Identifier SourceId{ "sourceId" };
    inline const juce::Identifier ProcessingMode{ "processingMode" };
    inline const juce::Identifier Colour{ "colour" };
    inline const juce::Identifier Opacity{ "opacity" };
    inline const juce::Identifier PaneId{ "paneId" };
    inline const juce::Identifier Order{ "order" };
    inline const juce::Identifier Visible{ "visible" };
    inline const juce::Identifier Name{ "name" };
    inline const juce::Identifier SchemaVersion{ "schemaVersion" };

    // Extended oscillator properties (PRD aligned)
    inline const juce::Identifier OscillatorState{ "oscillatorState" };
    inline const juce::Identifier LineWidth{ "lineWidth" };
    inline const juce::Identifier TimeWindow{ "timeWindow" };
    inline const juce::Identifier Persistence{ "persistence" };
    inline const juce::Identifier ShaderId{ "shaderId" };
    inline const juce::Identifier VisualPresetId{ "visualPresetId" };

    // Layout properties
    inline const juce::Identifier Columns{ "columns" };
    inline const juce::Identifier Collapsed{ "collapsed" };

    // Theme properties
    inline const juce::Identifier ThemeName{ "themeName" };

    // Timing properties (PRD aligned)
    inline const juce::Identifier TimeBase{ "timeBase" };
    inline const juce::Identifier WindowSize{ "windowSize" };
    inline const juce::Identifier TriggerMode{ "triggerMode" };
    inline const juce::Identifier TriggerThreshold{ "triggerThreshold" };
    inline const juce::Identifier TimingMode{ "timingMode" };
    inline const juce::Identifier TimeIntervalMs{ "timeIntervalMs" };
    inline const juce::Identifier NoteInterval{ "noteInterval" };
    inline const juce::Identifier HostSyncEnabled{ "hostSyncEnabled" };

    // Window layout properties (PRD aligned)
    inline const juce::Identifier SidebarWidth{ "sidebarWidth" };
    inline const juce::Identifier SidebarCollapsed{ "sidebarCollapsed" };
    inline const juce::Identifier StatusBarVisible{ "statusBarVisible" };

    // Display options
    inline const juce::Identifier ShowGrid{ "showGrid" };
    inline const juce::Identifier AutoScale{ "autoScale" };

    // Value Tree structure
    inline const juce::Identifier ConfigNode{ "config" };
    inline const juce::Identifier LayoutNode{ "layout" };

    inline const juce::Identifier GainDb{ "gainDb" };

    // Rendering mode
    inline const juce::Identifier GpuRenderingEnabled{ "gpuRenderingEnabled" };

    // Capture quality configuration
    inline const juce::Identifier CaptureQuality{ "CaptureQuality" };
    inline const juce::Identifier QualityPreset{ "qualityPreset" };
    inline const juce::Identifier BufferDuration{ "bufferDuration" };
    inline const juce::Identifier AutoAdjustQuality{ "autoAdjustQuality" };
    inline const juce::Identifier MemoryBudgetBytes{ "memoryBudgetBytes" };
}

/**
 * Central state manager for Oscil plugin.
 * Handles serialization, deserialization, and migration of plugin state.
 */
class OscilState
{
public:
    /// Create a default OscilState with initialized state tree.
    OscilState();

    /**
     * Create state from XML string (from DAW)
     */
    explicit OscilState(const juce::String& xmlString);

    /**
     * Get the root ValueTree
     */
    [[nodiscard]] juce::ValueTree& getState() { return state_; }
    [[nodiscard]] const juce::ValueTree& getState() const { return state_; }

    /**
     * Serialize to XML for DAW persistence.
     * Non-const because it syncs the layout manager to the state tree.
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
