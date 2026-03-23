/*
    Oscil - Waveform Presenter Implementation
*/

#include "ui/panels/WaveformPresenter.h"

#include "core/SharedCaptureBuffer.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace oscil
{

WaveformPresenter::WaveformPresenter() {}

void WaveformPresenter::setCaptureBuffer(std::shared_ptr<IAudioBuffer> buffer) { captureBuffer_ = buffer; }

void WaveformPresenter::setProcessingMode(ProcessingMode mode) { processingMode_ = mode; }

void WaveformPresenter::setDisplaySamples(int samples) { displaySamples_ = std::max(64, samples); }

void WaveformPresenter::setGainDb(float dB)
{
    dB = juce::jlimit(-60.0f, 24.0f, dB);
    gainLinear_ = std::pow(10.0f, dB / 20.0f);
}

void WaveformPresenter::setAutoScale(bool enabled) { autoScale_ = enabled; }

void WaveformPresenter::setDisplayWidth(int width) { decimator_.setDisplayWidth(width); }

void WaveformPresenter::requestRestartAtTimestamp(int64_t timelineSampleTimestamp)
{
    restartTimelineSample_ = timelineSampleTimestamp;
}

std::pair<int, bool> WaveformPresenter::resolveRequestedSamples()
{
    if (!restartTimelineSample_.has_value())
        return {displaySamples_, false};

    const auto metadata = captureBuffer_->getLatestMetadata();
    if (*restartTimelineSample_ <= 0)
    {
        *restartTimelineSample_ = metadata.timestamp + static_cast<int64_t>(juce::jmax(0, metadata.numSamples));
    }

    const int64_t frameEndSample = metadata.timestamp + static_cast<int64_t>(juce::jmax(0, metadata.numSamples));
    const int64_t availableSinceRestart = frameEndSample - *restartTimelineSample_;

    if (availableSinceRestart <= 0)
        return {0, false};

    if (availableSinceRestart < static_cast<int64_t>(displaySamples_))
        return {juce::jlimit(1, displaySamples_, static_cast<int>(availableSinceRestart)), true};

    restartTimelineSample_.reset();
    return {displaySamples_, false};
}

WaveformPresenter::ReadResult WaveformPresenter::readAndPadSamples(int requestedSamples, bool padTrailingSilence)
{
    if (scratchBufferLeft_.size() != static_cast<size_t>(displaySamples_))
        scratchBufferLeft_.resize(static_cast<size_t>(displaySamples_));

    if (scratchBufferRight_.size() != static_cast<size_t>(displaySamples_))
        scratchBufferRight_.resize(static_cast<size_t>(displaySamples_));

    // Use the multi-channel read to guarantee cross-channel epoch consistency.
    // The per-channel read() validates epochs independently, which can yield
    // L/R data from different write epochs under concurrent audio writes.
    juce::AudioBuffer<float> stereoScratch(2, requestedSamples);
    int samplesRead = captureBuffer_->read(stereoScratch, requestedSamples);

    if (samplesRead <= 0)
        return {};

    // Copy from the epoch-consistent stereoScratch into our scratch buffers
    std::memcpy(scratchBufferLeft_.data(), stereoScratch.getReadPointer(0),
                static_cast<size_t>(samplesRead) * sizeof(float));

    int samplesReadRight = 0;
    if (stereoScratch.getNumChannels() > 1)
    {
        std::memcpy(scratchBufferRight_.data(), stereoScratch.getReadPointer(1),
                    static_cast<size_t>(samplesRead) * sizeof(float));
        samplesReadRight = samplesRead;
    }

    if (padTrailingSilence && samplesRead < displaySamples_)
    {
        std::fill(scratchBufferLeft_.begin() + samplesRead, scratchBufferLeft_.end(), 0.0f);
        std::fill(scratchBufferRight_.begin() + samplesRead, scratchBufferRight_.end(), 0.0f);
        samplesRead = displaySamples_;
    }

    // Apply gain
    if (std::abs(gainLinear_ - 1.0f) > 1e-6f)
    {
        for (int i = 0; i < samplesRead; ++i)
        {
            scratchBufferLeft_[static_cast<size_t>(i)] *= gainLinear_;
            scratchBufferRight_[static_cast<size_t>(i)] *= gainLinear_;
        }
    }

    return {samplesRead, samplesReadRight};
}

void WaveformPresenter::processAndDecimate(int samplesRead, int samplesReadRight)
{
    std::span<const float> leftSpan(scratchBufferLeft_.data(), static_cast<size_t>(samplesRead));
    std::span<const float> rightSpan;
    if (samplesReadRight > 0)
        rightSpan = std::span<const float>(scratchBufferRight_.data(), static_cast<size_t>(samplesRead));

    signalProcessor_.process(leftSpan, rightSpan, processingMode_, processedSignal_);

    // Decimate for display
    std::span<const float> processedLeft(processedSignal_.channel1.data(), processedSignal_.channel1.size());
    decimator_.process(processedLeft, displayBuffer1_);

    if (processedSignal_.isStereo)
    {
        std::span<const float> processedRight(processedSignal_.channel2.data(), processedSignal_.channel2.size());
        decimator_.process(processedRight, displayBuffer2_);
    }
    else
    {
        displayBuffer2_.clear();
    }

    // Calculate levels
    std::span<const float> outputSpan(processedSignal_.channel1.data(),
                                      static_cast<size_t>(processedSignal_.numSamples));
    currentPeak_ = SignalProcessor::calculatePeak(outputSpan);
    currentRMS_ = SignalProcessor::calculateRMS(outputSpan);
}

void WaveformPresenter::updateAutoScale()
{
    float targetScale = 1.0f;

    if (autoScale_ && currentPeak_ > 0.001f)
    {
        constexpr float targetFill = 0.9f;
        float autoScaleFactor = targetFill / currentPeak_;
        targetScale = juce::jlimit(0.5f, 10.0f, autoScaleFactor);
    }

    if (!autoScale_)
    {
        effectiveScale_ = targetScale;
    }
    else
    {
        if (targetScale < effectiveScale_)
            effectiveScale_ += (targetScale - effectiveScale_) * 0.2f;
        else
            effectiveScale_ += (targetScale - effectiveScale_) * 0.02f;

        if (std::abs(targetScale - effectiveScale_) < 0.001f)
            effectiveScale_ = targetScale;
    }
}

void WaveformPresenter::process()
{
    auto clearDisplayState = [this]() {
        displayBuffer1_.clear();
        displayBuffer2_.clear();
        currentPeak_ = 0.0f;
        currentRMS_ = 0.0f;
    };

    if (!captureBuffer_)
    {
        clearDisplayState();
        return;
    }

    auto [requestedSamples, padTrailingSilence] = resolveRequestedSamples();
    if (requestedSamples <= 0)
    {
        clearDisplayState();
        return;
    }

    auto readResult = readAndPadSamples(requestedSamples, padTrailingSilence);
    if (readResult.samplesRead <= 0)
    {
        clearDisplayState();
        return;
    }

    processAndDecimate(readResult.samplesRead, readResult.rightChannelRead);
    updateAutoScale();
}

} // namespace oscil
