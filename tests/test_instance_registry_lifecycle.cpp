/*
    Oscil - Instance Registry Lifecycle Tests
    Tests for startup, shutdown, cleanup, null inputs, boundaries, and lifecycle scenarios
*/

#include <gtest/gtest.h>
#include <juce_events/juce_events.h>
#include "core/InstanceRegistry.h"
#include "core/SharedCaptureBuffer.h"

using namespace oscil;

class InstanceRegistryLifecycleTest : public ::testing::Test
{
protected:
    std::mutex dispatcherMutex;

    // Helper to access registry (friend access doesn't inherit to TEST_F generated classes)
    static InstanceRegistry& getRegistry() { return InstanceRegistry::getInstance(); }

    void SetUp() override
    {
        // Force synchronous dispatch for tests
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

// === Null/Invalid Input Tests ===

TEST_F(InstanceRegistryLifecycleTest, RegisterNullBuffer)
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

TEST_F(InstanceRegistryLifecycleTest, EmptyTrackIdentifier)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = getRegistry().registerInstance(
        "", buffer, "Empty ID Track");

    // Empty string is a valid key - should work
    EXPECT_TRUE(sourceId.isValid());
}

TEST_F(InstanceRegistryLifecycleTest, UnicodeTrackIdentifier)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = getRegistry().registerInstance(
        u8"日本語トラック", buffer, "Japanese Track");

    EXPECT_TRUE(sourceId.isValid());

    auto sourceInfo = getRegistry().getSource(sourceId);
    ASSERT_TRUE(sourceInfo.has_value());
    EXPECT_EQ(sourceInfo->name, "Japanese Track");
}

TEST_F(InstanceRegistryLifecycleTest, SpecialCharactersInTrackId)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = getRegistry().registerInstance(
        "track/with\\special:chars*?\"<>|", buffer, "Special Track");

    EXPECT_TRUE(sourceId.isValid());
}

TEST_F(InstanceRegistryLifecycleTest, VeryLongTrackIdentifier)
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

// === Boundary Value Tests ===

TEST_F(InstanceRegistryLifecycleTest, ZeroChannelCount)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_zero_ch", buffer, "Zero Channels", 0, 44100.0);

    EXPECT_TRUE(sourceId.isValid());

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->channelCount, 0);
}

TEST_F(InstanceRegistryLifecycleTest, NegativeSampleRate)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_neg_sr", buffer, "Negative SR", 2, -44100.0);

    // Should accept any value (validation is caller's responsibility)
    EXPECT_TRUE(sourceId.isValid());
}

TEST_F(InstanceRegistryLifecycleTest, HighChannelCount)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_many_ch", buffer, "Many Channels", 1000, 44100.0);

    EXPECT_TRUE(sourceId.isValid());

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->channelCount, 1000);
}

TEST_F(InstanceRegistryLifecycleTest, ZeroSampleRate)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_zero_sr", buffer, "Zero SR", 2, 0.0);

    EXPECT_TRUE(sourceId.isValid());
}

TEST_F(InstanceRegistryLifecycleTest, ExtremeSampleRate)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_extreme_sr", buffer, "Extreme SR", 2, 10000000.0);

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

// === Edge Case Tests for Empty/Default State ===

TEST_F(InstanceRegistryLifecycleTest, GetAllSourcesWhenEmpty)
{
    auto sources = getRegistry().getAllSources();
    EXPECT_EQ(sources.size(), 0);
}

TEST_F(InstanceRegistryLifecycleTest, GetSourceCountWhenEmpty)
{
    EXPECT_EQ(getRegistry().getSourceCount(), 0);
}

// === Stress Tests ===

TEST_F(InstanceRegistryLifecycleTest, RegisterAndUnregisterManyTimes)
{
    constexpr int iterations = 100;

    for (int i = 0; i < iterations; ++i)
    {
        auto buffer = std::make_shared<SharedCaptureBuffer>();
        auto sourceId = getRegistry().registerInstance(
            "stress_track", buffer, "Stress Track");

        EXPECT_TRUE(sourceId.isValid());
        EXPECT_EQ(getRegistry().getSourceCount(), 1);

        getRegistry().unregisterInstance(sourceId);
        EXPECT_EQ(getRegistry().getSourceCount(), 0);
    }
}

TEST_F(InstanceRegistryLifecycleTest, RegisterManyDifferentSources)
{
    constexpr int count = 50;
    std::vector<SourceId> sourceIds;

    for (int i = 0; i < count; ++i)
    {
        auto buffer = std::make_shared<SharedCaptureBuffer>();
        auto sourceId = getRegistry().registerInstance(
            "track_" + juce::String(i), buffer, "Track " + juce::String(i));

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

    auto sourceId = getRegistry().registerInstance(
        "track_long_name", buffer, longName);

    EXPECT_TRUE(sourceId.isValid());

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, longName);
}

TEST_F(InstanceRegistryLifecycleTest, EmptyDisplayName)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    auto sourceId = getRegistry().registerInstance(
        "track_empty_name", buffer, "");

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

    auto sourceId = getRegistry().registerInstance(
        "track_unicode", buffer, u8"日本語 🎵 Track");

    EXPECT_TRUE(sourceId.isValid());

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, u8"日本語 🎵 Track");
}

// === Multiple Update Operations ===

TEST_F(InstanceRegistryLifecycleTest, MultipleUpdatesToSameSource)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_updates", buffer, "Original Name");

    // Update multiple times
    for (int i = 0; i < 100; ++i)
    {
        getRegistry().updateSource(
            sourceId,
            "Name " + juce::String(i),
            i % 8 + 1,
            44100.0 + i * 1000.0);
    }

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, "Name 99");
    EXPECT_EQ(info->channelCount, 4);  // 99 % 8 + 1 = 4
    EXPECT_EQ(info->sampleRate, 44100.0 + 99 * 1000.0);
}
