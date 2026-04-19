/*
    MultiScoper - Plugin Editor Adapters
    Helper classes for handling listener callbacks
*/

#pragma once

#include "core/MemoryBudgetManager.h"
#include "core/dsp/TimingEngine.h"
#include "ui/controllers/OscillatorPanelController.h"
#include "ui/dialogs/OscillatorConfigDialog.h"
#include "ui/layout/SidebarComponent.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginFactory.h"

namespace multiscoper
{

/**
 * Adapter class to bridge OscillatorConfigDialog::Listener to MultiScoperPluginEditor
 */
class ConfigPopupListenerAdapter : public OscillatorConfigDialog::Listener
{
public:
    explicit ConfigPopupListenerAdapter(MultiScoperPluginEditor& editor) : editor_(editor) {}

    void oscillatorConfigChanged(const OscillatorId& oscillatorId, const Oscillator& updated) override
    {
        // Source-change path runs through the controller (which manages
        // source-registry re-binding + UI refresh side effects); all other
        // field changes commit straight to state via updateOscillator().
        if (auto* controller = editor_.getOscillatorPanelController())
        {
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

    void configDialogClosed() override { editor_.onConfigPopupClosed(); }

private:
    MultiScoperPluginEditor& editor_;
};

/**
 * Adapter class to forward TimingEngine::Listener events to MultiScoperPluginEditor
 * This handles automatic updates when host timing info changes (e.g., time signature)
 */
class TimingEngineListenerAdapter : public TimingEngine::Listener
{
public:
    explicit TimingEngineListenerAdapter(MultiScoperPluginEditor& editor) : editor_(editor) {}

    void timingModeChanged(TimingMode mode) override
    {
        editor_.updateTimingSidebarMode(mode);
        updateDisplayAndGrid();
    }

    void intervalChanged(float /*actualIntervalMs*/) override
    {
        // Interval changed - update display samples and grid
        updateDisplayAndGrid();
    }

    void hostBPMChanged(float bpm) override
    {
        editor_.updateTimingSidebarHostBpm(bpm);
        updateDisplayAndGrid();
    }

    void hostSyncStateChanged(bool enabled) override
    {
        editor_.updateTimingSidebarHostSyncEnabled(enabled);
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
        // IMPORTANT: Use capture rate (decimated) for display calculations
        int const captureRate = processor.getCaptureRate();

        if (captureRate > 0)
        {
            float const actualIntervalMs = timingEngine.getActualIntervalMs();
            int const displaySamples =
                static_cast<int>(static_cast<double>(captureRate) * (static_cast<double>(actualIntervalMs) / 1000.0));
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

    MultiScoperPluginEditor& editor_;
};

} // namespace multiscoper
