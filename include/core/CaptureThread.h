/*
    Oscil - Capture Thread
    Dedicated background thread for batch audio capture processing
    
    Design goals:
    - Batch reads all active source ring buffers
    - SIMD-accelerated decimation (uses existing SIMDDecimationFilter)
    - Produces processed buffers for UI/render consumption
    - Runs in parallel with rendering (~500Hz tick rate)
    - Non-realtime priority (doesn't block audio thread)
*/

#pragma once

#include "core/AudioCapturePool.h"
#include "core/DecimatingCaptureBuffer.h"
#include "core/SharedCaptureBuffer.h"
#include <juce_core/juce_core.h>
#include <array>
#include <memory>
#include <atomic>

namespace oscil
{

/**
 * Output buffer for processed (decimated) samples.
 * Consumed by WaveformPresenter and RenderEngine.
 */
struct ProcessedCaptureBuffer
{
    // Decimated samples for display
    std::shared_ptr<SharedCaptureBuffer> buffer;

    // SIMD decimation filter for this source
    std::unique_ptr<SIMDDecimationFilter> filter;

    // Scratch buffer for filter input
    juce::AudioBuffer<float> inputScratch;

    // Scratch buffer for filter output
    juce::AudioBuffer<float> outputScratch;

    // Last known sample rate (for detecting changes)
    double sampleRate = 44100.0;

    // Last known decimation ratio
    int decimationRatio = 1;

    ProcessedCaptureBuffer();
    ~ProcessedCaptureBuffer() = default;

    // Non-copyable, non-movable
    ProcessedCaptureBuffer(const ProcessedCaptureBuffer&) = delete;
    ProcessedCaptureBuffer& operator=(const ProcessedCaptureBuffer&) = delete;
    ProcessedCaptureBuffer(ProcessedCaptureBuffer&&) = delete;
    ProcessedCaptureBuffer& operator=(ProcessedCaptureBuffer&&) = delete;

    /**
     * Configure filter for given parameters.
     * NOT real-time safe - call from capture thread only.
     */
    void configure(double sampleRate, int decimationRatio, int maxBlockSize);

    /**
     * Reset filter state.
     */
    void reset();
};

/**
 * Dedicated thread for batch audio capture processing.
 * 
 * Thread safety model:
 * - run() executes on dedicated thread (non-realtime priority)
 * - getProcessedBuffer() is thread-safe (returns shared_ptr)
 * - start()/stop() called from message thread only
 * 
 * Processing flow (per tick, ~2ms):
 * 1. Iterate all active slots in AudioCapturePool
 * 2. Read raw samples from each RawCaptureBuffer
 * 3. Apply SIMD decimation filter
 * 4. Write to processed SharedCaptureBuffer
 * 
 * Performance characteristics:
 * - ~1-2ms per batch for 64 sources
 * - Runs in parallel with UI and render threads
 * - Non-blocking to audio thread
 */
class CaptureThread : public juce::Thread
{
public:
    // Tick interval in milliseconds
    static constexpr int TICK_INTERVAL_MS = 2;

    // Maximum samples to read per tick (prevents starvation)
    static constexpr int MAX_SAMPLES_PER_TICK = 8192;

    explicit CaptureThread(AudioCapturePool& pool);
    ~CaptureThread() override;

    // Non-copyable, non-movable
    CaptureThread(const CaptureThread&) = delete;
    CaptureThread& operator=(const CaptureThread&) = delete;
    CaptureThread(CaptureThread&&) = delete;
    CaptureThread& operator=(CaptureThread&&) = delete;

    //==========================================================================
    // Thread Control (message thread only)
    //==========================================================================

    /**
     * Start the capture thread.
     * Called from message thread only.
     */
    void startCapturing();

    /**
     * Stop the capture thread.
     * Called from message thread only. Blocks until thread exits.
     */
    void stopCapturing();

    /**
     * Check if capture thread is running.
     */
    [[nodiscard]] bool isCapturing() const;

    //==========================================================================
    // Processed Buffer Access (thread-safe)
    //==========================================================================

    /**
     * Get processed buffer for a slot.
     * 
     * Thread-safe: Returns shared_ptr to buffer.
     * 
     * @param slotIndex The slot index
     * @return Shared pointer to processed buffer, or nullptr if invalid
     */
    [[nodiscard]] std::shared_ptr<SharedCaptureBuffer> getProcessedBuffer(int slotIndex) const;

    /**
     * Get processed buffer as IAudioBuffer interface.
     * 
     * @param slotIndex The slot index
     * @return Shared pointer to IAudioBuffer, or nullptr if invalid
     */
    [[nodiscard]] std::shared_ptr<IAudioBuffer> getProcessedBufferAsInterface(int slotIndex) const;

    //==========================================================================
    // Statistics
    //==========================================================================

    /**
     * Get average processing time per tick (in microseconds).
     */
    [[nodiscard]] double getAverageTickTimeUs() const;

    /**
     * Get maximum processing time per tick (in microseconds).
     */
    [[nodiscard]] double getMaxTickTimeUs() const;

    /**
     * Reset statistics.
     */
    void resetStats();

protected:
    //==========================================================================
    // Thread Implementation
    //==========================================================================

    void run() override;

private:
    /**
     * Process a single slot: read raw samples, decimate, write to processed buffer.
     */
    void processSlot(int slotIndex);

    /**
     * Ensure processed buffer is configured for current slot settings.
     */
    void ensureBufferConfigured(int slotIndex);

    AudioCapturePool& pool_;

    // Processed buffers for each slot
    std::array<std::unique_ptr<ProcessedCaptureBuffer>, AudioCapturePool::MAX_SOURCES> processedBuffers_;

    // Running flag
    std::atomic<bool> capturing_{false};

    // Statistics
    std::atomic<double> avgTickTimeUs_{0.0};
    std::atomic<double> maxTickTimeUs_{0.0};
    std::atomic<uint64_t> tickCount_{0};
};

} // namespace oscil













