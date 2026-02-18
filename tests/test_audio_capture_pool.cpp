/*
    Oscil - Audio Capture Pool Tests
    Unit tests for the centralized capture buffer pool
*/

#include <gtest/gtest.h>
#include "core/AudioCapturePool.h"
#include <vector>

namespace oscil
{
namespace test
{

class AudioCapturePoolTest : public ::testing::Test
{
protected:
    AudioCapturePool pool;
};

// Basic slot allocation
TEST_F(AudioCapturePoolTest, BasicAllocation)
{
    SourceId source1{"test-source-1"};
    
    int slot = pool.allocateSlot(source1, 44100.0, 1);
    
    EXPECT_GE(slot, 0);
    EXPECT_LT(slot, AudioCapturePool::MAX_SOURCES);
    EXPECT_EQ(pool.getActiveCount(), 1);
    EXPECT_TRUE(pool.isSlotActive(slot));
}

// Multiple slot allocations
TEST_F(AudioCapturePoolTest, MultipleAllocations)
{
    std::vector<int> slots;
    
    for (int i = 0; i < 10; ++i)
    {
        SourceId source{juce::String("source-") + juce::String(i)};
        int slot = pool.allocateSlot(source, 48000.0, 2);
        EXPECT_GE(slot, 0);
        slots.push_back(slot);
    }
    
    EXPECT_EQ(pool.getActiveCount(), 10);
    
    // All slots should be unique
    for (size_t i = 0; i < slots.size(); ++i)
    {
        for (size_t j = i + 1; j < slots.size(); ++j)
        {
            EXPECT_NE(slots[i], slots[j]);
        }
    }
}

// Slot release
TEST_F(AudioCapturePoolTest, SlotRelease)
{
    SourceId source{"test-source"};
    
    int slot = pool.allocateSlot(source, 44100.0, 1);
    EXPECT_TRUE(pool.isSlotActive(slot));
    
    pool.releaseSlot(slot);
    EXPECT_FALSE(pool.isSlotActive(slot));
    EXPECT_EQ(pool.getActiveCount(), 0);
}

// Slot reuse after release
TEST_F(AudioCapturePoolTest, SlotReuse)
{
    SourceId source1{"source-1"};
    SourceId source2{"source-2"};
    
    int slot1 = pool.allocateSlot(source1, 44100.0, 1);
    pool.releaseSlot(slot1);
    
    int slot2 = pool.allocateSlot(source2, 44100.0, 1);
    EXPECT_EQ(slot1, slot2); // Should reuse the same slot
}

// Get raw buffer
TEST_F(AudioCapturePoolTest, GetRawBuffer)
{
    SourceId source{"test-source"};
    int slot = pool.allocateSlot(source, 44100.0, 1);
    
    RawCaptureBuffer* buffer = pool.getRawBuffer(slot);
    EXPECT_NE(buffer, nullptr);
    
    // Buffer should be functional
    std::vector<float> data(256, 0.5f);
    buffer->write(data.data(), data.data(), 256, 44100.0);
    EXPECT_EQ(buffer->getAvailableSamples(), 256u);
}

// Invalid slot access
TEST_F(AudioCapturePoolTest, InvalidSlotAccess)
{
    EXPECT_EQ(pool.getRawBuffer(-1), nullptr);
    EXPECT_EQ(pool.getRawBuffer(AudioCapturePool::MAX_SOURCES), nullptr);
    EXPECT_EQ(pool.getRawBuffer(1000), nullptr);
    
    EXPECT_FALSE(pool.isSlotActive(-1));
    EXPECT_FALSE(pool.isSlotActive(AudioCapturePool::MAX_SOURCES));
}

// Find slot by source ID
TEST_F(AudioCapturePoolTest, FindSlotBySourceId)
{
    SourceId source1{"source-1"};
    SourceId source2{"source-2"};
    SourceId source3{"source-3"};
    
    int slot1 = pool.allocateSlot(source1, 44100.0, 1);
    int slot2 = pool.allocateSlot(source2, 48000.0, 2);
    
    EXPECT_EQ(pool.findSlotBySourceId(source1), slot1);
    EXPECT_EQ(pool.findSlotBySourceId(source2), slot2);
    EXPECT_EQ(pool.findSlotBySourceId(source3), AudioCapturePool::INVALID_SLOT);
}

// Get slot metadata
TEST_F(AudioCapturePoolTest, SlotMetadata)
{
    SourceId source{"test-source"};
    
    int slot = pool.allocateSlot(source, 96000.0, 4);
    
    EXPECT_EQ(pool.getSlotSourceId(slot), source);
    EXPECT_DOUBLE_EQ(pool.getSlotSampleRate(slot), 96000.0);
    EXPECT_EQ(pool.getSlotDecimationRatio(slot), 4);
}

// Update slot config
TEST_F(AudioCapturePoolTest, UpdateSlotConfig)
{
    SourceId source{"test-source"};
    
    int slot = pool.allocateSlot(source, 44100.0, 1);
    
    pool.updateSlotConfig(slot, 96000.0, 2);
    
    EXPECT_DOUBLE_EQ(pool.getSlotSampleRate(slot), 96000.0);
    EXPECT_EQ(pool.getSlotDecimationRatio(slot), 2);
}

// Pool capacity
TEST_F(AudioCapturePoolTest, PoolCapacity)
{
    // Allocate all slots
    for (int i = 0; i < AudioCapturePool::MAX_SOURCES; ++i)
    {
        SourceId source{juce::String("source-") + juce::String(i)};
        int slot = pool.allocateSlot(source, 44100.0, 1);
        EXPECT_GE(slot, 0);
    }
    
    EXPECT_EQ(pool.getActiveCount(), AudioCapturePool::MAX_SOURCES);
    
    // Next allocation should fail
    SourceId extraSource{"extra-source"};
    int slot = pool.allocateSlot(extraSource, 44100.0, 1);
    EXPECT_EQ(slot, AudioCapturePool::INVALID_SLOT);
}

// Reset pool
TEST_F(AudioCapturePoolTest, Reset)
{
    // Allocate some slots
    for (int i = 0; i < 10; ++i)
    {
        SourceId source{juce::String("source-") + juce::String(i)};
        pool.allocateSlot(source, 44100.0, 1);
    }
    
    EXPECT_EQ(pool.getActiveCount(), 10);
    
    pool.reset();
    
    EXPECT_EQ(pool.getActiveCount(), 0);
    
    // Should be able to allocate again
    SourceId source{"new-source"};
    int slot = pool.allocateSlot(source, 44100.0, 1);
    EXPECT_GE(slot, 0);
}

// Buffer reset on release
TEST_F(AudioCapturePoolTest, BufferResetOnRelease)
{
    SourceId source{"test-source"};
    
    int slot = pool.allocateSlot(source, 44100.0, 1);
    RawCaptureBuffer* buffer = pool.getRawBuffer(slot);
    
    // Write some data
    std::vector<float> data(256, 0.5f);
    buffer->write(data.data(), data.data(), 256, 44100.0);
    EXPECT_GT(buffer->getAvailableSamples(), 0u);
    
    // Release and reallocate
    pool.releaseSlot(slot);
    
    SourceId source2{"source-2"};
    int newSlot = pool.allocateSlot(source2, 44100.0, 1);
    
    // Buffer should be reset
    RawCaptureBuffer* newBuffer = pool.getRawBuffer(newSlot);
    EXPECT_EQ(newBuffer->getAvailableSamples(), 0u);
}

} // namespace test
} // namespace oscil













