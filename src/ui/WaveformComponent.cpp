/*
    Oscil - Waveform Component Implementation
*/

#include "ui/WaveformComponent.h"
#include "ui/ThemeManager.h"
#include "rendering/ShaderRegistry.h"
#include "rendering/WaveformShader.h"
#include "rendering/VisualConfiguration.h"
#if OSCIL_ENABLE_OPENGL
#include "rendering/WaveformGLRenderer.h"
#include <juce_audio_processors/juce_audio_processors.h>
#endif

namespace oscil
{

// Static counter for unique waveform IDs
std::atomic<int> WaveformComponent::nextWaveformId_{ 1 };

//==============================================================================
// Accessibility Handler for WaveformComponent
//==============================================================================
class WaveformComponentAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit WaveformComponentAccessibilityHandler(WaveformComponent& waveform)
        : juce::AccessibilityHandler(waveform, juce::AccessibilityRole::image)
        , waveform_(waveform)
    {
    }

    juce::String getTitle() const override
    {
        // Build title based on processing mode
        switch (waveform_.getProcessingMode())
        {
            case ProcessingMode::FullStereo:
                return "Stereo Waveform Display";
            case ProcessingMode::Mono:
                return "Mono Waveform Display";
            case ProcessingMode::Left:
                return "Left Channel Waveform";
            case ProcessingMode::Right:
                return "Right Channel Waveform";
            case ProcessingMode::Mid:
                return "Mid Signal Waveform";
            case ProcessingMode::Side:
                return "Side Signal Waveform";
            default:
                return "Waveform Display";
        }
    }

    juce::String getDescription() const override
    {
        if (!waveform_.hasWaveformData())
        {
            return "No Signal";
        }

        // Convert peak and RMS to dB for screen reader
        float peakLevel = waveform_.getPeakLevel();
        float rmsLevel = waveform_.getRMSLevel();

        juce::String peakDb, rmsDb;
        if (peakLevel > 0.0001f)
        {
            float peakDbValue = 20.0f * std::log10(peakLevel);
            peakDb = juce::String(peakDbValue, 1) + " dB";
        }
        else
        {
            peakDb = "-inf dB";
        }

        if (rmsLevel > 0.0001f)
        {
            float rmsDbValue = 20.0f * std::log10(rmsLevel);
            rmsDb = juce::String(rmsDbValue, 1) + " dB";
        }
        else
        {
            rmsDb = "-inf dB";
        }

        return "Peak: " + peakDb + ", RMS: " + rmsDb;
    }

    juce::String getHelp() const override
    {
        return "Audio waveform visualization. Peak and RMS levels are announced.";
    }

private:
    WaveformComponent& waveform_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponentAccessibilityHandler)
};

std::unique_ptr<juce::AccessibilityHandler> WaveformComponent::createAccessibilityHandler()
{
    return std::make_unique<WaveformComponentAccessibilityHandler>(*this);
}

WaveformComponent::WaveformComponent()
    : presenter_(std::make_unique<WaveformPresenter>())
{
    setOpaque(false);
    // Auto-assign a unique ID
    waveformId_ = nextWaveformId_.fetch_add(1);
}

WaveformComponent::~WaveformComponent() = default;

void WaveformComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    const auto& theme = ThemeManager::getInstance().getCurrentTheme();

    // FIX: Only draw background in software mode - in GPU mode, OpenGL renders directly
    // and filling background here would cover the GL waveform
    if (!gpuRenderingEnabled_)
    {
        g.setColour(theme.backgroundPane);
        g.fillRect(bounds);
    }

    // Draw highlight border if selected (works in both modes - drawn on top)
    if (highlighted_)
    {
        g.setColour(theme.controlActive.withAlpha(0.8f));
        g.drawRect(bounds.toFloat(), 2.0f);
    }

    // Draw grid only in software mode (GPU mode should render grid via shader if needed)
    if (showGrid_ && !gpuRenderingEnabled_)
    {
        drawGrid(g, bounds);
    }

    // In GPU mode, waveform is rendered by OpenGL (WaveformGLRenderer::renderOpenGL)
    // In software mode, draw waveform here using JUCE Graphics
    if (!gpuRenderingEnabled_)
    {
        // Update and draw waveform using CPU rendering
        updateWaveformPath();
        drawWaveform(g, bounds);
    }

    // If no waveform data, show "No Signal" text (only in software mode)
    if (!gpuRenderingEnabled_ && waveformPath1_.isEmpty() && bounds.getWidth() > 50 && bounds.getHeight() > 30)
    {
        g.setColour(theme.textSecondary.withAlpha(0.5f));
        g.setFont(14.0f);
        g.drawText("No Signal", bounds, juce::Justification::centred);
    }
}

void WaveformComponent::resized()
{
    if (presenter_)
        presenter_->setDisplayWidth(getWidth());
}

void WaveformComponent::setCaptureBuffer(std::shared_ptr<SharedCaptureBuffer> buffer)
{
    if (presenter_)
        presenter_->setCaptureBuffer(buffer);
}

void WaveformComponent::setProcessingMode(ProcessingMode mode)
{
    if (presenter_)
        presenter_->setProcessingMode(mode);
}

void WaveformComponent::setColour(juce::Colour colour)
{
    colour_ = colour;
}

void WaveformComponent::setOpacity(float opacity)
{
    opacity_ = juce::jlimit(0.0f, 1.0f, opacity);
}

void WaveformComponent::setLineWidth(float width)
{
    // Use same constraints as Oscillator::MIN_LINE_WIDTH and MAX_LINE_WIDTH
    lineWidth_ = juce::jlimit(0.5f, 5.0f, width);
}

void WaveformComponent::setDisplaySamples(int samples)
{
    if (presenter_)
        presenter_->setDisplaySamples(samples);
}

void WaveformComponent::setShaderId(const juce::String& shaderId)
{
    shaderId_ = shaderId;
}

void WaveformComponent::setVisualPresetId(const juce::String& presetId)
{
    visualPresetId_ = presetId;
}

void WaveformComponent::setVisualOverrides(const juce::ValueTree& overrides)
{
    visualOverrides_ = overrides.createCopy();
}

void WaveformComponent::setGpuRenderingEnabled(bool enabled)
{
    gpuRenderingEnabled_ = enabled;
}

void WaveformComponent::setShowGrid(bool show)
{
    showGrid_ = show;
}

void WaveformComponent::setAutoScale(bool enabled)
{
    if (presenter_)
        presenter_->setAutoScale(enabled);
}

void WaveformComponent::setHoldDisplay(bool enabled)
{
    holdDisplay_ = enabled;
}

void WaveformComponent::setGainDb(float dB)
{
    if (presenter_)
        presenter_->setGainDb(dB);
}

void WaveformComponent::setHighlighted(bool highlighted)
{
    highlighted_ = highlighted;
    repaint();
}

void WaveformComponent::setStereoDisplayMode(StereoDisplayMode mode)
{
    stereoDisplayMode_ = mode;
}

void WaveformComponent::forceUpdateWaveformData()
{
    // Force the waveform path update which calculates peak/RMS levels
    // This is useful for testing to ensure data is processed before querying state
    processAudioData();
}

void WaveformComponent::processAudioData()
{
    if (holdDisplay_ || !presenter_)
        return;

    presenter_->process();
}

void WaveformComponent::drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();

    int width = bounds.getWidth();
    int height = bounds.getHeight();

    ProcessingMode mode = presenter_ ? presenter_->getProcessingMode() : ProcessingMode::FullStereo;

    if (mode == ProcessingMode::FullStereo)
    {
        // Stereo stacked: draw grid for both channels
        float halfHeight = static_cast<float>(height) * 0.5f;
        float topCenterY = halfHeight * 0.5f;     // Center of top half (L channel)
        float bottomCenterY = halfHeight * 1.5f;  // Center of bottom half (R channel)

        // Draw minor vertical grid lines (full height)
        g.setColour(theme.gridMinor);
        const int numVerticalLines = 10;
        for (int i = 1; i < numVerticalLines; ++i)
        {
            float x = (static_cast<float>(i) / numVerticalLines) * static_cast<float>(width);
            g.drawVerticalLine(static_cast<int>(x), 0.0f, static_cast<float>(height));
        }

        // Draw horizontal grid lines for top half (L channel)
        const int numMinorLines = 4;
        for (int i = 1; i < numMinorLines; ++i)
        {
            float y = (static_cast<float>(i) / numMinorLines) * halfHeight;
            g.drawHorizontalLine(static_cast<int>(y), 0.0f, static_cast<float>(width));
        }

        // Draw horizontal grid lines for bottom half (R channel)
        for (int i = 1; i < numMinorLines; ++i)
        {
            float y = halfHeight + (static_cast<float>(i) / numMinorLines) * halfHeight;
            g.drawHorizontalLine(static_cast<int>(y), 0.0f, static_cast<float>(width));
        }

        // Draw zero lines for both channels
        g.setColour(theme.gridZeroLine);
        g.drawHorizontalLine(static_cast<int>(topCenterY), 0.0f, static_cast<float>(width));
        g.drawHorizontalLine(static_cast<int>(bottomCenterY), 0.0f, static_cast<float>(width));

        // Draw separator line between channels
        g.setColour(theme.gridMajor);
        g.drawHorizontalLine(static_cast<int>(halfHeight), 0.0f, static_cast<float>(width));

        // Draw channel labels
        g.setColour(theme.textSecondary.withAlpha(0.6f));
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        g.drawText("L", juce::Rectangle<int>(4, 2, 20, 14), juce::Justification::centredLeft);
        g.drawText("R", juce::Rectangle<int>(4, static_cast<int>(halfHeight) + 2, 20, 14), juce::Justification::centredLeft);
    }
    else
    {
        // Mono/other modes: standard single-center grid
        float centerY = static_cast<float>(height) * 0.5f;

        // Draw minor grid lines (horizontal)
        g.setColour(theme.gridMinor);
        const int numMinorLines = 8;
        for (int i = 1; i < numMinorLines; ++i)
        {
            float y = (static_cast<float>(i) / numMinorLines) * static_cast<float>(height);
            g.drawHorizontalLine(static_cast<int>(y), 0.0f, static_cast<float>(width));
        }

        // Draw minor vertical grid lines
        const int numVerticalLines = 10;
        for (int i = 1; i < numVerticalLines; ++i)
        {
            float x = (static_cast<float>(i) / numVerticalLines) * static_cast<float>(width);
            g.drawVerticalLine(static_cast<int>(x), 0.0f, static_cast<float>(height));
        }

        // Draw zero line
        g.setColour(theme.gridZeroLine);
        g.drawHorizontalLine(static_cast<int>(centerY), 0.0f, static_cast<float>(width));

        // Draw major grid lines
        g.setColour(theme.gridMajor);
        g.drawHorizontalLine(static_cast<int>(centerY * 0.5f), 0.0f, static_cast<float>(width));
        g.drawHorizontalLine(static_cast<int>(centerY * 1.5f), 0.0f, static_cast<float>(width));
    }
}

void WaveformComponent::drawWaveformWithShader(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Get shader from registry
    auto* shader = ShaderRegistry::getInstance().getShader(shaderId_);
    if (!shader)
    {
        // Fallback to default shader
        shader = ShaderRegistry::getInstance().getShader(
            ShaderRegistry::getInstance().getDefaultShaderId());
    }

    if (!shader)
    {
        // Last resort: use standard path rendering
        drawWaveform(g, bounds);
        return;
    }

    if (!presenter_) return;

    // Set up render parameters
    ShaderRenderParams params;
    params.colour = colour_;
    params.opacity = opacity_;
    params.lineWidth = lineWidth_;
    params.bounds = bounds.toFloat();
    params.isStereo = presenter_->isStereo();
    params.verticalScale = presenter_->getEffectiveScale();

    // Use software rendering path (GPU rendering happens in OpenGL render callback)
    // The shader's software fallback provides the basic effect via multi-pass path rendering
    const auto& buffer2 = presenter_->getDisplayBuffer2();
    const std::vector<float>* channel2Ptr = params.isStereo ? &buffer2 : nullptr;
    shader->renderSoftware(g, presenter_->getDisplayBuffer1(), channel2Ptr, params);
}

void WaveformComponent::drawWaveform(juce::Graphics& g, juce::Rectangle<int> /*bounds*/)
{
    if (!presenter_ || !presenter_->hasData())
        return;

    // Draw first channel (L or mono)
    if (!waveformPath1_.isEmpty())
    {
        g.setColour(colour_.withAlpha(opacity_));
        g.strokePath(waveformPath1_, juce::PathStrokeType(lineWidth_));
    }

    // Draw second channel for stereo mode (stacked: same color, channels visually separated)
    if (presenter_->isStereo() && !waveformPath2_.isEmpty())
    {
        g.setColour(colour_.withAlpha(opacity_));
        g.strokePath(waveformPath2_, juce::PathStrokeType(lineWidth_));
    }
}

void WaveformComponent::updateWaveformPath()
{
    // Ensure data is fresh (CPU mode needs this called here)
    processAudioData();

    if (holdDisplay_ || !presenter_)
        return;

    waveformPath1_.clear();
    waveformPath2_.clear();

    const auto& displayBuffer1 = presenter_->getDisplayBuffer1();
    const auto& displayBuffer2 = presenter_->getDisplayBuffer2();
    
    if (displayBuffer1.empty())
        return;

    int width = getWidth();
    int height = getHeight();

    if (width <= 0 || height <= 0)
        return;

    // Calculate center Y positions based on processing mode
    float centerY1, centerY2;
    float amplitude1, amplitude2;  // Amplitude range for each channel

    bool isStereo = presenter_->isStereo();
    float effectiveScale = presenter_->getEffectiveScale();

    if (isStereo)
    {
        // Stereo stacked: L on top half, R on bottom half
        float halfHeight = static_cast<float>(height) * 0.5f;
        centerY1 = halfHeight * 0.5f;      // Center of top half (L channel)
        centerY2 = halfHeight * 1.5f;      // Center of bottom half (R channel)
        amplitude1 = halfHeight * 0.5f;    // Amplitude fits in quarter height
        amplitude2 = halfHeight * 0.5f;
    }
    else
    {
        // Mono/other modes: single channel centered
        centerY1 = static_cast<float>(height) * 0.5f;
        centerY2 = static_cast<float>(height) * 0.5f;
        amplitude1 = static_cast<float>(height) * 0.5f;
        amplitude2 = static_cast<float>(height) * 0.5f;
    }

    // Build path for first channel (L or mono)
    // Need at least 2 samples to draw a line
    if (displayBuffer1.size() >= 2)
    {
        float xScale = static_cast<float>(width) / static_cast<float>(displayBuffer1.size() - 1);

        float yStart = centerY1 - displayBuffer1[0] * amplitude1 * effectiveScale;
        waveformPath1_.startNewSubPath(0, yStart);

        for (size_t i = 1; i < displayBuffer1.size(); ++i)
        {
            float x = static_cast<float>(i) * xScale;
            float y = centerY1 - displayBuffer1[i] * amplitude1 * effectiveScale;

            // Clamp to appropriate region
            if (isStereo)
            {
                // Stereo stacked: L channel in top half
                y = juce::jlimit(0.0f, static_cast<float>(height) * 0.5f, y);
            }
            else
            {
                // Mono: full height
                y = juce::jlimit(0.0f, static_cast<float>(height), y);
            }

            waveformPath1_.lineTo(x, y);
        }
    }

    // Build path for second channel (R in stereo mode)
    // Need at least 2 samples to draw a line
    if (isStereo && displayBuffer2.size() >= 2)
    {
        float xScale = static_cast<float>(width) / static_cast<float>(displayBuffer2.size() - 1);

        float yStart = centerY2 - displayBuffer2[0] * amplitude2 * effectiveScale;
        waveformPath2_.startNewSubPath(0, yStart);

        for (size_t i = 1; i < displayBuffer2.size(); ++i)
        {
            float x = static_cast<float>(i) * xScale;
            float y = centerY2 - displayBuffer2[i] * amplitude2 * effectiveScale;

            // Stereo stacked: R channel in bottom half
            y = juce::jlimit(static_cast<float>(height) * 0.5f, static_cast<float>(height), y);

            waveformPath2_.lineTo(x, y);
        }
    }
}

void WaveformComponent::setWaveformId(int id)
{
    waveformId_ = id;
}

void WaveformComponent::populateGLRenderData(WaveformRenderData& data) const
{
#if OSCIL_ENABLE_OPENGL
    data.id = waveformId_;

    juce::Rectangle<int> relativeBounds;

    juce::Component* editorComponent = nullptr;
    for (auto* p = getParentComponent(); p != nullptr; p = p->getParentComponent())
    {
        if (dynamic_cast<juce::AudioProcessorEditor*>(p) != nullptr)
        {
            editorComponent = p;
            break;
        }
    }

    if (editorComponent != nullptr)
    {
        relativeBounds = editorComponent->getLocalArea(this, getLocalBounds());
    }
    else
    {
        relativeBounds = getLocalBounds();
    }
    data.bounds = relativeBounds.toFloat();

    // Copy display buffer data from presenter
    if (presenter_) {
        data.channel1 = presenter_->getDisplayBuffer1();
        data.channel2 = presenter_->getDisplayBuffer2();
        data.isStereo = presenter_->isStereo();
        data.verticalScale = presenter_->getEffectiveScale();
    }

    // Copy render parameters
    data.colour = colour_;
    data.opacity = opacity_;
    data.lineWidth = lineWidth_;
    data.shaderId = shaderId_;
    data.visible = isVisible() && !data.channel1.empty();
    

    // Populate advanced visual configuration from preset
    data.visualConfig = VisualConfiguration::getPreset(visualPresetId_);
    
    // Apply overrides if present
    if (visualOverrides_.isValid())
    {
        VisualConfiguration::applyOverrides(data.visualConfig, visualOverrides_);
    }
#else
    juce::ignoreUnused(data);
#endif
}

// Getters
float WaveformComponent::getPeakLevel() const { return presenter_ ? presenter_->getPeakLevel() : 0.0f; }
float WaveformComponent::getRMSLevel() const { return presenter_ ? presenter_->getRMSLevel() : 0.0f; }
ProcessingMode WaveformComponent::getProcessingMode() const { 
    return presenter_ ? presenter_->getProcessingMode() : ProcessingMode::FullStereo; 
}

bool WaveformComponent::isAutoScaleEnabled() const { return presenter_ ? presenter_->isAutoScaleEnabled() : true; }
float WaveformComponent::getGainLinear() const { return presenter_ ? presenter_->getGainLinear() : 1.0f; }
int WaveformComponent::getDisplaySamples() const { return presenter_ ? presenter_->getDisplaySamples() : 2048; }
std::shared_ptr<SharedCaptureBuffer> WaveformComponent::getCaptureBuffer() const { return presenter_ ? presenter_->getCaptureBuffer() : nullptr; }

} // namespace oscil
