/*
    Oscil - Waveform Component Implementation
*/

#include "ui/panels/WaveformComponent.h"

#include "ui/theme/ThemeManager.h"

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
        float const peakLevel = waveform_.getPeakLevel();
        float const rmsLevel = waveform_.getRMSLevel();

        juce::String peakDb;
        juce::String rmsDb;
        if (peakLevel > 0.0001f)
        {
            float const peakDbValue = 20.0f * std::log10(peakLevel);
            peakDb = juce::String(peakDbValue, 1) + " dB";
        }
        else
        {
            peakDb = "-inf dB";
        }

        if (rmsLevel > 0.0001f)
        {
            float const rmsDbValue = 20.0f * std::log10(rmsLevel);
            rmsDb = juce::String(rmsDbValue, 1) + " dB";
        }
        else
        {
            rmsDb = "-inf dB";
        }

        return "Peak: " + peakDb + ", RMS: " + rmsDb;
    }

    juce::String getHelp() const override { return "Audio waveform visualization. Peak and RMS levels are announced."; }

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
    , waveformId_(nextWaveformId_.fetch_add(1))
{
    setOpaque(false);
    // Auto-assign a unique ID

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
        ProcessingMode const mode = presenter_ ? presenter_->getProcessingMode() : ProcessingMode::FullStereo;
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
        ProcessingMode const mode = presenter_ ? presenter_->getProcessingMode() : ProcessingMode::FullStereo;
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

void WaveformComponent::setColour(juce::Colour colour) { colour_ = colour; }

void WaveformComponent::setOpacity(float opacity) { opacity_ = juce::jlimit(0.0f, 1.0f, opacity); }

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

void WaveformComponent::setShaderId(const juce::String& shaderId) { shaderId_ = shaderId; }

void WaveformComponent::setVisualPresetId(const juce::String& presetId)
{
    visualPresetId_ = presetId.isNotEmpty() ? presetId : "default";
}

void WaveformComponent::setVisualOverrides(const juce::ValueTree& overrides)
{
    visualOverrides_ = overrides.createCopy();
}

void WaveformComponent::setGpuRenderingEnabled(bool enabled) { gpuRenderingEnabled_ = enabled; }

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

void WaveformComponent::setHoldDisplay(bool enabled) { holdDisplay_ = enabled; }

void WaveformComponent::setGainDb(float dB)
{
    if (presenter_)
        presenter_->setGainDb(dB);
}

void WaveformComponent::requestRestartAtTimestamp(int64_t timelineSampleTimestamp)
{
    if (presenter_)
        presenter_->requestRestartAtTimestamp(timelineSampleTimestamp);
}

void WaveformComponent::setHighlighted(bool highlighted)
{
    highlighted_ = highlighted;
    repaint();
}

void WaveformComponent::setStereoDisplayMode(StereoDisplayMode mode) { stereoDisplayMode_ = mode; }

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

// drawWaveformWithShader, drawWaveform, and updateWaveformPath
// are in WaveformComponentRendering.cpp

void WaveformComponent::setWaveformId(int id) { waveformId_ = id; }

juce::Component* WaveformComponent::findEditorAncestor() const
{
    for (auto* p = getParentComponent(); p != nullptr; p = p->getParentComponent())
    {
        if (dynamic_cast<juce::AudioProcessorEditor*>(p) != nullptr)
            return p;
    }
    return nullptr;
}

void WaveformComponent::populateGLRenderData(WaveformRenderData& data) const
{
#if OSCIL_ENABLE_OPENGL
    data.id = waveformId_;

    auto* editorComponent = findEditorAncestor();
    data.bounds =
        (editorComponent != nullptr ? editorComponent->getLocalArea(this, getLocalBounds()) : getLocalBounds())
            .toFloat();

    if (presenter_)
    {
        data.channel1 = presenter_->getDisplayBuffer1();
        data.channel2 = presenter_->getDisplayBuffer2();
        data.isStereo = presenter_->isStereo();
        data.verticalScale = presenter_->getEffectiveScale();
    }

    data.colour = colour_;
    data.opacity = opacity_;
    data.lineWidth = lineWidth_;
    data.visible = isVisible();
    data.gridConfig = gridRenderer_ ? gridRenderer_->getGridConfig() : GridConfiguration{};

    const auto& theme = themeService_.getCurrentTheme();
    data.gridColors.gridMinor = theme.gridMinor;
    data.gridColors.gridMajor = theme.gridMajor;
    data.gridColors.gridZeroLine = theme.gridZeroLine;

    data.visualConfig = VisualConfiguration::getPreset(visualPresetId_);

    // Visual overrides reserved for future per-oscillator customization.
    // Currently no UI sets overrides, so the ValueTree is always empty.
    juce::ignoreUnused(visualOverrides_);
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
std::shared_ptr<IAudioBuffer> WaveformComponent::getCaptureBuffer() const
{
    return presenter_ ? presenter_->getCaptureBuffer() : nullptr;
}

} // namespace oscil
