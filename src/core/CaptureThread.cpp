/*
    Oscil - Capture Thread Implementation
    Dedicated background thread for batch audio capture processing
*/

#include "core/CaptureThread.h"
#include "core/DebugLogger.h"
#include <cmath>

namespace oscil
{

//==============================================================================
// ProcessedCaptureBuffer Implementation
//==============================================================================

ProcessedCaptureBuffer::ProcessedCaptureBuffer()
    : buffer(std::make_shared<SharedCaptureBuffer>())
    , filter(std::make_unique<SIMDDecimationFilter>())
{
    // Initialize scratch buffers with reasonable defaults
    inputScratch.setSize(2, CaptureThread::MAX_SAMPLES_PER_TICK);
    outputScratch.setSize(2, CaptureThread::MAX_SAMPLES_PER_TICK);
}

void ProcessedCaptureBuffer::configure(double newSampleRate, int newDecimationRatio, int maxBlockSize)
{
    // Check if reconfiguration is needed
    if (std::abs(sampleRate - newSampleRate) < 0.1 && decimationRatio == newDecimationRatio)
    {
        return; // No change needed
    }

    sampleRate = newSampleRate;
    decimationRatio = newDecimationRatio;

    // Configure the SIMD filter
    filter->configure(decimationRatio, sampleRate, maxBlockSize);

    // Resize scratch buffers if needed
    if (inputScratch.getNumSamples() < maxBlockSize)
    {
        inputScratch.setSize(2, maxBlockSize, false, false, false);
    }

    int maxOutputSamples = (maxBlockSize / std::max(1, decimationRatio)) + 1;
    if (outputScratch.getNumSamples() < maxOutputSamples)
    {
        outputScratch.setSize(2, maxOutputSamples, false, false, false);
    }
}

void ProcessedCaptureBuffer::reset()
{
    filter->reset();
    inputScratch.clear();
    outputScratch.clear();
}

//==============================================================================
// CaptureThread Implementation
//==============================================================================

CaptureThread::CaptureThread(AudioCapturePool& pool)
    : juce::Thread("OscilCaptureThread")
    , pool_(pool)
{
    // Pre-allocate processed buffers for all slots
    for (size_t i = 0; i < processedBuffers_.size(); ++i)
    {
        processedBuffers_[i] = std::make_unique<ProcessedCaptureBuffer>();
    }
}

CaptureThread::~CaptureThread()
{
    stopCapturing();
}

//==============================================================================
// Thread Control
//==============================================================================

void CaptureThread::startCapturing()
{
    if (capturing_.load(std::memory_order_relaxed))
        return;

    capturing_.store(true, std::memory_order_release);
    startThread(juce::Thread::Priority::normal);
}

void CaptureThread::stopCapturing()
{
    if (!capturing_.load(std::memory_order_relaxed))
        return;

    capturing_.store(false, std::memory_order_release);
    signalThreadShouldExit();
    stopThread(1000); // Wait up to 1 second
}

bool CaptureThread::isCapturing() const
{
    return capturing_.load(std::memory_order_relaxed) && isThreadRunning();
}

//==============================================================================
// Processed Buffer Access
//==============================================================================

std::shared_ptr<SharedCaptureBuffer> CaptureThread::getProcessedBuffer(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= AudioCapturePool::MAX_SOURCES)
        return nullptr;

    const auto& pb = processedBuffers_[static_cast<size_t>(slotIndex)];
    if (!pb)
        return nullptr;

    return pb->buffer;
}

std::shared_ptr<IAudioBuffer> CaptureThread::getProcessedBufferAsInterface(int slotIndex) const
{
    return getProcessedBuffer(slotIndex);
}

//==============================================================================
// Statistics
//==============================================================================

double CaptureThread::getAverageTickTimeUs() const
{
    return avgTickTimeUs_.load(std::memory_order_relaxed);
}

double CaptureThread::getMaxTickTimeUs() const
{
    return maxTickTimeUs_.load(std::memory_order_relaxed);
}

void CaptureThread::resetStats()
{
    avgTickTimeUs_.store(0.0, std::memory_order_relaxed);
    maxTickTimeUs_.store(0.0, std::memory_order_relaxed);
    tickCount_.store(0, std::memory_order_relaxed);
}

//==============================================================================
// Thread Implementation
//==============================================================================

void CaptureThread::run()
{
    const double ticksPerUs = 1.0 / (static_cast<double>(juce::Time::getHighResolutionTicksPerSecond()) / 1000000.0);

    // Log thread start (file-based logging for DAW debugging)
    static bool loggedStart = false;
    if (!loggedStart)
    {
        OSCIL_LOG_CAPTURE("CaptureThread STARTED, entering main loop");
        loggedStart = true;
    }

    while (!threadShouldExit() && capturing_.load(std::memory_order_relaxed))
    {
        auto tickStart = juce::Time::getHighResolutionTicks();

        // Process all active slots
        int activeSlotCount = 0;
        for (int i = 0; i < AudioCapturePool::MAX_SOURCES; ++i)
        {
            if (pool_.isSlotActive(i))
            {
                activeSlotCount++;
                processSlot(i);
            }
        }

        // Log first detection of active slots (one-time)
        static bool loggedFirstActive = false;
        if (!loggedFirstActive && activeSlotCount > 0)
        {
            loggedFirstActive = true;
            OSCIL_LOG_CAPTURE("First ACTIVE slot detected! activeSlotCount=" + juce::String(activeSlotCount));
        }

        // Log active slots periodically (every ~10 seconds to avoid log spam)
        static int tickCounter = 0;
        if (++tickCounter >= 1000) // Every ~10 seconds at 100 ticks/sec
        {
            tickCounter = 0;
            OSCIL_LOG_CAPTURE("Active slots: " + juce::String(activeSlotCount)
                             + ", avgTickTimeUs=" + juce::String(avgTickTimeUs_.load(std::memory_order_relaxed), 2));
        }

        // Calculate tick time
        auto tickEnd = juce::Time::getHighResolutionTicks();
        double tickTimeUs = static_cast<double>(tickEnd - tickStart) * ticksPerUs;

        // Update statistics (exponential moving average)
        uint64_t count = tickCount_.fetch_add(1, std::memory_order_relaxed);
        double currentAvg = avgTickTimeUs_.load(std::memory_order_relaxed);

        if (count == 0)
        {
            avgTickTimeUs_.store(tickTimeUs, std::memory_order_relaxed);
        }
        else
        {
            // EMA with alpha = 0.1
            double newAvg = currentAvg * 0.9 + tickTimeUs * 0.1;
            avgTickTimeUs_.store(newAvg, std::memory_order_relaxed);
        }

        // Update max
        double currentMax = maxTickTimeUs_.load(std::memory_order_relaxed);
        if (tickTimeUs > currentMax)
        {
            maxTickTimeUs_.store(tickTimeUs, std::memory_order_relaxed);
        }

        // Calculate remaining time in tick and sleep
        double elapsedMs = tickTimeUs / 1000.0;
        double sleepMs = static_cast<double>(TICK_INTERVAL_MS) - elapsedMs;

        if (sleepMs > 0.1)
        {
            // Use wait instead of sleep for better responsiveness to exit signal
            wait(static_cast<int>(sleepMs));
        }
    }
}

void CaptureThread::processSlot(int slotIndex)
{
    // Ensure buffer is configured for current slot settings
    ensureBufferConfigured(slotIndex);

    // Get raw buffer
    RawCaptureBuffer* rawBuffer = pool_.getRawBuffer(slotIndex);
    if (!rawBuffer)
        return;

    ProcessedCaptureBuffer* processed = processedBuffers_[static_cast<size_t>(slotIndex)].get();
    if (!processed || !processed->buffer)
        return;

    // Check if there's data to read
    size_t available = rawBuffer->getAvailableSamples();

    // Log first access to slot (one-time per slot)
    static std::array<bool, AudioCapturePool::MAX_SOURCES> loggedFirstAccess{};
    if (!loggedFirstAccess[static_cast<size_t>(slotIndex)])
    {
        loggedFirstAccess[static_cast<size_t>(slotIndex)] = true;
        OSCIL_LOG_CAPTURE("processSlot: FIRST ACCESS to slot " + juce::String(slotIndex)
                         + ", availableSamples=" + juce::String(static_cast<int>(available)));
    }

    if (available == 0)
        return;

    // Log data flow periodically (every ~10000 processed blocks to avoid spam)
    static int processCounter = 0;
    if (++processCounter >= 10000)
    {
        processCounter = 0;
        OSCIL_LOG_CAPTURE("processSlot: slot " + juce::String(slotIndex)
                         + " processed " + juce::String(static_cast<int>(available)) + " samples");
    }

    // Limit samples per tick to prevent starvation
    int samplesToRead = std::min(static_cast<int>(available), MAX_SAMPLES_PER_TICK);

    // Read raw samples into scratch buffer
    float* inputL = processed->inputScratch.getWritePointer(0);
    float* inputR = processed->inputScratch.getWritePointer(1);

    int samplesRead = rawBuffer->read(inputL, inputR, samplesToRead);
    if (samplesRead <= 0)
        return;

    // Get decimation ratio
    int decimationRatio = pool_.getSlotDecimationRatio(slotIndex);

    // Prepare output pointers
    float* outputL = processed->outputScratch.getWritePointer(0);
    float* outputR = processed->outputScratch.getWritePointer(1);

    int decimatedCount;

    if (decimationRatio <= 1 || !processed->filter->isActive())
    {
        // No decimation - direct copy
        std::memcpy(outputL, inputL, static_cast<size_t>(samplesRead) * sizeof(float));
        std::memcpy(outputR, inputR, static_cast<size_t>(samplesRead) * sizeof(float));
        decimatedCount = samplesRead;
    }
    else
    {
        // Apply SIMD decimation
        const float* inputPtrs[2] = { inputL, inputR };
        float* outputPtrs[2] = { outputL, outputR };

        decimatedCount = processed->filter->processBlock(inputPtrs, outputPtrs, samplesRead, 2);
    }

    if (decimatedCount <= 0)
        return;

    // Prepare metadata
    CaptureFrameMetadata metadata;
    metadata.sampleRate = pool_.getSlotSampleRate(slotIndex);
    if (decimationRatio > 1)
    {
        metadata.sampleRate = metadata.sampleRate / static_cast<double>(decimationRatio);
    }
    metadata.numChannels = 2;
    metadata.numSamples = decimatedCount;
    metadata.isPlaying = true; // Assume playing if we're getting data
    metadata.bpm = 120.0;
    metadata.timestamp = 0;

    // Write to processed buffer
    const float* outputPtrs[2] = { outputL, outputR };
    processed->buffer->writeLockFree(outputPtrs, decimatedCount, 2, metadata);
}

void CaptureThread::ensureBufferConfigured(int slotIndex)
{
    ProcessedCaptureBuffer* processed = processedBuffers_[static_cast<size_t>(slotIndex)].get();
    if (!processed)
        return;

    double sampleRate = pool_.getSlotSampleRate(slotIndex);
    int decimationRatio = pool_.getSlotDecimationRatio(slotIndex);

    // Check if reconfiguration is needed
    if (std::abs(processed->sampleRate - sampleRate) > 0.1 ||
        processed->decimationRatio != decimationRatio)
    {
        processed->configure(sampleRate, decimationRatio, MAX_SAMPLES_PER_TICK);
    }
}

} // namespace oscil













