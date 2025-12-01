/*
    Oscil - Waveform Presenter Implementation
*/

#include "ui/presenters/WaveformPresenter.h"
#include <algorithm>
#include <cmath>

namespace oscil
{

WaveformPresenter::WaveformPresenter()
{
}

void WaveformPresenter::setCaptureBuffer(std::shared_ptr<SharedCaptureBuffer> buffer)
{
    captureBuffer_ = buffer;
}

void WaveformPresenter::setProcessingMode(ProcessingMode mode)
{
    processingMode_ = mode;
}

void WaveformPresenter::setDisplaySamples(int samples)
{
    displaySamples_ = std::max(64, samples);
}

void WaveformPresenter::setGainDb(float dB)
{
    // Clamp to reasonable range: -60 dB to +24 dB
    dB = juce::jlimit(-60.0f, 24.0f, dB);
    // Convert dB to linear: 10^(dB/20)
    gainLinear_ = std::pow(10.0f, dB / 20.0f);
}

void WaveformPresenter::setAutoScale(bool enabled)
{
    autoScale_ = enabled;
}

void WaveformPresenter::setDisplayWidth(int width)
{
    decimator_.setDisplayWidth(width);
}

void WaveformPresenter::process()
{
    if (!captureBuffer_)
        return;

    // Read samples from capture buffer
    // Use stack vectors for reading to avoid reallocation if possible, or member variable
    // For thread safety/reentrancy stack is better, but allocation in audio/timer callback?
    // This runs on timer callback (Message Thread), so allocation is okay-ish, but vectors are better reused.
    // We'll use temporary vectors.
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

    // Calculate target scale based on mode
    float targetScale = 1.0f;
    
    if (autoScale_ && currentPeak_ > 0.001f)
    {
        // Target 90% of display height for the peak
        constexpr float targetFill = 0.9f;
        float autoScaleFactor = targetFill / currentPeak_;
        // Clamp to reasonable range to prevent extreme scaling
        targetScale = juce::jlimit(0.5f, 10.0f, autoScaleFactor);
    }

    // Apply smoothing to effectiveScale_
    if (!autoScale_)
    {
        // In manual mode, respond instantly
        effectiveScale_ = targetScale;
    }
    else
    {
        // Asymmetric smoothing for auto-scale
        if (targetScale < effectiveScale_)
        {
            // Signal is getting LOUDER (scale needs to shrink)
            // Respond fast to prevent visual clipping
            effectiveScale_ += (targetScale - effectiveScale_) * 0.2f;
        }
        else
        {
            // Signal is getting QUIETER (scale needs to grow)
            // Respond slow to prevent jitter and pumping
            effectiveScale_ += (targetScale - effectiveScale_) * 0.02f;
        }

        // Snap to target if very close to stop micro-updates
        if (std::abs(targetScale - effectiveScale_) < 0.001f)
            effectiveScale_ = targetScale;
    }
}

} // namespace oscil
