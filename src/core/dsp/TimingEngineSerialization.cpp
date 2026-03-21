/*
    Oscil - Timing Engine Configuration & Serialization
    Config setters, ValueTree persistence, and entity config conversion
*/

#include "core/dsp/TimingEngine.h"
#include <cmath>

namespace oscil
{

// === Configuration Getters ===

EngineTimingConfig TimingEngine::getConfig() const
{
    EngineTimingConfig config;
    config.timingMode = static_cast<TimingMode>(atomicTimingMode_.load(std::memory_order_relaxed));
    config.hostSyncEnabled = atomicHostSyncEnabled_.load(std::memory_order_relaxed);
    config.syncToPlayhead = atomicSyncToPlayhead_.load(std::memory_order_relaxed);
    config.timeIntervalMs = atomicTimeIntervalMs_.load(std::memory_order_relaxed);
    config.noteInterval = static_cast<EngineNoteInterval>(atomicNoteInterval_.load(std::memory_order_relaxed));
    config.internalBPM = atomicInternalBPM_.load(std::memory_order_relaxed);
    config.hostBPM = atomicHostBPM_.load(std::memory_order_relaxed);
    config.actualIntervalMs = atomicActualIntervalMs_.load(std::memory_order_relaxed);
    config.triggerMode = static_cast<WaveformTriggerMode>(atomicTriggerMode_.load(std::memory_order_relaxed));
    config.triggerThreshold = atomicTriggerThreshold_.load(std::memory_order_relaxed);
    config.triggerChannel = atomicTriggerChannel_.load(std::memory_order_relaxed);
    config.triggerHysteresis = atomicTriggerHysteresis_.load(std::memory_order_relaxed);
    config.midiTriggerNote = atomicMidiTriggerNote_.load(std::memory_order_relaxed);
    config.midiTriggerChannel = atomicMidiTriggerChannel_.load(std::memory_order_relaxed);
    config.lastSyncTimestamp = atomicLastSyncTimestamp_.load(std::memory_order_relaxed);
    return config;
}

HostTimingInfo TimingEngine::getHostInfo() const
{
    HostTimingInfo info;
    info.bpm = static_cast<double>(atomicHostBPM_.load(std::memory_order_relaxed));
    info.ppqPosition = atomicPpqPosition_.load(std::memory_order_relaxed);
    info.isPlaying = atomicIsPlaying_.load(std::memory_order_relaxed);
    info.sampleRate = atomicSampleRate_.load(std::memory_order_relaxed);
    info.timeInSamples = atomicTimeInSamples_.load(std::memory_order_relaxed);
    info.timeSigNumerator = atomicTimeSigNumerator_.load(std::memory_order_relaxed);
    info.timeSigDenominator = atomicTimeSigDenominator_.load(std::memory_order_relaxed);
    info.transportState = static_cast<HostTimingInfo::TransportState>(atomicTransportState_.load(std::memory_order_relaxed));
    return info;
}

// === Configuration Setters ===

void TimingEngine::setConfig(const EngineTimingConfig& config)
{
    atomicHostBPM_.store(config.hostBPM, std::memory_order_relaxed);
    atomicInternalBPM_.store(config.internalBPM, std::memory_order_relaxed);
    atomicTimeIntervalMs_.store(config.timeIntervalMs, std::memory_order_relaxed);
    atomicNoteInterval_.store(static_cast<int>(config.noteInterval), std::memory_order_relaxed);
    atomicTimingMode_.store(static_cast<int>(config.timingMode), std::memory_order_relaxed);
    atomicHostSyncEnabled_.store(config.hostSyncEnabled, std::memory_order_relaxed);
    atomicTriggerMode_.store(static_cast<int>(config.triggerMode), std::memory_order_relaxed);
    atomicTriggerChannel_.store(config.triggerChannel, std::memory_order_relaxed);
    atomicTriggerThreshold_.store(config.triggerThreshold, std::memory_order_relaxed);
    atomicTriggerHysteresis_.store(config.triggerHysteresis, std::memory_order_relaxed);
    atomicMidiTriggerNote_.store(config.midiTriggerNote, std::memory_order_relaxed);
    atomicMidiTriggerChannel_.store(config.midiTriggerChannel, std::memory_order_relaxed);
    atomicSyncToPlayhead_.store(config.syncToPlayhead, std::memory_order_relaxed);
    atomicLastSyncTimestamp_.store(config.lastSyncTimestamp, std::memory_order_relaxed);
    recalculateInterval();
}

void TimingEngine::setTimingMode(TimingMode mode)
{
    auto currentMode = static_cast<TimingMode>(atomicTimingMode_.load(std::memory_order_relaxed));
    if (currentMode != mode)
    {
        atomicTimingMode_.store(static_cast<int>(mode), std::memory_order_relaxed);
        recalculateInterval();
        notifyTimingModeChanged();
    }
}

void TimingEngine::setHostSyncEnabled(bool enabled)
{
    bool currentEnabled = atomicHostSyncEnabled_.load(std::memory_order_relaxed);
    if (currentEnabled != enabled)
    {
        atomicHostSyncEnabled_.store(enabled, std::memory_order_relaxed);
        auto timingMode = static_cast<TimingMode>(atomicTimingMode_.load(std::memory_order_relaxed));
        if (timingMode == TimingMode::MELODIC)
        {
            recalculateInterval();
        }
        notifyHostSyncStateChanged();
    }
}

void TimingEngine::setInternalBPM(float bpm)
{
    bpm = juce::jlimit(EngineTimingConfig::MIN_BPM, EngineTimingConfig::MAX_BPM, bpm);

    float currentBPM = atomicInternalBPM_.load(std::memory_order_relaxed);
    if (std::abs(currentBPM - bpm) > 0.01f)
    {
        atomicInternalBPM_.store(bpm, std::memory_order_relaxed);
        auto timingMode = static_cast<TimingMode>(atomicTimingMode_.load(std::memory_order_relaxed));
        bool hostSync = atomicHostSyncEnabled_.load(std::memory_order_relaxed);

        if (timingMode == TimingMode::MELODIC && !hostSync)
        {
            recalculateInterval();
        }
    }
}

void TimingEngine::setTimeIntervalMs(float ms)
{
    ms = juce::jlimit(EngineTimingConfig::MIN_TIME_INTERVAL_MS,
                      EngineTimingConfig::MAX_TIME_INTERVAL_MS, ms);

    float currentMs = atomicTimeIntervalMs_.load(std::memory_order_relaxed);
    if (std::abs(currentMs - ms) > 0.001f)
    {
        atomicTimeIntervalMs_.store(ms, std::memory_order_relaxed);
        auto timingMode = static_cast<TimingMode>(atomicTimingMode_.load(std::memory_order_relaxed));
        if (timingMode == TimingMode::TIME)
        {
            recalculateInterval();
        }
    }
}

void TimingEngine::setNoteInterval(EngineNoteInterval interval)
{
    auto currentInterval = static_cast<EngineNoteInterval>(atomicNoteInterval_.load(std::memory_order_relaxed));
    if (currentInterval != interval)
    {
        atomicNoteInterval_.store(static_cast<int>(interval), std::memory_order_relaxed);
        auto timingMode = static_cast<TimingMode>(atomicTimingMode_.load(std::memory_order_relaxed));
        if (timingMode == TimingMode::MELODIC)
        {
            recalculateInterval();
        }
    }
}

// === ValueTree Serialization ===

juce::ValueTree TimingEngine::toValueTree() const
{
    juce::ValueTree state(TimingIds::Timing);

    state.setProperty(TimingIds::TimingMode, atomicTimingMode_.load(std::memory_order_relaxed), nullptr);
    state.setProperty(TimingIds::HostSyncEnabled, atomicHostSyncEnabled_.load(std::memory_order_relaxed), nullptr);
    state.setProperty(TimingIds::SyncToPlayhead, atomicSyncToPlayhead_.load(std::memory_order_relaxed), nullptr);
    state.setProperty(TimingIds::TimeIntervalMs, atomicTimeIntervalMs_.load(std::memory_order_relaxed), nullptr);
    state.setProperty(TimingIds::NoteInterval, atomicNoteInterval_.load(std::memory_order_relaxed), nullptr);
    state.setProperty(TimingIds::TriggerMode, atomicTriggerMode_.load(std::memory_order_relaxed), nullptr);
    state.setProperty(TimingIds::TriggerThreshold, atomicTriggerThreshold_.load(std::memory_order_relaxed), nullptr);
    state.setProperty(TimingIds::MidiTriggerNote, atomicMidiTriggerNote_.load(std::memory_order_relaxed), nullptr);
    state.setProperty(TimingIds::MidiTriggerChannel, atomicMidiTriggerChannel_.load(std::memory_order_relaxed), nullptr);
    state.setProperty(TimingIds::InternalBPM, atomicInternalBPM_.load(std::memory_order_relaxed), nullptr);
    state.setProperty(TimingIds::TriggerChannel, atomicTriggerChannel_.load(std::memory_order_relaxed), nullptr);
    state.setProperty(TimingIds::TriggerHysteresis, atomicTriggerHysteresis_.load(std::memory_order_relaxed), nullptr);

    return state;
}

void TimingEngine::resetRuntimeStateForLoad()
{
    atomicLastSyncTimestamp_.store(0.0, std::memory_order_relaxed);
    manualTrigger_.store(false, std::memory_order_relaxed);
    triggered_.store(false, std::memory_order_relaxed);
    atomicHostBPM_.store(120.0f, std::memory_order_relaxed);
    atomicPpqPosition_.store(0.0, std::memory_order_relaxed);
    atomicIsPlaying_.store(false, std::memory_order_relaxed);
    atomicTimeInSamples_.store(0, std::memory_order_relaxed);
    atomicTimeSigNumerator_.store(4, std::memory_order_relaxed);
    atomicTimeSigDenominator_.store(4, std::memory_order_relaxed);
    atomicTransportState_.store(static_cast<int>(HostTimingInfo::TransportState::STOPPED), std::memory_order_relaxed);
    recalculateInterval();
    pendingTimingModeChange_.store(false, std::memory_order_relaxed);
    pendingIntervalChange_.store(false, std::memory_order_relaxed);
    pendingHostBPMChange_.store(false, std::memory_order_relaxed);
    pendingHostSyncChange_.store(false, std::memory_order_relaxed);
    pendingTimeSignatureChange_.store(false, std::memory_order_relaxed);
    resetTriggerHistoryPending_.store(true, std::memory_order_relaxed);
}

static int sanitizeTimingMode(int value)
{
    if (value == static_cast<int>(TimingMode::TIME) ||
        value == static_cast<int>(TimingMode::MELODIC))
        return value;
    return static_cast<int>(TimingMode::TIME);
}

static int sanitizeEnumRange(int value, int minVal, int maxVal, int defaultVal)
{
    return (value >= minVal && value <= maxVal) ? value : defaultVal;
}

static float sanitizeFloat(float value, float defaultVal, float minVal, float maxVal)
{
    if (!std::isfinite(value)) return defaultVal;
    return juce::jlimit(minVal, maxVal, value);
}

void TimingEngine::loadTimingProperties(const juce::ValueTree& state)
{
    atomicTimingMode_.store(
        sanitizeTimingMode(static_cast<int>(state.getProperty(TimingIds::TimingMode, 0))),
        std::memory_order_relaxed);
    atomicHostSyncEnabled_.store(state.getProperty(TimingIds::HostSyncEnabled, false), std::memory_order_relaxed);
    atomicSyncToPlayhead_.store(state.getProperty(TimingIds::SyncToPlayhead, false), std::memory_order_relaxed);

    atomicTimeIntervalMs_.store(
        sanitizeFloat(state.getProperty(TimingIds::TimeIntervalMs, 500.0f),
                      EngineTimingConfig::DEFAULT_TIME_INTERVAL_MS,
                      EngineTimingConfig::MIN_TIME_INTERVAL_MS,
                      EngineTimingConfig::MAX_TIME_INTERVAL_MS),
        std::memory_order_relaxed);

    atomicNoteInterval_.store(
        sanitizeEnumRange(static_cast<int>(state.getProperty(TimingIds::NoteInterval,
                          static_cast<int>(EngineNoteInterval::NOTE_1_4TH))),
                          static_cast<int>(EngineNoteInterval::NOTE_1_32ND),
                          static_cast<int>(EngineNoteInterval::NOTE_TRIPLET_1_8TH),
                          static_cast<int>(EngineNoteInterval::NOTE_1_4TH)),
        std::memory_order_relaxed);

    atomicTriggerMode_.store(
        sanitizeEnumRange(static_cast<int>(state.getProperty(TimingIds::TriggerMode,
                          static_cast<int>(WaveformTriggerMode::None))),
                          static_cast<int>(WaveformTriggerMode::None),
                          static_cast<int>(WaveformTriggerMode::BothEdges),
                          static_cast<int>(WaveformTriggerMode::None)),
        std::memory_order_relaxed);

    atomicTriggerThreshold_.store(sanitizeFloat(state.getProperty(TimingIds::TriggerThreshold, 0.1f), 0.1f, 0.0f, 1.0f), std::memory_order_relaxed);
    atomicMidiTriggerNote_.store(juce::jlimit(-1, 127, static_cast<int>(state.getProperty(TimingIds::MidiTriggerNote, -1))), std::memory_order_relaxed);
    atomicMidiTriggerChannel_.store(juce::jlimit(0, 16, static_cast<int>(state.getProperty(TimingIds::MidiTriggerChannel, 0))), std::memory_order_relaxed);
    atomicInternalBPM_.store(sanitizeFloat(state.getProperty(TimingIds::InternalBPM, 120.0f), 120.0f, EngineTimingConfig::MIN_BPM, EngineTimingConfig::MAX_BPM), std::memory_order_relaxed);
    atomicTriggerChannel_.store(juce::jmax(0, static_cast<int>(state.getProperty(TimingIds::TriggerChannel, 0))), std::memory_order_relaxed);
    atomicTriggerHysteresis_.store(sanitizeFloat(state.getProperty(TimingIds::TriggerHysteresis, 0.01f), 0.01f, 0.0f, 1.0f), std::memory_order_relaxed);
}

void TimingEngine::fromValueTree(const juce::ValueTree& state)
{
    if (!state.hasType(TimingIds::Timing))
    {
        resetRuntimeStateForLoad();
        return;
    }

    loadTimingProperties(state);
    resetRuntimeStateForLoad();
}

// === Entity Integration Methods ===

void TimingEngine::applyEntityConfig(const TimingConfig& entityConfig)
{
    // Apply timing mode (same enum, direct assignment)
    atomicTimingMode_.store(static_cast<int>(entityConfig.timingMode), std::memory_order_relaxed);

    // Apply time interval (both are float, direct assignment)
    atomicTimeIntervalMs_.store(entityConfig.timeIntervalMs, std::memory_order_relaxed);

    // Convert entity NoteInterval to engine EngineNoteInterval
    atomicNoteInterval_.store(static_cast<int>(entityToEngineNoteInterval(entityConfig.noteInterval)), std::memory_order_relaxed);

    // Apply host settings
    atomicHostSyncEnabled_.store(entityConfig.hostSyncEnabled, std::memory_order_relaxed);
    atomicHostBPM_.store(entityConfig.hostBPM, std::memory_order_relaxed);
    atomicSyncToPlayhead_.store(entityConfig.syncToPlayhead, std::memory_order_relaxed);

    // Apply trigger settings
    // Entity TriggerMode maps to sync mode, not waveform trigger
    WaveformTriggerMode newTriggerMode = WaveformTriggerMode::None;

    // Convert entity trigger mode to waveform trigger if needed
    if (entityConfig.triggerMode == TriggerMode::TRIGGERED)
    {
        // Convert TriggerEdge to WaveformTriggerMode
        switch (entityConfig.triggerEdge)
        {
            case TriggerEdge::Rising:
                newTriggerMode = WaveformTriggerMode::RisingEdge;
                break;
            case TriggerEdge::Falling:
                newTriggerMode = WaveformTriggerMode::FallingEdge;
                break;
            case TriggerEdge::Both:
                newTriggerMode = WaveformTriggerMode::BothEdges;
                break;
        }
    }
    else if (entityConfig.triggerMode == TriggerMode::MIDI)
    {
        newTriggerMode = WaveformTriggerMode::Midi;
    }

    atomicTriggerMode_.store(static_cast<int>(newTriggerMode), std::memory_order_relaxed);

    // Apply trigger threshold (convert dBFS to linear if needed)
    // Entity uses dBFS (-20.0f default), engine uses linear (0.0-1.0)
    // Convert: linear = 10^(dBFS/20)
    float thresholdDbfs = entityConfig.triggerThreshold;
    if (!std::isfinite(thresholdDbfs))
        thresholdDbfs = -20.0f;  // Default if NaN/Inf
    // Clamp dBFS to valid range to prevent Inf from pow() (0 dB = 1.0 linear)
    thresholdDbfs = juce::jlimit(-60.0f, 0.0f, thresholdDbfs);
    float thresholdLinear = std::pow(10.0f, thresholdDbfs / 20.0f);
    // Additional safety check for pow() output
    if (!std::isfinite(thresholdLinear))
        thresholdLinear = 0.1f;
    atomicTriggerThreshold_.store(juce::jlimit(0.0f, 1.0f, thresholdLinear), std::memory_order_relaxed);

    // Apply MIDI config
    atomicMidiTriggerNote_.store(entityConfig.midiTriggerNote, std::memory_order_relaxed);
    atomicMidiTriggerChannel_.store(entityConfig.midiTriggerChannel, std::memory_order_relaxed);

    recalculateInterval();
}

TimingConfig TimingEngine::toEntityConfig() const
{
    TimingConfig entityConfig;

    // Copy timing mode (same enum)
    entityConfig.timingMode = static_cast<TimingMode>(atomicTimingMode_.load(std::memory_order_relaxed));

    // Copy time interval
    entityConfig.timeIntervalMs = atomicTimeIntervalMs_.load(std::memory_order_relaxed);

    // Convert engine EngineNoteInterval to entity NoteInterval
    entityConfig.noteInterval = engineToEntityNoteInterval(static_cast<EngineNoteInterval>(atomicNoteInterval_.load(std::memory_order_relaxed)));

    // Copy host settings (use atomic for thread-safe read from UI thread)
    entityConfig.hostSyncEnabled = atomicHostSyncEnabled_.load(std::memory_order_relaxed);
    entityConfig.hostBPM = atomicHostBPM_.load(std::memory_order_relaxed);
    entityConfig.syncToPlayhead = atomicSyncToPlayhead_.load(std::memory_order_relaxed);

    // Convert waveform trigger mode to entity trigger mode
    auto triggerMode = static_cast<WaveformTriggerMode>(atomicTriggerMode_.load(std::memory_order_relaxed));

    if (triggerMode != WaveformTriggerMode::None &&
        triggerMode != WaveformTriggerMode::Manual)
    {
        if (triggerMode == WaveformTriggerMode::Midi)
        {
            entityConfig.triggerMode = TriggerMode::MIDI;
        }
        else
        {
            entityConfig.triggerMode = TriggerMode::TRIGGERED;
            // Convert WaveformTriggerMode to TriggerEdge
            switch (triggerMode)
            {
                case WaveformTriggerMode::RisingEdge:
                    entityConfig.triggerEdge = TriggerEdge::Rising;
                    break;
                case WaveformTriggerMode::FallingEdge:
                    entityConfig.triggerEdge = TriggerEdge::Falling;
                    break;
                case WaveformTriggerMode::BothEdges:
                    entityConfig.triggerEdge = TriggerEdge::Both;
                    break;
                case WaveformTriggerMode::Level:
                    entityConfig.triggerEdge = TriggerEdge::Both;
                    break;
                case WaveformTriggerMode::None:
                case WaveformTriggerMode::Midi:
                case WaveformTriggerMode::Manual:
                    // These modes are handled elsewhere or use defaults
                    break;
            }
        }
    }
    else
    {
        entityConfig.triggerMode = TriggerMode::FREE_RUNNING;
    }

    // Convert linear threshold to dBFS
    // dBFS = 20 * log10(linear)
    float threshold = atomicTriggerThreshold_.load(std::memory_order_relaxed);
    float thresholdDbfs = 20.0f * std::log10(juce::jmax(0.0001f, threshold));
    entityConfig.triggerThreshold = thresholdDbfs;

    // Copy MIDI settings
    entityConfig.midiTriggerNote = atomicMidiTriggerNote_.load(std::memory_order_relaxed);
    entityConfig.midiTriggerChannel = atomicMidiTriggerChannel_.load(std::memory_order_relaxed);

    // Copy actual interval (use atomic for thread-safe read from UI thread)
    entityConfig.actualIntervalMs = atomicActualIntervalMs_.load(std::memory_order_relaxed);

    return entityConfig;
}

} // namespace oscil
