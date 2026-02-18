/*
    Oscil - Waveform Presenter Implementation
*/

#include "ui/panels/WaveformPresenter.h"
#include <algorithm>
#include <cmath>

namespace oscil
{

WaveformPresenter::WaveformPresenter()
{
}

void WaveformPresenter::setCaptureBuffer(std::shared_ptr<IAudioBuffer> buffer)
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
    // Capture previous LOD tier before updating
    LODTier previousTier = decimator_.getCurrentTier();
    
    // Update decimator with new width (this may change the LOD tier)
    decimator_.setDisplayWidth(width);
    
    // Check if LOD tier changed
    LODTier newTier = decimator_.getCurrentTier();
    if (newTier != previousTier && lodTransitionProgress_ >= 1.0f)
    {
        // LOD tier changed - start a transition
        previousLODTier_ = previousTier;
        lodTransitionProgress_ = 0.0f;
        
        // Save current display buffers for cross-fade
        previousDisplayBuffer1_ = displayBuffer1_;
        previousDisplayBuffer2_ = displayBuffer2_;
    }
}

void WaveformPresenter::process()
{
    if (!captureBuffer_)
        return;

    // Read samples from capture buffer
    // Reuse member vectors to avoid reallocation in the timer callback loop
    if (scratchBufferLeft_.size() != static_cast<size_t>(displaySamples_))
        scratchBufferLeft_.resize(static_cast<size_t>(displaySamples_));
        
    if (scratchBufferRight_.size() != static_cast<size_t>(displaySamples_))
        scratchBufferRight_.resize(static_cast<size_t>(displaySamples_));

    int samplesReadLeft = captureBuffer_->read(scratchBufferLeft_.data(), displaySamples_, 0);
    int samplesReadRight = captureBuffer_->read(scratchBufferRight_.data(), displaySamples_, 1);

    if (samplesReadLeft <= 0)
        return;

    // Use minimum of both channels to ensure we only process valid samples
    int samplesRead = (samplesReadRight > 0) ? std::min(samplesReadLeft, samplesReadRight) : samplesReadLeft;

    // Apply gain to samples
    if (std::abs(gainLinear_ - 1.0f) > 1e-6f)
    {
        for (int i = 0; i < samplesRead; ++i)
        {
            scratchBufferLeft_[static_cast<size_t>(i)] *= gainLinear_;
            scratchBufferRight_[static_cast<size_t>(i)] *= gainLinear_;
        }
    }

    // Process samples according to mode
    std::span<const float> leftSpan(scratchBufferLeft_.data(), static_cast<size_t>(samplesRead));
    std::span<const float> rightSpan;
    if (samplesReadRight > 0)
    {
        rightSpan = std::span<const float>(scratchBufferRight_.data(), static_cast<size_t>(samplesRead));
    }

    signalProcessor_.process(leftSpan, rightSpan, processingMode_, processedSignal_);

    // Decimate for display with peak preservation
    std::span<const float> processedLeft(processedSignal_.channel1.data(), processedSignal_.channel1.size());
    decimator_.processWithPeaks(processedLeft, decimatedWaveform1_);
    
    // Copy samples to display buffer for backward compatibility
    displayBuffer1_ = decimatedWaveform1_.samples;

    if (processedSignal_.isStereo)
    {
        std::span<const float> processedRight(processedSignal_.channel2.data(), processedSignal_.channel2.size());
        decimator_.processWithPeaks(processedRight, decimatedWaveform2_);
        displayBuffer2_ = decimatedWaveform2_.samples;
    }
    else
    {
        displayBuffer2_.clear();
        decimatedWaveform2_.clear();
    }
    
    // Handle LOD transition blending
    if (lodTransitionProgress_ < 1.0f)
    {
        // Advance transition
        lodTransitionProgress_ += LOD_TRANSITION_SPEED;
        lodTransitionProgress_ = std::min(lodTransitionProgress_, 1.0f);
        
        // Cross-fade between previous and current buffers
        // This prevents visual "popping" when LOD changes
        if (!previousDisplayBuffer1_.empty())
        {
            float blend = lodTransitionProgress_;
            
            // Resample previous buffer to match new size if different
            if (previousDisplayBuffer1_.size() != displayBuffer1_.size())
            {
                // Simple case: just snap to new buffer since sizes differ
                // The hysteresis in AdaptiveDecimator prevents rapid tier changes
                // so this transition will be smooth enough
            }
            else
            {
                // Same size: blend samples
                for (size_t i = 0; i < displayBuffer1_.size(); ++i)
                {
                    displayBuffer1_[i] = previousDisplayBuffer1_[i] * (1.0f - blend) 
                                       + displayBuffer1_[i] * blend;
                }
                
                if (!previousDisplayBuffer2_.empty() && 
                    previousDisplayBuffer2_.size() == displayBuffer2_.size())
                {
                    for (size_t i = 0; i < displayBuffer2_.size(); ++i)
                    {
                        displayBuffer2_[i] = previousDisplayBuffer2_[i] * (1.0f - blend) 
                                           + displayBuffer2_[i] * blend;
                    }
                }
            }
        }
        
        // Clear previous buffers when transition completes
        if (lodTransitionProgress_ >= 1.0f)
        {
            previousDisplayBuffer1_.clear();
            previousDisplayBuffer2_.clear();
        }
    }

    // Calculate levels
    // Note: calculatePeak/RMS now take a span
    std::span<const float> outputSpan(processedSignal_.channel1.data(), static_cast<size_t>(processedSignal_.numSamples));
    currentPeak_ = SignalProcessor::calculatePeak(outputSpan);
    currentRMS_ = SignalProcessor::calculateRMS(outputSpan);

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
