/*
    Oscil - Instance Registry Tests
*/

#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include "core/InstanceRegistry.h"
#include "core/SharedCaptureBuffer.h"

using namespace oscil;

class InstanceRegistryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Clear any existing registrations
        auto sources = InstanceRegistry::getInstance().getAllSources();
        for (const auto& source : sources)
        {
            InstanceRegistry::getInstance().unregisterInstance(source.sourceId);
        }
    }

    void TearDown() override
    {
        // Clean up
        auto sources = InstanceRegistry::getInstance().getAllSources();
        for (const auto& source : sources)
        {
            InstanceRegistry::getInstance().unregisterInstance(source.sourceId);
        }
    }
};

// Test: Basic registration
TEST_F(InstanceRegistryTest, RegisterInstance)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "track_1", buffer, "Track 1", 2, 44100.0);

    EXPECT_TRUE(sourceId.isValid());
    EXPECT_EQ(InstanceRegistry::getInstance().getSourceCount(), 1);
}

// Test: Registration returns source info
TEST_F(InstanceRegistryTest, GetSourceInfo)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "track_1", buffer, "My Track", 2, 48000.0);

    auto sourceInfo = InstanceRegistry::getInstance().getSource(sourceId);

    ASSERT_TRUE(sourceInfo.has_value());
    EXPECT_EQ(sourceInfo->name, "My Track");
    EXPECT_EQ(sourceInfo->channelCount, 2);
    EXPECT_EQ(sourceInfo->sampleRate, 48000.0);
}

// Test: Unregistration removes source
TEST_F(InstanceRegistryTest, UnregisterInstance)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "track_1", buffer, "Track 1");

    EXPECT_EQ(InstanceRegistry::getInstance().getSourceCount(), 1);

    InstanceRegistry::getInstance().unregisterInstance(sourceId);

    EXPECT_EQ(InstanceRegistry::getInstance().getSourceCount(), 0);
    EXPECT_FALSE(InstanceRegistry::getInstance().getSource(sourceId).has_value());
}

// Test: Deduplication - same track ID returns same source
TEST_F(InstanceRegistryTest, DeduplicationSameTrack)
{
    auto buffer1 = std::make_shared<SharedCaptureBuffer>();
    auto buffer2 = std::make_shared<SharedCaptureBuffer>();

    auto sourceId1 = InstanceRegistry::getInstance().registerInstance(
        "track_1", buffer1, "Track 1");
    auto sourceId2 = InstanceRegistry::getInstance().registerInstance(
        "track_1", buffer2, "Track 1 Copy");

    // Should return the same source ID
    EXPECT_EQ(sourceId1, sourceId2);
    EXPECT_EQ(InstanceRegistry::getInstance().getSourceCount(), 1);
}

// Test: Different tracks get different source IDs
TEST_F(InstanceRegistryTest, DifferentTracksGetDifferentIds)
{
    auto buffer1 = std::make_shared<SharedCaptureBuffer>();
    auto buffer2 = std::make_shared<SharedCaptureBuffer>();

    auto sourceId1 = InstanceRegistry::getInstance().registerInstance(
        "track_1", buffer1, "Track 1");
    auto sourceId2 = InstanceRegistry::getInstance().registerInstance(
        "track_2", buffer2, "Track 2");

    EXPECT_NE(sourceId1, sourceId2);
    EXPECT_EQ(InstanceRegistry::getInstance().getSourceCount(), 2);
}

// Test: Get all sources
TEST_F(InstanceRegistryTest, GetAllSources)
{
    auto buffer1 = std::make_shared<SharedCaptureBuffer>();
    auto buffer2 = std::make_shared<SharedCaptureBuffer>();
    auto buffer3 = std::make_shared<SharedCaptureBuffer>();

    InstanceRegistry::getInstance().registerInstance("track_1", buffer1, "Track 1");
    InstanceRegistry::getInstance().registerInstance("track_2", buffer2, "Track 2");
    InstanceRegistry::getInstance().registerInstance("track_3", buffer3, "Track 3");

    auto sources = InstanceRegistry::getInstance().getAllSources();

    EXPECT_EQ(sources.size(), 3);
}

// Test: Maximum sources limit (64)
TEST_F(InstanceRegistryTest, MaxSourcesLimit)
{
    std::vector<std::shared_ptr<SharedCaptureBuffer>> buffers;
    std::vector<SourceId> sourceIds;

    // Register 64 sources
    for (int i = 0; i < 64; ++i)
    {
        auto buffer = std::make_shared<SharedCaptureBuffer>();
        buffers.push_back(buffer);
        auto sourceId = InstanceRegistry::getInstance().registerInstance(
            "track_" + juce::String(i), buffer, "Track " + juce::String(i));
        sourceIds.push_back(sourceId);
    }

    EXPECT_EQ(InstanceRegistry::getInstance().getSourceCount(), 64);

    // 65th should fail
    auto buffer65 = std::make_shared<SharedCaptureBuffer>();
    auto sourceId65 = InstanceRegistry::getInstance().registerInstance(
        "track_65", buffer65, "Track 65");

    EXPECT_FALSE(sourceId65.isValid());
    EXPECT_EQ(InstanceRegistry::getInstance().getSourceCount(), 64);
}

// Test: Get capture buffer
TEST_F(InstanceRegistryTest, GetCaptureBuffer)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "track_1", buffer, "Track 1");

    auto retrievedBuffer = InstanceRegistry::getInstance().getCaptureBuffer(sourceId);

    EXPECT_EQ(retrievedBuffer, buffer);
}

// Test: Update source metadata
TEST_F(InstanceRegistryTest, UpdateSource)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "track_1", buffer, "Track 1", 2, 44100.0);

    InstanceRegistry::getInstance().updateSource(sourceId, "Renamed Track", 1, 96000.0);

    auto sourceInfo = InstanceRegistry::getInstance().getSource(sourceId);

    ASSERT_TRUE(sourceInfo.has_value());
    EXPECT_EQ(sourceInfo->name, "Renamed Track");
    EXPECT_EQ(sourceInfo->channelCount, 1);
    EXPECT_EQ(sourceInfo->sampleRate, 96000.0);
}

// Listener test helper
class TestRegistryListener : public InstanceRegistryListener
{
public:
    int addedCount = 0;
    int removedCount = 0;
    int updatedCount = 0;
    SourceId lastSourceId;

    void sourceAdded(const SourceId& sourceId) override
    {
        addedCount++;
        lastSourceId = sourceId;
    }

    void sourceRemoved(const SourceId& sourceId) override
    {
        removedCount++;
        lastSourceId = sourceId;
    }

    void sourceUpdated(const SourceId& sourceId) override
    {
        updatedCount++;
        lastSourceId = sourceId;
    }
};

// Test: Listener notifications
TEST_F(InstanceRegistryTest, ListenerNotifications)
{
    TestRegistryListener listener;
    InstanceRegistry::getInstance().addListener(&listener);

    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "track_1", buffer, "Track 1");

    EXPECT_EQ(listener.addedCount, 1);
    EXPECT_EQ(listener.lastSourceId, sourceId);

    InstanceRegistry::getInstance().updateSource(sourceId, "New Name", 2, 44100.0);
    EXPECT_EQ(listener.updatedCount, 1);

    InstanceRegistry::getInstance().unregisterInstance(sourceId);
    EXPECT_EQ(listener.removedCount, 1);

    InstanceRegistry::getInstance().removeListener(&listener);
}

// =============================================================================
// Edge Case Tests - Null/Invalid Inputs
// =============================================================================

// Test: Null buffer registration
TEST_F(InstanceRegistryTest, RegisterNullBuffer)
{
    std::shared_ptr<SharedCaptureBuffer> nullBuffer = nullptr;

    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "track_null", nullBuffer, "Null Buffer Track");

    // Should still register (buffer might be set later)
    // But getCaptureBuffer should return nullptr
    if (sourceId.isValid())
    {
        auto retrievedBuffer = InstanceRegistry::getInstance().getCaptureBuffer(sourceId);
        EXPECT_EQ(retrievedBuffer, nullptr);
    }
}

// Test: Empty track identifier
TEST_F(InstanceRegistryTest, EmptyTrackIdentifier)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "", buffer, "Empty ID Track");

    // Empty string is a valid key - should work
    EXPECT_TRUE(sourceId.isValid());
}

// Test: Unicode track identifier
TEST_F(InstanceRegistryTest, UnicodeTrackIdentifier)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        u8"日本語トラック", buffer, "Japanese Track");

    EXPECT_TRUE(sourceId.isValid());

    auto sourceInfo = InstanceRegistry::getInstance().getSource(sourceId);
    ASSERT_TRUE(sourceInfo.has_value());
    EXPECT_EQ(sourceInfo->name, "Japanese Track");
}

// Test: Special characters in track identifier
TEST_F(InstanceRegistryTest, SpecialCharactersInTrackId)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "track/with\\special:chars*?\"<>|", buffer, "Special Track");

    EXPECT_TRUE(sourceId.isValid());
}

// Test: Very long track identifier
TEST_F(InstanceRegistryTest, VeryLongTrackIdentifier)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    juce::String longId;
    for (int i = 0; i < 10000; ++i)
        longId += "a";

    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        longId, buffer, "Long ID Track");

    // Should handle long strings gracefully
    EXPECT_TRUE(sourceId.isValid());
}

// =============================================================================
// Edge Case Tests - Non-existent Sources
// =============================================================================

// Test: Get non-existent source
TEST_F(InstanceRegistryTest, GetNonExistentSource)
{
    auto fakeId = SourceId::generate();

    auto sourceInfo = InstanceRegistry::getInstance().getSource(fakeId);
    EXPECT_FALSE(sourceInfo.has_value());
}

// Test: Unregister non-existent source
TEST_F(InstanceRegistryTest, UnregisterNonExistentSource)
{
    auto fakeId = SourceId::generate();
    size_t countBefore = InstanceRegistry::getInstance().getSourceCount();

    // Should not crash or throw
    InstanceRegistry::getInstance().unregisterInstance(fakeId);

    EXPECT_EQ(InstanceRegistry::getInstance().getSourceCount(), countBefore);
}

// Test: Update non-existent source
TEST_F(InstanceRegistryTest, UpdateNonExistentSource)
{
    auto fakeId = SourceId::generate();

    // Should not crash
    InstanceRegistry::getInstance().updateSource(fakeId, "Fake Name", 2, 44100.0);

    // Source still shouldn't exist
    EXPECT_FALSE(InstanceRegistry::getInstance().getSource(fakeId).has_value());
}

// Test: Get buffer for non-existent source
TEST_F(InstanceRegistryTest, GetBufferNonExistentSource)
{
    auto fakeId = SourceId::generate();

    auto buffer = InstanceRegistry::getInstance().getCaptureBuffer(fakeId);
    EXPECT_EQ(buffer, nullptr);
}

// =============================================================================
// Edge Case Tests - Re-registration and Lifecycle
// =============================================================================

// Test: Re-register after unregistration with same track ID
TEST_F(InstanceRegistryTest, ReRegisterAfterUnregister)
{
    auto buffer1 = std::make_shared<SharedCaptureBuffer>();

    auto sourceId1 = InstanceRegistry::getInstance().registerInstance(
        "track_reuse", buffer1, "First Registration");
    EXPECT_TRUE(sourceId1.isValid());

    InstanceRegistry::getInstance().unregisterInstance(sourceId1);
    EXPECT_EQ(InstanceRegistry::getInstance().getSourceCount(), 0);

    auto buffer2 = std::make_shared<SharedCaptureBuffer>();
    auto sourceId2 = InstanceRegistry::getInstance().registerInstance(
        "track_reuse", buffer2, "Second Registration");

    // Should get a new (different) source ID since the old one was unregistered
    EXPECT_TRUE(sourceId2.isValid());
    EXPECT_EQ(InstanceRegistry::getInstance().getSourceCount(), 1);
}

// Test: Buffer expires (weak_ptr scenario)
TEST_F(InstanceRegistryTest, BufferExpires)
{
    SourceId sourceId;
    {
        auto buffer = std::make_shared<SharedCaptureBuffer>();
        sourceId = InstanceRegistry::getInstance().registerInstance(
            "track_expire", buffer, "Expiring Track");
        EXPECT_TRUE(sourceId.isValid());

        auto retrievedBuffer = InstanceRegistry::getInstance().getCaptureBuffer(sourceId);
        EXPECT_NE(retrievedBuffer, nullptr);
    }
    // buffer goes out of scope here

    // Source info should still exist
    auto sourceInfo = InstanceRegistry::getInstance().getSource(sourceId);
    EXPECT_TRUE(sourceInfo.has_value());

    // But buffer should be nullptr (weak_ptr expired)
    auto expiredBuffer = InstanceRegistry::getInstance().getCaptureBuffer(sourceId);
    EXPECT_EQ(expiredBuffer, nullptr);
}

// =============================================================================
// Edge Case Tests - Multiple Listeners
// =============================================================================

// Test: Multiple listeners
TEST_F(InstanceRegistryTest, MultipleListeners)
{
    TestRegistryListener listener1;
    TestRegistryListener listener2;
    TestRegistryListener listener3;

    InstanceRegistry::getInstance().addListener(&listener1);
    InstanceRegistry::getInstance().addListener(&listener2);
    InstanceRegistry::getInstance().addListener(&listener3);

    auto buffer = std::make_shared<SharedCaptureBuffer>();
    InstanceRegistry::getInstance().registerInstance("track_multi", buffer, "Multi Track");

    EXPECT_EQ(listener1.addedCount, 1);
    EXPECT_EQ(listener2.addedCount, 1);
    EXPECT_EQ(listener3.addedCount, 1);

    InstanceRegistry::getInstance().removeListener(&listener1);
    InstanceRegistry::getInstance().removeListener(&listener2);
    InstanceRegistry::getInstance().removeListener(&listener3);
}

// Test: Remove listener twice (should not crash)
TEST_F(InstanceRegistryTest, RemoveListenerTwice)
{
    TestRegistryListener listener;

    InstanceRegistry::getInstance().addListener(&listener);
    InstanceRegistry::getInstance().removeListener(&listener);

    // Second removal should be safe
    InstanceRegistry::getInstance().removeListener(&listener);

    // Registering should not notify removed listener
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    InstanceRegistry::getInstance().registerInstance("track_removed", buffer, "Track");

    EXPECT_EQ(listener.addedCount, 0);
}

// Test: Listener that removes itself during callback
class SelfRemovingListener : public InstanceRegistryListener
{
public:
    int callCount = 0;

    void sourceAdded(const SourceId&) override
    {
        callCount++;
        InstanceRegistry::getInstance().removeListener(this);
    }

    void sourceRemoved(const SourceId&) override { callCount++; }
    void sourceUpdated(const SourceId&) override { callCount++; }
};

TEST_F(InstanceRegistryTest, ListenerRemovesDuringCallback)
{
    SelfRemovingListener listener;
    InstanceRegistry::getInstance().addListener(&listener);

    auto buffer1 = std::make_shared<SharedCaptureBuffer>();
    auto buffer2 = std::make_shared<SharedCaptureBuffer>();

    InstanceRegistry::getInstance().registerInstance("track_self1", buffer1, "Track 1");
    InstanceRegistry::getInstance().registerInstance("track_self2", buffer2, "Track 2");

    // Should only have been called once (removed self during first call)
    EXPECT_EQ(listener.callCount, 1);
}

// =============================================================================
// Concurrency Tests
// =============================================================================

// Test: Concurrent registration
TEST_F(InstanceRegistryTest, ConcurrentRegistration)
{
    constexpr int numThreads = 10;
    constexpr int registrationsPerThread = 10;

    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<SharedCaptureBuffer>> buffers;
    std::atomic<int> successCount{0};

    // Pre-create buffers
    for (int i = 0; i < numThreads * registrationsPerThread; ++i)
    {
        buffers.push_back(std::make_shared<SharedCaptureBuffer>());
    }

    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back([t, &buffers, &successCount, registrationsPerThread]() {
            for (int i = 0; i < registrationsPerThread; ++i)
            {
                int idx = t * registrationsPerThread + i;
                auto sourceId = InstanceRegistry::getInstance().registerInstance(
                    "concurrent_track_" + juce::String(idx),
                    buffers[static_cast<size_t>(idx)],
                    "Track " + juce::String(idx));

                if (sourceId.isValid())
                    successCount++;
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // All should succeed (64 or fewer unique tracks)
    EXPECT_EQ(successCount.load(), std::min(64, numThreads * registrationsPerThread));
}

// Test: Concurrent read and write
TEST_F(InstanceRegistryTest, ConcurrentReadWrite)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "concurrent_rw", buffer, "RW Track");

    std::atomic<bool> stopFlag{false};
    std::atomic<int> readCount{0};
    std::atomic<int> updateCount{0};

    // Reader thread
    std::thread reader([&]() {
        while (!stopFlag.load())
        {
            auto info = InstanceRegistry::getInstance().getSource(sourceId);
            if (info.has_value())
                readCount++;
        }
    });

    // Writer thread
    std::thread writer([&]() {
        for (int i = 0; i < 100; ++i)
        {
            InstanceRegistry::getInstance().updateSource(
                sourceId,
                "Name " + juce::String(i),
                (i % 2) + 1,
                44100.0 + static_cast<double>(i));
            updateCount++;
        }
        stopFlag.store(true);
    });

    reader.join();
    writer.join();

    EXPECT_EQ(updateCount.load(), 100);
    EXPECT_GT(readCount.load(), 0);
}

// Test: Concurrent registration and unregistration
TEST_F(InstanceRegistryTest, ConcurrentRegisterUnregister)
{
    constexpr int iterations = 100;

    std::atomic<int> registerSuccess{0};
    std::atomic<int> unregisterCalls{0};

    std::thread registerThread([&]() {
        for (int i = 0; i < iterations; ++i)
        {
            auto buffer = std::make_shared<SharedCaptureBuffer>();
            auto sourceId = InstanceRegistry::getInstance().registerInstance(
                "churn_track_" + juce::String(i % 10), buffer, "Track");

            if (sourceId.isValid())
                registerSuccess++;
        }
    });

    std::thread unregisterThread([&]() {
        for (int i = 0; i < iterations; ++i)
        {
            auto sources = InstanceRegistry::getInstance().getAllSources();
            for (const auto& source : sources)
            {
                InstanceRegistry::getInstance().unregisterInstance(source.sourceId);
                unregisterCalls++;
                break; // Just remove one at a time
            }
        }
    });

    registerThread.join();
    unregisterThread.join();

    // Should complete without crashes or deadlocks
    EXPECT_GT(registerSuccess.load(), 0);
}

// =============================================================================
// Edge Case Tests - Boundary Values
// =============================================================================

// Test: Zero channel count
TEST_F(InstanceRegistryTest, ZeroChannelCount)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "track_zero_ch", buffer, "Zero Channels", 0, 44100.0);

    EXPECT_TRUE(sourceId.isValid());

    auto info = InstanceRegistry::getInstance().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->channelCount, 0);
}

// Test: Negative sample rate
TEST_F(InstanceRegistryTest, NegativeSampleRate)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "track_neg_sr", buffer, "Negative SR", 2, -44100.0);

    // Should accept any value (validation is caller's responsibility)
    EXPECT_TRUE(sourceId.isValid());
}

// Test: Very high channel count
TEST_F(InstanceRegistryTest, HighChannelCount)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "track_many_ch", buffer, "Many Channels", 1000, 44100.0);

    EXPECT_TRUE(sourceId.isValid());

    auto info = InstanceRegistry::getInstance().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->channelCount, 1000);
}

// Test: Zero sample rate
TEST_F(InstanceRegistryTest, ZeroSampleRate)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "track_zero_sr", buffer, "Zero SR", 2, 0.0);

    EXPECT_TRUE(sourceId.isValid());
}

// Test: Extreme sample rate
TEST_F(InstanceRegistryTest, ExtremeSampleRate)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "track_extreme_sr", buffer, "Extreme SR", 2, 10000000.0);

    EXPECT_TRUE(sourceId.isValid());

    auto info = InstanceRegistry::getInstance().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->sampleRate, 10000000.0);
}

// =============================================================================
// Edge Case Tests - Active State
// =============================================================================

// Test: Source active state after registration
TEST_F(InstanceRegistryTest, SourceActiveStateAfterRegistration)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = InstanceRegistry::getInstance().registerInstance(
        "track_active", buffer, "Active Track");

    auto info = InstanceRegistry::getInstance().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_TRUE(info->active.load());
}

// Test: getAllSources returns copies not references
TEST_F(InstanceRegistryTest, GetAllSourcesReturnsCopies)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    InstanceRegistry::getInstance().registerInstance(
        "track_copy", buffer, "Original Name");

    auto sources = InstanceRegistry::getInstance().getAllSources();
    ASSERT_EQ(sources.size(), 1);

    // Modifying the copy shouldn't affect the registry
    sources[0].name = "Modified Name";

    auto freshSources = InstanceRegistry::getInstance().getAllSources();
    EXPECT_EQ(freshSources[0].name, "Original Name");
}
