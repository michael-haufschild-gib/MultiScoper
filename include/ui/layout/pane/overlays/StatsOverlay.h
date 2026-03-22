/*
    Oscil - Stats Overlay
    Displays real-time statistics overlay in pane body
*/

#pragma once

#include "ui/layout/pane/overlays/PaneOverlay.h"
#include "core/analysis/AnalysisTypes.h"
#include "ui/components/OscilButton.h"
#include <vector>

namespace oscil
{

struct MetricSnapshot
{
    float rmsDb = -100.0f;
    float peakDb = -100.0f;
    float crestFactorDb = 0.0f;
    float dcOffset = 0.0f;
    float attackTimeMs = 0.0f;
    float decayTimeMs = 0.0f;
    float maxPeakDb = -100.0f;
};

struct OscillatorStats
{
    juce::String name;
    // We store a summary metric or specific channel metric?
    // Let's store left/right for completeness, but UI might only show max/avg
    MetricSnapshot left;
    MetricSnapshot right;
    bool hasSignal = false;
};

/**
 * Statistics overlay showing real-time audio metrics.
 * Displays data per oscillator assigned to the pane in a table format.
 *
 * Features:
 * - Dynamic columns (one per oscillator)
 * - Selectable text (via TextEditor) for copy/paste
 * - Reset button for accumulated values
 *
 * Position: TopRight by default
 * Background: Semi-transparent, themed
 */
class StatsOverlay : public PaneOverlay
{
public:
    explicit StatsOverlay(IThemeService& themeService);
    StatsOverlay(IThemeService& themeService, const juce::String& testId);
    ~StatsOverlay() override = default;

    /// Refresh the displayed statistics for all oscillators in the owning pane.
    void updateStats(const std::vector<OscillatorStats>& stats);

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Callback for reset button
    std::function<void()> onResetStats;

    // Access for testing
    juce::String getDisplayedText() const;

protected:
    juce::Rectangle<int> getPreferredContentSize() const override;

private:
    void setupComponents();
    void rebuildTable(const std::vector<OscillatorStats>& stats);
    juce::String formatTable(const std::vector<OscillatorStats>& stats);
    
    // Formatting helpers
    static juce::String formatDb(float dB);
    static juce::String formatPercent(float value);
    static juce::String formatMs(float ms);

    // Components
    std::unique_ptr<juce::TextEditor> statsDisplay_;
    std::unique_ptr<OscilButton> resetButton_;

    // Layout constants
    static constexpr int ROW_HEIGHT = 15;
    static constexpr int HEADER_HEIGHT = 20;
    static constexpr int LABEL_COLUMN_WIDTH = 60;
    static constexpr int DATA_COLUMN_WIDTH = 70;
    static constexpr int PADDING = 8;
    static constexpr int RESET_BUTTON_SIZE = 16;

    // State for sizing
    int numOscillators_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatsOverlay)
};

} // namespace oscil