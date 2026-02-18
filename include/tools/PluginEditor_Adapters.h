/*
    Oscil - Plugin Editor Adapters
    Helper classes for handling listener callbacks
*/

#pragma once

#include "plugin/PluginEditor.h"
#include "plugin/PluginFactory.h"
#include "ui/layout/SidebarComponent.h"
#include "ui/dialogs/OscillatorConfigDialog.h"
#include "core/dsp/TimingEngine.h"
#include "core/MemoryBudgetManager.h"

namespace oscil
{

/**
 * Adapter class to bridge OscillatorConfigDialog::Listener to OscilPluginEditor
 */
class ConfigPopupListenerAdapter : public OscillatorConfigDialog::Listener
{
public:
    explicit ConfigPopupListenerAdapter(OscilPluginEditor& editor)
        : editor_(editor)
    {
    }

    void oscillatorConfigChanged(const OscillatorId& oscillatorId, const Oscillator& updated) override
    {
        // Forward to controller via editor accessor
        if (auto* controller = editor_.getOscillatorPanelController())
        {
            // This method isn't in the SidebarListener interface we inherited.
            // We need to expose `onOscillatorConfigChanged` on the Controller or access `updateOscillator` via state directly.
            // But the Controller handles state updates.
            
            // Let's use the `updateOscillator` method on state via processor, 
            // but also we need to handle source changes.
            // `updateOscillatorSource` is public on controller now.
            
            auto& state = editor_.getProcessor().getState();
            auto currentOscillators = state.getOscillators();
            for (const auto& existing : currentOscillators)
            {
                if (existing.getId() == oscillatorId)
                {
                    if (existing.getSourceId() != updated.getSourceId())
                    {
                        controller->updateOscillatorSource(oscillatorId, updated.getSourceId());
                    }
                    break;
                }
            }
            state.updateOscillator(updated);
        }
    }

    void oscillatorDeleteRequested(const OscillatorId& oscillatorId) override
    {
        if (auto* controller = editor_.getOscillatorPanelController())
        {
            controller->oscillatorDeleteRequested(oscillatorId);
        }
    }

    void configDialogClosed() override
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

    void hostBPMChanged(float bpm) override
    {
        // Dispatch to message thread - this callback may be invoked from audio thread
        // Use SafePointer to handle case where sidebar is destroyed before callback runs
        if (auto* sidebar = editor_.getSidebar())
        {
            juce::Component::SafePointer<SidebarComponent> safeSidebar(sidebar);
            juce::MessageManager::callAsync([safeSidebar, bpm]() {
                if (safeSidebar != nullptr)
                {
                    if (auto* timingSection = safeSidebar->getTimingSection())
                    {
                        timingSection->setHostBPM(bpm);
                    }
                }
            });
        }
    }

    void hostSyncStateChanged(bool enabled) override
    {
        // Dispatch to message thread - this callback may be invoked from audio thread
        // Use SafePointer to handle case where sidebar is destroyed before callback runs
        if (auto* sidebar = editor_.getSidebar())
        {
            juce::Component::SafePointer<SidebarComponent> safeSidebar(sidebar);
            juce::MessageManager::callAsync([safeSidebar, enabled]() {
                if (safeSidebar != nullptr)
                {
                    if (auto* timingSection = safeSidebar->getTimingSection())
                    {
                        timingSection->setHostSyncEnabled(enabled);
                    }
                }
            });
        }
    }

private:
    void updateDisplayAndGrid()
    {
        auto& processor = editor_.getProcessor();
        auto& timingEngine = processor.getTimingEngine();
        // IMPORTANT: Use capture rate (decimated) for display calculations
        int captureRate = processor.getCaptureRate();

        if (captureRate > 0)
        {
            float actualIntervalMs = timingEngine.getActualIntervalMs();
            int displaySamples = static_cast<int>(static_cast<double>(captureRate) * (static_cast<double>(actualIntervalMs) / 1000.0));
            editor_.setDisplaySamplesForAllPanes(displaySamples);
            editor_.setSampleRateForAllPanes(captureRate);

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

/**
 * Adapter class to bridge SidebarComponent::Listener timing events to TimingEngine
 */
class TimingSidebarListenerAdapter : public SidebarComponent::Listener
{
public:
    explicit TimingSidebarListenerAdapter(OscilPluginEditor& editor)
        : editor_(editor) {}

    void timingModeChanged(TimingMode mode) override
    {
        editor_.getProcessor().getTimingEngine().setTimingMode(mode);
    }

    void noteIntervalChanged(NoteInterval interval) override
    {
        editor_.getProcessor().getTimingEngine().setNoteIntervalFromEntity(interval);
    }

    void timeIntervalChanged(float ms) override
    {
        editor_.getProcessor().getTimingEngine().setTimeIntervalMs(ms);
    }

    void hostSyncChanged(bool enabled) override
    {
        editor_.getProcessor().getTimingEngine().setHostSyncEnabled(enabled);
    }

    void bpmChanged(float bpm) override
    {
        editor_.getProcessor().getTimingEngine().setInternalBPM(bpm);
    }

    void waveformModeChanged(WaveformMode mode) override
    {
        // Map WaveformMode (UI) to WaveformTriggerMode (Engine)
        // FreeRunning -> None (free-running display)
        // RestartOnPlay -> None (not yet implemented, falls back to free-running)
        // RestartOnNote -> Midi (trigger on MIDI note)

        auto& engine = editor_.getProcessor().getTimingEngine();

        switch (mode)
        {
            case WaveformMode::FreeRunning:
                engine.setWaveformTriggerMode(WaveformTriggerMode::None);
                break;
            case WaveformMode::RestartOnPlay:
                // RestartOnPlay currently not implemented - falls back to free-running
                // TODO: Implement transport-synced restart when DAW starts playback
                engine.setWaveformTriggerMode(WaveformTriggerMode::None);
                break;
            case WaveformMode::RestartOnNote:
                engine.setWaveformTriggerMode(WaveformTriggerMode::Midi);
                break;
        }

        // FIX: Force display refresh after waveform mode change to clear stale state
        // Without this, switching modes could leave stale displaySamples or buffer state
        updateDisplayAndGrid();
    }

private:
    void updateDisplayAndGrid()
    {
        auto& processor = editor_.getProcessor();
        auto& timingEngine = processor.getTimingEngine();
        int captureRate = processor.getCaptureRate();

        if (captureRate > 0)
        {
            float actualIntervalMs = timingEngine.getActualIntervalMs();
            int displaySamples = static_cast<int>(static_cast<double>(captureRate) * (static_cast<double>(actualIntervalMs) / 1000.0));
            editor_.setDisplaySamplesForAllPanes(displaySamples);
            editor_.setSampleRateForAllPanes(captureRate);

            GridConfiguration gridConfig;
            gridConfig.enabled = processor.getState().isShowGridEnabled();

            const auto& engineConfig = timingEngine.getConfig();
            gridConfig.timingMode = engineConfig.timingMode;
            gridConfig.visibleDurationMs = actualIntervalMs;

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

/**
 * Adapter class to bridge SidebarComponent::Listener options events to Editor/Processor
 */
class OptionsSidebarListenerAdapter : public SidebarComponent::Listener
{
public:
    explicit OptionsSidebarListenerAdapter(OscilPluginEditor& editor)
        : editor_(editor) {}

    void layoutChanged(int columnCount) override
    {
        // Safe cast to ColumnLayout (1-3)
        auto layout = static_cast<ColumnLayout>(juce::jlimit(1, 3, columnCount));
        editor_.getProcessor().getState().setColumnLayout(layout);
        // Trigger layout update
        editor_.resized();
    }

    void themeChanged(const juce::String& themeName) override
    {
        editor_.getProcessor().getThemeService().setCurrentTheme(themeName);
    }

    void gpuRenderingChanged(bool enabled) override
    {
        editor_.setGpuRenderingEnabled(enabled);
    }

    void qualityPresetChanged(QualityPreset preset) override
    {
        auto config = editor_.getProcessor().getState().getCaptureQualityConfig();
        config.qualityPreset = preset;
        editor_.getProcessor().getState().setCaptureQualityConfig(config);
    }

    void bufferDurationChanged(BufferDuration duration) override
    {
        auto config = editor_.getProcessor().getState().getCaptureQualityConfig();
        config.bufferDuration = duration;
        editor_.getProcessor().getState().setCaptureQualityConfig(config);
    }

    void autoAdjustQualityChanged(bool enabled) override
    {
        auto config = editor_.getProcessor().getState().getCaptureQualityConfig();
        config.autoAdjustQuality = enabled;
        editor_.getProcessor().getState().setCaptureQualityConfig(config);
    }

private:
    OscilPluginEditor& editor_;
};

} // namespace oscil
