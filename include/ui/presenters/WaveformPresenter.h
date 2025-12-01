/*
    Oscil - Waveform Presenter
    Handles audio processing and auto-scaling logic for waveform visualization.
    Separates logic from View (WaveformComponent).
*/

#pragma once

#include "core/Oscillator.h"
#include "core/SharedCaptureBuffer.h"
#include "dsp/SignalProcessor.h"
#include <memory>
#include <vector>
#include <juce_core/juce_core.h>

namespace oscil
{

class WaveformPresenter
{
public:
    WaveformPresenter();
    virtual ~WaveformPresenter() = default;

    /**
     * Set the capture buffer to read audio data from.
     */
    void setCaptureBuffer(std::shared_ptr<SharedCaptureBuffer> buffer);

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
    std::shared_ptr<SharedCaptureBuffer> getCaptureBuffer() const { return captureBuffer_; }

private:
    std::shared_ptr<SharedCaptureBuffer> captureBuffer_;
    ProcessingMode processingMode_ = ProcessingMode::FullStereo;
    int displaySamples_ = 2048;
    float gainLinear_ = 1.0f;
    bool autoScale_ = true;

    SignalProcessor signalProcessor_;
    AdaptiveDecimator decimator_;
    ProcessedSignal processedSignal_;

    std::vector<float> displayBuffer1_;
    std::vector<float> displayBuffer2_;

    float currentPeak_ = 0.0f;
    float currentRMS_ = 0.0f;
    float effectiveScale_ = 1.0f;
};

} // namespace oscil
