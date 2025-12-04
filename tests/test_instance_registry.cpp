/*
    Oscil - Instance Registry Tests
*/

#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <juce_events/juce_events.h>
#include "core/InstanceRegistry.h"
#include "core/SharedCaptureBuffer.h"

using namespace oscil;

class InstanceRegistryTest : public ::testing::Test
{
protected:
    std::mutex dispatcherMutex;

    // Helper to access registry (friend access doesn't inherit to TEST_F generated classes)
    static InstanceRegistry& getRegistry() { return InstanceRegistry::getInstance(); }

    void SetUp() override
    {
        // Force synchronous dispatch for tests, serialized with a mutex
        // to mimic MessageManager's single-threaded execution and prevent
        // ListenerList concurrency assertions during multi-threaded tests.
        getRegistry().setDispatcher([this](std::function<void()> f) {
            std::scoped_lock lock(dispatcherMutex);
            f();
        });

        // Clear any existing registrations
        auto sources = getRegistry().getAllSources();
        for (const auto& source : sources)
        {
            getRegistry().unregisterInstance(source.sourceId);
        }
    }

    void TearDown() override
    {
        // Clean up
        auto sources = getRegistry().getAllSources();
        for (const auto& source : sources)
        {
            getRegistry().unregisterInstance(source.sourceId);
        }
        
        // Reset dispatcher to default async behavior
        getRegistry().setDispatcher([](std::function<void()> f) {
             juce::MessageManager::callAsync(std::move(f));
        });
    }
};

// Test: Basic registration
TEST_F(InstanceRegistryTest, RegisterInstance)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_1", buffer, "Track 1", 2, 44100.0);

    EXPECT_TRUE(sourceId.isValid());
    EXPECT_EQ(getRegistry().getSourceCount(), 1);
}

// Test: Registration returns source info
TEST_F(InstanceRegistryTest, GetSourceInfo)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_1", buffer, "My Track", 2, 48000.0);

    auto sourceInfo = getRegistry().getSource(sourceId);

    ASSERT_TRUE(sourceInfo.has_value());
    EXPECT_EQ(sourceInfo->name, "My Track");
    EXPECT_EQ(sourceInfo->channelCount, 2);
    EXPECT_EQ(sourceInfo->sampleRate, 48000.0);
}

// Test: Unregistration removes source
TEST_F(InstanceRegistryTest, UnregisterInstance)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_1", buffer, "Track 1");

    EXPECT_EQ(getRegistry().getSourceCount(), 1);

    getRegistry().unregisterInstance(sourceId);

    EXPECT_EQ(getRegistry().getSourceCount(), 0);
    EXPECT_FALSE(getRegistry().getSource(sourceId).has_value());
}

// Test: Deduplication - same track ID returns same source
TEST_F(InstanceRegistryTest, DeduplicationSameTrack)
{
    auto buffer1 = std::make_shared<SharedCaptureBuffer>();
    auto buffer2 = std::make_shared<SharedCaptureBuffer>();

    auto sourceId1 = getRegistry().registerInstance(
        "track_1", buffer1, "Track 1");
    auto sourceId2 = getRegistry().registerInstance(
        "track_1", buffer2, "Track 1 Copy");

    // Should return the same source ID
    EXPECT_EQ(sourceId1, sourceId2);
    EXPECT_EQ(getRegistry().getSourceCount(), 1);
}

// Test: Different tracks get different source IDs
TEST_F(InstanceRegistryTest, DifferentTracksGetDifferentIds)
{
    auto buffer1 = std::make_shared<SharedCaptureBuffer>();
    auto buffer2 = std::make_shared<SharedCaptureBuffer>();

    auto sourceId1 = getRegistry().registerInstance(
        "track_1", buffer1, "Track 1");
    auto sourceId2 = getRegistry().registerInstance(
        "track_2", buffer2, "Track 2");

    EXPECT_NE(sourceId1, sourceId2);
    EXPECT_EQ(getRegistry().getSourceCount(), 2);
}

// Test: Get all sources
TEST_F(InstanceRegistryTest, GetAllSources)
{
    auto buffer1 = std::make_shared<SharedCaptureBuffer>();
    auto buffer2 = std::make_shared<SharedCaptureBuffer>();
    auto buffer3 = std::make_shared<SharedCaptureBuffer>();

    (void)getRegistry().registerInstance("track_1", buffer1, "Track 1");
    (void)getRegistry().registerInstance("track_2", buffer2, "Track 2");
    (void)getRegistry().registerInstance("track_3", buffer3, "Track 3");

    auto sources = getRegistry().getAllSources();

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
        auto sourceId = getRegistry().registerInstance(
            "track_" + juce::String(i), buffer, "Track " + juce::String(i));
        sourceIds.push_back(sourceId);
    }

    EXPECT_EQ(getRegistry().getSourceCount(), 64);

    // 65th should fail
    auto buffer65 = std::make_shared<SharedCaptureBuffer>();
    auto sourceId65 = getRegistry().registerInstance(
        "track_65", buffer65, "Track 65");

    EXPECT_FALSE(sourceId65.isValid());
    EXPECT_EQ(getRegistry().getSourceCount(), 64);
}

// Test: Get capture buffer
TEST_F(InstanceRegistryTest, GetCaptureBuffer)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_1", buffer, "Track 1");

    auto retrievedBuffer = getRegistry().getCaptureBuffer(sourceId);

    EXPECT_EQ(retrievedBuffer, buffer);
}

// Test: Update source metadata
TEST_F(InstanceRegistryTest, UpdateSource)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_1", buffer, "Track 1", 2, 44100.0);

    getRegistry().updateSource(sourceId, "Renamed Track", 1, 96000.0);

    auto sourceInfo = getRegistry().getSource(sourceId);

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
    getRegistry().addListener(&listener);

    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_1", buffer, "Track 1");

    EXPECT_EQ(listener.addedCount, 1);
    EXPECT_EQ(listener.lastSourceId, sourceId);

    getRegistry().updateSource(sourceId, "New Name", 2, 44100.0);
    EXPECT_EQ(listener.updatedCount, 1);

    getRegistry().unregisterInstance(sourceId);
    EXPECT_EQ(listener.removedCount, 1);

    getRegistry().removeListener(&listener);
}

// =============================================================================
// Edge Case Tests - Null/Invalid Inputs
// =============================================================================

// Test: Null buffer registration
TEST_F(InstanceRegistryTest, RegisterNullBuffer)
{
    std::shared_ptr<SharedCaptureBuffer> nullBuffer = nullptr;

    auto sourceId = getRegistry().registerInstance(
        "track_null", nullBuffer, "Null Buffer Track");

    // Should still register (buffer might be set later)
    // But getCaptureBuffer should return nullptr
    if (sourceId.isValid())
    {
        auto retrievedBuffer = getRegistry().getCaptureBuffer(sourceId);
        EXPECT_EQ(retrievedBuffer, nullptr);
    }
}

// Test: Empty track identifier
TEST_F(InstanceRegistryTest, EmptyTrackIdentifier)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = getRegistry().registerInstance(
        "", buffer, "Empty ID Track");

    // Empty string is a valid key - should work
    EXPECT_TRUE(sourceId.isValid());
}

// Test: Unicode track identifier
TEST_F(InstanceRegistryTest, UnicodeTrackIdentifier)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = getRegistry().registerInstance(
        u8"日本語トラック", buffer, "Japanese Track");

    EXPECT_TRUE(sourceId.isValid());

    auto sourceInfo = getRegistry().getSource(sourceId);
    ASSERT_TRUE(sourceInfo.has_value());
    EXPECT_EQ(sourceInfo->name, "Japanese Track");
}

// Test: Special characters in track identifier
TEST_F(InstanceRegistryTest, SpecialCharactersInTrackId)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = getRegistry().registerInstance(
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

    auto sourceId = getRegistry().registerInstance(
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

    auto sourceInfo = getRegistry().getSource(fakeId);
    EXPECT_FALSE(sourceInfo.has_value());
}

// Test: Unregister non-existent source
TEST_F(InstanceRegistryTest, UnregisterNonExistentSource)
{
    auto fakeId = SourceId::generate();
    size_t countBefore = getRegistry().getSourceCount();

    // Should not crash or throw
    getRegistry().unregisterInstance(fakeId);

    EXPECT_EQ(getRegistry().getSourceCount(), countBefore);
}

// Test: Update non-existent source
TEST_F(InstanceRegistryTest, UpdateNonExistentSource)
{
    auto fakeId = SourceId::generate();

    // Should not crash
    getRegistry().updateSource(fakeId, "Fake Name", 2, 44100.0);

    // Source still shouldn't exist
    EXPECT_FALSE(getRegistry().getSource(fakeId).has_value());
}

// Test: Get buffer for non-existent source
TEST_F(InstanceRegistryTest, GetBufferNonExistentSource)
{
    auto fakeId = SourceId::generate();

    auto buffer = getRegistry().getCaptureBuffer(fakeId);
    EXPECT_EQ(buffer, nullptr);
}

// =============================================================================
// Edge Case Tests - Re-registration and Lifecycle
// =============================================================================

// Test: Re-register after unregistration with same track ID
TEST_F(InstanceRegistryTest, ReRegisterAfterUnregister)
{
    auto buffer1 = std::make_shared<SharedCaptureBuffer>();

    auto sourceId1 = getRegistry().registerInstance(
        "track_reuse", buffer1, "First Registration");
    EXPECT_TRUE(sourceId1.isValid());

    getRegistry().unregisterInstance(sourceId1);
    EXPECT_EQ(getRegistry().getSourceCount(), 0);

    auto buffer2 = std::make_shared<SharedCaptureBuffer>();
    auto sourceId2 = getRegistry().registerInstance(
        "track_reuse", buffer2, "Second Registration");

    // Should get a new (different) source ID since the old one was unregistered
    EXPECT_TRUE(sourceId2.isValid());
    EXPECT_EQ(getRegistry().getSourceCount(), 1);
}

// Test: Buffer expires (weak_ptr scenario)
TEST_F(InstanceRegistryTest, BufferExpires)
{
    SourceId sourceId;
    {
        auto buffer = std::make_shared<SharedCaptureBuffer>();
        sourceId = getRegistry().registerInstance(
            "track_expire", buffer, "Expiring Track");
        EXPECT_TRUE(sourceId.isValid());

        auto retrievedBuffer = getRegistry().getCaptureBuffer(sourceId);
        EXPECT_NE(retrievedBuffer, nullptr);
    }
    // buffer goes out of scope here

    // Source info should still exist
    auto sourceInfo = getRegistry().getSource(sourceId);
    EXPECT_TRUE(sourceInfo.has_value());

    // But buffer should be nullptr (weak_ptr expired)
    auto expiredBuffer = getRegistry().getCaptureBuffer(sourceId);
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

    getRegistry().addListener(&listener1);
    getRegistry().addListener(&listener2);
    getRegistry().addListener(&listener3);

    auto buffer = std::make_shared<SharedCaptureBuffer>();
    (void)getRegistry().registerInstance("track_multi", buffer, "Multi Track");

    EXPECT_EQ(listener1.addedCount, 1);
    EXPECT_EQ(listener2.addedCount, 1);
    EXPECT_EQ(listener3.addedCount, 1);

    getRegistry().removeListener(&listener1);
    getRegistry().removeListener(&listener2);
    getRegistry().removeListener(&listener3);
}

// Test: Remove listener twice (should not crash)
TEST_F(InstanceRegistryTest, RemoveListenerTwice)
{
    TestRegistryListener listener;

    getRegistry().addListener(&listener);
    getRegistry().removeListener(&listener);

    // Second removal should be safe
    getRegistry().removeListener(&listener);

    // Registering should not notify removed listener
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    (void)getRegistry().registerInstance("track_removed", buffer, "Track");

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
        getRegistry().removeListener(this);
    }

    void sourceRemoved(const SourceId&) override { callCount++; }
    void sourceUpdated(const SourceId&) override { callCount++; }
};

TEST_F(InstanceRegistryTest, ListenerRemovesDuringCallback)
{
    SelfRemovingListener listener;
    getRegistry().addListener(&listener);

    auto buffer1 = std::make_shared<SharedCaptureBuffer>();
    auto buffer2 = std::make_shared<SharedCaptureBuffer>();

    (void)getRegistry().registerInstance("track_self1", buffer1, "Track 1");
    (void)getRegistry().registerInstance("track_self2", buffer2, "Track 2");

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
                auto sourceId = getRegistry().registerInstance(
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
    auto sourceId = getRegistry().registerInstance(
        "concurrent_rw", buffer, "RW Track");

    std::atomic<bool> stopFlag{false};
    std::atomic<int> readCount{0};
    std::atomic<int> updateCount{0};

    // Reader thread
    std::thread reader([&]() {
        while (!stopFlag.load())
        {
            auto info = getRegistry().getSource(sourceId);
            if (info.has_value())
                readCount++;
        }
    });

    // Writer thread
    std::thread writer([&]() {
        for (int i = 0; i < 100; ++i)
        {
            getRegistry().updateSource(
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
            auto sourceId = getRegistry().registerInstance(
                "churn_track_" + juce::String(i % 10), buffer, "Track");

            if (sourceId.isValid())
                registerSuccess++;
        }
    });

    std::thread unregisterThread([&]() {
        for (int i = 0; i < iterations; ++i)
        {
            auto sources = getRegistry().getAllSources();
            for (const auto& source : sources)
            {
                getRegistry().unregisterInstance(source.sourceId);
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
    auto sourceId = getRegistry().registerInstance(
        "track_zero_ch", buffer, "Zero Channels", 0, 44100.0);

    EXPECT_TRUE(sourceId.isValid());

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->channelCount, 0);
}

// Test: Negative sample rate
TEST_F(InstanceRegistryTest, NegativeSampleRate)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_neg_sr", buffer, "Negative SR", 2, -44100.0);

    // Should accept any value (validation is caller's responsibility)
    EXPECT_TRUE(sourceId.isValid());
}

// Test: Very high channel count
TEST_F(InstanceRegistryTest, HighChannelCount)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_many_ch", buffer, "Many Channels", 1000, 44100.0);

    EXPECT_TRUE(sourceId.isValid());

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->channelCount, 1000);
}

// Test: Zero sample rate
TEST_F(InstanceRegistryTest, ZeroSampleRate)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_zero_sr", buffer, "Zero SR", 2, 0.0);

    EXPECT_TRUE(sourceId.isValid());
}

// Test: Extreme sample rate
TEST_F(InstanceRegistryTest, ExtremeSampleRate)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_extreme_sr", buffer, "Extreme SR", 2, 10000000.0);

    EXPECT_TRUE(sourceId.isValid());

    auto info = getRegistry().getSource(sourceId);
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
    auto sourceId = getRegistry().registerInstance(
        "track_active", buffer, "Active Track");

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_TRUE(info->active.load());
}

// Test: getAllSources returns copies not references
TEST_F(InstanceRegistryTest, GetAllSourcesReturnsCopies)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    (void)getRegistry().registerInstance(
        "track_copy", buffer, "Original Name");

    auto sources = getRegistry().getAllSources();
    ASSERT_EQ(sources.size(), 1);

    // Modifying the copy shouldn't affect the registry
    sources[0].name = "Modified Name";

    auto freshSources = getRegistry().getAllSources();
    EXPECT_EQ(freshSources[0].name, "Original Name");
}
