/*
    Oscil - Analysis Types
    Data structures for statistical analysis
*/

#pragma once

#include <juce_core/juce_core.h>

#include <atomic>

namespace oscil
{

struct ChannelMetrics
{
    std::atomic<float> rmsDb{-100.0f};
    std::atomic<float> peakDb{-100.0f};
    std::atomic<float> crestFactorDb{0.0f};
    std::atomic<float> dcOffset{0.0f};
    std::atomic<float> attackTimeMs{0.0f};
    std::atomic<float> decayTimeMs{0.0f};

    // Max held values
    std::atomic<float> maxPeakDb{-100.0f};

    void reset()
    {
        rmsDb = -100.0f;
        peakDb = -100.0f;
        crestFactorDb = 0.0f;
        dcOffset = 0.0f;
        attackTimeMs = 0.0f;
        decayTimeMs = 0.0f;
        maxPeakDb = -100.0f;
    }
};

struct AnalysisMetrics
{
    ChannelMetrics left;
    ChannelMetrics right;
    ChannelMetrics mid;
    ChannelMetrics side;

    void reset()
    {
        left.reset();
        right.reset();
        mid.reset();
        side.reset();
    }
};

} // namespace oscil
