/*
    Oscil - Decimating Capture Buffer Implementation
    Uses JUCE 8's dsp::FIR::Filter for high-quality anti-aliasing
*/

#include "core/DecimatingCaptureBuffer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace oscil
{

namespace
{
int64_t scaleTimelineTimestamp(int64_t timestamp, int sourceRate, int targetRate)
{
    if (timestamp <= 0)
        return 0;

    if (sourceRate <= 0 || targetRate <= 0 || sourceRate == targetRate)
        return timestamp;

    const long double scaled = (static_cast<long double>(timestamp) * static_cast<long double>(targetRate)) /
                               static_cast<long double>(sourceRate);
    if (!std::isfinite(static_cast<double>(scaled)))
        return timestamp;
    if (scaled <= 0.0L)
        return 0;

    constexpr long double maxValue = static_cast<long double>(std::numeric_limits<int64_t>::max());
    if (scaled >= maxValue)
        return std::numeric_limits<int64_t>::max();

    return static_cast<int64_t>(std::llround(scaled));
}
} // namespace

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

    float normalisedTransitionWidth = static_cast<float>(juce::jlimit(0.01, 0.49, transitionBand / sourceNyquist));

    coefficients_ = juce::dsp::FilterDesign<float>::designFIRLowpassKaiserMethod(
        static_cast<float>(cutoffFrequency), sourceRate, normalisedTransitionWidth, STOPBAND_ATTENUATION_DB);

    filter_.coefficients = coefficients_;
    reset();
    // Logging removed for real-time safety (configure may be called from audio-adjacent paths)
}

float DecimationFilter::processSample(float sample) { return filter_.processSample(sample); }

void DecimationFilter::reset() { filter_.reset(); }

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
    // Precondition: configure allocates memory — must not be called from audio thread.
    // Some hosts (Pro Tools) call prepareToPlay from audio thread, which defers to message thread.
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());
    // Precondition: source rate must be positive for meaningful buffer sizing
    jassert(sourceRate > 0);

    config_ = config;
    sourceRate_.store(sourceRate, std::memory_order_relaxed);
    reconfigure();
}

void DecimatingCaptureBuffer::setQualityPreset(QualityPreset preset, int sourceRate)
{
    config_.qualityPreset = preset;
    sourceRate_.store(sourceRate, std::memory_order_relaxed);
    reconfigure();
}

std::shared_ptr<DecimatingCaptureBuffer::ProcessingContext> DecimatingCaptureBuffer::createProcessingContext()
{
    // Read atomics once on message thread — these are the authoritative values.
    const int srcRate = sourceRate_.load(std::memory_order_relaxed);
    const int decRatio = decimationRatio_.load(std::memory_order_relaxed);

    auto newContext = std::make_shared<ProcessingContext>();

    for (auto& filter : newContext->filters)
    {
        filter.configure(decRatio, static_cast<double>(srcRate));
    }

    std::fill(newContext->decimationCounters.begin(), newContext->decimationCounters.end(), 0);

    static constexpr size_t MAX_EXPECTED_BLOCK_SIZE = 8192;
    newContext->scratchBuffer.resize(SharedCaptureBuffer::MAX_CHANNELS * MAX_EXPECTED_BLOCK_SIZE);

    newContext->filterMemoryBytes = 0;
    for (const auto& filter : newContext->filters)
    {
        newContext->filterMemoryBytes += filter.getMemoryUsageBytes();
    }

    return newContext;
}

void DecimatingCaptureBuffer::reconfigure()
{
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    static constexpr size_t kGraveyardCapacity = 100;
    if (graveyard_.capacity() < kGraveyardCapacity)
        graveyard_.reserve(kGraveyardCapacity);

    const int srcRate = sourceRate_.load(std::memory_order_relaxed);
    const int newDecRatio = config_.getDecimationRatio(srcRate);
    const int newCapRate = (srcRate > 0 && newDecRatio > 0) ? juce::jmax(1, srcRate / newDecRatio)
                                                            : juce::jmax(1, config_.getCaptureRate(srcRate));

    // Store atomics before the lock — the SpinLock release provides the
    // happens-before edge that makes these visible to the audio thread.
    decimationRatio_.store(newDecRatio, std::memory_order_relaxed);
    captureRate_.store(newCapRate, std::memory_order_relaxed);

    size_t bufferSamples = config_.calculateBufferSizeSamples(newCapRate);
    size_t powerOf2Size = 1;
    while (powerOf2Size < bufferSamples && powerOf2Size <= (SIZE_MAX / 2))
        powerOf2Size *= 2;

    auto newBuffer = std::make_shared<SharedCaptureBuffer>(powerOf2Size);
    auto newContext = createProcessingContext();

    double timestamp = juce::Time::getMillisecondCounterHiRes();

    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);

        if (buffer_ || context_)
        {
            graveyard_.push_back({buffer_, context_, timestamp});
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
    const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
    graveyard_.erase(
        std::remove_if(graveyard_.begin(), graveyard_.end(),
                       [now](const GraveyardItem& item) { return (now - item.timestampMs) > RETENTION_MS; }),
        graveyard_.end());
}

void DecimatingCaptureBuffer::write(const juce::AudioBuffer<float>& buffer, const CaptureFrameMetadata& metadata)
{
    const int numChannels = juce::jmin(buffer.getNumChannels(), static_cast<int>(SharedCaptureBuffer::MAX_CHANNELS));
    const int numSamples = buffer.getNumSamples();

    const float* channels[SharedCaptureBuffer::MAX_CHANNELS] = {nullptr};
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
    int decRatio;
    int capRate;
    int srcRate;

    {
        // CRITICAL: Use tryLock for real-time safety - audio thread must never block
        // If reconfigure() is holding the lock (allocating memory), we drop this frame
        const juce::SpinLock::ScopedTryLockType sl(bufferSwapLock_);
        if (!sl.isLocked())
            return; // Drop frame rather than block audio thread
        buf = buffer_;
        ctx = context_;
        // Snapshot rates under lock for consistency with buf/ctx
        decRatio = decimationRatio_.load(std::memory_order_relaxed);
        capRate = captureRate_.load(std::memory_order_relaxed);
        srcRate = sourceRate_.load(std::memory_order_relaxed);
    }

    if (numSamples <= 0 || numChannels <= 0 || !buf || !ctx)
        return;

    const int actualChannels = juce::jmin(numChannels, static_cast<int>(SharedCaptureBuffer::MAX_CHANNELS));

    if (decRatio <= 1)
    {
        // No decimation - still normalize metadata into capture domain for consistency.
        CaptureFrameMetadata passthroughMeta = metadata;
        passthroughMeta.numSamples = numSamples;
        passthroughMeta.sampleRate = static_cast<double>(capRate);
        passthroughMeta.timestamp = scaleTimelineTimestamp(metadata.timestamp, srcRate, capRate);
        buf->write(samples, numSamples, actualChannels, passthroughMeta, true);
        return;
    }

    // Process with decimation — pass buf/ctx and snapshotted rates to avoid re-reading atomics
    processAndWriteDecimated(buf, ctx, samples, numSamples, actualChannels, metadata,
                             {.decimationRatio = decRatio, .captureRate = capRate, .sourceRate = srcRate});
}

int DecimatingCaptureBuffer::decimateChannel(const float* src, float* dest, DecimationFilter& filter, int& counter,
                                             int numSamples, int decRatio)
{
    int writeIdx = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        float filtered = filter.processSample(src ? src[i] : 0.0f);
        if (++counter >= decRatio)
        {
            dest[writeIdx++] = filtered;
            counter = 0;
        }
    }
    return writeIdx;
}

void DecimatingCaptureBuffer::processAndWriteDecimated(const std::shared_ptr<SharedCaptureBuffer>& buf,
                                                       const std::shared_ptr<ProcessingContext>& ctx,
                                                       const float* const* samples, int numSamples, int numChannels,
                                                       const CaptureFrameMetadata& metadata, const RateSnapshot& rates)
{
    if (!buf || !ctx)
        return;

    jassert(rates.decimationRatio >= 1);
    if (rates.decimationRatio < 1)
        return;

    const size_t maxPerCh = ctx->scratchBuffer.size() / SharedCaptureBuffer::MAX_CHANNELS;
    const int safeSamples =
        std::min(numSamples, static_cast<int>(maxPerCh * static_cast<size_t>(rates.decimationRatio)));

    float* scratchPtrs[SharedCaptureBuffer::MAX_CHANNELS];
    int decimatedCount = 0;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        scratchPtrs[ch] = ctx->scratchBuffer.data() + (static_cast<size_t>(ch) * maxPerCh);
        int count =
            decimateChannel(samples[ch], scratchPtrs[ch], ctx->filters[static_cast<size_t>(ch)],
                            ctx->decimationCounters[static_cast<size_t>(ch)], safeSamples, rates.decimationRatio);
        if (ch == 0)
            decimatedCount = count;
    }

    if (decimatedCount > 0)
    {
        CaptureFrameMetadata meta = metadata;
        meta.numSamples = decimatedCount;
        meta.sampleRate = static_cast<double>(rates.captureRate);
        meta.timestamp = scaleTimelineTimestamp(metadata.timestamp, rates.sourceRate, rates.captureRate);
        buf->write(const_cast<const float**>(scratchPtrs), decimatedCount, numChannels, meta, true);
    }
}

} // namespace oscil
