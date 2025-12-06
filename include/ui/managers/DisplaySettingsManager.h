/*
    Oscil - Display Settings Manager
    Manages global display settings across all panes
*/

#pragma once

#include <juce_core/juce_core.h>
#include <functional>
#include <memory>
#include <vector>

namespace oscil
{

// Forward declarations
class PaneComponent;
struct GridConfiguration;
struct OscillatorId;

/**
 * Manages display settings that apply globally to all panes.
 * Extracted from PluginEditor to reduce god class responsibilities.
 *
 * This manager owns no state - it simply coordinates applying settings
 * to all pane components in the editor.
 *
 * Note: Uses a callback to get panes rather than storing a reference,
 * to prevent iterator invalidation if the source vector is modified
 * during iteration (e.g., by callbacks triggered during settings updates).
 */
class DisplaySettingsManager
{
public:
    /** Callback type for getting current panes (returns raw pointers for safe iteration) */
    using PaneGetter = std::function<std::vector<PaneComponent*>()>;

    /**
     * Constructor
     * @param paneGetter Callback that returns the current list of pane pointers
     */
    explicit DisplaySettingsManager(PaneGetter paneGetter);

    /**
     * Enable/disable grid display for all panes
     * @param show True to show grid, false to hide
     */
    void setShowGridForAll(bool show);

    /**
     * Update grid configuration for all panes
     * @param config The grid configuration to apply
     */
    void setGridConfigForAll(const GridConfiguration& config);

    /**
     * Enable/disable auto-scaling for all panes
     * @param autoScale True to enable auto-scaling
     */
    void setAutoScaleForAll(bool autoScale);

    /**
     * Set gain in dB for all panes
     * @param gainDb Gain value in decibels
     */
    void setGainDbForAll(float gainDb);

    /**
     * Set number of display samples for all panes
     * @param samples Number of samples to display
     */
    void setDisplaySamplesForAll(int samples);

    /**
     * Set sample rate for all panes (for crosshair time calculations)
     * @param sampleRate Sample rate in Hz (should be capture rate, not source rate)
     */
    void setSampleRateForAll(int sampleRate);

    /**
     * Highlight a specific oscillator across all panes
     * @param id The oscillator ID to highlight
     */
    void highlightOscillator(const OscillatorId& id);

private:
    PaneGetter paneGetter_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DisplaySettingsManager)
};

} // namespace oscil
