/*
    Oscil - Trigger Detector
    Handles waveform trigger detection logic
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "core/dsp/EngineTypes.h"
#include <atomic>

namespace oscil
{

class TriggerDetector
{
public:
    TriggerDetector() = default;

    struct Config
    {
        WaveformTriggerMode mode = WaveformTriggerMode::None;
        float threshold = 0.1f;
        float hysteresis = 0.01f;
    };

    bool process(const float* samples, int numSamples, const Config& config);

    // Thread-safe reset - can be called from UI thread while audio thread
    // is processing. Uses relaxed ordering since we don't need synchronization
    // with other memory operations.
    void reset()
    {
        previousTriggerState_.store(false, std::memory_order_relaxed);
    }

private:
    bool detectRisingEdge(float sample, const Config& config);
    bool detectFallingEdge(float sample, const Config& config);
    bool detectLevel(float sample, const Config& config);

    // Atomic to allow safe reset() from UI thread while audio thread is processing.
    // Audio thread reads/writes during process(), UI thread writes during reset().
    std::atomic<bool> previousTriggerState_{false};
};

} // namespace oscil
