/*
    Oscil - Waveform Component Header
    Renders waveform visualization for an oscillator
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include "core/Oscillator.h"
#include "core/SharedCaptureBuffer.h"
#include "dsp/SignalProcessor.h"
#include <vector>

namespace oscil
{

/**
 * Display mode for stereo waveforms
 */
enum class StereoDisplayMode
{
    Stacked    // L channel on top, R channel on bottom
};

class OscilPluginProcessor;

/**
 * Component that renders a waveform visualization for a single oscillator
 */
class WaveformComponent : public juce::Component
{
public:
    WaveformComponent();
    ~WaveformComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * Set the capture buffer to read audio data from
     */
    void setCaptureBuffer(std::shared_ptr<SharedCaptureBuffer> buffer);

    /**
     * Set the processing mode for this waveform
     */
    void setProcessingMode(ProcessingMode mode);

    /**
     * Set the waveform color
     */
    void setColour(juce::Colour colour);

    /**
     * Set the opacity
     */
    void setOpacity(float opacity);

    /**
     * Set the number of samples to display
     */
    void setDisplaySamples(int samples);

    /**
     * Enable/disable grid rendering
     */
    void setShowGrid(bool show);

    /**
     * Enable/disable auto-scaling of waveform amplitude
     */
    void setAutoScale(bool enabled);

    /**
     * Enable/disable hold display (freeze waveform)
     */
    void setHoldDisplay(bool enabled);

    /**
     * Set input gain in decibels (-60 to +24 dB)
     */
    void setGainDb(float dB);

    /**
     * Set highlighted state (draws thicker outline when true)
     */
    void setHighlighted(bool highlighted);

    /**
     * Set stereo display mode (stacked vs overlaid)
     */
    void setStereoDisplayMode(StereoDisplayMode mode);

    /**
     * Get current stereo display mode
     */
    StereoDisplayMode getStereoDisplayMode() const { return stereoDisplayMode_; }

    /**
     * Set vertical scale factor
     */
    void setVerticalScale(float scale);

    /**
     * Get current peak level
     */
    float getPeakLevel() const { return currentPeak_; }

    /**
     * Get current RMS level
     */
    float getRMSLevel() const { return currentRMS_; }

    /**
     * Force update of waveform data (reads from buffer and calculates levels)
     * Useful for testing to ensure data is processed before querying state
     */
    void forceUpdateWaveformData();

    // Test access - for automated testing only
    bool isGridVisible() const { return showGrid_; }
    bool isAutoScaleEnabled() const { return autoScale_; }
    bool isHoldDisplayEnabled() const { return holdDisplay_; }
    float getGainLinear() const { return gainLinear_; }
    float getOpacity() const { return opacity_; }
    juce::Colour getColour() const { return colour_; }
    ProcessingMode getProcessingMode() const { return processingMode_; }
    bool hasWaveformData() const { return !waveformPath1_.isEmpty(); }
    int getDisplaySamples() const { return displaySamples_; }
    std::shared_ptr<SharedCaptureBuffer> getCaptureBuffer() const { return captureBuffer_; }

private:
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawWaveform(juce::Graphics& g, juce::Rectangle<int> bounds);
    void updateWaveformPath();

    std::shared_ptr<SharedCaptureBuffer> captureBuffer_;
    ProcessingMode processingMode_ = ProcessingMode::FullStereo;
    juce::Colour colour_{ 0xFF00FF00 };
    float opacity_ = 1.0f;
    int displaySamples_ = 2048;
    bool showGrid_ = true;
    bool autoScale_ = true;
    bool holdDisplay_ = false;
    float verticalScale_ = 1.0f;
    float gainLinear_ = 1.0f;  // Linear gain multiplier (converted from dB)
    bool highlighted_ = false;
    StereoDisplayMode stereoDisplayMode_ = StereoDisplayMode::Stacked;

    SignalProcessor signalProcessor_;
    AdaptiveDecimator decimator_;
    ProcessedSignal processedSignal_;

    std::vector<float> displayBuffer1_;
    std::vector<float> displayBuffer2_;
    juce::Path waveformPath1_;
    juce::Path waveformPath2_;

    float currentPeak_ = 0.0f;
    float currentRMS_ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent)
};

} // namespace oscil
