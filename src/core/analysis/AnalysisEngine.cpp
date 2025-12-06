/*
    Oscil - Analysis Engine Implementation
*/

#include "core/analysis/AnalysisEngine.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

namespace oscil
{

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
    (void) samplesPerBlock;  // Intentionally unused - allocation-free implementation
}

void AnalysisEngine::process(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    int numSamples = buffer.getNumSamples();
    if (numSamples == 0) return;
    
    // L/R Processing
    if (buffer.getNumChannels() > 0)
    {
        processChannel(buffer.getReadPointer(0), numSamples, metrics_.left, leftTransient_, leftState_, sampleRate);
    }
    
    if (buffer.getNumChannels() > 1)
    {
        processChannel(buffer.getReadPointer(1), numSamples, metrics_.right, rightTransient_, rightState_, sampleRate);
        
        // Mid/Side Calculation
        // Ensure we don't overrun buffers if block size is unexpectedly larger than prepared
        // In production, prepare() should cover the max block size, but we check for safety
        size_t requiredSize = static_cast<size_t>(numSamples);
        if (midBuffer_.size() < requiredSize || sideBuffer_.size() < requiredSize)
        {
            // This should not happen if prepare() was called correctly
            // Log in debug builds but don't allocate/log on audio thread in release
            jassertfalse;
            return;
        }
        
        const float* l = buffer.getReadPointer(0);
        const float* r = buffer.getReadPointer(1);
        
        // Vectorized Mid/Side: M = (L+R)/2, S = (L-R)/2
        // JUCE FloatVectorOperations are efficient
        juce::FloatVectorOperations::copy(midBuffer_.data(), l, numSamples);
        juce::FloatVectorOperations::add(midBuffer_.data(), r, numSamples);
        juce::FloatVectorOperations::multiply(midBuffer_.data(), 0.5f, numSamples);
        
        juce::FloatVectorOperations::copy(sideBuffer_.data(), l, numSamples);
        juce::FloatVectorOperations::subtract(sideBuffer_.data(), r, numSamples);
        juce::FloatVectorOperations::multiply(sideBuffer_.data(), 0.5f, numSamples);
        
        processChannel(midBuffer_.data(), numSamples, metrics_.mid, midTransient_, midState_, sampleRate);
        processChannel(sideBuffer_.data(), numSamples, metrics_.side, sideTransient_, sideState_, sampleRate);
    }
}

void AnalysisEngine::processChannel(const float* samples, int numSamples, ChannelMetrics& metrics,
                                   TransientDetector& transientDetector, AnalysisChannelState& state, double sampleRate)
{
    // Precondition: caller must ensure numSamples > 0
    // This is verified by process() which returns early if numSamples == 0
    // Runtime guard in case precondition is violated (prevents division by zero in release builds)
    if (numSamples <= 0)
        return;
    jassert(numSamples > 0);

    // RMS
    float rmsLinear = 0.0f;
    // Use JUCE for SIMD RMS if available, or simple loop
    // FloatVectorOperations doesn't have RMS, only add/mul
    // Need to square, sum, sqrt
    // Manual loop is fine for now, or vDSP on macOS

    float sumSq = 0.0f;
    float sum = 0.0f;
    float peakLinear = 0.0f;

    // Range finding for peak
    juce::Range<float> range = juce::FloatVectorOperations::findMinAndMax(samples, numSamples);
    peakLinear = std::max(std::abs(range.getStart()), std::abs(range.getEnd()));

    for (int i = 0; i < numSamples; ++i)
    {
        float s = samples[i];
        sumSq += s * s;
        sum += s;
    }

    // Division is safe: numSamples > 0 is guaranteed by precondition check above
    rmsLinear = std::sqrt(sumSq / static_cast<float>(numSamples));

    // DC Offset (same safety as above)
    float dc = sum / static_cast<float>(numSamples);
    
    // Apply Smoothing
    // RMS Smoothing
    state.rmsSmooth = state.rmsSmooth * (1.0f - config_.rmsSmoothing) + rmsLinear * config_.rmsSmoothing;
    float smoothedRms = state.rmsSmooth;
    
    // DC Smoothing (LPF)
    state.dcSmooth = state.dcSmooth * (1.0f - config_.dcSmoothing) + dc * config_.dcSmoothing;
    float smoothedDc = state.dcSmooth;
    
    // Update Atomics
    // Use smoothed values for RMS and DC
    // Peak is kept instantaneous, but Max Peak Hold handles the readability
    
    float rmsDb = (smoothedRms > 0.00001f) ? 20.0f * std::log10(smoothedRms) : -100.0f;
    float peakDb = (peakLinear > 0.00001f) ? 20.0f * std::log10(peakLinear) : -100.0f;
    
    // Crest Factor (using smoothed RMS to avoid division by noise)
    float crestDb = 0.0f;
    if (smoothedRms > 0.0001f)
    {
        float crest = peakLinear / smoothedRms;
        crestDb = 20.0f * std::log10(crest);
    }
    
    metrics.rmsDb.store(rmsDb, std::memory_order_relaxed);
    metrics.peakDb.store(peakDb, std::memory_order_relaxed);
    metrics.crestFactorDb.store(crestDb, std::memory_order_relaxed);
    metrics.dcOffset.store(smoothedDc, std::memory_order_relaxed);
    
    // Max Peak Hold
    float currentMax = metrics.maxPeakDb.load(std::memory_order_relaxed);
    if (peakDb > currentMax)
    {
        metrics.maxPeakDb.store(peakDb, std::memory_order_relaxed);
    }
    
    // Transients
    transientDetector.process(samples, numSamples, sampleRate);
    metrics.attackTimeMs.store(transientDetector.getAttackTimeMs(), std::memory_order_relaxed);
    metrics.decayTimeMs.store(transientDetector.getDecayTimeMs(), std::memory_order_relaxed);
}

} // namespace oscil
