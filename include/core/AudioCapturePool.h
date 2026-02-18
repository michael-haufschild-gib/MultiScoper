/*
    Oscil - Audio Capture Pool
    Centralized pool managing pre-allocated SPSC ring buffers for all sources
    
    Design goals:
    - Pre-allocated slots for up to 64 sources (no runtime allocation)
    - O(1) slot lookup for audio thread
    - Cache-line aligned to prevent false sharing
    - Thread-safe slot management (rare operations on message thread)
*/

#pragma once

#include "core/RawCaptureBuffer.h"
#include "core/Source.h"
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <memory>

namespace oscil
{

/**
 * Slot metadata for a capture buffer.
 * Cache-line aligned to prevent false sharing between slots.
 */
struct alignas(kRawBufferCacheLineSize) CaptureSlot
{
    // The raw capture buffer for this slot
    std::unique_ptr<RawCaptureBuffer> rawBuffer;

    // Active flag - set when slot is in use
    std::atomic<bool> active{false};

    // Source ID associated with this slot
    SourceId sourceId;

    // Decimation ratio for this source (set during allocation)
    int decimationRatio = 1;

    // Original sample rate (set during allocation)
    double sampleRate = 44100.0;

    CaptureSlot()
        : rawBuffer(std::make_unique<RawCaptureBuffer>())
    {
    }

    // Non-copyable, non-movable
    CaptureSlot(const CaptureSlot&) = delete;
    CaptureSlot& operator=(const CaptureSlot&) = delete;
    CaptureSlot(CaptureSlot&&) = delete;
    CaptureSlot& operator=(CaptureSlot&&) = delete;
};

/**
 * Centralized pool managing capture buffers for all audio sources.
 * 
 * Thread safety model:
 * - allocateSlot()/releaseSlot(): Called from message thread only (uses SpinLock)
 * - getRawBuffer(): Called from audio thread (lock-free, O(1) lookup)
 * - isSlotActive(): Called from capture thread (lock-free)
 * 
 * Memory layout:
 * - Fixed array of MAX_SOURCES slots (pre-allocated at construction)
 * - Each slot contains a RawCaptureBuffer and metadata
 * - Cache-line aligned to prevent false sharing
 * 
 * Usage:
 * 1. Plugin registers via allocateSlot() → gets slotIndex
 * 2. Audio thread uses getRawBuffer(slotIndex)->write()
 * 3. CaptureThread iterates active slots via isSlotActive(i)
 * 4. Plugin unregisters via releaseSlot(slotIndex)
 */
class AudioCapturePool
{
public:
    static constexpr int MAX_SOURCES = 64;
    static constexpr int INVALID_SLOT = -1;

    AudioCapturePool();
    ~AudioCapturePool() = default;

    // Non-copyable, non-movable (singleton-like usage)
    AudioCapturePool(const AudioCapturePool&) = delete;
    AudioCapturePool& operator=(const AudioCapturePool&) = delete;
    AudioCapturePool(AudioCapturePool&&) = delete;
    AudioCapturePool& operator=(AudioCapturePool&&) = delete;

    //==========================================================================
    // Message Thread Interface (slot management)
    //==========================================================================

    /**
     * Allocate a slot for a new audio source.
     * 
     * NOT real-time safe - called from message thread only.
     * 
     * @param sourceId The source ID to associate with this slot
     * @param sampleRate The sample rate for this source
     * @param decimationRatio Decimation ratio for capture thread processing
     * @return Slot index (0 to MAX_SOURCES-1), or INVALID_SLOT if pool is full
     */
    [[nodiscard]] int allocateSlot(const SourceId& sourceId, double sampleRate = 44100.0, int decimationRatio = 1);

    /**
     * Release a slot when source is removed.
     * 
     * NOT real-time safe - called from message thread only.
     * 
     * @param slotIndex The slot index to release
     */
    void releaseSlot(int slotIndex);

    /**
     * Update slot configuration (e.g., when sample rate changes).
     * 
     * NOT real-time safe - called from message thread only.
     * 
     * @param slotIndex The slot to update
     * @param sampleRate New sample rate
     * @param decimationRatio New decimation ratio
     */
    void updateSlotConfig(int slotIndex, double sampleRate, int decimationRatio);

    /**
     * Get number of active slots.
     */
    [[nodiscard]] int getActiveCount() const;

    /**
     * Find slot index by source ID.
     * 
     * @param sourceId The source ID to find
     * @return Slot index, or INVALID_SLOT if not found
     */
    [[nodiscard]] int findSlotBySourceId(const SourceId& sourceId) const;

    //==========================================================================
    // Audio Thread Interface (lock-free)
    //==========================================================================

    /**
     * Get raw capture buffer for a slot.
     * 
     * REAL-TIME SAFE: O(1) lookup, no locks.
     * 
     * @param slotIndex The slot index (must be valid)
     * @return Pointer to RawCaptureBuffer, or nullptr if invalid slot
     */
    [[nodiscard]] RawCaptureBuffer* getRawBuffer(int slotIndex);

    /**
     * Get raw capture buffer (const version).
     */
    [[nodiscard]] const RawCaptureBuffer* getRawBuffer(int slotIndex) const;

    //==========================================================================
    // Capture Thread Interface (lock-free)
    //==========================================================================

    /**
     * Check if a slot is active.
     * 
     * LOCK-FREE: Uses atomic load.
     * 
     * @param slotIndex The slot index to check
     * @return true if slot is active and has valid buffer
     */
    [[nodiscard]] bool isSlotActive(int slotIndex) const;

    /**
     * Get source ID for a slot.
     * 
     * @param slotIndex The slot index
     * @return Source ID (may be invalid if slot is not active)
     */
    [[nodiscard]] SourceId getSlotSourceId(int slotIndex) const;

    /**
     * Get decimation ratio for a slot.
     * 
     * @param slotIndex The slot index
     * @return Decimation ratio (1 = no decimation)
     */
    [[nodiscard]] int getSlotDecimationRatio(int slotIndex) const;

    /**
     * Get sample rate for a slot.
     * 
     * @param slotIndex The slot index
     * @return Sample rate in Hz
     */
    [[nodiscard]] double getSlotSampleRate(int slotIndex) const;

    /**
     * Reset all slots (clear all data, release all slots).
     * 
     * NOT real-time safe - called from message thread only.
     */
    void reset();

private:
    // Pre-allocated slots
    std::array<CaptureSlot, MAX_SOURCES> slots_;

    // Count of active slots (for quick capacity check)
    std::atomic<int> activeCount_{0};

    // Lock for slot management (allocation/release only)
    mutable juce::SpinLock managementLock_;
};

} // namespace oscil













