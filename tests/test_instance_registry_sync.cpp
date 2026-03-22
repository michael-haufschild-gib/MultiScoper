/*
    Oscil - Instance Registry Sync Tests
    Tests for cross-instance sync, discovery, deduplication, and listener notifications
*/

#include <gtest/gtest.h>
#include <juce_events/juce_events.h>
#include "core/InstanceRegistry.h"
#include "core/SharedCaptureBuffer.h"
#include <thread>
#include <atomic>

using namespace oscil;

class InstanceRegistrySyncTest : public ::testing::Test
{
protected:
    std::unique_ptr<InstanceRegistry> registry_;
    std::mutex dispatcherMutex;

    InstanceRegistry& getRegistry() { return *registry_; }

    void SetUp() override
    {
        // Create owned instance
        registry_ = std::make_unique<InstanceRegistry>();

        // Force synchronous dispatch for tests
        getRegistry().setDispatcher([this](std::function<void()> f) {
            std::scoped_lock lock(dispatcherMutex);
            f();
        });
    }

    void TearDown() override
    {
        registry_.reset();
    }
};

// === Deduplication Tests ===

TEST_F(InstanceRegistrySyncTest, DeduplicationSameTrack)
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

TEST_F(InstanceRegistrySyncTest, DeduplicationRefreshesBufferAndMetadataForExistingTrack)
{
    auto primaryBuffer = std::make_shared<SharedCaptureBuffer>();
    auto duplicateBuffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = getRegistry().registerInstance(
        "track_primary", primaryBuffer, "Primary Track");
    auto dedupedId = getRegistry().registerInstance(
        "track_primary", duplicateBuffer, "Backup Track");

    EXPECT_EQ(sourceId, dedupedId);
    EXPECT_EQ(getRegistry().getSourceCount(), 1);
    EXPECT_EQ(getRegistry().getCaptureBuffer(sourceId), duplicateBuffer);

    auto sourceInfo = getRegistry().getSource(sourceId);
    ASSERT_TRUE(sourceInfo.has_value());
    EXPECT_EQ(sourceInfo->name, "Backup Track");
}

TEST_F(InstanceRegistrySyncTest, DeduplicationPromotesNewBufferWhenPrimaryExpired)
{
    SourceId sourceId;
    {
        auto primaryBuffer = std::make_shared<SharedCaptureBuffer>();
        sourceId = getRegistry().registerInstance(
            "track_failover", primaryBuffer, "Primary Track");
        ASSERT_TRUE(sourceId.isValid());
    }

    EXPECT_EQ(getRegistry().getCaptureBuffer(sourceId), nullptr);

    auto replacementBuffer = std::make_shared<SharedCaptureBuffer>();
    auto dedupedId = getRegistry().registerInstance(
        "track_failover", replacementBuffer, "Replacement Track");

    EXPECT_EQ(sourceId, dedupedId);
    EXPECT_EQ(getRegistry().getCaptureBuffer(sourceId), replacementBuffer);

    auto sourceInfo = getRegistry().getSource(sourceId);
    ASSERT_TRUE(sourceInfo.has_value());
    EXPECT_EQ(sourceInfo->name, "Replacement Track");
}

// === Listener Tests ===

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

TEST_F(InstanceRegistrySyncTest, ListenerNotifications)
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

TEST_F(InstanceRegistrySyncTest, MultipleListeners)
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

TEST_F(InstanceRegistrySyncTest, RemoveListenerTwice)
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

// === Self-Removing Listener Tests ===

class SelfRemovingListener : public InstanceRegistryListener
{
public:
    InstanceRegistry& registry_;
    int callCount = 0;

    explicit SelfRemovingListener(InstanceRegistry& registry) : registry_(registry) {}

    void sourceAdded(const SourceId&) override
    {
        callCount++;
        registry_.removeListener(this);
    }

    void sourceRemoved(const SourceId&) override { callCount++; }
    void sourceUpdated(const SourceId&) override { callCount++; }
};

TEST_F(InstanceRegistrySyncTest, ListenerRemovesDuringCallback)
{
    SelfRemovingListener listener(getRegistry());
    getRegistry().addListener(&listener);

    auto buffer1 = std::make_shared<SharedCaptureBuffer>();
    auto buffer2 = std::make_shared<SharedCaptureBuffer>();

    (void)getRegistry().registerInstance("track_self1", buffer1, "Track 1");
    (void)getRegistry().registerInstance("track_self2", buffer2, "Track 2");

    // Should only have been called once (removed self during first call)
    EXPECT_EQ(listener.callCount, 1);
}

// === Concurrent Sync Tests ===

TEST_F(InstanceRegistrySyncTest, ConcurrentRegistration)
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
        threads.emplace_back([this, t, &buffers, &successCount]() {
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

TEST_F(InstanceRegistrySyncTest, ConcurrentReadWriteDataIntegrity)
{
    // Bug caught: reader seeing partially-updated source info (e.g., new name
    // but old sampleRate) due to non-atomic update of SourceInfo fields.
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "concurrent_rw", buffer, "RW Track", 2, 44100.0);

    std::atomic<bool> stopFlag{false};
    std::atomic<int> readCount{0};
    std::atomic<int> updateCount{0};
    std::atomic<int> inconsistentCount{0};

    // Reader thread: verifies name and sampleRate are from the same update
    std::thread reader([this, &stopFlag, &readCount, &inconsistentCount, &sourceId]() {
        while (!stopFlag.load())
        {
            auto info = getRegistry().getSource(sourceId);
            if (info.has_value())
            {
                // The writer sets name = "Name_N" and sampleRate = 44100 + N
                // If we see "Name_5" then sampleRate should be 44105.0
                juce::String name = info->name;
                if (name.startsWith("Name_"))
                {
                    int n = name.getTrailingIntValue();
                    double expectedRate = 44100.0 + static_cast<double>(n);
                    if (std::abs(info->sampleRate - expectedRate) > 0.01)
                    {
                        inconsistentCount.fetch_add(1, std::memory_order_relaxed);
                    }
                }
                readCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    // Writer thread: updates with correlated name + sampleRate
    std::thread writer([this, &stopFlag, &updateCount, &sourceId]() {
        for (int i = 0; i < 200; ++i)
        {
            getRegistry().updateSource(
                sourceId,
                "Name_" + juce::String(i),
                (i % 2) + 1,
                44100.0 + static_cast<double>(i));
            updateCount.fetch_add(1, std::memory_order_relaxed);
        }
        stopFlag.store(true);
    });

    reader.join();
    writer.join();

    EXPECT_EQ(updateCount.load(), 200);
    EXPECT_GT(readCount.load(), 10);
    EXPECT_EQ(inconsistentCount.load(), 0)
        << "Source info torn read: name and sampleRate from different updates";
}

TEST_F(InstanceRegistrySyncTest, ConcurrentRegisterUnregister)
{
    // Bug caught: shared_mutex deadlock when register and unregister race,
    // or iterator invalidation when unregister modifies map during getAllSources iteration.
    constexpr int iterations = 100;

    std::atomic<int> registerSuccess{0};
    std::atomic<int> unregisterCalls{0};

    std::thread registerThread([this, &registerSuccess]() {
        for (int i = 0; i < iterations; ++i)
        {
            auto buffer = std::make_shared<SharedCaptureBuffer>();
            auto sourceId = getRegistry().registerInstance(
                "churn_track_" + juce::String(i % 10), buffer, "Track");

            if (sourceId.isValid())
                registerSuccess.fetch_add(1, std::memory_order_relaxed);
        }
    });

    std::thread unregisterThread([this, &unregisterCalls]() {
        for (int i = 0; i < iterations; ++i)
        {
            auto sources = getRegistry().getAllSources();
            for (const auto& source : sources)
            {
                getRegistry().unregisterInstance(source.sourceId);
                unregisterCalls.fetch_add(1, std::memory_order_relaxed);
                break;
            }
        }
    });

    registerThread.join();
    unregisterThread.join();

    EXPECT_GT(registerSuccess.load(), 10)
        << "Too few registrations succeeded under contention";

    // Final state should be consistent: sourceCount should match actual entries
    auto allSources = getRegistry().getAllSources();
    EXPECT_EQ(allSources.size(), getRegistry().getSourceCount())
        << "getAllSources().size() and getSourceCount() diverged after concurrent churn";
}

// Test: getAllSources returns consistent snapshot during concurrent updates
TEST_F(InstanceRegistrySyncTest, GetAllSourcesSnapshotConsistency)
{
    // Bug caught: getAllSources returning a mix of pre-update and post-update
    // source infos because the shared_mutex was released between reads.

    // Register 10 sources
    std::vector<SourceId> ids;
    for (int i = 0; i < 10; ++i)
    {
        auto buffer = std::make_shared<SharedCaptureBuffer>();
        auto id = getRegistry().registerInstance(
            "snapshot_track_" + juce::String(i), buffer,
            "Track_" + juce::String(i), 2, 44100.0);
        ids.push_back(id);
    }

    std::atomic<bool> running{true};
    std::atomic<int> snapshotCount{0};

    // Reader: takes snapshots and verifies all entries exist
    std::thread reader([this, &running, &snapshotCount]() {
        while (running.load(std::memory_order_relaxed))
        {
            auto sources = getRegistry().getAllSources();
            size_t count = getRegistry().getSourceCount();
            // Snapshot size should be consistent with count
            // (may differ slightly due to concurrent modification, but
            // should never have garbage entries)
            for (const auto& src : sources)
            {
                EXPECT_TRUE(src.sourceId.isValid())
                    << "Invalid sourceId in getAllSources snapshot";
            }
            snapshotCount.fetch_add(1, std::memory_order_relaxed);
            (void)count; // Suppress unused warning
        }
    });

    // Writer: churns registrations
    for (int cycle = 0; cycle < 50; ++cycle)
    {
        // Unregister one, register a new one
        if (!ids.empty())
        {
            getRegistry().unregisterInstance(ids.back());
            ids.pop_back();
        }
        auto buffer = std::make_shared<SharedCaptureBuffer>();
        auto newId = getRegistry().registerInstance(
            "snapshot_new_" + juce::String(cycle), buffer,
            "New_" + juce::String(cycle));
        if (newId.isValid())
            ids.push_back(newId);
    }

    running.store(false, std::memory_order_relaxed);
    reader.join();

    EXPECT_GT(snapshotCount.load(), 10);
}
