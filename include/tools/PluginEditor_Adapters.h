/*
    Oscil - Plugin Editor Adapters
    Helper classes for handling listener callbacks
*/

#pragma once

#include "plugin/PluginEditor.h"
#include "ui/layout/SidebarComponent.h"
#include "ui/panels/OscillatorConfigPopup.h"
#include "core/dsp/TimingEngine.h"

namespace oscil
{

/**
 * Adapter class to forward SidebarComponent::Listener events to OscilPluginEditor
 */
class SidebarListenerAdapter : public SidebarComponent::Listener
{
public:
    explicit SidebarListenerAdapter(OscilPluginEditor& editor) : editor_(editor) {}

    void sidebarWidthChanged(int newWidth) override
    {
        editor_.onSidebarWidthChanged(newWidth);
    }

    void sidebarCollapsedStateChanged(bool collapsed) override
    {
        editor_.onSidebarCollapsedStateChanged(collapsed);
    }

    void oscillatorSelected(const OscillatorId& oscillatorId) override
    {
        editor_.onOscillatorSelected(oscillatorId);
    }

    void oscillatorConfigRequested(const OscillatorId& oscillatorId) override
    {
        editor_.onOscillatorConfigRequested(oscillatorId);
    }

    void oscillatorColorConfigRequested(const OscillatorId& oscillatorId) override
    {
        editor_.onOscillatorColorConfigRequested(oscillatorId);
    }

    void oscillatorDeleteRequested(const OscillatorId& oscillatorId) override
    {
        editor_.onOscillatorDeleteRequested(oscillatorId);
    }

    void oscillatorModeChanged(const OscillatorId& oscillatorId, ProcessingMode mode) override
    {
        editor_.onOscillatorModeChanged(oscillatorId, mode);
    }

    void oscillatorVisibilityChanged(const OscillatorId& oscillatorId, bool visible) override
    {
        editor_.onOscillatorVisibilityChanged(oscillatorId, visible);
    }

    // Timing section events - forward to processor's TimingEngine
    void timingModeChanged(TimingMode mode) override
    {
        editor_.getProcessor().getTimingEngine().setTimingMode(mode);
        updateDisplaySamplesFromTimingEngine();
    }

    void noteIntervalChanged(NoteInterval interval) override
    {
        editor_.getProcessor().getTimingEngine().setNoteIntervalFromEntity(interval);
        updateDisplaySamplesFromTimingEngine();
    }

    void timeIntervalChanged(float ms) override
    {
        editor_.getProcessor().getTimingEngine().setTimeIntervalMs(ms);
        updateDisplaySamplesFromTimingEngine();
    }

    void hostSyncChanged(bool enabled) override
    {
        editor_.getProcessor().getTimingEngine().setHostSyncEnabled(enabled);
        updateDisplaySamplesFromTimingEngine();
    }

    void waveformModeChanged(WaveformMode mode) override
    {
        // This controls when waveforms reset (free running, on play, on note)
        auto& timingEngine = editor_.getProcessor().getTimingEngine();

        switch (mode)
        {
            case WaveformMode::FreeRunning:
                // Disable host sync and set trigger mode to None
                timingEngine.setHostSyncEnabled(false);
                timingEngine.setWaveformTriggerMode(WaveformTriggerMode::None);
                break;
            case WaveformMode::RestartOnPlay:
                // Enable host sync (resets on play/stop transitions)
                // Set trigger mode to None (no audio triggering)
                timingEngine.setHostSyncEnabled(true);
                timingEngine.setWaveformTriggerMode(WaveformTriggerMode::None);
                break;
            case WaveformMode::RestartOnNote:
                // For now, map to a basic audio trigger (e.g., RisingEdge)
                // TODO: Future: Implement proper MIDI note trigger logic in TimingEngine.
                // This would involve passing MIDI messages from PluginProcessor to TimingEngine.
                timingEngine.setHostSyncEnabled(false); // Not host sync based
                timingEngine.setWaveformTriggerMode(WaveformTriggerMode::RisingEdge);
                break;
        }
        updateDisplaySamplesFromTimingEngine();
    }

    void bpmChanged(float bpm) override
    {
        // Update internal BPM in the timing engine
        // The engine handles whether to recalculate based on sync state
        editor_.getProcessor().getTimingEngine().setInternalBPM(bpm);
        updateDisplaySamplesFromTimingEngine();
    }

private:
    /**
     * Update display samples for all panes based on the TimingEngine's
     * calculated actual interval. This works for both TIME and MELODIC modes.
     */
    void updateDisplaySamplesFromTimingEngine()
    {
        auto& processor = editor_.getProcessor();
        auto& timingEngine = processor.getTimingEngine();
        double sampleRate = processor.getSampleRate();
        
        if (sampleRate > 0)
        {
            float actualIntervalMs = timingEngine.getActualIntervalMs();
            int displaySamples = static_cast<int>(sampleRate * (static_cast<double>(actualIntervalMs) / 1000.0));
            editor_.setDisplaySamplesForAllPanes(displaySamples);

            // Update Grid Configuration
            GridConfiguration gridConfig;
            gridConfig.enabled = processor.getState().isShowGridEnabled();
            
            const auto& engineConfig = timingEngine.getConfig();
            gridConfig.timingMode = engineConfig.timingMode;
            gridConfig.visibleDurationMs = actualIntervalMs;
            
            // Musical params
            gridConfig.noteInterval = timingEngine.getNoteIntervalAsEntity();
            
            const auto& hostInfo = timingEngine.getHostInfo();
            gridConfig.bpm = static_cast<float>(hostInfo.bpm);
            gridConfig.timeSigNumerator = hostInfo.timeSigNumerator;
            gridConfig.timeSigDenominator = hostInfo.timeSigDenominator;
            
            editor_.setGridConfigForAllPanes(gridConfig);
        }
    }

    // Master controls events
    void gainChanged(float dB) override
    {
        editor_.setGainDbForAllPanes(dB);
        editor_.getProcessor().getState().setGainDb(dB);
    }

    // Display options events
    void showGridChanged(bool enabled) override
    {
        editor_.setShowGridForAllPanes(enabled);
        editor_.getProcessor().getState().setShowGridEnabled(enabled);
        
        // Also force update grid config to sync state (though setShowGridForAllPanes handles the boolean)
        // We might want to trigger a full update to ensure consistency
        updateDisplaySamplesFromTimingEngine();
    }

    void autoScaleChanged(bool enabled) override
    {
        editor_.setAutoScaleForAllPanes(enabled);
        editor_.getProcessor().getState().setAutoScaleEnabled(enabled);
    }

    void holdDisplayChanged(bool enabled) override
    {
        editor_.setHoldDisplayForAllPanes(enabled);
        editor_.getProcessor().getState().setHoldDisplayEnabled(enabled);
    }

    void oscillatorsReordered(int fromIndex, int toIndex) override
    {
        // Reorder oscillators in state
        editor_.getProcessor().getState().reorderOscillators(fromIndex, toIndex);

        // Refresh the UI to show the new order
        auto oscillators = editor_.getProcessor().getState().getOscillators();

        // Sort by orderIndex to get the display order
        std::sort(oscillators.begin(), oscillators.end(),
                  [](const Oscillator& a, const Oscillator& b) {
                      return a.getOrderIndex() < b.getOrderIndex();
                  });

        // Refresh the sidebar with updated oscillator list immediately for feedback
        editor_.refreshSidebarOscillatorList(oscillators);
        
        // Full UI refresh will be triggered by state listener (valueTreeChildOrderChanged)
    }

    void addOscillatorDialogRequested() override
    {
        editor_.onAddOscillatorDialogRequested();
    }

    // Layout and theme events (from Options section)
    void layoutChanged(int columnCount) override
    {
        auto& state = editor_.getProcessor().getState();
        ColumnLayout layout = static_cast<ColumnLayout>(juce::jlimit(1, 3, columnCount));
        state.getLayoutManager().setColumnLayout(layout);
        state.setColumnLayout(layout);
        editor_.refreshPanels();
    }

    void themeChanged(const juce::String& themeName) override
    {
        editor_.getProcessor().getThemeService().setCurrentTheme(themeName);
        editor_.getProcessor().getState().setThemeName(themeName);
    }

    void gpuRenderingChanged(bool enabled) override
    {
        editor_.setGpuRenderingEnabled(enabled);
    }

private:
    OscilPluginEditor& editor_;
};

/**
 * Adapter class to forward OscillatorConfigPopup::Listener events to OscilPluginEditor
 */
class ConfigPopupListenerAdapter : public OscillatorConfigPopup::Listener
{
public:
    explicit ConfigPopupListenerAdapter(OscilPluginEditor& editor) : editor_(editor) {}

    void oscillatorConfigChanged(const OscillatorId& oscillatorId, const Oscillator& updated) override
    {
        editor_.onOscillatorConfigChanged(oscillatorId, updated);
    }

    void oscillatorDeleteRequested(const OscillatorId& oscillatorId) override
    {
        editor_.onOscillatorDeleteRequested(oscillatorId);
    }

    void configPopupClosed() override
    {
        editor_.onConfigPopupClosed();
    }

private:
    OscilPluginEditor& editor_;
};

/**
 * Adapter class to forward TimingEngine::Listener events to OscilPluginEditor
 * This handles automatic updates when host timing info changes (e.g., time signature)
 */
class TimingEngineListenerAdapter : public TimingEngine::Listener
{
public:
    explicit TimingEngineListenerAdapter(OscilPluginEditor& editor)
        : editor_(editor) {}

    void intervalChanged(float /*actualIntervalMs*/) override
    {
        // Interval changed - update display samples and grid
        updateDisplayAndGrid();
    }

    void timeSignatureChanged(int /*numerator*/, int /*denominator*/) override
    {
        // Time signature changed - update display samples and grid
        // This is critical for bar-based intervals (1 Bar, 2 Bars, etc.)
        updateDisplayAndGrid();
    }

private:
    void updateDisplayAndGrid()
    {
        auto& processor = editor_.getProcessor();
        auto& timingEngine = processor.getTimingEngine();
        double sampleRate = processor.getSampleRate();

        if (sampleRate > 0)
        {
            float actualIntervalMs = timingEngine.getActualIntervalMs();
            int displaySamples = static_cast<int>(sampleRate * (static_cast<double>(actualIntervalMs) / 1000.0));
            editor_.setDisplaySamplesForAllPanes(displaySamples);

            // Update Grid Configuration
            GridConfiguration gridConfig;
            gridConfig.enabled = processor.getState().isShowGridEnabled();

            const auto& engineConfig = timingEngine.getConfig();
            gridConfig.timingMode = engineConfig.timingMode;
            gridConfig.visibleDurationMs = actualIntervalMs;

            // Musical params
            gridConfig.noteInterval = timingEngine.getNoteIntervalAsEntity();

            const auto& hostInfo = timingEngine.getHostInfo();
            gridConfig.bpm = static_cast<float>(hostInfo.bpm);
            gridConfig.timeSigNumerator = hostInfo.timeSigNumerator;
            gridConfig.timeSigDenominator = hostInfo.timeSigDenominator;

            editor_.setGridConfigForAllPanes(gridConfig);
        }
    }

    OscilPluginEditor& editor_;
};

} // namespace oscil
