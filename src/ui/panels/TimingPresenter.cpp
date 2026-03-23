/*
    Oscil - Timing Presenter Implementation
*/

#include "ui/panels/TimingPresenter.h"

namespace oscil
{

TimingPresenter::TimingPresenter() = default;

void TimingPresenter::setTimingMode(TimingMode mode)
{
    if (currentMode_ != mode)
    {
        currentMode_ = mode;
        if (onTimingModeChanged_)
            onTimingModeChanged_(mode);
        notifyStateChanged();
    }
}

void TimingPresenter::setTimeIntervalMs(float ms)
{
    if (std::abs(currentTimeIntervalMs_ - ms) > 1e-4f)
    {
        currentTimeIntervalMs_ = ms;
        if (onTimeIntervalChanged_)
            onTimeIntervalChanged_(ms);
        // No state change (layout) needed usually
    }
}

void TimingPresenter::setNoteInterval(NoteInterval interval)
{
    if (currentNoteInterval_ != interval)
    {
        currentNoteInterval_ = interval;
        if (onNoteIntervalChanged_)
            onNoteIntervalChanged_(interval);
    }
}

void TimingPresenter::setHostSyncEnabled(bool enabled)
{
    if (hostSyncEnabled_ != enabled)
    {
        hostSyncEnabled_ = enabled;
        if (onHostSyncChanged_)
            onHostSyncChanged_(enabled);
        notifyStateChanged();
    }
}

void TimingPresenter::setWaveformMode(WaveformMode mode)
{
    if (waveformMode_ != mode)
    {
        waveformMode_ = mode;
        if (onWaveformModeChanged_)
            onWaveformModeChanged_(mode);
    }
}

void TimingPresenter::setHostBPM(float bpm)
{
    if (std::abs(hostBPM_ - bpm) > 1e-4f)
    {
        hostBPM_ = bpm;
        // Host BPM only affects visible UI while displaying host-synced melodic mode.
        if (shouldShowBPMValue())
            notifyStateChanged();
    }
}

void TimingPresenter::setInternalBPM(float bpm)
{
    if (std::abs(internalBPM_ - bpm) > 1e-4f)
    {
        internalBPM_ = bpm;
        if (onBPMChanged_)
            onBPMChanged_(bpm);
    }
}

void TimingPresenter::setSyncStatus(bool synced)
{
    if (isSynced_ != synced)
    {
        isSynced_ = synced;
        if (onSyncStatusChanged_)
            onSyncStatusChanged_(synced);
        notifyStateChanged();
    }
}

void TimingPresenter::notifyStateChanged()
{
    if (onStateChanged_)
        onStateChanged_();
}

} // namespace oscil
