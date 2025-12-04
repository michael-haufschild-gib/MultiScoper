/*
    Oscil - Software Grid Renderer
    Handles CPU-based grid rendering for waveform visualization
    Extracted from WaveformComponent to reduce complexity
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include "core/dsp/TimingConfig.h"
#include "core/Oscillator.h"
#include "ui/theme/IThemeService.h"

namespace oscil
{

/**
 * Handles CPU-based (JUCE Graphics) grid rendering for waveform displays
 * Supports both TIME and MELODIC timing modes
 * This is the software fallback for grid rendering, distinct from the GPU GridRenderer
 */
class SoftwareGridRenderer
{
public:
    explicit SoftwareGridRenderer(IThemeService& themeService);

    /**
     * Enable/disable grid rendering
     */
    void setShowGrid(bool show);

    /**
     * Check if grid is visible
     */
    bool isGridVisible() const { return showGrid_; }

    /**
     * Set the grid configuration
     */
    void setGridConfig(const GridConfiguration& config);

    /**
     * Get the current grid configuration
     */
    const GridConfiguration& getGridConfig() const { return gridConfig_; }

    /**
     * Render the grid lines
     * @param g Graphics context to draw with
     * @param bounds Drawing area
     * @param processingMode Current processing mode (determines stereo vs mono layout)
     */
    void render(juce::Graphics& g,
                juce::Rectangle<int> bounds,
                ProcessingMode processingMode);

    /**
     * Render grid labels (time/beat axis and amplitude axis)
     * @param g Graphics context to draw with
     * @param bounds Drawing area
     * @param processingMode Current processing mode (determines stereo vs mono layout)
     */
    void renderLabels(juce::Graphics& g,
                     juce::Rectangle<int> bounds,
                     ProcessingMode processingMode);

private:
    /**
     * Draw grid for a single channel area
     */
    void drawChannelGrid(juce::Graphics& g,
                        juce::Rectangle<int> area);

    /**
     * Draw amplitude labels for a channel area
     */
    void drawAmplitudeLabels(juce::Graphics& g,
                           juce::Rectangle<int> area,
                           bool showChannelLabel,
                           const juce::String& channelLabel);

    /**
     * Format time value for display
     */
    static juce::String formatTimeLabel(float ms);

    /**
     * Calculate nice grid step size for time-based mode
     */
    static float calculateGridStepSize(float totalDuration, int targetDivisions);

    IThemeService& themeService_;
    bool showGrid_ = true;
    GridConfiguration gridConfig_;
};

} // namespace oscil
