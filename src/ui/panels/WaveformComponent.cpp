/*
    Oscil - Waveform Component Implementation
*/

#include "ui/panels/WaveformComponent.h"
#include "ui/theme/ThemeManager.h"
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

    // Draw grid only in software mode (GPU mode renders grid via GridRenderer)
    if (showGrid_ && !gpuRenderingEnabled_)
    {
        drawGrid(g, bounds);
    }

    // In GPU mode, waveform is rendered by OpenGL (WaveformGLRenderer::renderOpenGL)
    // In software mode, draw waveform here using JUCE Graphics
    if (!gpuRenderingEnabled_)
    {
        // Update and draw waveform using CPU rendering (with shader support)
        updateWaveformPath();
        drawWaveformWithShader(g, bounds);
    }

    // Draw grid labels on top of everything (works in both GPU and software mode)
    // Labels are drawn using JUCE Graphics which renders on top of OpenGL content
    if (showGrid_ && bounds.getWidth() > 100 && bounds.getHeight() > 60)
    {
        drawGridLabels(g, bounds);
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

void WaveformComponent::setGridConfig(const GridConfiguration& config)
{
    gridConfig_ = config;
    showGrid_ = config.enabled;
    repaint();
}

void WaveformComponent::setShowGrid(bool show)
{
    showGrid_ = show;
    gridConfig_.enabled = show;
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

void WaveformComponent::drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();
    
    // Helper to draw a grid for a specific channel area
    auto drawChannelGrid = [&](juce::Rectangle<int> area)
    {
        int width = area.getWidth();
        int height = area.getHeight();
        float centerY = area.getCentreY(); // Center relative to parent, not area
        // Actually, getCentreY() returns y + height/2, which is correct for drawing in 'g' (relative to component)
        
        // --- Horizontal Lines ---
        
        // Minor horizontal lines (8 divisions)
        g.setColour(theme.gridMinor);
        const int numMinorLines = 8;
        for (int i = 1; i < numMinorLines; ++i)
        {
            float y = area.getY() + (static_cast<float>(i) / numMinorLines) * static_cast<float>(height);
            if (std::abs(y - centerY) > 1.0f) // Don't draw over zero line
                g.drawHorizontalLine(static_cast<int>(y), static_cast<float>(area.getX()), static_cast<float>(area.getRight()));
        }

        // Major horizontal lines (top/bottom quarters)
        g.setColour(theme.gridMajor);
        g.drawHorizontalLine(static_cast<int>(area.getY() + height * 0.25f), static_cast<float>(area.getX()), static_cast<float>(area.getRight()));
        g.drawHorizontalLine(static_cast<int>(area.getY() + height * 0.75f), static_cast<float>(area.getX()), static_cast<float>(area.getRight()));
        
        // Zero line
        g.setColour(theme.gridZeroLine);
        g.drawHorizontalLine(static_cast<int>(centerY), static_cast<float>(area.getX()), static_cast<float>(area.getRight()));
        
        // --- Vertical Lines ---
        
        if (gridConfig_.timingMode == TimingMode::TIME)
        {
            // Time-based grid
            float durationMs = gridConfig_.visibleDurationMs;
            if (durationMs <= 0.0001f) durationMs = 1.0f;

            float targetStep = durationMs / 8.0f;
            float magnitude = std::pow(10.0f, std::floor(std::log10(targetStep)));
            float normalizedStep = targetStep / magnitude;

            float stepSize;
            if (normalizedStep < 2.0f) stepSize = 1.0f * magnitude;
            else if (normalizedStep < 5.0f) stepSize = 2.0f * magnitude;
            else stepSize = 5.0f * magnitude;

            g.setColour(theme.gridMajor.withAlpha(0.5f));
            
            for (float t = stepSize; t < durationMs; t += stepSize)
            {
                float x = area.getX() + (t / durationMs) * static_cast<float>(width);
                g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
                
                // Labels (only if space permits and it's the top channel or mono)
                // For now, keeping it simple: labels might clutter if drawn on every grid
            }
        }
        else // MELODIC Mode - aligned with labels
        {
            int numDivisions = 4;
            bool isBarBased = false;
            int beatsPerBar = gridConfig_.timeSigNumerator;

            switch (gridConfig_.noteInterval)
            {
                // Single note intervals - show 4 subdivisions
                case NoteInterval::THIRTY_SECOND:
                case NoteInterval::SIXTEENTH:
                case NoteInterval::TWELFTH:
                case NoteInterval::EIGHTH:
                case NoteInterval::TRIPLET_EIGHTH:
                case NoteInterval::DOTTED_EIGHTH:
                case NoteInterval::QUARTER:
                case NoteInterval::TRIPLET_QUARTER:
                case NoteInterval::DOTTED_QUARTER:
                case NoteInterval::HALF:
                case NoteInterval::TRIPLET_HALF:
                case NoteInterval::DOTTED_HALF:
                    numDivisions = 4;
                    break;
                // Bar-based intervals
                case NoteInterval::WHOLE:
                    numDivisions = beatsPerBar;
                    isBarBased = true;
                    break;
                case NoteInterval::TWO_BARS:
                    numDivisions = 2;
                    isBarBased = true;
                    break;
                case NoteInterval::THREE_BARS:
                    numDivisions = 3;
                    isBarBased = true;
                    break;
                case NoteInterval::FOUR_BARS:
                    numDivisions = 4;
                    isBarBased = true;
                    break;
                case NoteInterval::EIGHT_BARS:
                    numDivisions = 8;
                    isBarBased = true;
                    break;
                default:
                    numDivisions = 4;
                    break;
            }

            float widthPerDiv = static_cast<float>(width) / static_cast<float>(numDivisions);

            // Draw major division lines
            for (int i = 1; i < numDivisions; ++i)
            {
                float x = area.getX() + i * widthPerDiv;

                if (isBarBased)
                    g.setColour(theme.gridMajor);
                else
                    g.setColour(theme.gridMajor.withAlpha(0.6f));

                g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
            }

            // For bar-based modes, add sub-beat lines within each bar
            if (isBarBased && gridConfig_.noteInterval >= NoteInterval::TWO_BARS && widthPerDiv > 40.0f)
            {
                int subBeatsPerDiv = beatsPerBar;
                float subBeatWidth = widthPerDiv / static_cast<float>(subBeatsPerDiv);
                g.setColour(theme.gridMinor.withAlpha(0.3f));

                for (int i = 0; i < numDivisions; ++i)
                {
                    float baseX = area.getX() + i * widthPerDiv;
                    for (int j = 1; j < subBeatsPerDiv; ++j)
                    {
                        float x = baseX + j * subBeatWidth;
                        g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
                    }
                }
            }
        }
    };

    ProcessingMode mode = presenter_ ? presenter_->getProcessingMode() : ProcessingMode::FullStereo;
    
    if (mode == ProcessingMode::FullStereo)
    {
        // Stereo: Two separate grids
        int halfHeight = bounds.getHeight() / 2;
        
        // Left Channel (Top)
        auto topBounds = bounds.removeFromTop(halfHeight);
        drawChannelGrid(topBounds);
        
        // Right Channel (Bottom)
        auto bottomBounds = bounds; // Remaining
        drawChannelGrid(bottomBounds);
        
        // Separator
        g.setColour(theme.gridMajor);
        g.drawHorizontalLine(topBounds.getBottom(), static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));
        
        // Labels
        g.setColour(theme.textSecondary.withAlpha(0.6f));
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        g.drawText("L", topBounds.removeFromLeft(20).removeFromTop(14).translated(4, 2), juce::Justification::centredLeft);
        g.drawText("R", bottomBounds.removeFromLeft(20).removeFromTop(14).translated(4, 2), juce::Justification::centredLeft);
    }
    else
    {
        // Mono: Single grid
        drawChannelGrid(bounds);
    }
}

juce::String WaveformComponent::formatTimeLabel(float ms)
{
    if (ms >= 1000.0f)
    {
        // Show in seconds for values >= 1s
        float seconds = ms / 1000.0f;
        if (seconds >= 10.0f)
            return juce::String(static_cast<int>(seconds)) + "s";
        else
            return juce::String(seconds, 1) + "s";
    }
    else if (ms >= 1.0f)
    {
        // Show in ms for values >= 1ms
        if (ms >= 100.0f)
            return juce::String(static_cast<int>(ms)) + "ms";
        else if (ms >= 10.0f)
            return juce::String(ms, 0) + "ms";
        else
            return juce::String(ms, 1) + "ms";
    }
    else
    {
        // Show in microseconds for very small values
        return juce::String(ms * 1000.0f, 0) + "us";
    }
}

float WaveformComponent::calculateGridStepSize(float totalDuration, int targetDivisions)
{
    if (totalDuration <= 0.0f || targetDivisions <= 0)
        return totalDuration;

    float targetStep = totalDuration / static_cast<float>(targetDivisions);

    // Find the order of magnitude
    float magnitude = std::pow(10.0f, std::floor(std::log10(targetStep)));
    if (magnitude <= 0.0f || !std::isfinite(magnitude))
        return targetStep;

    float normalizedStep = targetStep / magnitude;

    // Snap to nice values: 1, 2, 5, 10
    float niceStep;
    if (normalizedStep < 1.5f)
        niceStep = 1.0f * magnitude;
    else if (normalizedStep < 3.5f)
        niceStep = 2.0f * magnitude;
    else if (normalizedStep < 7.5f)
        niceStep = 5.0f * magnitude;
    else
        niceStep = 10.0f * magnitude;

    return niceStep;
}

void WaveformComponent::drawGridLabels(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();

    // Use a small, readable font for labels
    g.setFont(juce::FontOptions(9.0f));
    g.setColour(theme.textSecondary.withAlpha(0.7f));

    int width = bounds.getWidth();
    int height = bounds.getHeight();

    ProcessingMode mode = presenter_ ? presenter_->getProcessingMode() : ProcessingMode::FullStereo;
    bool isStereo = (mode == ProcessingMode::FullStereo);

    // Reserve space for amplitude labels on the left (avoid overlap)
    const int leftMargin = 22;

    // === TIME AXIS LABELS (bottom of waveform area) ===
    if (gridConfig_.timingMode == TimingMode::TIME)
    {
        float durationMs = gridConfig_.visibleDurationMs;
        if (durationMs <= 0.0001f)
            durationMs = 500.0f; // Default fallback

        // Calculate nice step size for ~6-8 labels
        float stepSize = calculateGridStepSize(durationMs, 6);

        // Draw "0" at the left edge (offset to avoid amplitude label overlap)
        g.drawText("0", bounds.getX() + leftMargin, bounds.getBottom() - 14, 30, 12,
                   juce::Justification::centredLeft);

        // Draw time labels at each step
        for (float t = stepSize; t < durationMs - stepSize * 0.1f; t += stepSize)
        {
            float xRatio = t / durationMs;
            int x = bounds.getX() + static_cast<int>(xRatio * static_cast<float>(width));

            juce::String label = formatTimeLabel(t);
            int labelWidth = 40;

            // Center the label on the grid line
            g.drawText(label, x - labelWidth / 2, bounds.getBottom() - 14, labelWidth, 12,
                       juce::Justification::centred);
        }

        // Draw total duration at right edge
        juce::String totalLabel = formatTimeLabel(durationMs);
        int totalLabelWidth = 50;
        g.drawText(totalLabel, bounds.getRight() - totalLabelWidth - 2, bounds.getBottom() - 14,
                   totalLabelWidth, 12, juce::Justification::centredRight);
    }
    else // MELODIC mode
    {
        // Determine the number of major divisions and what they represent
        int numDivisions = 4;  // Number of major grid divisions
        bool isBarBased = false;
        int beatsPerBar = gridConfig_.timeSigNumerator;

        switch (gridConfig_.noteInterval)
        {
            // Single note intervals - show subdivisions within the note
            case NoteInterval::THIRTY_SECOND:
            case NoteInterval::SIXTEENTH:
            case NoteInterval::TWELFTH:
                numDivisions = 4; // Show 4 subdivisions within the note
                break;
            case NoteInterval::EIGHTH:
            case NoteInterval::TRIPLET_EIGHTH:
            case NoteInterval::DOTTED_EIGHTH:
                numDivisions = 4;
                break;
            case NoteInterval::QUARTER:
            case NoteInterval::TRIPLET_QUARTER:
            case NoteInterval::DOTTED_QUARTER:
                numDivisions = 4; // 4 subdivisions of a quarter note (16ths)
                break;
            case NoteInterval::HALF:
            case NoteInterval::TRIPLET_HALF:
            case NoteInterval::DOTTED_HALF:
                numDivisions = 4; // 4 subdivisions of a half note (8ths)
                break;
            // Bar-based intervals - show beats or bars
            case NoteInterval::WHOLE:
                numDivisions = beatsPerBar; // Show beat divisions within the bar
                isBarBased = true;
                break;
            case NoteInterval::TWO_BARS:
                numDivisions = 2; // 2 bars = 2 divisions
                isBarBased = true;
                break;
            case NoteInterval::THREE_BARS:
                numDivisions = 3;
                isBarBased = true;
                break;
            case NoteInterval::FOUR_BARS:
                numDivisions = 4;
                isBarBased = true;
                break;
            case NoteInterval::EIGHT_BARS:
                numDivisions = 8;
                isBarBased = true;
                break;
            default:
                numDivisions = 4;
                break;
        }

        float widthPerDiv = static_cast<float>(width) / static_cast<float>(numDivisions);

        // Draw division labels if there's enough space
        if (widthPerDiv >= 25.0f)
        {
            for (int i = 0; i < numDivisions; ++i)
            {
                int x = bounds.getX() + static_cast<int>(i * widthPerDiv);

                juce::String label;
                if (isBarBased)
                {
                    if (gridConfig_.noteInterval == NoteInterval::WHOLE)
                    {
                        // For 1 bar, show beat numbers (1, 2, 3, 4)
                        label = juce::String(i + 1);
                    }
                    else
                    {
                        // For multi-bar, show bar numbers
                        label = juce::String(i + 1);
                    }
                }
                else
                {
                    // For sub-beat intervals, show fraction positions
                    // e.g., for quarter note with 4 divisions: 1, 2, 3, 4
                    label = juce::String(i + 1);
                }

                int labelWidth = 20;
                int xPos = (i == 0) ? x + leftMargin : x - labelWidth / 2;
                auto justify = (i == 0) ? juce::Justification::centredLeft : juce::Justification::centred;

                g.drawText(label, xPos, bounds.getBottom() - 14, labelWidth, 12, justify);
            }
        }

        // Note: BPM and note interval indicators removed per user request
        // Only X-axis (time/beat) and Y-axis (amplitude) labels are shown
    }

    // === AMPLITUDE AXIS LABELS (left side) ===
    g.setColour(theme.textSecondary.withAlpha(0.6f));

    auto drawAmplitudeLabels = [&](juce::Rectangle<int> area, bool showChannelLabel, const juce::String& channelLabel)
    {
        int centerY = area.getCentreY();

        // Draw +1 at top
        g.drawText("+1", area.getX() + 2, area.getY() + 2, 20, 12, juce::Justification::centredLeft);

        // Draw 0 at center
        g.drawText("0", area.getX() + 2, centerY - 6, 20, 12, juce::Justification::centredLeft);

        // Draw -1 at bottom (offset up to avoid overlap with time axis labels)
        g.drawText("-1", area.getX() + 2, area.getBottom() - 26, 20, 12, juce::Justification::centredLeft);

        // Draw channel label if stereo
        if (showChannelLabel && !channelLabel.isEmpty())
        {
            g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
            g.drawText(channelLabel, area.getRight() - 18, area.getY() + 2, 14, 12,
                       juce::Justification::centredRight);
            g.setFont(juce::FontOptions(9.0f)); // Reset font
        }
    };

    if (isStereo)
    {
        int halfHeight = height / 2;
        auto topBounds = bounds.withHeight(halfHeight);
        auto bottomBounds = bounds.withTrimmedTop(halfHeight);

        drawAmplitudeLabels(topBounds, true, "L");
        drawAmplitudeLabels(bottomBounds, true, "R");
    }
    else
    {
        drawAmplitudeLabels(bounds, false, "");
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
    data.gridConfig = gridConfig_;

    // Copy grid colors from theme (thread-safe: this runs on message thread)
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();
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
ProcessingMode WaveformComponent::getProcessingMode() const { 
    return presenter_ ? presenter_->getProcessingMode() : ProcessingMode::FullStereo; 
}

bool WaveformComponent::isAutoScaleEnabled() const { return presenter_ ? presenter_->isAutoScaleEnabled() : true; }
float WaveformComponent::getGainLinear() const { return presenter_ ? presenter_->getGainLinear() : 1.0f; }
int WaveformComponent::getDisplaySamples() const { return presenter_ ? presenter_->getDisplaySamples() : 2048; }
std::shared_ptr<SharedCaptureBuffer> WaveformComponent::getCaptureBuffer() const { return presenter_ ? presenter_->getCaptureBuffer() : nullptr; }

} // namespace oscil
