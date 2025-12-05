/*
    Oscil - Decimating Capture Buffer Implementation
    Uses JUCE 8's dsp::FIR::Filter for high-quality anti-aliasing
*/

#include "core/DecimatingCaptureBuffer.h"
#include <cmath>
#include <algorithm>

namespace oscil
{

//==============================================================================
// DecimationFilter Implementation
// Uses JUCE's FIR::Filter with Kaiser window for optimal anti-aliasing
//==============================================================================

DecimationFilter::DecimationFilter()
{
    // Create a passthrough filter by default
    coefficients_ = new juce::dsp::FIR::Coefficients<float>(1);
    coefficients_->getRawCoefficients()[0] = 1.0f;
    filter_.coefficients = coefficients_;
}

void DecimationFilter::configure(int decimationRatio, double sourceRate)
{
    decimationRatio_ = juce::jmax(1, decimationRatio);

    if (decimationRatio_ <= 1 || sourceRate <= 0)
    {
        // No filtering needed - passthrough
        coefficients_ = new juce::dsp::FIR::Coefficients<float>(1);
        coefficients_->getRawCoefficients()[0] = 1.0f;
        filter_.coefficients = coefficients_;
        reset();
        return;
    }

    // Calculate cutoff frequency for anti-aliasing
    double targetRate = sourceRate / static_cast<double>(decimationRatio_);
    double targetNyquist = targetRate / 2.0;
    double cutoffFrequency = targetNyquist * 0.9;

    double sourceNyquist = sourceRate / 2.0;
    double transitionBand = targetNyquist - cutoffFrequency;

    float normalisedTransitionWidth = static_cast<float>(
        juce::jlimit(0.01, 0.49, transitionBand / sourceNyquist));

    coefficients_ = juce::dsp::FilterDesign<float>::designFIRLowpassKaiserMethod(
        static_cast<float>(cutoffFrequency),
        sourceRate,
        normalisedTransitionWidth,
        STOPBAND_ATTENUATION_DB
    );

    filter_.coefficients = coefficients_;
    reset();
    // Logging removed for real-time safety (configure may be called from audio-adjacent paths)
}

float DecimationFilter::processSample(float sample)
{
    return filter_.processSample(sample);
}

void DecimationFilter::reset()
{
    filter_.reset();
}

size_t DecimationFilter::getFilterOrder() const
{
    if (coefficients_)
        return coefficients_->getFilterOrder();
    return 0;
}

size_t DecimationFilter::getMemoryUsageBytes() const
{
    size_t bytes = 0;

    if (coefficients_)
    {
        bytes += (coefficients_->getFilterOrder() + 1) * sizeof(float);
        bytes += (coefficients_->getFilterOrder() + 1) * sizeof(float);
    }

    return bytes;
}

//==============================================================================
// DecimatingCaptureBuffer Implementation
//==============================================================================

DecimatingCaptureBuffer::DecimatingCaptureBuffer()
    : buffer_(std::make_shared<SharedCaptureBuffer>())
    , context_(std::make_shared<ProcessingContext>())
{
    reconfigure();
}

DecimatingCaptureBuffer::DecimatingCaptureBuffer(const CaptureQualityConfig& config, int sourceRate)
    : config_(config)
    , sourceRate_(sourceRate)
    , context_(std::make_shared<ProcessingContext>())
{
    reconfigure();
}

void DecimatingCaptureBuffer::configure(const CaptureQualityConfig& config, int sourceRate)
{
    config_ = config;
    sourceRate_ = sourceRate;
    reconfigure();
}

void DecimatingCaptureBuffer::setQualityPreset(QualityPreset preset, int sourceRate)
{
    config_.qualityPreset = preset;
    sourceRate_ = sourceRate;
    reconfigure();
}

void DecimatingCaptureBuffer::reconfigure()
{
    // Reconfigure should only be called from the message thread to avoid
    // potential allocation on the audio thread via graveyard_.push_back()
    jassert(!juce::MessageManager::existsAndIsCurrentThread() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread() ||
            !juce::MessageManager::getInstanceWithoutCreating());

    // Pre-allocate graveyard to avoid allocation during reconfigure
    if (graveyard_.capacity() < 10)
        graveyard_.reserve(10);

    // Calculate capture rate and decimation ratio
    captureRate_ = config_.getCaptureRate(sourceRate_);
    decimationRatio_ = config_.getDecimationRatio(sourceRate_);

    // Calculate required buffer size
    size_t bufferSamples = config_.calculateBufferSizeSamples(captureRate_);

    // Round up to next power of 2 for efficient ring buffer
    // Guard against overflow: stop when powerOf2Size would overflow
    size_t powerOf2Size = 1;
    while (powerOf2Size < bufferSamples && powerOf2Size <= (SIZE_MAX / 2))
        powerOf2Size *= 2;

    // Create new buffer with appropriate size
    auto newBuffer = std::make_shared<SharedCaptureBuffer>(powerOf2Size);
    
    // Create new processing context
    auto newContext = std::make_shared<ProcessingContext>();
    
    // Configure filters in the new context
    for (auto& filter : newContext->filters)
    {
        filter.configure(decimationRatio_, static_cast<double>(sourceRate_));
    }

    // Reset decimation counters
    std::fill(newContext->decimationCounters.begin(), newContext->decimationCounters.end(), 0);

    // Resize scratch buffer to accommodate worst-case block size
    static constexpr size_t MAX_EXPECTED_BLOCK_SIZE = 8192;
    newContext->scratchBuffer.resize(SharedCaptureBuffer::MAX_CHANNELS * MAX_EXPECTED_BLOCK_SIZE);

    // Calculate filter memory usage
    newContext->filterMemoryBytes = 0;
    for (const auto& filter : newContext->filters)
    {
        newContext->filterMemoryBytes += filter.getMemoryUsageBytes();
    }

    {
        // Protect swap with lock
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        
        // CRITICAL: Prevent destruction on audio thread
        // We move the OLD resources to a graveyard to be cleaned up later
        // when we are sure the audio thread is done with them.
        if (buffer_ || context_)
        {
            graveyard_.push_back({buffer_, context_, juce::Time::getMillisecondCounterHiRes()});
        }

        buffer_ = newBuffer;
        context_ = newContext;
    }

    cleanUpGarbage();
}

void DecimatingCaptureBuffer::cleanUpGarbage()
{
    double now = juce::Time::getMillisecondCounterHiRes();
    // Keep items for at least 2 seconds to be safe
    // This ensures that even if the audio thread held a reference just before the swap,
    // it will have finished its block long before we delete the object.
    static constexpr double RETENTION_MS = 2000.0;

    // Remove old items
    graveyard_.erase(
        std::remove_if(graveyard_.begin(), graveyard_.end(),
            [now](const GraveyardItem& item) {
                return (now - item.timestampMs) > RETENTION_MS;
            }),
        graveyard_.end());
}

void DecimatingCaptureBuffer::write(const juce::AudioBuffer<float>& buffer,
                                     const CaptureFrameMetadata& metadata)
{
    const int numChannels = juce::jmin(buffer.getNumChannels(),
                                        static_cast<int>(SharedCaptureBuffer::MAX_CHANNELS));
    const int numSamples = buffer.getNumSamples();

    const float* channels[SharedCaptureBuffer::MAX_CHANNELS] = { nullptr };
    for (int ch = 0; ch < numChannels; ++ch)
    {
        channels[ch] = buffer.getReadPointer(ch);
    }

    write(channels, numSamples, numChannels, metadata);
}

void DecimatingCaptureBuffer::write(const float* const* samples, int numSamples, int numChannels,
                                     const CaptureFrameMetadata& metadata)
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    std::shared_ptr<ProcessingContext> ctx;

    {
        // CRITICAL: Use tryLock for real-time safety - audio thread must never block
        // If reconfigure() is holding the lock (allocating memory), we drop this frame
        const juce::SpinLock::ScopedTryLockType sl(bufferSwapLock_);
        if (!sl.isLocked())
            return;  // Drop frame rather than block audio thread
        buf = buffer_;
        ctx = context_;
    }

    if (numSamples <= 0 || numChannels <= 0 || !buf || !ctx)
        return;

    const int actualChannels = juce::jmin(numChannels,
                                           static_cast<int>(SharedCaptureBuffer::MAX_CHANNELS));

    if (decimationRatio_ <= 1)
    {
        // No decimation - write directly
        buf->write(samples, numSamples, numChannels, metadata, true);
        return;
    }

    // Process with decimation
    processAndWriteDecimated(samples, numSamples, actualChannels, metadata);
}

void DecimatingCaptureBuffer::processAndWriteDecimated(const float* const* samples,
                                                        int numSamples, int numChannels,
                                                        const CaptureFrameMetadata& metadata)
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    std::shared_ptr<ProcessingContext> ctx;

    {
        // CRITICAL: Use tryLock for real-time safety - audio thread must never block
        const juce::SpinLock::ScopedTryLockType sl(bufferSwapLock_);
        if (!sl.isLocked())
            return;  // Drop frame rather than block audio thread
        buf = buffer_;
        ctx = context_;
    }

    if (!buf || !ctx) return;

    // Safety check: Ensure scratch buffer is large enough
    const size_t maxScratchPerChannel = ctx->scratchBuffer.size() / SharedCaptureBuffer::MAX_CHANNELS;
    const size_t maxSamplesFromScratch = maxScratchPerChannel * static_cast<size_t>(decimationRatio_);
    const int safeNumSamples = std::min(numSamples, static_cast<int>(maxSamplesFromScratch));

    int decimatedCount = 0;

    // Use pointers to scratch buffer segments
    float* scratchPtrs[SharedCaptureBuffer::MAX_CHANNELS];
    for (int ch = 0; ch < numChannels; ++ch)
    {
        scratchPtrs[ch] = ctx->scratchBuffer.data() + (static_cast<size_t>(ch) * maxScratchPerChannel);
    }
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        int writeIdx = 0;
        size_t chIdx = static_cast<size_t>(ch);
        
        // Use filter and counter from the context
        auto& filter = ctx->filters[chIdx];
        int& counter = ctx->decimationCounters[chIdx];
        float* dest = scratchPtrs[ch];
        const float* src = samples[ch]; 
        
        if (src)
        {
            for (int i = 0; i < safeNumSamples; ++i)
            {
                float inputSample = src[i];
                float filteredSample = filter.processSample(inputSample);
                
                counter++;
                if (counter >= decimationRatio_)
                {
                    dest[writeIdx++] = filteredSample;
                    counter = 0;
                }
            }
        }
        else
        {
            for (int i = 0; i < safeNumSamples; ++i)
            {
                float filteredSample = filter.processSample(0.0f);
                counter++;
                if (counter >= decimationRatio_)
                {
                    dest[writeIdx++] = filteredSample;
                    counter = 0;
                }
            }
        }
        
        if (ch == 0) decimatedCount = writeIdx;
    }

    // Write decimated samples to downstream buffer
    if (decimatedCount > 0)
    {
        CaptureFrameMetadata decimatedMeta = metadata;
        decimatedMeta.numSamples = decimatedCount;
        decimatedMeta.sampleRate = static_cast<double>(captureRate_);

        buf->write(const_cast<const float**>(scratchPtrs), decimatedCount, numChannels, decimatedMeta, true);
    }
}

int DecimatingCaptureBuffer::read(float* output, int numSamples, int channel) const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return 0;
    return buf->read(output, numSamples, channel);
}

int DecimatingCaptureBuffer::read(std::span<float> output, int channel) const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return 0;
    return buf->read(output, channel);
}

int DecimatingCaptureBuffer::read(juce::AudioBuffer<float>& output, int numSamples) const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return 0;
    return buf->read(output, numSamples);
}

CaptureFrameMetadata DecimatingCaptureBuffer::getLatestMetadata() const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return {};
    return buf->getLatestMetadata();
}

size_t DecimatingCaptureBuffer::getCapacity() const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return 0;
    return buf->getCapacity();
}

size_t DecimatingCaptureBuffer::getAvailableSamples() const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return 0;
    return buf->getAvailableSamples();
}

float DecimatingCaptureBuffer::getPeakLevel(int channel, int numSamples) const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return 0.0f;
    return buf->getPeakLevel(channel, numSamples);
}

float DecimatingCaptureBuffer::getRMSLevel(int channel, int numSamples) const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return 0.0f;
    return buf->getRMSLevel(channel, numSamples);
}

size_t DecimatingCaptureBuffer::getMemoryUsageBytes() const
{
    size_t total = 0;

    std::shared_ptr<SharedCaptureBuffer> buf;
    std::shared_ptr<ProcessingContext> ctx;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
        ctx = context_;
    }

    if (buf)
    {
        total += buf->getCapacity() * SharedCaptureBuffer::MAX_CHANNELS * sizeof(float);
    }

    if (ctx)
    {
        // Filter state
        total += ctx->filterMemoryBytes;

        // Scratch buffer
        total += ctx->scratchBuffer.capacity() * sizeof(float);
    }

    return total;
}

juce::String DecimatingCaptureBuffer::getMemoryUsageString() const
{
    size_t bytes = getMemoryUsageBytes();

    if (bytes >= 1024 * 1024)
    {
        float mb = static_cast<float>(bytes) / (1024.0f * 1024.0f);
        return juce::String(mb, 1) + " MB";
    }
    else if (bytes >= 1024)
    {
        float kb = static_cast<float>(bytes) / 1024.0f;
        return juce::String(kb, 0) + " KB";
    }
    else
    {
        return juce::String(bytes) + " B";
    }
}

void DecimatingCaptureBuffer::clear()
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    std::shared_ptr<ProcessingContext> ctx;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
        ctx = context_;
    }

    if (buf)
        buf->clear();

    if (ctx)
    {
        for (auto& filter : ctx->filters)
            filter.reset();

        std::fill(ctx->decimationCounters.begin(), ctx->decimationCounters.end(), 0);
    }
}

std::shared_ptr<SharedCaptureBuffer> DecimatingCaptureBuffer::getInternalBuffer() const
{
    const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
    return buffer_;
}

} // namespace oscil