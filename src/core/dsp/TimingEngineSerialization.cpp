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
    auto cfg = configLock_.read();
    auto hostInfo = hostInfoLock_.read();

    EngineTimingConfig result;
    result.timingMode = cfg.timingMode;
    result.hostSyncEnabled = cfg.hostSyncEnabled;
    result.syncToPlayhead = cfg.syncToPlayhead;
    result.timeIntervalMs = cfg.timeIntervalMs;
    result.noteInterval = cfg.noteInterval;
    result.internalBPM = cfg.internalBPM;
    result.hostBPM = static_cast<float>(hostInfo.bpm);
    result.actualIntervalMs = atomicActualIntervalMs_.load(std::memory_order_relaxed);
    result.triggerMode = cfg.triggerMode;
    result.triggerThreshold = cfg.triggerThreshold;
    result.triggerChannel = cfg.triggerChannel;
    result.triggerHysteresis = cfg.triggerHysteresis;
    result.midiTriggerNote = cfg.midiTriggerNote;
    result.midiTriggerChannel = cfg.midiTriggerChannel;
    result.lastSyncTimestamp = atomicLastSyncTimestamp_.load(std::memory_order_relaxed);
    return result;
}

HostTimingInfo TimingEngine::getHostInfo() const { return hostInfoLock_.read(); }

// === Configuration Setters ===

void TimingEngine::setConfig(const EngineTimingConfig& config)
{
    TimingConfigData cfg;
    cfg.timingMode = config.timingMode;
    cfg.hostSyncEnabled = config.hostSyncEnabled;
    cfg.syncToPlayhead = config.syncToPlayhead;
    cfg.timeIntervalMs = config.timeIntervalMs;
    cfg.noteInterval = config.noteInterval;
    cfg.internalBPM = config.internalBPM;
    cfg.triggerMode = config.triggerMode;
    cfg.triggerChannel = config.triggerChannel;
    cfg.triggerThreshold = config.triggerThreshold;
    cfg.triggerHysteresis = config.triggerHysteresis;
    cfg.midiTriggerNote = config.midiTriggerNote;
    cfg.midiTriggerChannel = config.midiTriggerChannel;
    configLock_.write(cfg);

    // Update host BPM in the host info (written from UI thread for setConfig)
    auto hostInfo = hostInfoLock_.read();
    hostInfo.bpm = static_cast<double>(config.hostBPM);
    hostInfoLock_.write(hostInfo);
    audioThreadHostInfo_.bpm = static_cast<double>(config.hostBPM);

    atomicLastSyncTimestamp_.store(config.lastSyncTimestamp, std::memory_order_relaxed);
    recalculateInterval();
}

void TimingEngine::setTimingMode(TimingMode mode)
{
    auto cfg = configLock_.read();
    if (cfg.timingMode != mode)
    {
        cfg.timingMode = mode;
        configLock_.write(cfg);
        recalculateInterval();
        notifyTimingModeChanged();
    }
}

void TimingEngine::setHostSyncEnabled(bool enabled)
{
    auto cfg = configLock_.read();
    if (cfg.hostSyncEnabled != enabled)
    {
        cfg.hostSyncEnabled = enabled;
        configLock_.write(cfg);
        if (cfg.timingMode == TimingMode::MELODIC)
            recalculateInterval();
        notifyHostSyncStateChanged();
    }
}

void TimingEngine::setInternalBPM(float bpm)
{
    bpm = juce::jlimit(EngineTimingConfig::MIN_BPM, EngineTimingConfig::MAX_BPM, bpm);

    auto cfg = configLock_.read();
    if (std::abs(cfg.internalBPM - bpm) > 0.01f)
    {
        cfg.internalBPM = bpm;
        configLock_.write(cfg);
        if (cfg.timingMode == TimingMode::MELODIC && !cfg.hostSyncEnabled)
            recalculateInterval();
    }
}

void TimingEngine::setTimeIntervalMs(float ms)
{
    ms = juce::jlimit(EngineTimingConfig::MIN_TIME_INTERVAL_MS, EngineTimingConfig::MAX_TIME_INTERVAL_MS, ms);

    auto cfg = configLock_.read();
    if (std::abs(cfg.timeIntervalMs - ms) > 0.001f)
    {
        cfg.timeIntervalMs = ms;
        configLock_.write(cfg);
        if (cfg.timingMode == TimingMode::TIME)
            recalculateInterval();
    }
}

void TimingEngine::setNoteInterval(EngineNoteInterval interval)
{
    auto cfg = configLock_.read();
    if (cfg.noteInterval != interval)
    {
        cfg.noteInterval = interval;
        configLock_.write(cfg);
        if (cfg.timingMode == TimingMode::MELODIC)
            recalculateInterval();
    }
}

void TimingEngine::setWaveformTriggerMode(WaveformTriggerMode mode)
{
    auto cfg = configLock_.read();
    cfg.triggerMode = mode;
    configLock_.write(cfg);
}

void TimingEngine::setTriggerThreshold(float threshold)
{
    threshold = juce::jlimit(0.0f, 1.0f, threshold);
    auto cfg = configLock_.read();
    cfg.triggerThreshold = threshold;
    configLock_.write(cfg);
}

void TimingEngine::setTriggerChannel(int channel)
{
    auto cfg = configLock_.read();
    cfg.triggerChannel = channel;
    configLock_.write(cfg);
}

void TimingEngine::setTriggerHysteresis(float hysteresis)
{
    auto cfg = configLock_.read();
    cfg.triggerHysteresis = hysteresis;
    configLock_.write(cfg);
}

void TimingEngine::setMidiTriggerNote(int note)
{
    auto cfg = configLock_.read();
    cfg.midiTriggerNote = note;
    configLock_.write(cfg);
}

void TimingEngine::setMidiTriggerChannel(int channel)
{
    auto cfg = configLock_.read();
    cfg.midiTriggerChannel = channel;
    configLock_.write(cfg);
}

void TimingEngine::setSyncToPlayhead(bool enabled)
{
    auto cfg = configLock_.read();
    cfg.syncToPlayhead = enabled;
    configLock_.write(cfg);
}

// === ValueTree Serialization ===

juce::ValueTree TimingEngine::toValueTree() const
{
    juce::ValueTree state(TimingIds::Timing);
    auto cfg = configLock_.read();

    state.setProperty(TimingIds::TimingMode, static_cast<int>(cfg.timingMode), nullptr);
    state.setProperty(TimingIds::HostSyncEnabled, cfg.hostSyncEnabled, nullptr);
    state.setProperty(TimingIds::SyncToPlayhead, cfg.syncToPlayhead, nullptr);
    state.setProperty(TimingIds::TimeIntervalMs, cfg.timeIntervalMs, nullptr);
    state.setProperty(TimingIds::NoteInterval, static_cast<int>(cfg.noteInterval), nullptr);
    state.setProperty(TimingIds::TriggerMode, static_cast<int>(cfg.triggerMode), nullptr);
    state.setProperty(TimingIds::TriggerThreshold, cfg.triggerThreshold, nullptr);
    state.setProperty(TimingIds::MidiTriggerNote, cfg.midiTriggerNote, nullptr);
    state.setProperty(TimingIds::MidiTriggerChannel, cfg.midiTriggerChannel, nullptr);
    state.setProperty(TimingIds::InternalBPM, cfg.internalBPM, nullptr);
    state.setProperty(TimingIds::TriggerChannel, cfg.triggerChannel, nullptr);
    state.setProperty(TimingIds::TriggerHysteresis, cfg.triggerHysteresis, nullptr);

    return state;
}

void TimingEngine::resetRuntimeStateForLoad()
{
    atomicLastSyncTimestamp_.store(0.0, std::memory_order_relaxed);
    manualTrigger_.store(false, std::memory_order_relaxed);
    triggered_.store(false, std::memory_order_relaxed);

    // Reset host info to defaults
    audioThreadHostInfo_ = HostTimingInfo{};
    hostInfoLock_.write(audioThreadHostInfo_);

    recalculateInterval();
    pendingFlags_.store(0, std::memory_order_relaxed);
    resetTriggerHistoryPending_.store(true, std::memory_order_relaxed);
}

static int sanitizeTimingMode(int value)
{
    if (value == static_cast<int>(TimingMode::TIME) || value == static_cast<int>(TimingMode::MELODIC))
        return value;
    return static_cast<int>(TimingMode::TIME);
}

static int sanitizeEnumRange(int value, int minVal, int maxVal, int defaultVal)
{
    return (value >= minVal && value <= maxVal) ? value : defaultVal;
}

static float sanitizeFloat(float value, float defaultVal, float minVal, float maxVal)
{
    if (!std::isfinite(value))
        return defaultVal;
    return juce::jlimit(minVal, maxVal, value);
}

void TimingEngine::loadTimingProperties(const juce::ValueTree& state)
{
    TimingConfigData cfg;

    cfg.timingMode =
        static_cast<TimingMode>(sanitizeTimingMode(static_cast<int>(state.getProperty(TimingIds::TimingMode, 0))));
    cfg.hostSyncEnabled = state.getProperty(TimingIds::HostSyncEnabled, false);
    cfg.syncToPlayhead = state.getProperty(TimingIds::SyncToPlayhead, false);

    cfg.timeIntervalMs = sanitizeFloat(
        state.getProperty(TimingIds::TimeIntervalMs, 500.0f), EngineTimingConfig::DEFAULT_TIME_INTERVAL_MS,
        EngineTimingConfig::MIN_TIME_INTERVAL_MS, EngineTimingConfig::MAX_TIME_INTERVAL_MS);

    cfg.noteInterval = static_cast<EngineNoteInterval>(sanitizeEnumRange(
        static_cast<int>(state.getProperty(TimingIds::NoteInterval, static_cast<int>(EngineNoteInterval::NOTE_1_4TH))),
        static_cast<int>(EngineNoteInterval::NOTE_1_32ND), static_cast<int>(EngineNoteInterval::NOTE_TRIPLET_1_8TH),
        static_cast<int>(EngineNoteInterval::NOTE_1_4TH)));

    cfg.triggerMode = static_cast<WaveformTriggerMode>(sanitizeEnumRange(
        static_cast<int>(state.getProperty(TimingIds::TriggerMode, static_cast<int>(WaveformTriggerMode::None))),
        static_cast<int>(WaveformTriggerMode::None), static_cast<int>(WaveformTriggerMode::BothEdges),
        static_cast<int>(WaveformTriggerMode::None)));

    cfg.triggerThreshold = sanitizeFloat(state.getProperty(TimingIds::TriggerThreshold, 0.1f), 0.1f, 0.0f, 1.0f);
    cfg.midiTriggerNote = juce::jlimit(-1, 127, static_cast<int>(state.getProperty(TimingIds::MidiTriggerNote, -1)));
    cfg.midiTriggerChannel = juce::jlimit(0, 16, static_cast<int>(state.getProperty(TimingIds::MidiTriggerChannel, 0)));
    cfg.internalBPM = sanitizeFloat(state.getProperty(TimingIds::InternalBPM, 120.0f), 120.0f,
                                    EngineTimingConfig::MIN_BPM, EngineTimingConfig::MAX_BPM);
    cfg.triggerChannel = juce::jmax(0, static_cast<int>(state.getProperty(TimingIds::TriggerChannel, 0)));
    cfg.triggerHysteresis = sanitizeFloat(state.getProperty(TimingIds::TriggerHysteresis, 0.01f), 0.01f, 0.0f, 1.0f);

    configLock_.write(cfg);
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
    TimingConfigData cfg;

    cfg.timingMode = entityConfig.timingMode;
    cfg.timeIntervalMs = entityConfig.timeIntervalMs;
    cfg.noteInterval = entityToEngineNoteInterval(entityConfig.noteInterval);
    cfg.hostSyncEnabled = entityConfig.hostSyncEnabled;
    cfg.syncToPlayhead = entityConfig.syncToPlayhead;
    cfg.internalBPM = configLock_.read().internalBPM; // Preserve existing

    // Convert entity trigger mode to waveform trigger
    WaveformTriggerMode newTriggerMode = WaveformTriggerMode::None;
    if (entityConfig.triggerMode == TriggerMode::TRIGGERED)
    {
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
    cfg.triggerMode = newTriggerMode;

    // Convert dBFS threshold to linear
    float thresholdDbfs = entityConfig.triggerThreshold;
    if (!std::isfinite(thresholdDbfs))
        thresholdDbfs = -20.0f;
    thresholdDbfs = juce::jlimit(-60.0f, 0.0f, thresholdDbfs);
    float thresholdLinear = std::pow(10.0f, thresholdDbfs / 20.0f);
    if (!std::isfinite(thresholdLinear))
        thresholdLinear = 0.1f;
    cfg.triggerThreshold = juce::jlimit(0.0f, 1.0f, thresholdLinear);

    cfg.midiTriggerNote = entityConfig.midiTriggerNote;
    cfg.midiTriggerChannel = entityConfig.midiTriggerChannel;

    configLock_.write(cfg);

    // Update host BPM in the host info
    auto hostInfo = hostInfoLock_.read();
    hostInfo.bpm = static_cast<double>(entityConfig.hostBPM);
    hostInfoLock_.write(hostInfo);
    audioThreadHostInfo_.bpm = static_cast<double>(entityConfig.hostBPM);

    recalculateInterval();
}

namespace
{
void convertTriggerMode(const TimingConfigData& cfg, TimingConfig& out)
{
    if (cfg.triggerMode == WaveformTriggerMode::None || cfg.triggerMode == WaveformTriggerMode::Manual)
    {
        out.triggerMode = TriggerMode::FREE_RUNNING;
        return;
    }

    if (cfg.triggerMode == WaveformTriggerMode::Midi)
    {
        out.triggerMode = TriggerMode::MIDI;
        return;
    }

    out.triggerMode = TriggerMode::TRIGGERED;
    switch (cfg.triggerMode)
    {
        case WaveformTriggerMode::RisingEdge:
            out.triggerEdge = TriggerEdge::Rising;
            break;
        case WaveformTriggerMode::FallingEdge:
            out.triggerEdge = TriggerEdge::Falling;
            break;
        case WaveformTriggerMode::BothEdges:
        case WaveformTriggerMode::Level:
            out.triggerEdge = TriggerEdge::Both;
            break;
        default:
            break;
    }
}
} // namespace

TimingConfig TimingEngine::toEntityConfig() const
{
    auto cfg = configLock_.read();
    auto hostInfo = hostInfoLock_.read();

    TimingConfig entityConfig;
    entityConfig.timingMode = cfg.timingMode;
    entityConfig.timeIntervalMs = cfg.timeIntervalMs;
    entityConfig.noteInterval = engineToEntityNoteInterval(cfg.noteInterval);
    entityConfig.hostSyncEnabled = cfg.hostSyncEnabled;
    entityConfig.hostBPM = static_cast<float>(hostInfo.bpm);
    entityConfig.syncToPlayhead = cfg.syncToPlayhead;

    convertTriggerMode(cfg, entityConfig);

    entityConfig.triggerThreshold = 20.0f * std::log10(juce::jmax(0.0001f, cfg.triggerThreshold));
    entityConfig.midiTriggerNote = cfg.midiTriggerNote;
    entityConfig.midiTriggerChannel = cfg.midiTriggerChannel;
    entityConfig.actualIntervalMs = atomicActualIntervalMs_.load(std::memory_order_relaxed);

    return entityConfig;
}

} // namespace oscil
