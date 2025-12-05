/*
    Oscil - Instance Registry CRUD Tests
    Tests for register, unregister, lookup, and update operations
*/

#include <gtest/gtest.h>
#include <juce_events/juce_events.h>
#include "core/InstanceRegistry.h"
#include "core/SharedCaptureBuffer.h"

using namespace oscil;

class InstanceRegistryCrudTest : public ::testing::Test
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

// === Basic Registration Tests ===

TEST_F(InstanceRegistryCrudTest, RegisterInstance)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_1", buffer, "Track 1", 2, 44100.0);

    EXPECT_TRUE(sourceId.isValid());
    EXPECT_EQ(getRegistry().getSourceCount(), 1);
}

TEST_F(InstanceRegistryCrudTest, GetSourceInfo)
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

TEST_F(InstanceRegistryCrudTest, GetCaptureBuffer)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_1", buffer, "Track 1");

    auto retrievedBuffer = getRegistry().getCaptureBuffer(sourceId);

    EXPECT_EQ(retrievedBuffer, buffer);
}

// === Unregistration Tests ===

TEST_F(InstanceRegistryCrudTest, UnregisterInstance)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_1", buffer, "Track 1");

    EXPECT_EQ(getRegistry().getSourceCount(), 1);

    getRegistry().unregisterInstance(sourceId);

    EXPECT_EQ(getRegistry().getSourceCount(), 0);
    EXPECT_FALSE(getRegistry().getSource(sourceId).has_value());
}

TEST_F(InstanceRegistryCrudTest, UnregisterNonExistentSource)
{
    auto fakeId = SourceId::generate();
    size_t countBefore = getRegistry().getSourceCount();

    // Should not crash or throw
    getRegistry().unregisterInstance(fakeId);

    EXPECT_EQ(getRegistry().getSourceCount(), countBefore);
}

// === Update Tests ===

TEST_F(InstanceRegistryCrudTest, UpdateSource)
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

TEST_F(InstanceRegistryCrudTest, UpdateNonExistentSource)
{
    auto fakeId = SourceId::generate();

    // Should not crash
    getRegistry().updateSource(fakeId, "Fake Name", 2, 44100.0);

    // Source still shouldn't exist
    EXPECT_FALSE(getRegistry().getSource(fakeId).has_value());
}

// === Lookup Tests ===

TEST_F(InstanceRegistryCrudTest, GetNonExistentSource)
{
    auto fakeId = SourceId::generate();

    auto sourceInfo = getRegistry().getSource(fakeId);
    EXPECT_FALSE(sourceInfo.has_value());
}

TEST_F(InstanceRegistryCrudTest, GetBufferNonExistentSource)
{
    auto fakeId = SourceId::generate();

    auto buffer = getRegistry().getCaptureBuffer(fakeId);
    EXPECT_EQ(buffer, nullptr);
}

TEST_F(InstanceRegistryCrudTest, GetAllSources)
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

TEST_F(InstanceRegistryCrudTest, GetAllSourcesReturnsCopies)
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

// === Different Tracks Tests ===

TEST_F(InstanceRegistryCrudTest, DifferentTracksGetDifferentIds)
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

// === Maximum Sources Limit Tests ===

TEST_F(InstanceRegistryCrudTest, MaxSourcesLimit)
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

// === Re-registration Tests ===

TEST_F(InstanceRegistryCrudTest, ReRegisterAfterUnregister)
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

// === Active State Tests ===

TEST_F(InstanceRegistryCrudTest, SourceActiveStateAfterRegistration)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto sourceId = getRegistry().registerInstance(
        "track_active", buffer, "Active Track");

    auto info = getRegistry().getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_TRUE(info->active.load());
}
