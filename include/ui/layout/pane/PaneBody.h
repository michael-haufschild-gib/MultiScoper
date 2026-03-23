/*
    Oscil - Pane Body
    Body component for pane containing waveforms and overlays
*/

#pragma once

#include "core/Oscillator.h"
#include "core/dsp/TimingConfig.h"
#include "ui/components/TestId.h"
#include "ui/layout/pane/WaveformStack.h"
#include "ui/layout/pane/overlays/CrosshairOverlay.h"
#include "ui/layout/pane/overlays/StatsOverlay.h"
#include "ui/theme/IThemeService.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <memory>

namespace oscil
{

// Forward declarations
class IAudioDataProvider;
class IInstanceRegistry;
class ShaderRegistry;

/**
 * Body component for a pane.
 *
 * Contains:
 * - WaveformStack (manages waveform components)
 * - CrosshairOverlay (mouse-following crosshair with tooltip)
 * - StatsOverlay (statistics display)
 */
class PaneBody
    : public juce::Component
    , public TestIdSupport
    , private juce::Timer
{
public:
    PaneBody(IAudioDataProvider& dataProvider, IInstanceRegistry& instanceRegistry, IThemeService& themeService,
             ShaderRegistry& shaderRegistry);
    ~PaneBody() override;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    // Oscillator management (delegates to WaveformStack)
    void addOscillator(const Oscillator& oscillator);
    void removeOscillator(const OscillatorId& oscillatorId);
    void clearOscillators();
    void updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId);
    void updateOscillator(const OscillatorId& oscillatorId, ProcessingMode mode, bool visible);
    void updateOscillatorName(const OscillatorId& oscillatorId, const juce::String& name);
    void updateOscillatorColor(const OscillatorId& oscillatorId, juce::Colour color);
    void updateOscillatorFull(const Oscillator& oscillator);
    size_t getOscillatorCount() const;

    // Display settings (delegates to WaveformStack)
    void setShowGrid(bool enabled);
    void setGridConfig(const GridConfiguration& config);
    void setAutoScale(bool enabled);
    void setHoldDisplay(bool enabled);
    void setGainDb(float dB);
    void setDisplaySamples(int samples);
    void setSampleRate(int sampleRate);
    void requestWaveformRestartAtTimestamp(int64_t timelineSampleTimestamp);

    // Highlight oscillator
    void highlightOscillator(const OscillatorId& oscillatorId);

    // Overlay control
    void setStatsVisible(bool visible);
    bool isStatsVisible() const;

    // Access for testing
    WaveformStack* getWaveformStack() { return waveformStack_.get(); }
    CrosshairOverlay* getCrosshairOverlay() { return crosshairOverlay_.get(); }
    StatsOverlay* getStatsOverlay() { return statsOverlay_.get(); }

private:
    void updateCrosshairPosition(juce::Point<int> pos);
    void calculateCrosshairValues(juce::Point<int> pos, float& timeMs, float& ampDb);
    void timerCallback() override;
    void updateStats();
    void resetStats();

    IAudioDataProvider& dataProvider_;
    IInstanceRegistry& instanceRegistry_;
    IThemeService& themeService_;

    // Child components
    std::unique_ptr<WaveformStack> waveformStack_;
    std::unique_ptr<CrosshairOverlay> crosshairOverlay_;
    std::unique_ptr<StatsOverlay> statsOverlay_;

    // Display parameters for crosshair calculation
    int displaySamples_ = 1024;
    int sampleRate_ = 44100;

    static constexpr int PADDING = 4;
    static constexpr int STATS_UPDATE_HZ = 15; // Approx 66ms

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PaneBody)
};

} // namespace oscil
