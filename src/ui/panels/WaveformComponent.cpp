/*
    Oscil - Waveform Component Implementation
*/

#include "ui/panels/WaveformComponent.h"

#include "ui/theme/ThemeManager.h"
#include <fstream>

#include "rendering/ShaderRegistry.h"
#include "rendering/VisualConfiguration.h"
#include "rendering/WaveformShader.h"
#if OSCIL_ENABLE_OPENGL
    #include "rendering/WaveformGLRenderer.h"

    #include <juce_audio_processors/juce_audio_processors.h>
#endif

namespace oscil
{

// Static counter for unique waveform IDs
std::atomic<int> WaveformComponent::nextWaveformId_{1};

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

WaveformComponent::WaveformComponent(IThemeService& themeService, ShaderRegistry& shaderRegistry)
    : themeService_(themeService)
    , shaderRegistry_(shaderRegistry)
    , presenter_(std::make_unique<WaveformPresenter>())
    , gridRenderer_(std::make_unique<SoftwareGridRenderer>(themeService))
    , visualPresetId_("default")
{
    setOpaque(false);
    // Auto-assign a unique ID
    waveformId_ = nextWaveformId_.fetch_add(1);

    // Use VBlank attachment to drive updates decoupled from paint()
    vBlankAttachment_ = std::make_unique<juce::VBlankAttachment>(this, [this]() {
        updateWaveformPath();
        repaint();
    });
}

WaveformComponent::~WaveformComponent() = default;

void WaveformComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    const auto& theme = themeService_.getCurrentTheme();

    // #region agent log
    { static int logCounter = 0; if (++logCounter % 30 == 1) { std::ofstream f("/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log", std::ios::app); f << "{\"hypothesisId\":\"H12\",\"location\":\"WaveformComponent.cpp:paint\",\"message\":\"Paint called\",\"data\":{\"gpuEnabled\":" << (gpuRenderingEnabled_ ? "true" : "false") << ",\"willFillBg\":" << (!gpuRenderingEnabled_ ? "true" : "false") << "},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n"; f.close(); } }
    // #endregion

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

    // Draw grid only in software mode (GPU mode renders grid via GridRenderer)
    if (gridRenderer_ && gridRenderer_->isGridVisible() && !gpuRenderingEnabled_)
    {
        ProcessingMode mode = presenter_ ? presenter_->getProcessingMode() : ProcessingMode::FullStereo;
        gridRenderer_->render(g, bounds, mode);
    }

    // In GPU mode, waveform is rendered by OpenGL (WaveformGLRenderer::renderOpenGL)
    // In software mode, draw waveform here using JUCE Graphics
    if (!gpuRenderingEnabled_)
    {
        // Waveform path is updated in VBlank callback
        drawWaveformWithShader(g, bounds);
    }

    // Draw grid labels on top of everything (works in both GPU and software mode)
    // Labels are drawn using JUCE Graphics which renders on top of OpenGL content
    if (gridRenderer_ && gridRenderer_->isGridVisible() && bounds.getWidth() > 100 && bounds.getHeight() > 60)
    {
        ProcessingMode mode = presenter_ ? presenter_->getProcessingMode() : ProcessingMode::FullStereo;
        gridRenderer_->renderLabels(g, bounds, mode);
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

void WaveformComponent::setCaptureBuffer(std::shared_ptr<IAudioBuffer> buffer)
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
    visualPresetId_ = presetId.isNotEmpty() ? presetId : "default";
}

void WaveformComponent::setVisualOverrides(const juce::ValueTree& overrides)
{
    visualOverrides_ = overrides.createCopy();
}

void WaveformComponent::setGpuRenderingEnabled(bool enabled)
{
    gpuRenderingEnabled_ = enabled;

    // Conditionally manage VBlank attachment to save CPU when GPU rendering is active
    // In GPU mode, WaveformGLRenderer handles updates at VBlank rate, so we don't need
    // this component to drive updateWaveformPath() or repaint() calls
    if (enabled)
    {
        // Disable VBlank attachment - GPU rendering handles updates
        vBlankAttachment_.reset();
    }
    else if (!vBlankAttachment_)
    {
        // Re-enable for software rendering
        vBlankAttachment_ = std::make_unique<juce::VBlankAttachment>(this, [this]() {
            updateWaveformPath();
            repaint();
        });
    }
}

void WaveformComponent::setGridConfig(const GridConfiguration& config)
{
    if (gridRenderer_)
        gridRenderer_->setGridConfig(config);
    repaint();
}

void WaveformComponent::setShowGrid(bool show)
{
    if (gridRenderer_)
        gridRenderer_->setShowGrid(show);
    repaint();
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

void WaveformComponent::drawWaveformWithShader(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Get shader from registry
    auto* shader = shaderRegistry_.getShader(shaderId_);
    if (!shader)
    {
        // Fallback to default shader
        shader = shaderRegistry_.getShader(
            shaderRegistry_.getDefaultShaderId());
    }

    if (!shader)
    {
        // Last resort: use standard path rendering
        drawWaveform(g, bounds);
        return;
    }

    if (!presenter_)
        return;

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
    float amplitude1, amplitude2; // Amplitude range for each channel

    bool isStereo = presenter_->isStereo();
    float effectiveScale = presenter_->getEffectiveScale();

    if (isStereo)
    {
        // Stereo stacked: L on top half, R on bottom half
        float halfHeight = static_cast<float>(height) * 0.5f;
        centerY1 = halfHeight * 0.5f;   // Center of top half (L channel)
        centerY2 = halfHeight * 1.5f;   // Center of bottom half (R channel)
        amplitude1 = halfHeight * 0.5f; // Amplitude fits in quarter height
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
    if (presenter_)
    {
        // Optimization: Don't send silence to GPU
        if (presenter_->getGainLinear() < 0.002f)
        {
            data.channel1.clear();
            data.channel2.clear();
        }
        else
        {
            data.channel1 = presenter_->getDisplayBuffer1();
            data.channel2 = presenter_->getDisplayBuffer2();
        }
        data.isStereo = presenter_->isStereo();
        data.verticalScale = presenter_->getEffectiveScale();
    }

    // Copy render parameters
    data.colour = colour_;
    data.opacity = opacity_;
    data.lineWidth = lineWidth_;
    data.visible = isVisible() && !data.channel1.empty();
    data.gridConfig = gridRenderer_ ? gridRenderer_->getGridConfig() : GridConfiguration{};

    // Copy grid colors from theme (thread-safe: this runs on message thread)
    const auto& theme = themeService_.getCurrentTheme();
    data.gridColors.gridMinor = theme.gridMinor;
    data.gridColors.gridMajor = theme.gridMajor;
    data.gridColors.gridZeroLine = theme.gridZeroLine;

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
ProcessingMode WaveformComponent::getProcessingMode() const
{
    return presenter_ ? presenter_->getProcessingMode() : ProcessingMode::FullStereo;
}

bool WaveformComponent::isGridVisible() const { return gridRenderer_ ? gridRenderer_->isGridVisible() : true; }
bool WaveformComponent::isAutoScaleEnabled() const { return presenter_ ? presenter_->isAutoScaleEnabled() : true; }
float WaveformComponent::getGainLinear() const { return presenter_ ? presenter_->getGainLinear() : 1.0f; }
int WaveformComponent::getDisplaySamples() const { return presenter_ ? presenter_->getDisplaySamples() : 2048; }
std::shared_ptr<IAudioBuffer> WaveformComponent::getCaptureBuffer() const { return presenter_ ? presenter_->getCaptureBuffer() : nullptr; }

} // namespace oscil
