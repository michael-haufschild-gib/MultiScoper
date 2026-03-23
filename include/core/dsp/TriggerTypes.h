/*
    Oscil - Trigger Type Definitions
    Trigger and waveform mode enums for timing synchronization
*/

#pragma once

#include <juce_core/juce_core.h>

namespace oscil
{

/**
 * Trigger mode for timing synchronization
 */
enum class TriggerMode
{
    FREE_RUNNING,  // Continuous display without sync
    HOST_SYNC,     // Sync to host transport (play/stop)
    TRIGGERED,     // Trigger on signal threshold
    MIDI           // Trigger on MIDI Note On
};

inline juce::String triggerModeToString(TriggerMode mode)
{
    switch (mode)
    {
        case TriggerMode::FREE_RUNNING: return "FREE_RUNNING";
        case TriggerMode::HOST_SYNC:    return "HOST_SYNC";
        case TriggerMode::TRIGGERED:    return "TRIGGERED";
        case TriggerMode::MIDI:         return "MIDI";
    }
    return "FREE_RUNNING";
}

inline TriggerMode stringToTriggerMode(const juce::String& str)
{
    if (str == "HOST_SYNC")  return TriggerMode::HOST_SYNC;
    if (str == "TRIGGERED")  return TriggerMode::TRIGGERED;
    if (str == "MIDI")       return TriggerMode::MIDI;
    return TriggerMode::FREE_RUNNING;
}

/**
 * Edge detection mode for triggered timing
 */
enum class TriggerEdge
{
    Rising,
    Falling,
    Both
};

inline juce::String triggerEdgeToString(TriggerEdge edge)
{
    switch (edge)
    {
        case TriggerEdge::Rising:  return "Rising";
        case TriggerEdge::Falling: return "Falling";
        case TriggerEdge::Both:    return "Both";
    }
    return "Rising";
}

inline TriggerEdge stringToTriggerEdge(const juce::String& str)
{
    if (str == "Falling") return TriggerEdge::Falling;
    if (str == "Both")    return TriggerEdge::Both;
    return TriggerEdge::Rising;
}

/**
 * Waveform trigger/restart mode
 */
enum class WaveformMode
{
    FreeRunning,    // Waveforms keep running continuously
    RestartOnPlay,  // Waveforms reset when playback starts
    RestartOnNote   // Waveforms reset on MIDI note
};

inline juce::String waveformModeToString(WaveformMode mode)
{
    switch (mode)
    {
        case WaveformMode::FreeRunning:    return "Free Running";
        case WaveformMode::RestartOnPlay:  return "Restart on Play";
        case WaveformMode::RestartOnNote:  return "Restart on Note";
    }
    return "Free Running";
}

inline WaveformMode stringToWaveformMode(const juce::String& str)
{
    if (str == "Restart on Play")  return WaveformMode::RestartOnPlay;
    if (str == "Restart on Note")  return WaveformMode::RestartOnNote;
    return WaveformMode::FreeRunning;
}

} // namespace oscil
