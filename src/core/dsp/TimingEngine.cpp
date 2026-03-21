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
    double currentHostBPM = atomicHostBPM_.load(std::memory_order_relaxed);
    if (std::abs(currentHostBPM - newBPM) <= 0.01)
        return;

    atomicHostBPM_.store(static_cast<float>(newBPM), std::memory_order_relaxed);

    auto timingMode = static_cast<TimingMode>(atomicTimingMode_.load(std::memory_order_relaxed));
    if (timingMode == TimingMode::MELODIC)
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
    bool hostSyncEnabled = atomicHostSyncEnabled_.load(std::memory_order_relaxed);
    bool syncToPlayhead = atomicSyncToPlayhead_.load(std::memory_order_relaxed);

    if ((hostSyncEnabled || syncToPlayhead) && playStateChanged)
    {
        double syncTimestamp = atomicTimeInSamples_.load(std::memory_order_relaxed);
        if (timeInSamples)
            syncTimestamp = static_cast<double>(*timeInSamples);
        atomicLastSyncTimestamp_.store(syncTimestamp, std::memory_order_relaxed);
    }

    if (syncToPlayhead && !wasPlaying && isPlaying)
        triggered_.store(true, std::memory_order_relaxed);

    // Detect backward playhead jumps (loop wrap or seek) while playing
    if (syncToPlayhead && isPlaying && !playStateChanged && timeInSamples)
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

    int oldNum = atomicTimeSigNumerator_.load(std::memory_order_relaxed);
    int oldDen = atomicTimeSigDenominator_.load(std::memory_order_relaxed);

    if (timeSig->numerator != oldNum || timeSig->denominator != oldDen)
    {
        atomicTimeSigNumerator_.store(timeSig->numerator, std::memory_order_relaxed);
        atomicTimeSigDenominator_.store(timeSig->denominator, std::memory_order_relaxed);
        pendingTimeSignatureChange_.store(true, std::memory_order_relaxed);
        recalculateInterval();
    }
}

void TimingEngine::updateHostInfo(const juce::AudioPlayHead::PositionInfo& positionInfo)
{
    const int64_t previousTimeInSamples = atomicTimeInSamples_.load(std::memory_order_relaxed);
    auto timeInSamples = positionInfo.getTimeInSamples();
    if (timeInSamples && *timeInSamples < 0)
        timeInSamples.reset();

    updateHostBPM(positionInfo);

    if (auto ppq = positionInfo.getPpqPosition())
        atomicPpqPosition_.store(*ppq, std::memory_order_relaxed);

    bool wasPlaying = atomicIsPlaying_.load(std::memory_order_relaxed);
    bool isPlaying = positionInfo.getIsPlaying();
    atomicIsPlaying_.store(isPlaying, std::memory_order_relaxed);

    HostTimingInfo::TransportState transportState = HostTimingInfo::TransportState::STOPPED;
    if (isPlaying)
        transportState = positionInfo.getIsRecording()
            ? HostTimingInfo::TransportState::RECORDING
            : HostTimingInfo::TransportState::PLAYING;
    atomicTransportState_.store(static_cast<int>(transportState), std::memory_order_relaxed);

    updateSyncState(wasPlaying, isPlaying, timeInSamples, previousTimeInSamples);

    if (timeInSamples)
        atomicTimeInSamples_.store(*timeInSamples, std::memory_order_relaxed);

    updateHostTimeSignature(positionInfo);
}

bool TimingEngine::processBlock(const juce::AudioBuffer<float>& buffer)
{
    // Protect against denormal numbers which can cause 100x slowdown on some CPUs
    juce::ScopedNoDenormals noDenormals;

    // Apply deferred history reset on the audio thread.
    if (resetTriggerHistoryPending_.exchange(false, std::memory_order_relaxed))
    {
        previousTriggerState_ = false;
        previousSample_ = 0.0f;
    }

    // Read from atomics for thread-safe access (UI thread may modify via setters)
    auto triggerMode = static_cast<WaveformTriggerMode>(
        atomicTriggerMode_.load(std::memory_order_relaxed));

    if (triggerMode == WaveformTriggerMode::None)
        return true;

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
        atomicLastSyncTimestamp_.store(
            static_cast<double>(atomicTimeInSamples_.load(std::memory_order_relaxed)),
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
            atomicLastSyncTimestamp_.store(
                static_cast<double>(atomicTimeInSamples_.load(std::memory_order_relaxed)),
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
    atomicLastSyncTimestamp_.store(
        static_cast<double>(atomicTimeInSamples_.load(std::memory_order_relaxed)),
        std::memory_order_relaxed);
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

            case WaveformTriggerMode::BothEdges:
                triggered = detectBothEdges(sample);
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

bool TimingEngine::detectBothEdges(float sample)
{
    float threshold = atomicTriggerThreshold_.load(std::memory_order_relaxed);
    float hysteresis = atomicTriggerHysteresis_.load(std::memory_order_relaxed);

    // Rising edge: was below (threshold - hysteresis), now at or above threshold
    bool risingEdge = previousSample_ < (threshold - hysteresis) && sample >= threshold;
    // Falling edge: was above (threshold + hysteresis), now at or below threshold
    bool fallingEdge = previousSample_ > (threshold + hysteresis) && sample <= threshold;

    return risingEdge || fallingEdge;
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
    if (pendingTimingModeChange_.exchange(false, std::memory_order_relaxed))
    {
        auto mode = static_cast<TimingMode>(atomicTimingMode_.load(std::memory_order_relaxed));
        listeners_.call([mode](Listener& l) { l.timingModeChanged(mode); });
    }

    if (pendingIntervalChange_.exchange(false, std::memory_order_relaxed))
    {
        float interval = atomicActualIntervalMs_.load(std::memory_order_relaxed);
        listeners_.call([interval](Listener& l) { l.intervalChanged(interval); });
    }

    if (pendingHostBPMChange_.exchange(false, std::memory_order_relaxed))
    {
        float bpm = atomicHostBPM_.load(std::memory_order_relaxed);
        listeners_.call([bpm](Listener& l) { l.hostBPMChanged(bpm); });
    }

    if (pendingHostSyncChange_.exchange(false, std::memory_order_relaxed))
    {
        bool enabled = atomicHostSyncEnabled_.load(std::memory_order_relaxed);
        listeners_.call([enabled](Listener& l) { l.hostSyncStateChanged(enabled); });
    }

    if (pendingTimeSignatureChange_.exchange(false, std::memory_order_relaxed))
    {
        int numerator = atomicTimeSigNumerator_.load(std::memory_order_relaxed);
        int denominator = atomicTimeSigDenominator_.load(std::memory_order_relaxed);
        listeners_.call([numerator, denominator](Listener& l) { l.timeSignatureChanged(numerator, denominator); });
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
