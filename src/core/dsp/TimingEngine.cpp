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

void TimingEngine::updateHostBPM(const juce::AudioPlayHead::PositionInfo& positionInfo)
{
    auto bpm = positionInfo.getBpm();
    if (!bpm) return;

    double newBPM = juce::jlimit<double>(EngineTimingConfig::MIN_BPM, EngineTimingConfig::MAX_BPM, *bpm);
    if (std::abs(audioThreadHostInfo_.bpm - newBPM) <= 0.01)
        return;

    audioThreadHostInfo_.bpm = newBPM;
    // Publish immediately so recalculateInterval() sees updated BPM
    hostInfoLock_.write(audioThreadHostInfo_);

    auto cfg = configLock_.read();
    if (cfg.timingMode == TimingMode::MELODIC)
        recalculateInterval();

    if (std::abs(previousBPM_ - static_cast<float>(newBPM)) > 0.5f)
    {
        previousBPM_ = static_cast<float>(newBPM);
        notifyHostBPMChanged();
    }
}

void TimingEngine::updateSyncState(bool wasPlaying, bool isPlaying,
                                    const juce::Optional<int64_t>& timeInSamples,
                                    int64_t previousTimeInSamples)
{
    bool playStateChanged = (wasPlaying != isPlaying);
    auto cfg = configLock_.read();

    if ((cfg.hostSyncEnabled || cfg.syncToPlayhead) && playStateChanged)
    {
        double syncTimestamp = static_cast<double>(audioThreadHostInfo_.timeInSamples);
        if (timeInSamples)
            syncTimestamp = static_cast<double>(*timeInSamples);
        atomicLastSyncTimestamp_.store(syncTimestamp, std::memory_order_relaxed);
    }

    if (cfg.syncToPlayhead && !wasPlaying && isPlaying)
        triggered_.store(true, std::memory_order_relaxed);

    // Detect backward playhead jumps (loop wrap or seek) while playing
    if (cfg.syncToPlayhead && isPlaying && !playStateChanged && timeInSamples)
    {
        constexpr int64_t kBackwardJumpRetriggerMinSamples = 3;
        if (previousTimeInSamples - *timeInSamples >= kBackwardJumpRetriggerMinSamples)
        {
            atomicLastSyncTimestamp_.store(static_cast<double>(*timeInSamples), std::memory_order_relaxed);
            triggered_.store(true, std::memory_order_relaxed);
        }
    }
}

void TimingEngine::updateHostTimeSignature(const juce::AudioPlayHead::PositionInfo& positionInfo)
{
    auto timeSig = positionInfo.getTimeSignature();
    if (!timeSig) return;

    if (timeSig->numerator != audioThreadHostInfo_.timeSigNumerator ||
        timeSig->denominator != audioThreadHostInfo_.timeSigDenominator)
    {
        audioThreadHostInfo_.timeSigNumerator = timeSig->numerator;
        audioThreadHostInfo_.timeSigDenominator = timeSig->denominator;
        // Publish before recalculate so it reads the updated time signature
        hostInfoLock_.write(audioThreadHostInfo_);
        pendingTimeSignatureChange_.store(true, std::memory_order_relaxed);
        recalculateInterval();
    }
}

void TimingEngine::updateHostInfo(const juce::AudioPlayHead::PositionInfo& positionInfo)
{
    const int64_t previousTimeInSamples = audioThreadHostInfo_.timeInSamples;
    auto timeInSamples = positionInfo.getTimeInSamples();
    if (timeInSamples && *timeInSamples < 0)
        timeInSamples.reset();

    updateHostBPM(positionInfo);

    if (auto ppq = positionInfo.getPpqPosition())
        audioThreadHostInfo_.ppqPosition = *ppq;

    bool wasPlaying = audioThreadHostInfo_.isPlaying;
    bool isPlaying = positionInfo.getIsPlaying();
    audioThreadHostInfo_.isPlaying = isPlaying;

    HostTimingInfo::TransportState transportState = HostTimingInfo::TransportState::STOPPED;
    if (isPlaying)
        transportState = positionInfo.getIsRecording()
            ? HostTimingInfo::TransportState::RECORDING
            : HostTimingInfo::TransportState::PLAYING;
    audioThreadHostInfo_.transportState = transportState;

    updateSyncState(wasPlaying, isPlaying, timeInSamples, previousTimeInSamples);

    if (timeInSamples)
        audioThreadHostInfo_.timeInSamples = *timeInSamples;

    updateHostTimeSignature(positionInfo);

    // Publish the complete host info snapshot for cross-thread readers
    hostInfoLock_.write(audioThreadHostInfo_);
}

bool TimingEngine::processBlock(const juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    if (resetTriggerHistoryPending_.exchange(false, std::memory_order_relaxed))
    {
        previousTriggerState_ = false;
        previousSample_ = 0.0f;
    }

    auto cfg = configLock_.read();

    if (cfg.triggerMode == WaveformTriggerMode::None)
        return true;

    if (cfg.triggerMode == WaveformTriggerMode::Manual)
        return manualTrigger_.load(std::memory_order_relaxed);

    if (cfg.triggerMode == WaveformTriggerMode::Midi)
        return triggered_.load(std::memory_order_relaxed);

    int channel = std::min(cfg.triggerChannel, buffer.getNumChannels() - 1);
    if (channel < 0)
        return false;

    const float* samples = buffer.getReadPointer(channel);
    if (detectTrigger(samples, buffer.getNumSamples()))
    {
        triggered_.store(true, std::memory_order_relaxed);
        atomicLastSyncTimestamp_.store(
            static_cast<double>(audioThreadHostInfo_.timeInSamples),
            std::memory_order_relaxed);
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
    return atomicActualIntervalMs_.load(std::memory_order_relaxed);
}

double TimingEngine::getWindowSizeSeconds() const
{
    return static_cast<double>(atomicActualIntervalMs_.load(std::memory_order_relaxed)) / 1000.0;
}

void TimingEngine::recalculateInterval()
{
    float newInterval = 0.0f;

    auto cfg = configLock_.read();
    auto hostInfo = hostInfoLock_.read();

    switch (cfg.timingMode)
    {
        case TimingMode::TIME:
            newInterval = cfg.timeIntervalMs;
            break;

        case TimingMode::MELODIC:
        {
            int timeSigNumerator = hostInfo.timeSigNumerator;
            double beats = engineNoteIntervalToBeats(cfg.noteInterval, timeSigNumerator);

            float effectiveBPM = cfg.hostSyncEnabled
                ? static_cast<float>(hostInfo.bpm)
                : cfg.internalBPM;

            if (!std::isfinite(effectiveBPM))
                effectiveBPM = EngineTimingConfig::MIN_BPM;

            double bpm = static_cast<double>(juce::jmax(effectiveBPM, EngineTimingConfig::MIN_BPM));
            newInterval = static_cast<float>((beats * 60000.0) / bpm);
            break;
        }
    }

    newInterval = juce::jlimit(
        static_cast<float>(EngineTimingConfig::MIN_TIME_INTERVAL_MS),
        static_cast<float>(EngineTimingConfig::MAX_TIME_INTERVAL_MS),
        newInterval
    );

    float oldInterval = atomicActualIntervalMs_.exchange(newInterval, std::memory_order_relaxed);

    if (std::abs(oldInterval - newInterval) > 0.1f)
        notifyIntervalChanged();
}

bool TimingEngine::processMidi(const juce::MidiBuffer& midiMessages)
{
    auto cfg = configLock_.read();

    if (cfg.triggerMode != WaveformTriggerMode::Midi)
        return false;

    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            if (cfg.midiTriggerChannel != 0 && msg.getChannel() != cfg.midiTriggerChannel)
                continue;

            if (cfg.midiTriggerNote != -1 && msg.getNoteNumber() != cfg.midiTriggerNote)
                continue;

            triggered_.store(true, std::memory_order_relaxed);
            atomicLastSyncTimestamp_.store(
                static_cast<double>(audioThreadHostInfo_.timeInSamples),
                std::memory_order_relaxed);
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
    if (manualTrigger_.exchange(false, std::memory_order_relaxed))
        return true;

    return triggered_.exchange(false, std::memory_order_relaxed);
}

void TimingEngine::requestManualTrigger()
{
    manualTrigger_.store(true, std::memory_order_relaxed);
    triggered_.store(true, std::memory_order_relaxed);
    auto hostInfo = hostInfoLock_.read();
    atomicLastSyncTimestamp_.store(
        static_cast<double>(hostInfo.timeInSamples),
        std::memory_order_relaxed);
}

void TimingEngine::setSampleRate(double sampleRate)
{
    auto info = hostInfoLock_.read();
    info.sampleRate = sampleRate;
    hostInfoLock_.write(info);
    // Also update the audio-thread mirror in case this is called from prepareToPlay
    audioThreadHostInfo_.sampleRate = sampleRate;
}

bool TimingEngine::detectTrigger(const float* samples, int numSamples)
{
    auto cfg = configLock_.read();

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = samples[i];
        bool trig = false;

        switch (cfg.triggerMode)
        {
            case WaveformTriggerMode::RisingEdge:
                trig = detectRisingEdge(sample);
                break;

            case WaveformTriggerMode::FallingEdge:
                trig = detectFallingEdge(sample);
                break;

            case WaveformTriggerMode::BothEdges:
                trig = detectBothEdges(sample);
                break;

            case WaveformTriggerMode::Level:
                trig = detectLevel(sample);
                break;

            case WaveformTriggerMode::None:
            case WaveformTriggerMode::Manual:
            case WaveformTriggerMode::Midi:
                break;
        }

        previousSample_ = sample;

        if (trig)
            return true;
    }

    return false;
}

bool TimingEngine::detectRisingEdge(float sample)
{
    auto cfg = configLock_.read();
    bool wasBelow = previousSample_ < (cfg.triggerThreshold - cfg.triggerHysteresis);
    bool isAbove = sample >= cfg.triggerThreshold;
    return wasBelow && isAbove;
}

bool TimingEngine::detectFallingEdge(float sample)
{
    auto cfg = configLock_.read();
    bool wasAbove = previousSample_ > (cfg.triggerThreshold + cfg.triggerHysteresis);
    bool isBelow = sample <= cfg.triggerThreshold;
    return wasAbove && isBelow;
}

bool TimingEngine::detectBothEdges(float sample)
{
    auto cfg = configLock_.read();
    bool risingEdge = previousSample_ < (cfg.triggerThreshold - cfg.triggerHysteresis) &&
                      sample >= cfg.triggerThreshold;
    bool fallingEdge = previousSample_ > (cfg.triggerThreshold + cfg.triggerHysteresis) &&
                       sample <= cfg.triggerThreshold;
    return risingEdge || fallingEdge;
}

bool TimingEngine::detectLevel(float sample)
{
    auto cfg = configLock_.read();
    float absLevel = std::abs(sample);
    bool wasBelow = !previousTriggerState_;
    bool isAbove = absLevel > cfg.triggerThreshold;

    if (wasBelow && isAbove)
    {
        previousTriggerState_ = true;
        return true;
    }
    else if (!isAbove && absLevel < (cfg.triggerThreshold - cfg.triggerHysteresis))
    {
        previousTriggerState_ = false;
    }

    return false;
}

void TimingEngine::addListener(Listener* listener)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    listeners_.add(listener);
}

void TimingEngine::removeListener(Listener* listener)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    listeners_.remove(listener);
}

void TimingEngine::dispatchPendingUpdates()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (pendingTimingModeChange_.exchange(false, std::memory_order_relaxed))
    {
        auto cfg = configLock_.read();
        listeners_.call([mode = cfg.timingMode](Listener& l) { l.timingModeChanged(mode); });
    }

    if (pendingIntervalChange_.exchange(false, std::memory_order_relaxed))
    {
        float interval = atomicActualIntervalMs_.load(std::memory_order_relaxed);
        listeners_.call([interval](Listener& l) { l.intervalChanged(interval); });
    }

    if (pendingHostBPMChange_.exchange(false, std::memory_order_relaxed))
    {
        auto hostInfo = hostInfoLock_.read();
        float bpm = static_cast<float>(hostInfo.bpm);
        listeners_.call([bpm](Listener& l) { l.hostBPMChanged(bpm); });
    }

    if (pendingHostSyncChange_.exchange(false, std::memory_order_relaxed))
    {
        auto cfg = configLock_.read();
        listeners_.call([enabled = cfg.hostSyncEnabled](Listener& l) { l.hostSyncStateChanged(enabled); });
    }

    if (pendingTimeSignatureChange_.exchange(false, std::memory_order_relaxed))
    {
        auto hostInfo = hostInfoLock_.read();
        listeners_.call([num = hostInfo.timeSigNumerator, den = hostInfo.timeSigDenominator](Listener& l) {
            l.timeSignatureChanged(num, den);
        });
    }
}

void TimingEngine::notifyTimingModeChanged()
{
    pendingTimingModeChange_.store(true, std::memory_order_relaxed);
}

void TimingEngine::notifyIntervalChanged()
{
    pendingIntervalChange_.store(true, std::memory_order_relaxed);
}

void TimingEngine::notifyHostBPMChanged()
{
    pendingHostBPMChange_.store(true, std::memory_order_relaxed);
}

void TimingEngine::notifyHostSyncStateChanged()
{
    pendingHostSyncChange_.store(true, std::memory_order_relaxed);
}

} // namespace oscil
