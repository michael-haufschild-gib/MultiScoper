/*
    Oscil - Timing Engine Implementation
    Aligned with PRD TimingConfig specification
*/

#include "core/dsp/TimingEngine.h"
#include <cmath>

namespace oscil
{

TimingEngine::TimingEngine()
{
    recalculateInterval();
}

TimingEngine::~TimingEngine() = default;

void TimingEngine::updateHostInfo(const juce::AudioPlayHead::PositionInfo& positionInfo)
{
    if (auto bpm = positionInfo.getBpm())
    {
        double newBPM = *bpm;
        // Clamp BPM to valid range
        newBPM = juce::jlimit<double>(EngineTimingConfig::MIN_BPM, EngineTimingConfig::MAX_BPM, newBPM);

        double currentHostBPM = atomicHostBPM_.load(std::memory_order_relaxed);
        if (std::abs(currentHostBPM - newBPM) > 0.01)
        {
            atomicHostBPM_.store(static_cast<float>(newBPM), std::memory_order_relaxed);

            // Recalculate interval when BPM changes in MELODIC mode
            auto timingMode = static_cast<TimingMode>(atomicTimingMode_.load(std::memory_order_relaxed));
            if (timingMode == TimingMode::MELODIC)
            {
                recalculateInterval();
            }

            // Notify if BPM changed significantly
            if (std::abs(previousBPM_ - static_cast<float>(newBPM)) > 0.5f)
            {
                previousBPM_ = static_cast<float>(newBPM);
                notifyHostBPMChanged();
            }
        }
    }

    if (auto ppq = positionInfo.getPpqPosition())
        atomicPpqPosition_.store(*ppq, std::memory_order_relaxed);

    bool wasPlaying = atomicIsPlaying_.load(std::memory_order_relaxed);
    bool isPlaying = positionInfo.getIsPlaying();
    atomicIsPlaying_.store(isPlaying, std::memory_order_relaxed);

    // Update transport state
    HostTimingInfo::TransportState transportState = HostTimingInfo::TransportState::STOPPED;
    if (isPlaying)
    {
        if (positionInfo.getIsRecording())
            transportState = HostTimingInfo::TransportState::RECORDING;
        else
            transportState = HostTimingInfo::TransportState::PLAYING;
    }
    atomicTransportState_.store(static_cast<int>(transportState), std::memory_order_relaxed);

    // Handle transport state changes for host sync
    if (atomicHostSyncEnabled_.load(std::memory_order_relaxed) && wasPlaying != isPlaying)
    {
        // Reset sync timestamp on play/stop transition
        if (auto timeInSamples = positionInfo.getTimeInSamples())
        {
            atomicLastSyncTimestamp_.store(static_cast<double>(*timeInSamples), std::memory_order_relaxed);
        }
    }

    if (auto timeInSamples = positionInfo.getTimeInSamples())
        atomicTimeInSamples_.store(*timeInSamples, std::memory_order_relaxed);

    if (auto timeSig = positionInfo.getTimeSignature())
    {
        int oldNumerator = atomicTimeSigNumerator_.load(std::memory_order_relaxed);
        int oldDenominator = atomicTimeSigDenominator_.load(std::memory_order_relaxed);

        if (timeSig->numerator != oldNumerator || timeSig->denominator != oldDenominator)
        {
            atomicTimeSigNumerator_.store(timeSig->numerator, std::memory_order_relaxed);
            atomicTimeSigDenominator_.store(timeSig->denominator, std::memory_order_relaxed);

            // Time signature changed - set pending flag for listener notification
            pendingTimeSignatureChange_.store(true, std::memory_order_relaxed);

            // Recalculate interval since bar-based intervals depend on time signature
            recalculateInterval();
        }
    }
}

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

bool TimingEngine::processBlock(const juce::AudioBuffer<float>& buffer)
{
    // Protect against denormal numbers which can cause 100x slowdown on some CPUs
    juce::ScopedNoDenormals noDenormals;

    // Read from atomics for thread-safe access (UI thread may modify via setters)
    auto triggerMode = static_cast<WaveformTriggerMode>(
        atomicTriggerMode_.load(std::memory_order_relaxed));

    if (triggerMode == WaveformTriggerMode::None)
        return true;  // Free-running mode: always trigger (render every frame)

    if (triggerMode == WaveformTriggerMode::Manual)
    {
        // checkAndClearManualTrigger is called by polling, not here
        return manualTrigger_.load(std::memory_order_relaxed);
    }

    // In MIDI mode, we don't check audio buffer for triggers
    if (triggerMode == WaveformTriggerMode::Midi)
        return triggered_.load(std::memory_order_relaxed);

    int channel = std::min(atomicTriggerChannel_.load(std::memory_order_relaxed),
                           buffer.getNumChannels() - 1);
    if (channel < 0)
        return false;

    const float* samples = buffer.getReadPointer(channel);
    if (detectTrigger(samples, buffer.getNumSamples()))
    {
        triggered_.store(true, std::memory_order_relaxed);
        return true;
    }

    return false;
}

int TimingEngine::getDisplaySampleCount(double sampleRate) const
{
    double windowSeconds = getWindowSizeSeconds();
    return static_cast<int>(windowSeconds * sampleRate);
}

float TimingEngine::getActualIntervalMs() const
{
    // Use atomic for thread-safe read from UI thread
    return atomicActualIntervalMs_.load(std::memory_order_relaxed);
}

double TimingEngine::getWindowSizeSeconds() const
{
    // Use atomic for thread-safe read from UI thread
    return static_cast<double>(atomicActualIntervalMs_.load(std::memory_order_relaxed)) / 1000.0;
}

void TimingEngine::recalculateInterval()
{
    float newInterval = 0.0f;

    // Read from atomics for thread safety
    auto timingMode = static_cast<TimingMode>(atomicTimingMode_.load(std::memory_order_relaxed));
    
    switch (timingMode)
    {
        case TimingMode::TIME:
            newInterval = atomicTimeIntervalMs_.load(std::memory_order_relaxed);
            break;

        case TimingMode::MELODIC:
        {
            // Calculate interval from BPM and note interval
            // Formula: interval_ms = (beats * 60000) / BPM
            auto noteInterval = static_cast<EngineNoteInterval>(atomicNoteInterval_.load(std::memory_order_relaxed));

            // Get time signature numerator for bar-based interval calculations
            // Bar-based intervals (1 Bar, 2 Bars, etc.) depend on time signature
            int timeSigNumerator = atomicTimeSigNumerator_.load(std::memory_order_relaxed);
            double beats = engineNoteIntervalToBeats(noteInterval, timeSigNumerator);

            // Use host BPM when synced, internal BPM when free-running
            bool hostSync = atomicHostSyncEnabled_.load(std::memory_order_relaxed);
            float effectiveBPM = hostSync ?
                atomicHostBPM_.load(std::memory_order_relaxed) :
                atomicInternalBPM_.load(std::memory_order_relaxed);

            // Guard against NaN/Inf - jmax doesn't handle NaN correctly
            if (!std::isfinite(effectiveBPM))
                effectiveBPM = EngineTimingConfig::MIN_BPM;

            double bpm = static_cast<double>(juce::jmax(effectiveBPM, EngineTimingConfig::MIN_BPM));
            newInterval = static_cast<float>((beats * 60000.0) / bpm);
            break;
        }
    }

    // Clamp to valid range
    newInterval = juce::jlimit(
        static_cast<float>(EngineTimingConfig::MIN_TIME_INTERVAL_MS),
        static_cast<float>(EngineTimingConfig::MAX_TIME_INTERVAL_MS),
        newInterval
    );

    // Store computed interval atomically and get previous value for change detection
    // This avoids reading the non-atomic config_.actualIntervalMs from multiple threads
    float oldInterval = atomicActualIntervalMs_.exchange(newInterval, std::memory_order_relaxed);
    
    // Notify if changed significantly
    if (std::abs(oldInterval - newInterval) > 0.1f)
    {
        notifyIntervalChanged();
    }
}

void TimingEngine::setConfig(const EngineTimingConfig& config)
{
    // Sync all atomics with new config values for thread-safe cross-thread reads
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
        // Recalculate interval since BPM source changed (host vs internal)
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
        // Only recalculate if we're in MELODIC mode and NOT synced to host
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

bool TimingEngine::processMidi(const juce::MidiBuffer& midiMessages)
{
    // Only process if in MIDI trigger mode
    auto triggerMode = static_cast<WaveformTriggerMode>(
        atomicTriggerMode_.load(std::memory_order_relaxed));

    if (triggerMode != WaveformTriggerMode::Midi)
        return false;

    int triggerNote = atomicMidiTriggerNote_.load(std::memory_order_relaxed);
    int triggerChannel = atomicMidiTriggerChannel_.load(std::memory_order_relaxed);

    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            // Check channel (0 = omni, or 1-16)
            if (triggerChannel != 0 && msg.getChannel() != triggerChannel)
                continue;

            // Check note (-1 = any, or 0-127)
            if (triggerNote != -1 && msg.getNoteNumber() != triggerNote)
                continue;

            // Trigger detected
            triggered_.store(true, std::memory_order_relaxed);
            return true;
        }
    }

    return false;
}

bool TimingEngine::checkAndClearManualTrigger()
{
    return manualTrigger_.exchange(false, std::memory_order_relaxed);
}

bool TimingEngine::checkAndClearTrigger()
{
    // Check manual trigger
    if (manualTrigger_.exchange(false, std::memory_order_relaxed))
        return true;

    // Check auto/midi trigger
    return triggered_.exchange(false, std::memory_order_relaxed);
}

void TimingEngine::requestManualTrigger()
{
    manualTrigger_.store(true, std::memory_order_relaxed);
    triggered_.store(true, std::memory_order_relaxed); // Set general flag too
}

bool TimingEngine::detectTrigger(const float* samples, int numSamples)
{
    // Read trigger mode from atomic once per block for consistency
    auto triggerMode = static_cast<WaveformTriggerMode>(
        atomicTriggerMode_.load(std::memory_order_relaxed));

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = samples[i];
        bool triggered = false;

        switch (triggerMode)
        {
            case WaveformTriggerMode::RisingEdge:
                triggered = detectRisingEdge(sample);
                break;

            case WaveformTriggerMode::FallingEdge:
                triggered = detectFallingEdge(sample);
                break;

            case WaveformTriggerMode::Level:
                triggered = detectLevel(sample);
                break;

            case WaveformTriggerMode::None:
            case WaveformTriggerMode::Manual:
            case WaveformTriggerMode::Midi:
                // These modes don't use sample-based detection
                break;
        }

        previousSample_ = sample;

        if (triggered)
            return true;
    }

    return false;
}

bool TimingEngine::detectRisingEdge(float sample)
{
    // Read from atomics for thread-safe access
    float threshold = atomicTriggerThreshold_.load(std::memory_order_relaxed);
    float hysteresis = atomicTriggerHysteresis_.load(std::memory_order_relaxed);

    bool wasBelow = previousSample_ < (threshold - hysteresis);
    bool isAbove = sample >= threshold;

    return wasBelow && isAbove;
}

bool TimingEngine::detectFallingEdge(float sample)
{
    // Read from atomics for thread-safe access
    float threshold = atomicTriggerThreshold_.load(std::memory_order_relaxed);
    float hysteresis = atomicTriggerHysteresis_.load(std::memory_order_relaxed);

    bool wasAbove = previousSample_ > (threshold + hysteresis);
    bool isBelow = sample <= threshold;

    return wasAbove && isBelow;
}

bool TimingEngine::detectLevel(float sample)
{
    // Read from atomics for thread-safe access
    float threshold = atomicTriggerThreshold_.load(std::memory_order_relaxed);
    float hysteresis = atomicTriggerHysteresis_.load(std::memory_order_relaxed);

    float absLevel = std::abs(sample);
    bool wasBelow = !previousTriggerState_;
    bool isAbove = absLevel > threshold;

    if (wasBelow && isAbove)
    {
        previousTriggerState_ = true;
        return true;
    }
    else if (!isAbove && absLevel < (threshold - hysteresis))
    {
        previousTriggerState_ = false;
    }

    return false;
}

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

void TimingEngine::fromValueTree(const juce::ValueTree& state)
{
    if (!state.hasType(TimingIds::Timing))
    {
        return;
    }

    atomicTimingMode_.store(static_cast<int>(state.getProperty(TimingIds::TimingMode, 0)), std::memory_order_relaxed);
    atomicHostSyncEnabled_.store(state.getProperty(TimingIds::HostSyncEnabled, false), std::memory_order_relaxed);
    atomicSyncToPlayhead_.store(state.getProperty(TimingIds::SyncToPlayhead, false), std::memory_order_relaxed);
    
    float timeInterval = static_cast<float>(state.getProperty(TimingIds::TimeIntervalMs, 500.0f));
    // Validate constraints (using float comparison)
    timeInterval = juce::jlimit(
        EngineTimingConfig::MIN_TIME_INTERVAL_MS,
        EngineTimingConfig::MAX_TIME_INTERVAL_MS,
        timeInterval);
    atomicTimeIntervalMs_.store(timeInterval, std::memory_order_relaxed);
        
    atomicNoteInterval_.store(static_cast<int>(state.getProperty(TimingIds::NoteInterval, 4)), std::memory_order_relaxed); // Default NOTE_1_4TH
    atomicTriggerMode_.store(static_cast<int>(state.getProperty(TimingIds::TriggerMode, 0)), std::memory_order_relaxed);
    atomicTriggerThreshold_.store(state.getProperty(TimingIds::TriggerThreshold, 0.1f), std::memory_order_relaxed);
    atomicMidiTriggerNote_.store(state.getProperty(TimingIds::MidiTriggerNote, -1), std::memory_order_relaxed);
    atomicMidiTriggerChannel_.store(state.getProperty(TimingIds::MidiTriggerChannel, 0), std::memory_order_relaxed);
    atomicInternalBPM_.store(state.getProperty(TimingIds::InternalBPM, 120.0f), std::memory_order_relaxed);
    atomicTriggerChannel_.store(state.getProperty(TimingIds::TriggerChannel, 0), std::memory_order_relaxed);
    atomicTriggerHysteresis_.store(state.getProperty(TimingIds::TriggerHysteresis, 0.01f), std::memory_order_relaxed);

    recalculateInterval();
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
                // "Both" triggers on any crossing - use Level mode for now
                newTriggerMode = WaveformTriggerMode::Level;
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

void TimingEngine::addListener(Listener* listener)
{
    // ListenerList::add() is NOT thread-safe. Must be called from message thread.
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    listeners_.add(listener);
}

void TimingEngine::removeListener(Listener* listener)
{
    // ListenerList::remove() is NOT thread-safe. Must be called from message thread.
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    listeners_.remove(listener);
}

void TimingEngine::dispatchPendingUpdates()
{
    // Ensure this is called on the message thread to avoid race conditions with UI listeners
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Check flags and notify listeners on the current thread (expected Message Thread)
    // Use atomic reads for thread-safe access to values that may be written from audio thread
    if (pendingTimingModeChange_.exchange(false, std::memory_order_acquire))
    {
        auto mode = static_cast<TimingMode>(atomicTimingMode_.load(std::memory_order_relaxed));
        listeners_.call([mode](Listener& l) { l.timingModeChanged(mode); });
    }

    if (pendingIntervalChange_.exchange(false, std::memory_order_acquire))
    {
        float interval = atomicActualIntervalMs_.load(std::memory_order_relaxed);
        listeners_.call([interval](Listener& l) { l.intervalChanged(interval); });
    }

    if (pendingHostBPMChange_.exchange(false, std::memory_order_acquire))
    {
        float bpm = atomicHostBPM_.load(std::memory_order_relaxed);
        listeners_.call([bpm](Listener& l) { l.hostBPMChanged(bpm); });
    }

    if (pendingHostSyncChange_.exchange(false, std::memory_order_acquire))
    {
        bool enabled = atomicHostSyncEnabled_.load(std::memory_order_relaxed);
        listeners_.call([enabled](Listener& l) { l.hostSyncStateChanged(enabled); });
    }

    if (pendingTimeSignatureChange_.exchange(false, std::memory_order_acquire))
    {
        int numerator = atomicTimeSigNumerator_.load(std::memory_order_relaxed);
        int denominator = atomicTimeSigDenominator_.load(std::memory_order_relaxed);
        listeners_.call([numerator, denominator](Listener& l) { l.timeSignatureChanged(numerator, denominator); });
    }
}

void TimingEngine::notifyTimingModeChanged()
{
    pendingTimingModeChange_.store(true, std::memory_order_release);
}

void TimingEngine::notifyIntervalChanged()
{
    pendingIntervalChange_.store(true, std::memory_order_release);
}

void TimingEngine::notifyHostBPMChanged()
{
    pendingHostBPMChange_.store(true, std::memory_order_release);
}

void TimingEngine::notifyHostSyncStateChanged()
{
    pendingHostSyncChange_.store(true, std::memory_order_release);
}

} // namespace oscil
