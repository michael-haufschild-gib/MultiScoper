/*
    Oscil - Analysis Engine
    Calculates real-time statistical metrics for audio signals
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "core/analysis/AnalysisTypes.h"
#include "core/analysis/TransientDetector.h"
#include <memory>

namespace oscil
{

struct AnalysisConfig
{
    float rmsSmoothing = 0.15f;  // approx 15Hz at 60Hz update or similar block rate
    float dcSmoothing = 0.05f;   // slower for DC
};

struct AnalysisChannelState
{
    float rmsSmooth = 0.0f;
    float dcSmooth = 0.0f;
};

class AnalysisEngine
{
public:
    AnalysisEngine();
    
    /**
     * Process a block of audio samples
     */
    void process(const juce::AudioBuffer<float>& buffer, double sampleRate);
    
    /**
     * Reset all metrics
     */
    void reset();
    
    /**
     * Get the latest metrics
     */
    const AnalysisMetrics& getMetrics() const { return metrics_; }
    
    /**
     * Reset accumulated values (like Max Peak)
     */
    void resetAccumulated();

private:
    void processChannel(const float* samples, int numSamples, ChannelMetrics& metrics, 
                       TransientDetector& transientDetector, AnalysisChannelState& state, double sampleRate);
    
    AnalysisMetrics metrics_;
    AnalysisConfig config_;
    
    // State per channel
    AnalysisChannelState leftState_;
    AnalysisChannelState rightState_;
    AnalysisChannelState midState_;
    AnalysisChannelState sideState_;
    
    // Transient detectors per channel
    TransientDetector leftTransient_;
    TransientDetector rightTransient_;
    TransientDetector midTransient_;
    TransientDetector sideTransient_;
    
    // Scratch buffers for Mid/Side calculation
    std::vector<float> midBuffer_;
    std::vector<float> sideBuffer_;
};

} // namespace oscil
