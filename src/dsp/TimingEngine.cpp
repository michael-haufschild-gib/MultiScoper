/*
    Oscil - Timing Engine Implementation
    Aligned with PRD TimingConfig specification
*/

#include "dsp/TimingEngine.h"
#include <cmath>

namespace oscil
{

TimingEngine::TimingEngine()
{
    recalculateInterval();
}

TimingEngine::~TimingEngine()
{
    // Mark as invalid to prevent pending async callbacks from executing
    isValid_->store(false);
}

void TimingEngine::updateHostInfo(const juce::AudioPlayHead::PositionInfo& positionInfo)
{
    if (auto bpm = positionInfo.getBpm())
    {
        float newBPM = static_cast<float>(*bpm);
        newBPM = juce::jlimit(EngineTimingConfig::MIN_BPM, EngineTimingConfig::MAX_BPM, newBPM);

        if (std::abs(hostInfo_.bpm - newBPM) > 0.01)
        {
            hostInfo_.bpm = newBPM;
            config_.hostBPM = newBPM;

            // Recalculate interval when BPM changes in MELODIC mode
            if (config_.timingMode == TimingMode::MELODIC)
            {
                recalculateInterval();
            }

            // Notify if BPM changed significantly
            if (std::abs(previousBPM_ - newBPM) > 0.5f)
            {
                previousBPM_ = newBPM;
                notifyHostBPMChanged();
            }
        }
    }

    if (auto ppq = positionInfo.getPpqPosition())
        hostInfo_.ppqPosition = *ppq;

    bool wasPlaying = hostInfo_.isPlaying;
    hostInfo_.isPlaying = positionInfo.getIsPlaying();

    // Update transport state
    if (hostInfo_.isPlaying)
    {
        if (positionInfo.getIsRecording())
            hostInfo_.transportState = HostTimingInfo::TransportState::RECORDING;
        else
            hostInfo_.transportState = HostTimingInfo::TransportState::PLAYING;
    }
    else
    {
        hostInfo_.transportState = HostTimingInfo::TransportState::STOPPED;
    }

    // Handle transport state changes for host sync
    if (config_.hostSyncEnabled && wasPlaying != hostInfo_.isPlaying)
    {
        // Reset sync timestamp on play/stop transition
        if (auto timeInSamples = positionInfo.getTimeInSamples())
        {
            config_.lastSyncTimestamp = static_cast<double>(*timeInSamples);
        }
    }

    if (auto timeInSamples = positionInfo.getTimeInSamples())
        hostInfo_.timeInSamples = *timeInSamples;

    if (auto timeSig = positionInfo.getTimeSignature())
    {
        hostInfo_.timeSigNumerator = timeSig->numerator;
        hostInfo_.timeSigDenominator = timeSig->denominator;
    }
}

bool TimingEngine::processBlock(const juce::AudioBuffer<float>& buffer)
{
    if (config_.triggerMode == WaveformTriggerMode::None)
        return true;

    if (config_.triggerMode == WaveformTriggerMode::Manual)
        return checkAndClearManualTrigger();

    int channel = std::min(config_.triggerChannel, buffer.getNumChannels() - 1);
    if (channel < 0)
        return false;

    const float* samples = buffer.getReadPointer(channel);
    return detectTrigger(samples, buffer.getNumSamples());
}

int TimingEngine::getDisplaySampleCount(double sampleRate) const
{
    double windowSeconds = getWindowSizeSeconds();
    return static_cast<int>(windowSeconds * sampleRate);
}

float TimingEngine::getActualIntervalMs() const
{
    return config_.actualIntervalMs;
}

double TimingEngine::getWindowSizeSeconds() const
{
    return config_.actualIntervalMs / 1000.0;
}

void TimingEngine::recalculateInterval()
{
    float newInterval = 0.0f;

    switch (config_.timingMode)
    {
        case TimingMode::TIME:
            newInterval = static_cast<float>(config_.timeIntervalMs);
            break;

        case TimingMode::MELODIC:
        {
            // Calculate interval from BPM and note interval
            // Formula: interval_ms = (beats * 60000) / BPM
            // Use double throughout for precision, then cast to float at the end
            double beats = engineNoteIntervalToBeats(config_.noteInterval);
            // Use host BPM when synced, internal BPM when free-running
            float effectiveBPM = config_.hostSyncEnabled ? config_.hostBPM : config_.internalBPM;
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

    if (std::abs(config_.actualIntervalMs - newInterval) > 0.1f)
    {
        config_.actualIntervalMs = newInterval;
        notifyIntervalChanged();
    }
}

void TimingEngine::setConfig(const EngineTimingConfig& config)
{
    config_ = config;
    recalculateInterval();
}

void TimingEngine::setTimingMode(TimingMode mode)
{
    if (config_.timingMode != mode)
    {
        config_.timingMode = mode;
        recalculateInterval();
        notifyTimingModeChanged();
    }
}

void TimingEngine::setHostSyncEnabled(bool enabled)
{
    if (config_.hostSyncEnabled != enabled)
    {
        config_.hostSyncEnabled = enabled;
        // Recalculate interval since BPM source changed (host vs internal)
        if (config_.timingMode == TimingMode::MELODIC)
        {
            recalculateInterval();
        }
        notifyHostSyncStateChanged();
    }
}

void TimingEngine::setInternalBPM(float bpm)
{
    bpm = juce::jlimit(EngineTimingConfig::MIN_BPM, EngineTimingConfig::MAX_BPM, bpm);

    if (std::abs(config_.internalBPM - bpm) > 0.01f)
    {
        config_.internalBPM = bpm;
        // Only recalculate if we're in MELODIC mode and NOT synced to host
        if (config_.timingMode == TimingMode::MELODIC && !config_.hostSyncEnabled)
        {
            recalculateInterval();
        }
    }
}

void TimingEngine::setTimeIntervalMs(int ms)
{
    ms = juce::jlimit(EngineTimingConfig::MIN_TIME_INTERVAL_MS,
                      EngineTimingConfig::MAX_TIME_INTERVAL_MS, ms);

    if (config_.timeIntervalMs != ms)
    {
        config_.timeIntervalMs = ms;
        if (config_.timingMode == TimingMode::TIME)
        {
            recalculateInterval();
        }
    }
}

void TimingEngine::setNoteInterval(EngineNoteInterval interval)
{
    if (config_.noteInterval != interval)
    {
        config_.noteInterval = interval;
        if (config_.timingMode == TimingMode::MELODIC)
        {
            recalculateInterval();
        }
    }
}

bool TimingEngine::checkAndClearManualTrigger()
{
    return manualTrigger_.exchange(false);
}

void TimingEngine::requestManualTrigger()
{
    manualTrigger_.store(true);
}

bool TimingEngine::detectTrigger(const float* samples, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = samples[i];
        bool triggered = false;

        switch (config_.triggerMode)
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
    bool wasBelow = previousSample_ < (config_.triggerThreshold - config_.triggerHysteresis);
    bool isAbove = sample >= config_.triggerThreshold;

    return wasBelow && isAbove;
}

bool TimingEngine::detectFallingEdge(float sample)
{
    bool wasAbove = previousSample_ > (config_.triggerThreshold + config_.triggerHysteresis);
    bool isBelow = sample <= config_.triggerThreshold;

    return wasAbove && isBelow;
}

bool TimingEngine::detectLevel(float sample)
{
    float absLevel = std::abs(sample);
    bool wasBelow = !previousTriggerState_;
    bool isAbove = absLevel > config_.triggerThreshold;

    if (wasBelow && isAbove)
    {
        previousTriggerState_ = true;
        return true;
    }
    else if (!isAbove && absLevel < (config_.triggerThreshold - config_.triggerHysteresis))
    {
        previousTriggerState_ = false;
    }

    return false;
}

juce::ValueTree TimingEngine::toValueTree() const
{
    juce::ValueTree state(TimingIds::Timing);

    state.setProperty(TimingIds::TimingMode, static_cast<int>(config_.timingMode), nullptr);
    state.setProperty(TimingIds::HostSyncEnabled, config_.hostSyncEnabled, nullptr);
    state.setProperty(TimingIds::SyncToPlayhead, config_.syncToPlayhead, nullptr);
    state.setProperty(TimingIds::TimeIntervalMs, config_.timeIntervalMs, nullptr);
    state.setProperty(TimingIds::NoteInterval, static_cast<int>(config_.noteInterval), nullptr);
    state.setProperty(TimingIds::TriggerMode, static_cast<int>(config_.triggerMode), nullptr);
    state.setProperty(TimingIds::TriggerThreshold, config_.triggerThreshold, nullptr);

    return state;
}

void TimingEngine::fromValueTree(const juce::ValueTree& state)
{
    if (!state.hasType(TimingIds::Timing))
    {
        return;
    }

    config_.timingMode = static_cast<TimingMode>(
        static_cast<int>(state.getProperty(TimingIds::TimingMode, 0)));
    config_.hostSyncEnabled = state.getProperty(TimingIds::HostSyncEnabled, false);
    config_.syncToPlayhead = state.getProperty(TimingIds::SyncToPlayhead, false);
    config_.timeIntervalMs = state.getProperty(TimingIds::TimeIntervalMs, 1000);
    config_.noteInterval = static_cast<EngineNoteInterval>(
        static_cast<int>(state.getProperty(TimingIds::NoteInterval, 4))); // Default NOTE_1_4TH
    config_.triggerMode = static_cast<WaveformTriggerMode>(
        static_cast<int>(state.getProperty(TimingIds::TriggerMode, 0)));
    config_.triggerThreshold = state.getProperty(TimingIds::TriggerThreshold, 0.1f);

    // Validate constraints
    config_.timeIntervalMs = juce::jlimit(
        EngineTimingConfig::MIN_TIME_INTERVAL_MS,
        EngineTimingConfig::MAX_TIME_INTERVAL_MS,
        config_.timeIntervalMs);

    recalculateInterval();
}

// === Entity Integration Methods ===

void TimingEngine::applyEntityConfig(const TimingConfig& entityConfig)
{
    // Apply timing mode (same enum, direct assignment)
    config_.timingMode = entityConfig.timingMode;

    // Apply time interval
    config_.timeIntervalMs = static_cast<int>(entityConfig.timeIntervalMs);

    // Convert entity NoteInterval to engine EngineNoteInterval
    config_.noteInterval = entityToEngineNoteInterval(entityConfig.noteInterval);

    // Apply host settings
    config_.hostSyncEnabled = entityConfig.hostSyncEnabled;
    config_.hostBPM = entityConfig.hostBPM;
    config_.syncToPlayhead = entityConfig.syncToPlayhead;

    // Apply trigger settings
    // Entity TriggerMode maps to sync mode, not waveform trigger
    // Convert entity trigger mode to waveform trigger if needed
    if (entityConfig.triggerMode == TriggerMode::TRIGGERED)
    {
        // Convert TriggerEdge to WaveformTriggerMode
        switch (entityConfig.triggerEdge)
        {
            case TriggerEdge::Rising:
                config_.triggerMode = WaveformTriggerMode::RisingEdge;
                break;
            case TriggerEdge::Falling:
                config_.triggerMode = WaveformTriggerMode::FallingEdge;
                break;
            case TriggerEdge::Both:
                // "Both" triggers on any crossing - use Level mode for now
                config_.triggerMode = WaveformTriggerMode::Level;
                break;
        }
    }
    else
    {
        config_.triggerMode = WaveformTriggerMode::None;
    }

    // Apply trigger threshold (convert dBFS to linear if needed)
    // Entity uses dBFS (-20.0f default), engine uses linear (0.0-1.0)
    // Convert: linear = 10^(dBFS/20)
    float thresholdLinear = std::pow(10.0f, entityConfig.triggerThreshold / 20.0f);
    config_.triggerThreshold = juce::jlimit(0.0f, 1.0f, thresholdLinear);

    recalculateInterval();
}

TimingConfig TimingEngine::toEntityConfig() const
{
    TimingConfig entityConfig;

    // Copy timing mode (same enum)
    entityConfig.timingMode = config_.timingMode;

    // Copy time interval
    entityConfig.timeIntervalMs = static_cast<float>(config_.timeIntervalMs);

    // Convert engine EngineNoteInterval to entity NoteInterval
    entityConfig.noteInterval = engineToEntityNoteInterval(config_.noteInterval);

    // Copy host settings
    entityConfig.hostSyncEnabled = config_.hostSyncEnabled;
    entityConfig.hostBPM = config_.hostBPM;
    entityConfig.syncToPlayhead = config_.syncToPlayhead;

    // Convert waveform trigger mode to entity trigger mode
    if (config_.triggerMode != WaveformTriggerMode::None &&
        config_.triggerMode != WaveformTriggerMode::Manual)
    {
        entityConfig.triggerMode = TriggerMode::TRIGGERED;
        // Convert WaveformTriggerMode to TriggerEdge
        switch (config_.triggerMode)
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
            case WaveformTriggerMode::Manual:
                // Already filtered above, but handle explicitly for enum completeness
                entityConfig.triggerEdge = TriggerEdge::Rising;
                break;
        }
    }
    else
    {
        entityConfig.triggerMode = TriggerMode::FREE_RUNNING;
    }

    // Convert linear threshold to dBFS
    // dBFS = 20 * log10(linear)
    float thresholdDbfs = 20.0f * std::log10(juce::jmax(0.0001f, config_.triggerThreshold));
    entityConfig.triggerThreshold = thresholdDbfs;

    // Copy actual interval
    entityConfig.actualIntervalMs = config_.actualIntervalMs;

    return entityConfig;
}

void TimingEngine::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void TimingEngine::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

void TimingEngine::notifyTimingModeChanged()
{
    // Capture values and validity flag for async callback safety
    auto mode = config_.timingMode;
    auto validFlag = isValid_;

    juce::MessageManager::callAsync([this, mode, validFlag]()
    {
        if (validFlag->load())
        {
            listeners_.call([mode](Listener& l) { l.timingModeChanged(mode); });
        }
    });
}

void TimingEngine::notifyIntervalChanged()
{
    // Capture values and validity flag for async callback safety
    auto interval = config_.actualIntervalMs;
    auto validFlag = isValid_;

    juce::MessageManager::callAsync([this, interval, validFlag]()
    {
        if (validFlag->load())
        {
            listeners_.call([interval](Listener& l) { l.intervalChanged(interval); });
        }
    });
}

void TimingEngine::notifyHostBPMChanged()
{
    // Capture values and validity flag for async callback safety
    auto bpm = config_.hostBPM;
    auto validFlag = isValid_;

    juce::MessageManager::callAsync([this, bpm, validFlag]()
    {
        if (validFlag->load())
        {
            listeners_.call([bpm](Listener& l) { l.hostBPMChanged(bpm); });
        }
    });
}

void TimingEngine::notifyHostSyncStateChanged()
{
    // Capture values and validity flag for async callback safety
    auto enabled = config_.hostSyncEnabled;
    auto validFlag = isValid_;

    juce::MessageManager::callAsync([this, enabled, validFlag]()
    {
        if (validFlag->load())
        {
            listeners_.call([enabled](Listener& l) { l.hostSyncStateChanged(enabled); });
        }
    });
}

} // namespace oscil
