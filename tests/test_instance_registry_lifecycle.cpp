/*
    Oscil - Instance Registry Lifecycle Tests
    Tests for startup, shutdown, cleanup, null inputs, boundaries, and lifecycle scenarios
*/

#include "core/InstanceRegistry.h"
#include "core/SharedCaptureBuffer.h"

#include "Oscil.h"

#include <juce_events/juce_events.h>

#include <gtest/gtest.h>

namespace oscil
{

class InstanceRegistryLifecycleTest : public ::testing::Test
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

    void TearDown() override { registry_.reset(); }
};

class CountingRegistryListener final : public InstanceRegistryListener
{
public:
    void sourceAdded(const SourceId&) override { ++addedCount; }
    void sourceRemoved(const SourceId&) override { ++removedCount; }
    void sourceUpdated(const SourceId&) override { ++updatedCount; }

    int addedCount = 0;
    int removedCount = 0;
    int updatedCount = 0;
};

// === Null/Invalid Input Tests ===

TEST_F(InstanceRegistryLifecycleTest, RegisterNullBuffer)
{
    std::shared_ptr<SharedCaptureBuffer> nullBuffer = nullptr;

    auto sourceId = getRegistry().registerInstance("track_null", nullBuffer, "Null Buffer Track");

    // Registration succeeds — buffer can be set later via dedup re-registration.
    // getCaptureBuffer returns nullptr since weak_ptr to null shared_ptr is expired.
    ASSERT_TRUE(sourceId.isValid());

    auto retrievedBuffer = getRegistry().getCaptureBuffer(sourceId);
    EXPECT_EQ(retrievedBuffer, nullptr);

    // Source metadata should still be accessible
    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, "Null Buffer Track");
}

TEST_F(InstanceRegistryLifecycleTest, EmptyTrackIdentifier)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = getRegistry().registerInstance("", buffer, "Empty ID Track");

    EXPECT_TRUE(sourceId.isValid());
    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, "Empty ID Track");
    // Empty track ID gets a unique auto-generated identifier to prevent dedup collisions
    EXPECT_TRUE(info->trackIdentifier.startsWith("__auto_"));
}

TEST_F(InstanceRegistryLifecycleTest, TwoEmptyTrackIdentifiersDontCollide)
{
    auto buffer1 = std::make_shared<SharedCaptureBuffer>();
    auto buffer2 = std::make_shared<SharedCaptureBuffer>();

    auto sourceId1 = getRegistry().registerInstance("", buffer1, "Track A");
    auto sourceId2 = getRegistry().registerInstance("", buffer2, "Track B");

    // Two empty track IDs must not deduplicate — they are unrelated sources
    EXPECT_NE(sourceId1, sourceId2);
    EXPECT_EQ(getRegistry().getSourceCount(), 2);

    auto info1 = getRegistry().getSource(sourceId1);
    auto info2 = getRegistry().getSource(sourceId2);
    ASSERT_TRUE(info1.has_value());
    ASSERT_TRUE(info2.has_value());
    EXPECT_EQ(info1->name, "Track A");
    EXPECT_EQ(info2->name, "Track B");
}

TEST_F(InstanceRegistryLifecycleTest, UnicodeTrackIdentifier)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = getRegistry().registerInstance(u8"日本語トラック", buffer, "Japanese Track");

    EXPECT_TRUE(sourceId.isValid());

    auto sourceInfo = getRegistry().getSource(sourceId);
    ASSERT_TRUE(sourceInfo.has_value());
    EXPECT_EQ(sourceInfo->name, "Japanese Track");
}

TEST_F(InstanceRegistryLifecycleTest, SpecialCharactersInTrackId)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = getRegistry().registerInstance("track/with\\special:chars*?\"<>|", buffer, "Special Track");

    EXPECT_TRUE(sourceId.isValid());
    auto info = getRegistry().getSource(sourceId);
    EXPECT_TRUE(info.has_value());
    EXPECT_EQ(info->name, "Special Track");
}

TEST_F(InstanceRegistryLifecycleTest, VeryLongTrackIdentifier)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    juce::String longId;
    for (int i = 0; i < 10000; ++i)
        longId += "a";

    auto sourceId = getRegistry().registerInstance(longId, buffer, "Long ID Track");

    EXPECT_TRUE(sourceId.isValid());
    auto retrievedBuffer = getRegistry().getCaptureBuffer(sourceId);
    EXPECT_EQ(retrievedBuffer, buffer);
}

// === Boundary Value Tests ===

TEST_F(InstanceRegistryLifecycleTest, ZeroChannelCountClampedToOne)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance("track_zero_ch", buffer, "Zero Channels", 0, 44100.0);

    EXPECT_TRUE(sourceId.isValid());

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    // Zero channels is nonsensical — clamped to minimum of 1
    EXPECT_EQ(info->channelCount, 1);
}

TEST_F(InstanceRegistryLifecycleTest, NegativeSampleRateClampedToMinimum)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance("track_neg_sr", buffer, "Negative SR", 2, -44100.0);

    EXPECT_TRUE(sourceId.isValid());
    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    // Negative sample rate is nonsensical — clamped to minimum of 1.0
    EXPECT_DOUBLE_EQ(info->sampleRate, 1.0);
}

TEST_F(InstanceRegistryLifecycleTest, HighChannelCountClampedToTwo)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance("track_many_ch", buffer, "Many Channels", 1000, 44100.0);

    EXPECT_TRUE(sourceId.isValid());

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    // Channel count clamped to maximum of 2 (stereo)
    EXPECT_EQ(info->channelCount, 2);
}

TEST_F(InstanceRegistryLifecycleTest, ZeroSampleRateClampedToMinimum)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance("track_zero_sr", buffer, "Zero SR", 2, 0.0);

    EXPECT_TRUE(sourceId.isValid());
    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    // Zero sample rate is nonsensical — clamped to minimum of 1.0
    EXPECT_DOUBLE_EQ(info->sampleRate, 1.0);
}

TEST_F(InstanceRegistryLifecycleTest, ExtremeSampleRate)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance("track_extreme_sr", buffer, "Extreme SR", 2, 10000000.0);

    EXPECT_TRUE(sourceId.isValid());

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->sampleRate, 10000000.0);
}

// === Lifecycle Tests ===

TEST_F(InstanceRegistryLifecycleTest, BufferExpires)
{
    SourceId sourceId;
    {
        auto buffer = std::make_shared<SharedCaptureBuffer>();
        sourceId = getRegistry().registerInstance("track_expire", buffer, "Expiring Track");
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

TEST_F(InstanceRegistryLifecycleTest, PendingAsyncNotificationsAreIgnoredAfterDestruction)
{
    std::vector<std::function<void()>> queuedCallbacks;
    getRegistry().setDispatcher(
        [&queuedCallbacks](std::function<void()> callback) { queuedCallbacks.push_back(std::move(callback)); });

    CountingRegistryListener listener;
    getRegistry().addListener(&listener);

    auto buffer = std::make_shared<SharedCaptureBuffer>();
    (void) getRegistry().registerInstance("async_teardown_track", buffer, "Async Teardown Track");

    EXPECT_EQ(listener.addedCount, 0);
    ASSERT_FALSE(queuedCallbacks.empty());

    registry_.reset();

    for (auto& callback : queuedCallbacks)
    {
        callback();
    }

    EXPECT_EQ(listener.addedCount, 0);
    EXPECT_EQ(listener.removedCount, 0);
    EXPECT_EQ(listener.updatedCount, 0);
}

// === Edge Case Tests for Empty/Default State ===

TEST_F(InstanceRegistryLifecycleTest, GetAllSourcesWhenEmpty)
{
    auto sources = getRegistry().getAllSources();
    EXPECT_EQ(sources.size(), 0);
}

TEST_F(InstanceRegistryLifecycleTest, GetSourceCountWhenEmpty) { EXPECT_EQ(getRegistry().getSourceCount(), 0); }

// === Stress Tests ===

TEST_F(InstanceRegistryLifecycleTest, RegisterAndUnregisterManyTimes)
{
    constexpr int iterations = 100;

    for (int i = 0; i < iterations; ++i)
    {
        auto buffer = std::make_shared<SharedCaptureBuffer>();
        auto sourceId = getRegistry().registerInstance("stress_track", buffer, "Stress Track");

        EXPECT_TRUE(sourceId.isValid());
        EXPECT_EQ(getRegistry().getSourceCount(), 1);

        getRegistry().unregisterInstance(sourceId);
        EXPECT_EQ(getRegistry().getSourceCount(), 0);
    }
}

// Registration must refuse silently at the hard limit so a pathological
// DAW session (many plugin instances) cannot blow past MAX_TRACKS and
// wedge the registry. Verifies SourceId::invalid() is returned and the
// source count is clamped at the limit.
TEST_F(InstanceRegistryLifecycleTest, RegisterAtMaxTracksLimitRejectsExtras)
{
    std::vector<std::shared_ptr<SharedCaptureBuffer>> buffers;
    std::vector<SourceId> originalIds;
    buffers.reserve(MAX_TRACKS);
    originalIds.reserve(MAX_TRACKS);

    for (int i = 0; i < MAX_TRACKS; ++i)
    {
        auto buffer = std::make_shared<SharedCaptureBuffer>();
        buffers.push_back(buffer);
        auto id = getRegistry().registerInstance("max_track_" + juce::String(i), buffer, "Track " + juce::String(i));
        ASSERT_TRUE(id.isValid()) << "Registration " << i << " below the limit should succeed";
        originalIds.push_back(id);
    }

    ASSERT_EQ(getRegistry().getSourceCount(), static_cast<size_t>(MAX_TRACKS));

    // One past the limit — must reject without throwing.
    auto overflowBuffer = std::make_shared<SharedCaptureBuffer>();
    auto overflowId = getRegistry().registerInstance("overflow_track", overflowBuffer, "Overflow");

    EXPECT_FALSE(overflowId.isValid()) << "Registration beyond MAX_TRACKS must return an invalid SourceId";
    EXPECT_EQ(getRegistry().getSourceCount(), static_cast<size_t>(MAX_TRACKS))
        << "Failed registration must not inflate the source count";

    // Re-registering the same track id for an already-known source must still
    // succeed through deduplication, even when we are at the limit. This
    // guards against a DAW session where a track updates its metadata after
    // the registry hit the cap.
    auto dedupId = getRegistry().registerInstance("max_track_0", buffers[0], "Track 0 renamed", 2, 48000.0);
    EXPECT_TRUE(dedupId.isValid()) << "Dedup re-registration at the limit must still update the existing source";

    // Dedup must return the same SourceId as the original registration.
    EXPECT_EQ(dedupId, originalIds[0]) << "Dedup must return the original SourceId, not a new one";

    // The source count must not grow — dedup must reuse the existing slot.
    EXPECT_EQ(getRegistry().getSourceCount(), static_cast<size_t>(MAX_TRACKS))
        << "Dedup re-registration must not inflate the source count";

    // The returned id must match the original registration for that track.
    auto info = getRegistry().getSource(dedupId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, "Track 0 renamed") << "Dedup must update the name";
    EXPECT_EQ(info->channelCount, 2) << "Dedup must update the channel count";
    EXPECT_DOUBLE_EQ(info->sampleRate, 48000.0) << "Dedup must update the sample rate";
}

TEST_F(InstanceRegistryLifecycleTest, UpdateSourceWithEmptyNamePreservesExistingName)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance("track_preserve", buffer, "Original Name");
    ASSERT_TRUE(sourceId.isValid());

    // Empty name must be a no-op on the name field — only channel/rate update.
    getRegistry().updateSource(sourceId, juce::String{}, 1, 48000.0);

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, "Original Name") << "Empty name on updateSource must not wipe the existing name";
    EXPECT_EQ(info->channelCount, 1);
    EXPECT_DOUBLE_EQ(info->sampleRate, 48000.0);
}

TEST_F(InstanceRegistryLifecycleTest, RegisterManyDifferentSources)
{
    constexpr int count = 50;
    std::vector<SourceId> sourceIds;

    for (int i = 0; i < count; ++i)
    {
        auto buffer = std::make_shared<SharedCaptureBuffer>();
        auto sourceId = getRegistry().registerInstance("track_" + juce::String(i), buffer, "Track " + juce::String(i));

        EXPECT_TRUE(sourceId.isValid());
        sourceIds.push_back(sourceId);
    }

    EXPECT_EQ(getRegistry().getSourceCount(), count);

    // Verify all sources are accessible
    for (const auto& sid : sourceIds)
    {
        auto info = getRegistry().getSource(sid);
        EXPECT_TRUE(info.has_value());
    }

    // Clean up
    for (const auto& sid : sourceIds)
    {
        getRegistry().unregisterInstance(sid);
    }

    EXPECT_EQ(getRegistry().getSourceCount(), 0);
}

// === Name and Metadata Tests ===

TEST_F(InstanceRegistryLifecycleTest, VeryLongDisplayName)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    juce::String longName;
    for (int i = 0; i < 10000; ++i)
        longName += "N";

    auto sourceId = getRegistry().registerInstance("track_long_name", buffer, longName);

    EXPECT_TRUE(sourceId.isValid());

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, longName);
}

TEST_F(InstanceRegistryLifecycleTest, EmptyDisplayName)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = getRegistry().registerInstance("track_empty_name", buffer, "");

    EXPECT_TRUE(sourceId.isValid());

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    // Registry assigns a default name if empty
    EXPECT_FALSE(info->name.isEmpty());
    EXPECT_TRUE(info->name.startsWith("Track"));
}

TEST_F(InstanceRegistryLifecycleTest, UnicodeDisplayName)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = getRegistry().registerInstance("track_unicode", buffer, u8"日本語 🎵 Track");

    EXPECT_TRUE(sourceId.isValid());

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, u8"日本語 🎵 Track");
}

// === Multiple Update Operations ===

TEST_F(InstanceRegistryLifecycleTest, MultipleUpdatesToSameSource)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance("track_updates", buffer, "Original Name");

    // Update multiple times with valid channel counts (1 or 2)
    for (int i = 0; i < 100; ++i)
    {
        getRegistry().updateSource(sourceId, "Name " + juce::String(i), i % 2 + 1, 44100.0 + i * 1000.0);
    }

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, "Name 99");
    EXPECT_EQ(info->channelCount, 2); // 99 % 2 + 1 = 2
    EXPECT_EQ(info->sampleRate, 44100.0 + 99 * 1000.0);
}

} // namespace oscil
