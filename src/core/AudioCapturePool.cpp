/*
    Oscil - Audio Capture Pool Implementation
    Centralized pool managing pre-allocated SPSC ring buffers for all sources
*/

#include "core/AudioCapturePool.h"

namespace oscil
{

AudioCapturePool::AudioCapturePool()
{
    // Slots are pre-allocated via default CaptureSlot constructor
    // Each slot already has a RawCaptureBuffer created
}

//==============================================================================
// Message Thread Interface (slot management)
//==============================================================================

int AudioCapturePool::allocateSlot(const SourceId& sourceId, double sampleRate, int decimationRatio)
{
    const juce::SpinLock::ScopedLockType lock(managementLock_);

    // Quick capacity check
    if (activeCount_.load(std::memory_order_relaxed) >= MAX_SOURCES)
    {
        return INVALID_SLOT;
    }

    // Find first available slot
    for (int i = 0; i < MAX_SOURCES; ++i)
    {
        if (!slots_[static_cast<size_t>(i)].active.load(std::memory_order_relaxed))
        {
            // Found an available slot
            CaptureSlot& slot = slots_[static_cast<size_t>(i)];

            // Reset the buffer before use
            if (slot.rawBuffer)
            {
                slot.rawBuffer->reset();
            }

            // Configure slot
            slot.sourceId = sourceId;
            slot.sampleRate = sampleRate;
            slot.decimationRatio = decimationRatio;

            // Mark as active (release ensures visibility)
            slot.active.store(true, std::memory_order_release);

            // Increment active count
            activeCount_.fetch_add(1, std::memory_order_relaxed);

            return i;
        }
    }

    // No slots available (shouldn't happen if activeCount check passed)
    return INVALID_SLOT;
}

void AudioCapturePool::releaseSlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= MAX_SOURCES)
        return;

    const juce::SpinLock::ScopedLockType lock(managementLock_);

    CaptureSlot& slot = slots_[static_cast<size_t>(slotIndex)];

    if (slot.active.load(std::memory_order_relaxed))
    {
        // Mark as inactive first (acquire to sync with any in-flight writes)
        slot.active.store(false, std::memory_order_release);

        // Clear metadata
        slot.sourceId = SourceId{};
        slot.sampleRate = 44100.0;
        slot.decimationRatio = 1;

        // Reset buffer (clears data, resets positions)
        if (slot.rawBuffer)
        {
            slot.rawBuffer->reset();
        }

        // Decrement active count
        activeCount_.fetch_sub(1, std::memory_order_relaxed);
    }
}

void AudioCapturePool::updateSlotConfig(int slotIndex, double sampleRate, int decimationRatio)
{
    if (slotIndex < 0 || slotIndex >= MAX_SOURCES)
        return;

    const juce::SpinLock::ScopedLockType lock(managementLock_);

    CaptureSlot& slot = slots_[static_cast<size_t>(slotIndex)];

    if (slot.active.load(std::memory_order_relaxed))
    {
        slot.sampleRate = sampleRate;
        slot.decimationRatio = decimationRatio;
    }
}

int AudioCapturePool::getActiveCount() const
{
    return activeCount_.load(std::memory_order_relaxed);
}

int AudioCapturePool::findSlotBySourceId(const SourceId& sourceId) const
{
    const juce::SpinLock::ScopedLockType lock(managementLock_);

    for (int i = 0; i < MAX_SOURCES; ++i)
    {
        const CaptureSlot& slot = slots_[static_cast<size_t>(i)];
        if (slot.active.load(std::memory_order_relaxed) && slot.sourceId == sourceId)
        {
            return i;
        }
    }

    return INVALID_SLOT;
}

//==============================================================================
// Audio Thread Interface (lock-free)
//==============================================================================

RawCaptureBuffer* AudioCapturePool::getRawBuffer(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= MAX_SOURCES)
        return nullptr;

    // Direct array access - O(1), no locks
    return slots_[static_cast<size_t>(slotIndex)].rawBuffer.get();
}

const RawCaptureBuffer* AudioCapturePool::getRawBuffer(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= MAX_SOURCES)
        return nullptr;

    return slots_[static_cast<size_t>(slotIndex)].rawBuffer.get();
}

//==============================================================================
// Capture Thread Interface (lock-free)
//==============================================================================

bool AudioCapturePool::isSlotActive(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= MAX_SOURCES)
        return false;

    return slots_[static_cast<size_t>(slotIndex)].active.load(std::memory_order_acquire);
}

SourceId AudioCapturePool::getSlotSourceId(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= MAX_SOURCES)
        return SourceId{};

    return slots_[static_cast<size_t>(slotIndex)].sourceId;
}

int AudioCapturePool::getSlotDecimationRatio(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= MAX_SOURCES)
        return 1;

    return slots_[static_cast<size_t>(slotIndex)].decimationRatio;
}

double AudioCapturePool::getSlotSampleRate(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= MAX_SOURCES)
        return 44100.0;

    return slots_[static_cast<size_t>(slotIndex)].sampleRate;
}

void AudioCapturePool::reset()
{
    const juce::SpinLock::ScopedLockType lock(managementLock_);

    for (int i = 0; i < MAX_SOURCES; ++i)
    {
        CaptureSlot& slot = slots_[static_cast<size_t>(i)];

        slot.active.store(false, std::memory_order_release);
        slot.sourceId = SourceId{};
        slot.sampleRate = 44100.0;
        slot.decimationRatio = 1;

        if (slot.rawBuffer)
        {
            slot.rawBuffer->reset();
        }
    }

    activeCount_.store(0, std::memory_order_release);
}

} // namespace oscil













