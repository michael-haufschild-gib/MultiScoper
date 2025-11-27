/*
    Oscil - Instance Registry Tests
*/

#include <gtest/gtest.h>
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
