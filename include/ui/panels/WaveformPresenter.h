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
#include <optional>
#include <cstdint>
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

    /**
     * Request display restart from a specific timeline sample timestamp.
     * Subsequent process() calls keep a fixed display window and zero-fill
     * the not-yet-captured tail until the restart window is fully populated.
     */
    void requestRestartAtTimestamp(int64_t timelineSampleTimestamp);

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

    // Scratch buffers to avoid re-allocation in process loop
    std::vector<float> scratchBufferLeft_;
    std::vector<float> scratchBufferRight_;

    float currentPeak_ = 0.0f;
    float currentRMS_ = 0.0f;
    float effectiveScale_ = 1.0f;
    std::optional<int64_t> restartTimelineSample_;
};

} // namespace oscil
