/*
    MultiScoper - Decimating Capture Buffer Implementation
    Uses JUCE 8's dsp::FIR::Filter for high-quality anti-aliasing
*/

#include "core/DecimatingCaptureBuffer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace multiscoper
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

    constexpr auto maxValue = static_cast<long double>(std::numeric_limits<int64_t>::max());
    if (scaled >= maxValue)
        return std::numeric_limits<int64_t>::max();

    return static_cast<int64_t>(std::llround(scaled));
}
} // namespace

//==============================================================================
// DecimationFilter Implementation
// Uses JUCE's FIR::Filter with Kaiser window for optimal anti-aliasing
//==============================================================================

DecimationFilter::DecimationFilter() : coefficients_(new juce::dsp::FIR::Coefficients<float>(1))
{
    // Create a passthrough filter by default

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
    double const targetRate = sourceRate / static_cast<double>(decimationRatio_);
    double const targetNyquist = targetRate / 2.0;
    double const cutoffFrequency = targetNyquist * 0.9;

    double const sourceNyquist = sourceRate / 2.0;
    double const transitionBand = targetNyquist - cutoffFrequency;

    auto const normalisedTransitionWidth = static_cast<float>(juce::jlimit(0.01, 0.49, transitionBand / sourceNyquist));

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

    size_t const bufferSamples = config_.calculateBufferSizeSamples(newCapRate);
    size_t powerOf2Size = 1;
    while (powerOf2Size < bufferSamples && powerOf2Size <= (SIZE_MAX / 2))
        powerOf2Size *= 2;

    auto newBuffer = std::make_shared<SharedCaptureBuffer>(powerOf2Size);
    auto newContext = createProcessingContext();

    double const timestamp = juce::Time::getMillisecondCounterHiRes();

    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);

        // Seqlock write: odd sequence signals "update in progress" to the
        // audio thread, which will drop the frame rather than read torn state.
        configSeq_.fetch_add(1, std::memory_order_release); // → odd

        // Publish rate atomics and raw pointers for the audio thread's
        // lock-free seqlock read path.
        decimationRatio_.store(newDecRatio, std::memory_order_relaxed);
        captureRate_.store(newCapRate, std::memory_order_relaxed);
        publishedBuffer_.store(newBuffer.get(), std::memory_order_relaxed);
        publishedContext_.store(newContext.get(), std::memory_order_relaxed);

        configSeq_.fetch_add(1, std::memory_order_release); // → even

        // Retire old shared_ptrs to the graveyard so the audio thread's raw
        // pointers remain valid until well after the next swap.
        if (buffer_ || context_)
        {
            graveyard_.push_back({.buffer = buffer_, .context = context_, .timestampMs = timestamp});
        }

        buffer_ = newBuffer;
        context_ = newContext;
    }

    cleanUpGarbage();
}

void DecimatingCaptureBuffer::cleanUpGarbage()
{
    double const now = juce::Time::getMillisecondCounterHiRes();
    // Keep items for at least 2 seconds to be safe
    // This ensures that even if the audio thread held a reference just before the swap,
    // it will have finished its block long before we delete the object.
    static constexpr double RETENTION_MS = 2000.0;

    // Remove old items
    const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
    std::erase_if(graveyard_, [now](const GraveyardItem& item) { return (now - item.timestampMs) > RETENTION_MS; });
}

void DecimatingCaptureBuffer::write(const juce::AudioBuffer<float>& buffer, const CaptureFrameMetadata& metadata)
{
    const int numChannels = juce::jmin(buffer.getNumChannels(), static_cast<int>(SharedCaptureBuffer::MAX_CHANNELS));
    const int numSamples = buffer.getNumSamples();

    const float* channels[SharedCaptureBuffer::MAX_CHANNELS] = {nullptr}; // NOLINT(misc-const-correctness)
    for (int ch = 0; ch < numChannels; ++ch)
    {
        channels[ch] = buffer.getReadPointer(ch);
    }

    write(channels, numSamples, numChannels, metadata);
}

void DecimatingCaptureBuffer::write(const float* const* samples, int numSamples, int numChannels,
                                    const CaptureFrameMetadata& metadata)
{
    // Lock-free seqlock read: snapshot published pointers and rates without
    // touching the SpinLock.  If reconfigure() is mid-update (odd sequence)
    // or finishes between our two reads (sequence mismatch), drop the frame.
    const uint32_t seq1 = configSeq_.load(std::memory_order_acquire);
    if ((seq1 & 1u) != 0u)
        return; // reconfigure() in progress — drop frame

    auto* buf = publishedBuffer_.load(std::memory_order_relaxed);
    auto* ctx = publishedContext_.load(std::memory_order_relaxed);
    int const decRatio = decimationRatio_.load(std::memory_order_relaxed);
    int const capRate = captureRate_.load(std::memory_order_relaxed);
    int const srcRate = sourceRate_.load(std::memory_order_relaxed);

    const uint32_t seq2 = configSeq_.load(std::memory_order_acquire);
    if (seq1 != seq2)
        return; // reconfigure() started or completed during snapshot — drop frame

    if (numSamples <= 0 || numChannels <= 0 || buf == nullptr || ctx == nullptr)
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
    processAndWriteDecimated(*buf, *ctx, samples, numSamples, actualChannels, metadata,
                             {.decimationRatio = decRatio, .captureRate = capRate, .sourceRate = srcRate});
}

int DecimatingCaptureBuffer::decimateChannel(const float* src, float* dest, DecimationFilter& filter, int& counter,
                                             int numSamples, int decRatio)
{
    int writeIdx = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        float const filtered = filter.processSample(src ? src[i] : 0.0f);
        if (++counter >= decRatio)
        {
            dest[writeIdx++] = filtered;
            counter = 0;
        }
    }
    return writeIdx;
}

void DecimatingCaptureBuffer::processAndWriteDecimated(SharedCaptureBuffer& buf, ProcessingContext& ctx,
                                                       const float* const* samples, int numSamples, int numChannels,
                                                       const CaptureFrameMetadata& metadata, const RateSnapshot& rates)
{
    jassert(rates.decimationRatio >= 1);
    if (rates.decimationRatio < 1)
        return;

    const size_t maxPerCh = ctx.scratchBuffer.size() / SharedCaptureBuffer::MAX_CHANNELS;
    const int safeSamples =
        std::min(numSamples, static_cast<int>(maxPerCh * static_cast<size_t>(rates.decimationRatio)));

    float* scratchPtrs[SharedCaptureBuffer::MAX_CHANNELS];
    int decimatedCount = 0;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        scratchPtrs[ch] = ctx.scratchBuffer.data() + (static_cast<size_t>(ch) * maxPerCh);
        int const count =
            decimateChannel(samples[ch], scratchPtrs[ch], ctx.filters[static_cast<size_t>(ch)],
                            ctx.decimationCounters[static_cast<size_t>(ch)], safeSamples, rates.decimationRatio);
        if (ch == 0)
            decimatedCount = count;
    }

    if (decimatedCount > 0)
    {
        CaptureFrameMetadata meta = metadata;
        meta.numSamples = decimatedCount;
        meta.sampleRate = static_cast<double>(rates.captureRate);
        meta.timestamp = scaleTimelineTimestamp(metadata.timestamp, rates.sourceRate, rates.captureRate);
        buf.write(const_cast<const float**>(scratchPtrs), decimatedCount, numChannels, meta, true);
    }
}

} // namespace multiscoper
