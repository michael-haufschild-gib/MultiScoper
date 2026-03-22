/*
    Oscil - Waveform Stack
    Manages multiple waveform components within a pane body
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/Oscillator.h"
#include "ui/panels/WaveformComponent.h"
#include "ui/theme/IThemeService.h"
#include "ui/components/TestId.h"
#include "core/dsp/TimingConfig.h"
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

namespace oscil
{

// Forward declarations
class IAudioDataProvider;
class ShaderRegistry;
class IAudioBuffer;

/**
 * Manages a stack of waveform components.
 *
 * Responsibilities:
 * - Owns and manages WaveformComponent instances
 * - Handles layout (stacked vertically, or overlapped in future)
 * - Propagates display settings to all waveforms
 * - Handles oscillator add/remove/update operations
 *
 * Extracted from PaneComponent for better separation of concerns.
 */
class WaveformStack : public juce::Component,
                       public TestIdSupport
{
public:
    /**
     * Layout mode for waveforms
     */
    enum class LayoutMode
    {
        Stacked,    // Waveforms stacked vertically (default)
        Overlapped  // Waveforms overlaid on same area (future)
    };

    WaveformStack(IAudioDataProvider& dataProvider,
                  IThemeService& themeService,
                  ShaderRegistry& shaderRegistry);
    ~WaveformStack() override = default;

    // Component overrides
    void resized() override;

    // Oscillator management
    void addOscillator(const Oscillator& oscillator);
    void removeOscillator(const OscillatorId& oscillatorId);
    void clearOscillators();

    // Update oscillator properties
    void updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId);
    void updateOscillator(const OscillatorId& oscillatorId, ProcessingMode mode, bool visible);
    void updateOscillatorName(const OscillatorId& oscillatorId, const juce::String& name);
    void updateOscillatorColor(const OscillatorId& oscillatorId, juce::Colour color);
    void updateOscillatorFull(const Oscillator& oscillator);

    // Get oscillator count
    size_t getOscillatorCount() const { return entries_.size(); }

    // Layout mode
    void setLayoutMode(LayoutMode mode);
    LayoutMode getLayoutMode() const { return layoutMode_; }

    // Display settings - apply to all waveforms
    void setShowGrid(bool enabled);
    void setGridConfig(const GridConfiguration& config);
    void setAutoScale(bool enabled);
    void setHoldDisplay(bool enabled);
    void setGainDb(float dB);
    void setDisplaySamples(int samples);
    /// Request all child waveforms to restart rendering from the given DAW timeline position.
    void requestRestartAtTimestamp(int64_t timelineSampleTimestamp);

    // Highlight a specific oscillator
    void highlightOscillator(const OscillatorId& oscillatorId);

    // Test access
    WaveformComponent* getWaveformAt(size_t index) const {
        return (index < entries_.size()) ? entries_[index].waveform.get() : nullptr;
    }
    const Oscillator* getOscillatorAt(size_t index) const {
        return (index < entries_.size()) ? &entries_[index].oscillator : nullptr;
    }

private:
    void updateLayout();

    IAudioDataProvider& dataProvider_;
    IThemeService& themeService_;
    ShaderRegistry& shaderRegistry_;

    struct OscillatorEntry
    {
        Oscillator oscillator;
        std::unique_ptr<WaveformComponent> waveform;
    };

    std::vector<OscillatorEntry> entries_;
    LayoutMode layoutMode_ = LayoutMode::Stacked;

    // Minimum waveform height
    static constexpr int MIN_WAVEFORM_HEIGHT = 100;

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformStack)
};

} // namespace oscil
