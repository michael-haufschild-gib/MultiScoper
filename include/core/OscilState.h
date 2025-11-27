/*
    Oscil - State Management
    ValueTree-based state persistence for plugin settings
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "Oscillator.h"
#include "Pane.h"
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

    // Oscillator properties
    static const juce::Identifier Id{ "id" };
    static const juce::Identifier SourceId{ "sourceId" };
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
    static const juce::Identifier VerticalScale{ "verticalScale" };
    static const juce::Identifier VerticalOffset{ "verticalOffset" };
    static const juce::Identifier TimeWindow{ "timeWindow" };

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
    static const juce::Identifier HoldDisplay{ "holdDisplay" };
    static const juce::Identifier GainDb{ "gainDb" };

    // Legacy identifiers for migration
    static const juce::Identifier LegacyTracks{ "Tracks" };
    static const juce::Identifier LegacyGlobalMode{ "globalMode" };
    static const juce::Identifier LegacyGridRows{ "gridRows" };
    static const juce::Identifier LegacyGridCols{ "gridCols" };
}

/**
 * Central state manager for Oscil plugin.
 * Handles serialization, deserialization, and migration of plugin state.
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
     */
    juce::ValueTree& getState() { return state_; }
    const juce::ValueTree& getState() const { return state_; }

    /**
     * Serialize to XML for DAW persistence
     */
    juce::String toXmlString() const;

    /**
     * Load from XML string
     */
    bool fromXmlString(const juce::String& xmlString);

    /**
     * Get all oscillators
     */
    std::vector<Oscillator> getOscillators() const;

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
    std::optional<Oscillator> getOscillator(const OscillatorId& oscillatorId) const;

    /**
     * Get the pane layout manager
     */
    PaneLayoutManager& getLayoutManager() { return layoutManager_; }
    const PaneLayoutManager& getLayoutManager() const { return layoutManager_; }

    /**
     * Get/set current theme name
     */
    juce::String getThemeName() const;
    void setThemeName(const juce::String& themeName);

    /**
     * Get/set column layout
     */
    ColumnLayout getColumnLayout() const;
    void setColumnLayout(ColumnLayout layout);

    /**
     * Get/set sidebar configuration
     */
    int getSidebarWidth() const;
    void setSidebarWidth(int width);

    bool isSidebarCollapsed() const;
    void setSidebarCollapsed(bool collapsed);

    /**
     * Get/set status bar visibility
     */
    bool isStatusBarVisible() const;
    void setStatusBarVisible(bool visible);

    /**
     * Display options
     */
    bool isShowGridEnabled() const;
    void setShowGridEnabled(bool enabled);

    bool isAutoScaleEnabled() const;
    void setAutoScaleEnabled(bool enabled);

    bool isHoldDisplayEnabled() const;
    void setHoldDisplayEnabled(bool enabled);

    float getGainDb() const;
    void setGainDb(float dB);

    /**
     * Add a listener for state changes
     */
    void addListener(juce::ValueTree::Listener* listener);
    void removeListener(juce::ValueTree::Listener* listener);

    /**
     * Get the current schema version
     */
    int getSchemaVersion() const;

    /**
     * Check if migration is needed
     */
    bool needsMigration() const;

    // Current schema version
    static constexpr int CURRENT_SCHEMA_VERSION = 2;

private:
    // Mutable because syncLayoutManagerToState() syncs the layoutManager_ runtime
    // representation back to state_ during serialization (toXmlString). This is
    // conceptually a cache synchronization, not a state change.
    mutable juce::ValueTree state_;
    PaneLayoutManager layoutManager_;

    void initializeDefaultState();
    void migrateIfNeeded();
    void migrateFromLegacy();

    /**
     * Sync layoutManager_ back to state_ before serialization.
     * This ensures pane changes made at runtime are persisted.
     */
    void syncLayoutManagerToState() const;

    // Non-const versions for modification - ensure nodes exist
    juce::ValueTree getOrCreateOscillatorsNode();
    juce::ValueTree getOrCreatePanesNode();
    juce::ValueTree getOrCreateLayoutNode();

    // Const versions for reading - return empty if not found
    juce::ValueTree getOscillatorsNode() const;
    juce::ValueTree getPanesNode() const;
    juce::ValueTree getLayoutNode() const;
};

/**
 * Global user preferences (stored separately from project state)
 */
class GlobalPreferences
{
public:
    static GlobalPreferences& getInstance();

    /**
     * Get the preferences file
     */
    juce::File getPreferencesFile() const;

    /**
     * Load preferences from disk
     */
    void load();

    /**
     * Save preferences to disk
     */
    void save();

    // Preference accessors
    juce::String getDefaultTheme() const;
    void setDefaultTheme(const juce::String& themeName);

    int getDefaultColumnLayout() const;
    void setDefaultColumnLayout(int columns);

    bool getShowStatusBar() const;
    void setShowStatusBar(bool show);

    // UI customization settings
    bool getReducedMotion() const;
    void setReducedMotion(bool reduced);

    bool getUIAudioFeedback() const;
    void setUIAudioFeedback(bool enabled);

    bool getTooltipsEnabled() const;
    void setTooltipsEnabled(bool enabled);

    int getDefaultSidebarWidth() const;
    void setDefaultSidebarWidth(int width);

    GlobalPreferences(const GlobalPreferences&) = delete;
    GlobalPreferences& operator=(const GlobalPreferences&) = delete;

private:
    GlobalPreferences();
    ~GlobalPreferences();

    juce::ValueTree preferences_;
};

} // namespace oscil
