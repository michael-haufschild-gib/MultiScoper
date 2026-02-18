/*
    Oscil - Waveform Presenter
    Handles audio processing and auto-scaling logic for waveform visualization.
    Separates logic from View (WaveformComponent).
*/

#pragma once

#include "core/Oscillator.h"
#include "core/interfaces/IAudioBuffer.h"
#include "core/dsp/SignalProcessor.h"
#include <memory>
#include <vector>
#include <juce_core/juce_core.h>

namespace oscil
{

class SharedCaptureBuffer; // Forward declare just in case

class WaveformPresenter
{
public:
    WaveformPresenter();
    virtual ~WaveformPresenter() = default;

    /**
     * Set the capture buffer to read audio data from.
     */
    void setCaptureBuffer(std::shared_ptr<IAudioBuffer> buffer);

    /**
     * Set the processing mode (Stereo, Mono, L, R, etc.).
     */
    void setProcessingMode(ProcessingMode mode);

    /**
     * Set the number of samples to display (read window size).
     */
    void setDisplaySamples(int samples);

    /**
     * Set input gain in decibels.
     */
    void setGainDb(float dB);

    /**
     * Enable or disable auto-scaling of amplitude.
     */
    void setAutoScale(bool enabled);

    /**
     * Set the width of the display in pixels (for decimation).
     */
    void setDisplayWidth(int width);

    /**
     * Process audio data from the capture buffer.
     * Reads samples, applies processing, decimates, and updates levels.
     */
    void process();

    // Accessors
    const std::vector<float>& getDisplayBuffer1() const { return displayBuffer1_; }
    const std::vector<float>& getDisplayBuffer2() const { return displayBuffer2_; }
    float getPeakLevel() const { return currentPeak_; }
    float getRMSLevel() const { return currentRMS_; }
    float getEffectiveScale() const { return effectiveScale_; }
    bool isStereo() const { return processedSignal_.isStereo; }
    ProcessingMode getProcessingMode() const { return processingMode_; }
    bool hasData() const { return !displayBuffer1_.empty(); }
    
    // Configuration Accessors
    bool isAutoScaleEnabled() const { return autoScale_; }
    float getGainLinear() const { return gainLinear_; }
    int getDisplaySamples() const { return displaySamples_; }
    std::shared_ptr<IAudioBuffer> getCaptureBuffer() const { return captureBuffer_; }
    
    // LOD Accessors
    [[nodiscard]] LODTier getCurrentLODTier() const { return decimator_.getCurrentTier(); }
    [[nodiscard]] int getTargetSampleCount() const { return decimator_.getTargetSampleCount(); }
    [[nodiscard]] int getDecimatorDisplayWidth() const { return decimator_.getDisplayWidth(); }
    [[nodiscard]] bool isLODTransitioning() const { return lodTransitionProgress_ < 1.0f; }
    [[nodiscard]] float getLODTransitionProgress() const { return lodTransitionProgress_; }
    
    // Min/Max envelope accessors for GPU rendering with peak preservation
    [[nodiscard]] const std::vector<float>& getMinEnvelope1() const { return decimatedWaveform1_.minEnvelope; }
    [[nodiscard]] const std::vector<float>& getMaxEnvelope1() const { return decimatedWaveform1_.maxEnvelope; }
    [[nodiscard]] const std::vector<float>& getMinEnvelope2() const { return decimatedWaveform2_.minEnvelope; }
    [[nodiscard]] const std::vector<float>& getMaxEnvelope2() const { return decimatedWaveform2_.maxEnvelope; }
    [[nodiscard]] bool hasMinMaxEnvelopes() const { return !decimatedWaveform1_.minEnvelope.empty(); }

private:
    std::shared_ptr<IAudioBuffer> captureBuffer_;
    ProcessingMode processingMode_ = ProcessingMode::FullStereo;
    int displaySamples_ = 2048;
    float gainLinear_ = 1.0f;
    bool autoScale_ = true;

    SignalProcessor signalProcessor_;
    AdaptiveDecimator decimator_;
    ProcessedSignal processedSignal_;

    std::vector<float> displayBuffer1_;
    std::vector<float> displayBuffer2_;
    
    // Peak-preserving decimated waveforms with min/max envelopes
    DecimatedWaveform decimatedWaveform1_;
    DecimatedWaveform decimatedWaveform2_;

    // Scratch buffers to avoid re-allocation in process loop
    std::vector<float> scratchBufferLeft_;
    std::vector<float> scratchBufferRight_;

    float currentPeak_ = 0.0f;
    float currentRMS_ = 0.0f;
    float effectiveScale_ = 1.0f;
    
    // LOD Transition State
    // Smooth transitions between LOD tiers to prevent visual "popping"
    static constexpr float LOD_TRANSITION_SPEED = 0.15f;  // Per-frame blend factor
    LODTier previousLODTier_ = LODTier::Full;
    float lodTransitionProgress_ = 1.0f;  // 1.0 = transition complete
    
    // Previous buffers for cross-fade during LOD transitions
    std::vector<float> previousDisplayBuffer1_;
    std::vector<float> previousDisplayBuffer2_;
};

} // namespace oscil
