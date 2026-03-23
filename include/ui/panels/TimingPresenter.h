/*
    Oscil - Timing Presenter
    Handles logic for timing configuration and state management.
    Separates logic from View (TimingSidebarSection).
*/

#pragma once

#include "core/dsp/TimingConfig.h"

#include <juce_core/juce_core.h>

#include <functional>

namespace oscil
{

class TimingPresenter
{
public:
    TimingPresenter();
    virtual ~TimingPresenter() = default;

    // Setters (trigger updates)
    void setTimingMode(TimingMode mode);
    void setTimeIntervalMs(float ms);
    void setNoteInterval(NoteInterval interval);
    void setHostSyncEnabled(bool enabled);
    void setWaveformMode(WaveformMode mode);
    void setHostBPM(float bpm);
    void setInternalBPM(float bpm);
    void setSyncStatus(bool synced);

    // Getters
    TimingMode getTimingMode() const { return currentMode_; }
    float getTimeIntervalMs() const { return currentTimeIntervalMs_; }
    NoteInterval getNoteInterval() const { return currentNoteInterval_; }
    bool isHostSyncEnabled() const { return hostSyncEnabled_; }
    WaveformMode getWaveformMode() const { return waveformMode_; }
    float getInternalBPM() const { return internalBPM_; }
    float getHostBPM() const { return hostBPM_; }
    bool isSynced() const { return isSynced_; }

    // Helpers for UI state
    bool isTimeMode() const { return currentMode_ == TimingMode::TIME; }
    bool isMelodicMode() const { return currentMode_ == TimingMode::MELODIC; }
    bool shouldShowBPMField() const { return isMelodicMode() && !hostSyncEnabled_; }
    bool shouldShowBPMValue() const { return isMelodicMode() && hostSyncEnabled_; }
    bool shouldShowSyncToggle() const { return isMelodicMode(); }
    bool shouldShowSyncedBadge() const { return isMelodicMode() && hostSyncEnabled_ && isSynced_; }

    // Callbacks
    using TimingModeCallback = std::function<void(TimingMode)>;
    using NoteIntervalCallback = std::function<void(NoteInterval)>;
    using TimeIntervalCallback = std::function<void(float)>;
    using HostSyncCallback = std::function<void(bool)>;
    using WaveformModeCallback = std::function<void(WaveformMode)>;
    using BPMCallback = std::function<void(float)>;
    using SyncStatusCallback = std::function<void(bool)>;
    using StateChangedCallback = std::function<void()>; // Generic update for UI refresh

    void setOnTimingModeChanged(TimingModeCallback cb) { onTimingModeChanged_ = std::move(cb); }
    void setOnNoteIntervalChanged(NoteIntervalCallback cb) { onNoteIntervalChanged_ = std::move(cb); }
    void setOnTimeIntervalChanged(TimeIntervalCallback cb) { onTimeIntervalChanged_ = std::move(cb); }
    void setOnHostSyncChanged(HostSyncCallback cb) { onHostSyncChanged_ = std::move(cb); }
    void setOnWaveformModeChanged(WaveformModeCallback cb) { onWaveformModeChanged_ = std::move(cb); }
    void setOnBPMChanged(BPMCallback cb) { onBPMChanged_ = std::move(cb); }
    void setOnSyncStatusChanged(SyncStatusCallback cb) { onSyncStatusChanged_ = std::move(cb); }

    // Combined callback for when the View needs to refresh visibility/layout
    void setOnStateChanged(StateChangedCallback cb) { onStateChanged_ = std::move(cb); }

private:
    void notifyStateChanged();

    TimingMode currentMode_ = TimingMode::TIME;
    float currentTimeIntervalMs_ = 500.0f;
    NoteInterval currentNoteInterval_ = NoteInterval::QUARTER;
    WaveformMode waveformMode_ = WaveformMode::FreeRunning;
    float hostBPM_ = 120.0f;
    float internalBPM_ = 120.0f;
    bool hostSyncEnabled_ = false;
    bool isSynced_ = false;

    TimingModeCallback onTimingModeChanged_;
    NoteIntervalCallback onNoteIntervalChanged_;
    TimeIntervalCallback onTimeIntervalChanged_;
    HostSyncCallback onHostSyncChanged_;
    WaveformModeCallback onWaveformModeChanged_;
    BPMCallback onBPMChanged_;
    SyncStatusCallback onSyncStatusChanged_;
    StateChangedCallback onStateChanged_;
};

} // namespace oscil
