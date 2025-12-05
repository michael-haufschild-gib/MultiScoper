/*
    Oscil - Display Settings Manager
    Manages global display settings across all panes
*/

#pragma once

#include <juce_core/juce_core.h>
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
 */
class DisplaySettingsManager
{
public:
    /**
     * Constructor
     * @param panes Reference to the vector of pane components to manage
     */
    explicit DisplaySettingsManager(std::vector<std::unique_ptr<PaneComponent>>& panes);

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
     * Highlight a specific oscillator across all panes
     * @param id The oscillator ID to highlight
     */
    void highlightOscillator(const OscillatorId& id);

private:
    std::vector<std::unique_ptr<PaneComponent>>& panes_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DisplaySettingsManager)
};

} // namespace oscil
