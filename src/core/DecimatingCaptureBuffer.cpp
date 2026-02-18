/*
    Oscil - Decimating Capture Buffer Implementation
    Lock-free capture with SIMD-optimized decimation filtering
*/

#include "core/DecimatingCaptureBuffer.h"
#include <algorithm>
#include <cmath>
#include <thread>

namespace oscil
{

//==============================================================================
// SIMDDecimationFilter Implementation
//==============================================================================

SIMDDecimationFilter::SIMDDecimationFilter()
{
    // Initialize with passthrough filter
    coefficients_ = new juce::dsp::FIR::Coefficients<float>(1);
    coefficients_->getRawCoefficients()[0] = 1.0f;
    
    for (auto& filter : filters_)
    {
        filter.coefficients = coefficients_;
    }
}

void SIMDDecimationFilter::configure(int decimationRatio, double sourceRate, int maxBlockSize)
{
    decimationRatio_ = juce::jmax(1, decimationRatio);
    decimationPhase_ = 0;

    if (decimationRatio_ <= 1 || sourceRate <= 0)
    {
        // No filtering needed - passthrough
        coefficients_ = new juce::dsp::FIR::Coefficients<float>(1);
        coefficients_->getRawCoefficients()[0] = 1.0f;
        
        for (auto& filter : filters_)
        {
            filter.coefficients = coefficients_;
        }
        
        scratchBuffer_.setSize(static_cast<int>(MAX_CHANNELS), maxBlockSize);
        reset();
        return;
    }

    // Calculate cutoff frequency for anti-aliasing
    double targetRate = sourceRate / static_cast<double>(decimationRatio_);
    double targetNyquist = targetRate / 2.0;
    double cutoffFrequency = targetNyquist * 0.9;  // 10% headroom

    double sourceNyquist = sourceRate / 2.0;
    double transitionBand = targetNyquist - cutoffFrequency;

    float normalisedTransitionWidth = static_cast<float>(
        juce::jlimit(0.01, 0.49, transitionBand / sourceNyquist));

    // Design FIR filter using Kaiser window (optimal for anti-aliasing)
    coefficients_ = juce::dsp::FilterDesign<float>::designFIRLowpassKaiserMethod(
        static_cast<float>(cutoffFrequency),
        sourceRate,
        normalisedTransitionWidth,
        STOPBAND_ATTENUATION_DB
    );

    // Apply coefficients to all channel filters
    for (auto& filter : filters_)
    {
        filter.coefficients = coefficients_;
    }

    // Allocate scratch buffer for in-place filtering
    scratchBuffer_.setSize(static_cast<int>(MAX_CHANNELS), maxBlockSize);
    
    reset();
}

int SIMDDecimationFilter::processBlock(const float* const* input, float* const* output,
                                        int numInputSamples, int numChannels)
{
    if (decimationRatio_ <= 1)
    {
        // No decimation - direct copy
        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (input[ch] && output[ch])
            {
                std::memcpy(output[ch], input[ch], 
                           static_cast<size_t>(numInputSamples) * sizeof(float));
            }
        }
        return numInputSamples;
    }

    const int actualChannels = std::min(numChannels, static_cast<int>(MAX_CHANNELS));
    
    // Ensure scratch buffer is large enough
    if (scratchBuffer_.getNumSamples() < numInputSamples)
    {
        // This should not happen if configure() was called correctly
        // But handle gracefully - just copy without filtering
        for (int ch = 0; ch < actualChannels; ++ch)
        {
            if (input[ch] && output[ch])
            {
                int outIdx = 0;
                for (int i = decimationPhase_; i < numInputSamples; i += decimationRatio_)
                {
                    output[ch][outIdx++] = input[ch][i];
                }
            }
        }
        int numOutput = (numInputSamples - decimationPhase_ + decimationRatio_ - 1) / decimationRatio_;
        decimationPhase_ = (decimationPhase_ + numInputSamples) % decimationRatio_;
        return numOutput;
    }

    // Copy input to scratch buffer for in-place filtering
    for (int ch = 0; ch < actualChannels; ++ch)
    {
        if (input[ch])
        {
            std::memcpy(scratchBuffer_.getWritePointer(ch), input[ch],
                       static_cast<size_t>(numInputSamples) * sizeof(float));
        }
        else
        {
            scratchBuffer_.clear(ch, 0, numInputSamples);
        }
    }

    // SIMD-optimized block processing for each channel
    // JUCE's FIR::Filter uses SIMD internally when processing blocks
    for (int ch = 0; ch < actualChannels; ++ch)
    {
        juce::dsp::AudioBlock<float> block(scratchBuffer_.getArrayOfWritePointers(), 
                                            static_cast<size_t>(actualChannels),
                                            static_cast<size_t>(numInputSamples));
        
        // Process single channel
        auto channelBlock = block.getSingleChannelBlock(static_cast<size_t>(ch));
        juce::dsp::ProcessContextReplacing<float> context(channelBlock);
        filters_[static_cast<size_t>(ch)].process(context);
    }

    // Decimate: pick every Nth sample starting from current phase
    int numOutput = 0;
    for (int i = decimationPhase_; i < numInputSamples; i += decimationRatio_)
    {
        for (int ch = 0; ch < actualChannels; ++ch)
        {
            if (output[ch])
            {
                output[ch][numOutput] = scratchBuffer_.getSample(ch, i);
            }
        }
        numOutput++;
    }

    // Update phase for seamless block boundaries
    decimationPhase_ = (decimationPhase_ + numInputSamples) % decimationRatio_;

    return numOutput;
}

void SIMDDecimationFilter::reset()
{
    for (auto& filter : filters_)
    {
        filter.reset();
    }
    decimationPhase_ = 0;
}

size_t SIMDDecimationFilter::getFilterOrder() const
{
    if (coefficients_)
        return coefficients_->getFilterOrder();
    return 0;
}

size_t SIMDDecimationFilter::getMemoryUsageBytes() const
{
    size_t bytes = 0;

    if (coefficients_)
    {
        // Coefficients storage
        bytes += (coefficients_->getFilterOrder() + 1) * sizeof(float);
        // Filter state per channel
        bytes += (coefficients_->getFilterOrder() + 1) * sizeof(float) * MAX_CHANNELS;
    }

    // Scratch buffer
    bytes += static_cast<size_t>(scratchBuffer_.getNumSamples() * 
                                  scratchBuffer_.getNumChannels()) * sizeof(float);

    return bytes;
}

//==============================================================================
// DecimatingCaptureBuffer Implementation
//==============================================================================

DecimatingCaptureBuffer::DecimatingCaptureBuffer()
{
    reconfigure();
}

DecimatingCaptureBuffer::DecimatingCaptureBuffer(const CaptureQualityConfig& config, int sourceRate)
    : config_(config)
    , sourceRate_(sourceRate)
{
    reconfigure();
}

DecimatingCaptureBuffer::~DecimatingCaptureBuffer()
{
    // Clean up active state
    CaptureState* state = activeState_.exchange(nullptr, std::memory_order_acquire);
    if (state)
    {
        // Wait for any in-flight audio callbacks
        while (state->epoch.load(std::memory_order_acquire) > 0)
        {
            std::this_thread::yield();
        }
        delete state;
    }

    // Clean up retired states
    retiredStates_.clear();
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

std::unique_ptr<CaptureState> DecimatingCaptureBuffer::createNewState()
{
    auto state = std::make_unique<CaptureState>();

    // Calculate decimation parameters
    int captureRate = config_.getCaptureRate(sourceRate_);
    int decimationRatio = config_.getDecimationRatio(sourceRate_);

    state->decimationRatio = decimationRatio;
    state->captureRate = captureRate;

    // Calculate buffer size
    size_t bufferSamples = config_.calculateBufferSizeSamples(captureRate);

    // Round up to power of 2
    size_t powerOf2Size = 1;
    while (powerOf2Size < bufferSamples && powerOf2Size <= (SIZE_MAX / 2))
        powerOf2Size *= 2;

    // Create buffer
    state->buffer = std::make_unique<SharedCaptureBuffer>(powerOf2Size);

    // Create and configure processing context
    state->context = std::make_unique<ProcessingContext>();
    
    state->context->filter.configure(decimationRatio, static_cast<double>(sourceRate_),
                                      static_cast<int>(MAX_EXPECTED_BLOCK_SIZE));

    // Allocate decimated output buffer
    int maxDecimatedSamples = static_cast<int>(MAX_EXPECTED_BLOCK_SIZE / 
                                                std::max(1, decimationRatio)) + 1;
    state->context->decimatedBuffer.setSize(static_cast<int>(SharedCaptureBuffer::MAX_CHANNELS),
                                             maxDecimatedSamples);

    state->context->updateMemoryUsage();

    return state;
}

void DecimatingCaptureBuffer::reconfigure()
{
    // Reconfigure must be called from message thread
    // Runtime check works in release builds (not just debug)
    auto* mm = juce::MessageManager::getInstanceWithoutCreating();
    if (mm != nullptr && !mm->isThisTheMessageThread())
    {
        jassertfalse;  // Debug assertion for development
        return;        // Runtime guard for release builds
    }

    // Create new state
    auto newState = createNewState();

    // Atomic swap - audio thread will see new state on next callback
    CaptureState* oldState = activeState_.exchange(newState.release(), 
                                                    std::memory_order_acq_rel);

    if (oldState)
    {
        // Wait for audio thread to finish with old state
        // This spin is brief - audio callbacks are typically < 10ms
        int spinCount = 0;
        while (oldState->epoch.load(std::memory_order_acquire) > 0)
        {
            if (++spinCount > 1000)
            {
                std::this_thread::yield();
                spinCount = 0;
            }
        }

        // Move to retired list for deferred cleanup
        double now = juce::Time::getMillisecondCounterHiRes();
        retiredStates_.push_back({std::unique_ptr<CaptureState>(oldState), now});
    }

    cleanUpGarbage();
}

void DecimatingCaptureBuffer::cleanUpGarbage()
{
    double now = juce::Time::getMillisecondCounterHiRes();
    static constexpr double RETENTION_MS = 2000.0;

    retiredStates_.erase(
        std::remove_if(retiredStates_.begin(), retiredStates_.end(),
            [now](const RetiredState& item) {
                return (now - item.retiredTimeMs) > RETENTION_MS;
            }),
        retiredStates_.end());
}

//==============================================================================
// Write Interface (Audio Thread - Lock Free)
//==============================================================================

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
    // Load active state atomically - NO LOCK
    CaptureState* state = activeState_.load(std::memory_order_acquire);
    if (!state || !state->buffer || !state->context)
        return;

    if (numSamples <= 0 || numChannels <= 0)
        return;

    // Increment epoch to signal we're using this state
    // Use acq_rel for proper release semantics on ARM/Apple Silicon
    state->epoch.fetch_add(1, std::memory_order_acq_rel);

    const int actualChannels = juce::jmin(numChannels,
                                           static_cast<int>(SharedCaptureBuffer::MAX_CHANNELS));

    if (state->decimationRatio <= 1)
    {
        // No decimation - write directly using lock-free path
        state->buffer->writeLockFree(samples, numSamples, actualChannels, metadata);
    }
    else
    {
        // SIMD decimation + write
        float* outputPtrs[SharedCaptureBuffer::MAX_CHANNELS];
        for (int ch = 0; ch < actualChannels; ++ch)
        {
            outputPtrs[ch] = state->context->decimatedBuffer.getWritePointer(ch);
        }

        int decimatedCount = state->context->filter.processBlock(
            samples, outputPtrs, numSamples, actualChannels);

        if (decimatedCount > 0)
        {
            CaptureFrameMetadata decimatedMeta = metadata;
            decimatedMeta.numSamples = decimatedCount;
            decimatedMeta.sampleRate = static_cast<double>(state->captureRate);

            const float* constOutputPtrs[SharedCaptureBuffer::MAX_CHANNELS];
            for (int ch = 0; ch < actualChannels; ++ch)
            {
                constOutputPtrs[ch] = outputPtrs[ch];
            }

            state->buffer->writeLockFree(constOutputPtrs, decimatedCount, 
                                          actualChannels, decimatedMeta);
        }
    }

    // Decrement epoch to signal we're done
    state->epoch.fetch_sub(1, std::memory_order_release);
}

//==============================================================================
// Read Interface (Any Thread - Lock Free)
//==============================================================================

int DecimatingCaptureBuffer::read(float* output, int numSamples, int channel) const
{
    CaptureState* state = activeState_.load(std::memory_order_acquire);
    if (!state || !state->buffer)
        return 0;
    return state->buffer->read(output, numSamples, channel);
}

int DecimatingCaptureBuffer::read(std::span<float> output, int channel) const
{
    CaptureState* state = activeState_.load(std::memory_order_acquire);
    if (!state || !state->buffer)
        return 0;
    return state->buffer->read(output, channel);
}

int DecimatingCaptureBuffer::read(juce::AudioBuffer<float>& output, int numSamples) const
{
    CaptureState* state = activeState_.load(std::memory_order_acquire);
    if (!state || !state->buffer)
        return 0;
    return state->buffer->read(output, numSamples);
}

CaptureFrameMetadata DecimatingCaptureBuffer::getLatestMetadata() const
{
    CaptureState* state = activeState_.load(std::memory_order_acquire);
    if (!state || !state->buffer)
        return {};
    return state->buffer->getLatestMetadata();
}

size_t DecimatingCaptureBuffer::getCapacity() const
{
    CaptureState* state = activeState_.load(std::memory_order_acquire);
    if (!state || !state->buffer)
        return 0;
    return state->buffer->getCapacity();
}

size_t DecimatingCaptureBuffer::getAvailableSamples() const
{
    CaptureState* state = activeState_.load(std::memory_order_acquire);
    if (!state || !state->buffer)
        return 0;
    return state->buffer->getAvailableSamples();
}

float DecimatingCaptureBuffer::getPeakLevel(int channel, int numSamples) const
{
    CaptureState* state = activeState_.load(std::memory_order_acquire);
    if (!state || !state->buffer)
        return 0.0f;
    return state->buffer->getPeakLevel(channel, numSamples);
}

float DecimatingCaptureBuffer::getRMSLevel(int channel, int numSamples) const
{
    CaptureState* state = activeState_.load(std::memory_order_acquire);
    if (!state || !state->buffer)
        return 0.0f;
    return state->buffer->getRMSLevel(channel, numSamples);
}

int DecimatingCaptureBuffer::getCaptureRate() const
{
    CaptureState* state = activeState_.load(std::memory_order_acquire);
    if (!state)
        return CaptureRate::STANDARD;
    return state->captureRate;
}

int DecimatingCaptureBuffer::getDecimationRatio() const
{
    CaptureState* state = activeState_.load(std::memory_order_acquire);
    if (!state)
        return 1;
    return state->decimationRatio;
}

//==============================================================================
// Memory Management
//==============================================================================

size_t DecimatingCaptureBuffer::getMemoryUsageBytes() const
{
    CaptureState* state = activeState_.load(std::memory_order_acquire);
    if (!state)
        return 0;

    size_t total = 0;

    if (state->buffer)
    {
        total += state->buffer->getCapacity() * SharedCaptureBuffer::MAX_CHANNELS * sizeof(float);
    }

    if (state->context)
    {
        total += state->context->totalMemoryBytes;
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
    // clear() must only be called from the message thread for thread safety
    // Runtime check works in release builds (not just debug)
    auto* mm = juce::MessageManager::getInstanceWithoutCreating();
    if (mm != nullptr && !mm->isThisTheMessageThread())
    {
        jassertfalse;  // Debug assertion for development
        return;        // Runtime guard for release builds
    }

    CaptureState* state = activeState_.load(std::memory_order_acquire);
    if (!state)
        return;

    // Wait for any in-flight audio thread writes to complete
    // The epoch is > 0 while write() is actively using this state
    int spinCount = 0;
    while (state->epoch.load(std::memory_order_acquire) > 0)
    {
        if (++spinCount > 1000)
        {
            std::this_thread::yield();
            spinCount = 0;
        }
    }

    // Now safe to clear - audio thread is not using this state
    // Note: There's still a small window where audio thread could start a write
    // after we check epoch but before we clear. However, the buffer and filter
    // clear() operations are thread-safe themselves (they use atomics internally).
    if (state->buffer)
        state->buffer->clear();

    if (state->context)
        state->context->filter.reset();
}

std::shared_ptr<SharedCaptureBuffer> DecimatingCaptureBuffer::getInternalBuffer() const
{
    // This method modifies mutable cached state without synchronization,
    // so it must only be called from the message thread to avoid data races.
    // The allocation (shared_ptr creation) is acceptable on message thread.
    // Runtime check works in release builds (not just debug)
    auto* mm = juce::MessageManager::getInstanceWithoutCreating();
    if (mm != nullptr && !mm->isThisTheMessageThread())
    {
        jassertfalse;  // Debug assertion for development
        return nullptr;  // Runtime guard for release builds
    }

    CaptureState* state = activeState_.load(std::memory_order_acquire);

    // Check if cached pointer is still valid
    if (cachedBufferState_.load(std::memory_order_relaxed) == state && cachedBufferPtr_)
    {
        return cachedBufferPtr_;
    }

    if (!state || !state->buffer)
    {
        return nullptr;
    }

    // Create a shared_ptr wrapper around the raw pointer
    // Note: This is a compatibility shim - the buffer is owned by CaptureState
    // The shared_ptr has a custom deleter that does nothing
    cachedBufferPtr_ = std::shared_ptr<SharedCaptureBuffer>(
        state->buffer.get(),
        [](SharedCaptureBuffer*) { /* No-op deleter - CaptureState owns the buffer */ }
    );
    cachedBufferState_.store(state, std::memory_order_relaxed);

    return cachedBufferPtr_;
}

} // namespace oscil
