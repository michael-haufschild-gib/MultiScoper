/*
    Oscil - Waveform Component Header
    Renders waveform visualization for an oscillator
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include "core/Oscillator.h"
#include "core/SharedCaptureBuffer.h"
#include "ui/panels/WaveformPresenter.h"
#include "core/dsp/TimingConfig.h"
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
    ~WaveformComponent() override; // Not default anymore, needs to destroy unique_ptr

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
     * Set the grid configuration
     */
    void setGridConfig(const GridConfiguration& config);

    /**
     * Set the shader ID for GPU rendering
     */
    void setShaderId(const juce::String& shaderId);

    /**
     * Set the visual preset ID for advanced rendering effects
     */
    void setVisualPresetId(const juce::String& presetId);

    /**
     * Set visual configuration overrides.
     */
    void setVisualOverrides(const juce::ValueTree& overrides);

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
     * Get current peak level
     */
    float getPeakLevel() const;

    /**
     * Get current RMS level
     */
    float getRMSLevel() const;

    /**
     * Force update of waveform data (reads from buffer and calculates levels)
     * Useful for testing to ensure data is processed before querying state
     */
    void forceUpdateWaveformData();

    /**
     * Process audio data from the capture buffer.
     * Populates the display buffers and calculates levels (Peak/RMS).
     * Must be called periodically (e.g. from timer) when GPU rendering is enabled,
     * as paint() is not called in that mode.
     */
    void processAudioData();

    // Test access - for automated testing only
    bool isGridVisible() const { return showGrid_; }
    bool isAutoScaleEnabled() const;
    bool isHoldDisplayEnabled() const { return holdDisplay_; }
    float getGainLinear() const;
    float getOpacity() const { return opacity_; }
    float getLineWidth() const { return lineWidth_; }
    juce::Colour getColour() const { return colour_; }
    ProcessingMode getProcessingMode() const;
    bool hasWaveformData() const { return !waveformPath1_.isEmpty(); }
    int getDisplaySamples() const;
    std::shared_ptr<SharedCaptureBuffer> getCaptureBuffer() const;
    juce::String getShaderId() const { return shaderId_; }
    bool isGpuRenderingEnabled() const { return gpuRenderingEnabled_; }

    // Accessibility support
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawGridLabels(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawWaveform(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawWaveformWithShader(juce::Graphics& g, juce::Rectangle<int> bounds);
    void updateWaveformPath();

    // Helper to format time value for display
    static juce::String formatTimeLabel(float ms);
    // Helper to calculate nice grid step size
    static float calculateGridStepSize(float totalDuration, int targetDivisions);

    std::unique_ptr<WaveformPresenter> presenter_;
    
    juce::Colour colour_{ 0xFF00FFFF }; // Default, overwritten by oscillator
    float opacity_ = 1.0f;
    float lineWidth_ = 1.5f;  // Default line width
    bool showGrid_ = true;
    GridConfiguration gridConfig_;
    bool holdDisplay_ = false;
    bool highlighted_ = false;
    StereoDisplayMode stereoDisplayMode_ = StereoDisplayMode::Stacked;

    // Shader rendering
    juce::String shaderId_ = "basic";
    juce::String visualPresetId_ = "default";  // Visual preset for advanced rendering
    juce::ValueTree visualOverrides_; // Custom visual settings overrides
    bool gpuRenderingEnabled_ = false;
    int waveformId_ = 0;  // Unique ID for GL renderer registration
    static std::atomic<int> nextWaveformId_;  // Counter for generating unique IDs

    juce::Path waveformPath1_;
    juce::Path waveformPath2_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent)
};

} // namespace oscil
