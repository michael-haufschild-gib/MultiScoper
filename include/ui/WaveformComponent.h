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
#include <atomic>

namespace oscil
{

// Forward declarations
class WaveformShader;
struct WaveformRenderData;

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
     * Set the line width for waveform rendering
     */
    void setLineWidth(float width);

    /**
     * Set the number of samples to display
     */
    void setDisplaySamples(int samples);

    /**
     * Set the shader ID for GPU rendering
     */
    void setShaderId(const juce::String& shaderId);

    /**
     * Set the visual preset ID for advanced rendering effects
     */
    void setVisualPresetId(const juce::String& presetId);

    /**
     * Enable/disable GPU shader rendering
     * When enabled and a valid shader is set, uses GPU rendering
     */
    void setGpuRenderingEnabled(bool enabled);

    /**
     * Set unique waveform ID for GL renderer registration
     */
    void setWaveformId(int id);

    /**
     * Get the waveform ID
     */
    int getWaveformId() const { return waveformId_; }

    /**
     * Populate render data for the GL renderer (thread-safe snapshot)
     * Call this from the message thread to prepare data for GL rendering
     * @param data Output struct to fill with current waveform state
     */
    void populateGLRenderData(WaveformRenderData& data) const;

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
    float getLineWidth() const { return lineWidth_; }
    juce::Colour getColour() const { return colour_; }
    ProcessingMode getProcessingMode() const { return processingMode_; }
    bool hasWaveformData() const { return !waveformPath1_.isEmpty(); }
    int getDisplaySamples() const { return displaySamples_; }
    std::shared_ptr<SharedCaptureBuffer> getCaptureBuffer() const { return captureBuffer_; }
    juce::String getShaderId() const { return shaderId_; }
    bool isGpuRenderingEnabled() const { return gpuRenderingEnabled_; }

private:
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawWaveform(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawWaveformWithShader(juce::Graphics& g, juce::Rectangle<int> bounds);
    void updateWaveformPath();

    std::shared_ptr<SharedCaptureBuffer> captureBuffer_;
    ProcessingMode processingMode_ = ProcessingMode::FullStereo;
    juce::Colour colour_{ 0xFF00FF00 };
    float opacity_ = 1.0f;
    float lineWidth_ = 1.5f;  // Default line width
    int displaySamples_ = 2048;
    bool showGrid_ = true;
    bool autoScale_ = true;
    bool holdDisplay_ = false;
    float verticalScale_ = 1.0f;
    float gainLinear_ = 1.0f;  // Linear gain multiplier (converted from dB)
    bool highlighted_ = false;
    StereoDisplayMode stereoDisplayMode_ = StereoDisplayMode::Stacked;

    // Shader rendering
    juce::String shaderId_ = "basic";
    juce::String visualPresetId_ = "default";  // Visual preset for advanced rendering
    bool gpuRenderingEnabled_ = false;
    int waveformId_ = 0;  // Unique ID for GL renderer registration
    static std::atomic<int> nextWaveformId_;  // Counter for generating unique IDs

    SignalProcessor signalProcessor_;
    AdaptiveDecimator decimator_;
    ProcessedSignal processedSignal_;

    std::vector<float> displayBuffer1_;
    std::vector<float> displayBuffer2_;
    juce::Path waveformPath1_;
    juce::Path waveformPath2_;

    float currentPeak_ = 0.0f;
    float currentRMS_ = 0.0f;
    float effectiveScale_ = 1.0f;  // Computed scale factor (includes auto-scale)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent)
};

} // namespace oscil
