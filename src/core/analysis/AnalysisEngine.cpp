/*
    Oscil - Analysis Engine Implementation
*/

#include "core/analysis/AnalysisEngine.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <cmath>

namespace oscil
{

namespace
{
void clearChannelMetrics(ChannelMetrics& metrics, AnalysisChannelState& state, TransientDetector& transientDetector)
{
    state = {};
    transientDetector.reset();

    metrics.rmsDb.store(-100.0f, std::memory_order_relaxed);
    metrics.peakDb.store(-100.0f, std::memory_order_relaxed);
    metrics.crestFactorDb.store(0.0f, std::memory_order_relaxed);
    metrics.dcOffset.store(0.0f, std::memory_order_relaxed);
    metrics.attackTimeMs.store(0.0f, std::memory_order_relaxed);
    metrics.decayTimeMs.store(0.0f, std::memory_order_relaxed);
    metrics.maxPeakDb.store(-100.0f, std::memory_order_relaxed);
}
} // namespace

AnalysisEngine::AnalysisEngine()
{
    // Pre-allocate scratch buffers to avoid allocation in prepare()
    // which may be called from audio thread in some hosts
    midBuffer_.resize(kDefaultBufferSize);
    sideBuffer_.resize(kDefaultBufferSize);
    reset();
}

void AnalysisEngine::reset()
{
    metrics_.reset();
    leftTransient_.reset();
    rightTransient_.reset();
    midTransient_.reset();
    sideTransient_.reset();

    leftState_ = {};
    rightState_ = {};
    midState_ = {};
    sideState_ = {};
}

void AnalysisEngine::resetAccumulated()
{
    // Only reset max held values
    metrics_.left.maxPeakDb = -100.0f;
    metrics_.right.maxPeakDb = -100.0f;
    metrics_.mid.maxPeakDb = -100.0f;
    metrics_.side.maxPeakDb = -100.0f;

    // Reset transient detectors to recalculate attack/decay
    leftTransient_.reset();
    rightTransient_.reset();
    midTransient_.reset();
    sideTransient_.reset();

    metrics_.left.attackTimeMs = 0.0f;
    metrics_.left.decayTimeMs = 0.0f;
    metrics_.right.attackTimeMs = 0.0f;
    metrics_.right.decayTimeMs = 0.0f;
    metrics_.mid.attackTimeMs = 0.0f;
    metrics_.mid.decayTimeMs = 0.0f;
    metrics_.side.attackTimeMs = 0.0f;
    metrics_.side.decayTimeMs = 0.0f;
}

void AnalysisEngine::prepare(double /*sampleRate*/, int samplesPerBlock)
{
    // REAL-TIME SAFETY: This method may be called from audio thread in some hosts
    // (Pro Tools, Reaper). We NEVER allocate here - buffers are pre-allocated in
    // constructor to kDefaultBufferSize (8192) which exceeds all standard DAW block sizes.
    //
    // If samplesPerBlock > kDefaultBufferSize, process() will safely skip Mid/Side
    // analysis rather than risk audio dropouts from allocation.
    (void) samplesPerBlock; // Intentionally unused - allocation-free implementation
}

void AnalysisEngine::processMidSide(const float* left, const float* right, int numSamples, double sampleRate)
{
    auto const requiredSize = static_cast<size_t>(numSamples);
    if (midBuffer_.size() < requiredSize || sideBuffer_.size() < requiredSize)
    {
        jassertfalse;
        clearChannelMetrics(metrics_.mid, midState_, midTransient_);
        clearChannelMetrics(metrics_.side, sideState_, sideTransient_);
        return;
    }

    // M = (L+R)/2, S = (L-R)/2
    juce::FloatVectorOperations::copy(midBuffer_.data(), left, numSamples);
    juce::FloatVectorOperations::add(midBuffer_.data(), right, numSamples);
    juce::FloatVectorOperations::multiply(midBuffer_.data(), 0.5f, numSamples);

    juce::FloatVectorOperations::copy(sideBuffer_.data(), left, numSamples);
    juce::FloatVectorOperations::subtract(sideBuffer_.data(), right, numSamples);
    juce::FloatVectorOperations::multiply(sideBuffer_.data(), 0.5f, numSamples);

    processChannel(midBuffer_.data(), numSamples, metrics_.mid, midTransient_, midState_, sampleRate);
    processChannel(sideBuffer_.data(), numSamples, metrics_.side, sideTransient_, sideState_, sampleRate);
}

void AnalysisEngine::process(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    int const numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;
    const int numChannels = buffer.getNumChannels();

    if (numChannels <= 0)
    {
        clearChannelMetrics(metrics_.left, leftState_, leftTransient_);
        clearChannelMetrics(metrics_.right, rightState_, rightTransient_);
        clearChannelMetrics(metrics_.mid, midState_, midTransient_);
        clearChannelMetrics(metrics_.side, sideState_, sideTransient_);
        return;
    }

    processChannel(buffer.getReadPointer(0), numSamples, metrics_.left, leftTransient_, leftState_, sampleRate);

    if (numChannels > 1)
    {
        processChannel(buffer.getReadPointer(1), numSamples, metrics_.right, rightTransient_, rightState_, sampleRate);
        processMidSide(buffer.getReadPointer(0), buffer.getReadPointer(1), numSamples, sampleRate);
    }
    else
    {
        clearChannelMetrics(metrics_.right, rightState_, rightTransient_);
        clearChannelMetrics(metrics_.mid, midState_, midTransient_);
        clearChannelMetrics(metrics_.side, sideState_, sideTransient_);
    }
}

namespace
{

struct SampleStats
{
    float peak = 0.0f;
    float rms = 0.0f;
    float dc = 0.0f;
};

SampleStats computeSampleStats(const float* samples, int numSamples)
{
    float sumSq = 0.0f;
    float sum = 0.0f;
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float const s = std::isfinite(samples[i]) ? samples[i] : 0.0f;
        peak = juce::jmax(peak, std::abs(s));
        sumSq += s * s;
        sum += s;
    }
    auto const n = static_cast<float>(numSamples);
    return {.peak = peak, .rms = std::sqrt(sumSq / n), .dc = sum / n};
}

float safeToDb(float linear, float floor = -100.0f)
{
    float const db = (linear > 0.00001f) ? 20.0f * std::log10(linear) : floor;
    return std::isfinite(db) ? db : floor;
}

} // namespace

void AnalysisEngine::processChannel(const float* samples, int numSamples, ChannelMetrics& metrics,
                                    TransientDetector& transientDetector, AnalysisChannelState& state,
                                    double sampleRate) const
{
    if (numSamples <= 0)
        return;

    if (!std::isfinite(state.rmsSmooth))
        state.rmsSmooth = 0.0f;
    if (!std::isfinite(state.dcSmooth))
        state.dcSmooth = 0.0f;

    auto stats = computeSampleStats(samples, numSamples);

    state.rmsSmooth = (state.rmsSmooth * (1.0f - config_.rmsSmoothing)) + (stats.rms * config_.rmsSmoothing);
    state.dcSmooth = (state.dcSmooth * (1.0f - config_.dcSmoothing)) + (stats.dc * config_.dcSmoothing);
    float const smoothedRms = std::isfinite(state.rmsSmooth) ? state.rmsSmooth : 0.0f;
    float const smoothedDc = std::isfinite(state.dcSmooth) ? state.dcSmooth : 0.0f;

    float const rmsDb = safeToDb(smoothedRms);
    float const peakDb = safeToDb(stats.peak);
    float const crestDb = (smoothedRms > 0.0001f) ? safeToDb(stats.peak / smoothedRms, 0.0f) : 0.0f;

    metrics.rmsDb.store(rmsDb, std::memory_order_relaxed);
    metrics.peakDb.store(peakDb, std::memory_order_relaxed);
    metrics.crestFactorDb.store(crestDb, std::memory_order_relaxed);
    metrics.dcOffset.store(smoothedDc, std::memory_order_relaxed);

    if (peakDb > metrics.maxPeakDb.load(std::memory_order_relaxed))
        metrics.maxPeakDb.store(peakDb, std::memory_order_relaxed);

    transientDetector.process(samples, numSamples, sampleRate);
    metrics.attackTimeMs.store(transientDetector.getAttackTimeMs(), std::memory_order_relaxed);
    metrics.decayTimeMs.store(transientDetector.getDecayTimeMs(), std::memory_order_relaxed);
}

} // namespace oscil
