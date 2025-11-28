/*
    Oscil - Waveform Component Implementation
*/

#include "ui/WaveformComponent.h"
#include "ui/ThemeManager.h"

namespace oscil
{

WaveformComponent::WaveformComponent()
{
    setOpaque(false);
}

void WaveformComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    const auto& theme = ThemeManager::getInstance().getCurrentTheme();

    // Draw background
    g.setColour(theme.backgroundPane);
    g.fillRect(bounds);

    // Draw highlight border if selected
    if (highlighted_)
    {
        g.setColour(theme.controlActive.withAlpha(0.8f));
        g.drawRect(bounds.toFloat(), 2.0f);
    }

    // Draw grid
    if (showGrid_)
    {
        drawGrid(g, bounds);
    }

    // Update and draw waveform
    updateWaveformPath();
    drawWaveform(g, bounds);

    // If no waveform data, show "No Signal" text
    if (waveformPath1_.isEmpty() && bounds.getWidth() > 50 && bounds.getHeight() > 30)
    {
        g.setColour(theme.textSecondary.withAlpha(0.5f));
        g.setFont(14.0f);
        g.drawText("No Signal", bounds, juce::Justification::centred);
    }
}

void WaveformComponent::resized()
{
    decimator_.setDisplayWidth(getWidth());
}

void WaveformComponent::setCaptureBuffer(std::shared_ptr<SharedCaptureBuffer> buffer)
{
    captureBuffer_ = buffer;
}

void WaveformComponent::setProcessingMode(ProcessingMode mode)
{
    processingMode_ = mode;
}

void WaveformComponent::setColour(juce::Colour colour)
{
    colour_ = colour;
}

void WaveformComponent::setOpacity(float opacity)
{
    opacity_ = juce::jlimit(0.0f, 1.0f, opacity);
}

void WaveformComponent::setDisplaySamples(int samples)
{
    displaySamples_ = std::max(64, samples);
}

void WaveformComponent::setShowGrid(bool show)
{
    showGrid_ = show;
}

void WaveformComponent::setAutoScale(bool enabled)
{
    autoScale_ = enabled;
}

void WaveformComponent::setHoldDisplay(bool enabled)
{
    holdDisplay_ = enabled;
}

void WaveformComponent::setGainDb(float dB)
{
    // Clamp to reasonable range: -60 dB to +24 dB
    dB = juce::jlimit(-60.0f, 24.0f, dB);
    // Convert dB to linear: 10^(dB/20)
    gainLinear_ = std::pow(10.0f, dB / 20.0f);
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

void WaveformComponent::setVerticalScale(float scale)
{
    verticalScale_ = std::max(0.1f, scale);
}

void WaveformComponent::forceUpdateWaveformData()
{
    // Force the waveform path update which calculates peak/RMS levels
    // This is useful for testing to ensure data is processed before querying state
    updateWaveformPath();
}

void WaveformComponent::drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();

    int width = bounds.getWidth();
    int height = bounds.getHeight();

    if (processingMode_ == ProcessingMode::FullStereo)
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

void WaveformComponent::drawWaveform(juce::Graphics& g, juce::Rectangle<int> /*bounds*/)
{
    if (!captureBuffer_)
        return;

    // Draw first channel (L or mono)
    if (!waveformPath1_.isEmpty())
    {
        g.setColour(colour_.withAlpha(opacity_));
        g.strokePath(waveformPath1_, juce::PathStrokeType(1.5f));
    }

    // Draw second channel for stereo mode (stacked: same color, channels visually separated)
    if (processingMode_ == ProcessingMode::FullStereo && !waveformPath2_.isEmpty())
    {
        g.setColour(colour_.withAlpha(opacity_));
        g.strokePath(waveformPath2_, juce::PathStrokeType(1.5f));
    }
}

void WaveformComponent::updateWaveformPath()
{
    // If hold display is enabled, don't update the waveform
    if (holdDisplay_)
        return;

    waveformPath1_.clear();
    waveformPath2_.clear();

    if (!captureBuffer_)
        return;

    int width = getWidth();
    int height = getHeight();

    if (width <= 0 || height <= 0)
        return;

    // Calculate center Y positions based on processing mode
    float centerY1, centerY2;
    float amplitude1, amplitude2;  // Amplitude range for each channel

    if (processingMode_ == ProcessingMode::FullStereo)
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

    // Read samples from capture buffer
    std::vector<float> leftSamples(static_cast<size_t>(displaySamples_));
    std::vector<float> rightSamples(static_cast<size_t>(displaySamples_));

    int samplesReadLeft = captureBuffer_->read(leftSamples.data(), displaySamples_, 0);
    int samplesReadRight = captureBuffer_->read(rightSamples.data(), displaySamples_, 1);

    if (samplesReadLeft <= 0)
        return;

    // Use minimum of both channels to ensure we only process valid samples
    int samplesRead = (samplesReadRight > 0) ? std::min(samplesReadLeft, samplesReadRight) : samplesReadLeft;

    // Apply gain to samples
    if (std::abs(gainLinear_ - 1.0f) > 1e-6f)
    {
        for (int i = 0; i < samplesRead; ++i)
        {
            leftSamples[static_cast<size_t>(i)] *= gainLinear_;
            rightSamples[static_cast<size_t>(i)] *= gainLinear_;
        }
    }

    // Process samples according to mode
    signalProcessor_.process(leftSamples.data(), rightSamples.data(), samplesRead,
                             processingMode_, processedSignal_);

    // Decimate for display
    decimator_.process(processedSignal_.channel1.data(),
                       static_cast<int>(processedSignal_.channel1.size()),
                       displayBuffer1_);

    if (processedSignal_.isStereo)
    {
        decimator_.process(processedSignal_.channel2.data(),
                           static_cast<int>(processedSignal_.channel2.size()),
                           displayBuffer2_);
    }

    // Calculate levels
    currentPeak_ = SignalProcessor::calculatePeak(processedSignal_.channel1.data(),
                                                   processedSignal_.numSamples);
    currentRMS_ = SignalProcessor::calculateRMS(processedSignal_.channel1.data(),
                                                 processedSignal_.numSamples);

    // Calculate effective vertical scale (apply autoScale if enabled)
    float effectiveScale = verticalScale_;
    if (autoScale_ && currentPeak_ > 0.001f)
    {
        // Target 80% of display height for the peak
        constexpr float targetFill = 0.8f;
        float autoScaleFactor = targetFill / currentPeak_;
        // Clamp to reasonable range to prevent extreme scaling
        autoScaleFactor = juce::jlimit(0.5f, 10.0f, autoScaleFactor);
        effectiveScale = autoScaleFactor;
    }

    // Build path for first channel (L or mono)
    // Need at least 2 samples to draw a line
    if (displayBuffer1_.size() >= 2)
    {
        float xScale = static_cast<float>(width) / static_cast<float>(displayBuffer1_.size() - 1);

        float yStart = centerY1 - displayBuffer1_[0] * amplitude1 * effectiveScale;
        waveformPath1_.startNewSubPath(0, yStart);

        for (size_t i = 1; i < displayBuffer1_.size(); ++i)
        {
            float x = static_cast<float>(i) * xScale;
            float y = centerY1 - displayBuffer1_[i] * amplitude1 * effectiveScale;

            // Clamp to appropriate region
            if (processingMode_ == ProcessingMode::FullStereo)
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
    if (processedSignal_.isStereo && displayBuffer2_.size() >= 2)
    {
        float xScale = static_cast<float>(width) / static_cast<float>(displayBuffer2_.size() - 1);

        float yStart = centerY2 - displayBuffer2_[0] * amplitude2 * effectiveScale;
        waveformPath2_.startNewSubPath(0, yStart);

        for (size_t i = 1; i < displayBuffer2_.size(); ++i)
        {
            float x = static_cast<float>(i) * xScale;
            float y = centerY2 - displayBuffer2_[i] * amplitude2 * effectiveScale;

            // Stereo stacked: R channel in bottom half
            y = juce::jlimit(static_cast<float>(height) * 0.5f, static_cast<float>(height), y);

            waveformPath2_.lineTo(x, y);
        }
    }
}

} // namespace oscil
