/*
    Oscil - Trigger Detector Implementation
*/

#include "core/dsp/TriggerDetector.h"
#include <cmath>

namespace oscil
{

bool TriggerDetector::process(const float* samples, int numSamples, const Config& config)
{
    if (config.mode == WaveformTriggerMode::None ||
        config.mode == WaveformTriggerMode::Manual ||
        config.mode == WaveformTriggerMode::Midi)
    {
        return false;
    }

    // Optimization: Lift switch out of the loop to avoid per-sample branching
    switch (config.mode)
    {
        case WaveformTriggerMode::RisingEdge:
            for (int i = 0; i < numSamples; ++i)
            {
                float sample = samples[i];
                if (!std::isfinite(sample)) continue;
                
                if (detectRisingEdge(sample, config))
                    return true;
            }
            break;

        case WaveformTriggerMode::FallingEdge:
            for (int i = 0; i < numSamples; ++i)
            {
                float sample = samples[i];
                if (!std::isfinite(sample)) continue;
                
                if (detectFallingEdge(sample, config))
                    return true;
            }
            break;

        case WaveformTriggerMode::Level:
            for (int i = 0; i < numSamples; ++i)
            {
                float sample = samples[i];
                if (!std::isfinite(sample)) continue;
                
                if (detectLevel(sample, config))
                    return true;
            }
            break;

        default:
            break;
    }

    return false;
}

bool TriggerDetector::detectRisingEdge(float sample, const Config& config)
{
    // Stateful hysteresis
    // Armed (false) -> Triggered (true) when crossing threshold
    // Triggered (true) -> Armed (false) when dropping below threshold - hysteresis

    bool prevState = previousTriggerState_.load(std::memory_order_relaxed);

    if (prevState) // Already triggered, waiting to arm
    {
        if (sample < (config.threshold - config.hysteresis))
        {
            previousTriggerState_.store(false, std::memory_order_relaxed); // Arm
        }
        return false;
    }
    else // Armed, waiting for trigger
    {
        if (sample >= config.threshold)
        {
            previousTriggerState_.store(true, std::memory_order_relaxed);
            return true;
        }
        return false;
    }
}

bool TriggerDetector::detectFallingEdge(float sample, const Config& config)
{
    // Stateful hysteresis
    // Armed (false) -> Triggered (true) when dropping below threshold
    // Triggered (true) -> Armed (false) when rising above threshold + hysteresis

    bool prevState = previousTriggerState_.load(std::memory_order_relaxed);

    if (prevState) // Already triggered, waiting to arm
    {
        if (sample > (config.threshold + config.hysteresis))
        {
            previousTriggerState_.store(false, std::memory_order_relaxed); // Arm
        }
        return false;
    }
    else // Armed, waiting for trigger
    {
        if (sample <= config.threshold)
        {
            previousTriggerState_.store(true, std::memory_order_relaxed);
            return true;
        }
        return false;
    }
}

bool TriggerDetector::detectLevel(float sample, const Config& config)
{
    float absLevel = std::abs(sample);
    bool isAbove = absLevel > config.threshold;

    // Note: detectLevel uses previousTriggerState_ to latch the trigger
    // until it drops below threshold - hysteresis

    bool prevState = previousTriggerState_.load(std::memory_order_relaxed);

    if (prevState) // Already triggered
    {
        if (absLevel < (config.threshold - config.hysteresis))
        {
            previousTriggerState_.store(false, std::memory_order_relaxed);
        }
        return false; // Already triggered in previous samples
    }
    else // Armed
    {
        if (isAbove)
        {
            previousTriggerState_.store(true, std::memory_order_relaxed);
            return true;
        }
        return false;
    }
}

} // namespace oscil