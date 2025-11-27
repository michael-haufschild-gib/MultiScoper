/*
    Oscil - Source Entity Tests
*/

#include <gtest/gtest.h>
#include "core/Source.h"

using namespace oscil;

class SourceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Reset for each test
    }
};

// === SourceId Tests ===

TEST_F(SourceTest, SourceIdGeneration)
{
    auto id1 = SourceId::generate();
    auto id2 = SourceId::generate();

    EXPECT_TRUE(id1.isValid());
    EXPECT_TRUE(id2.isValid());
    EXPECT_NE(id1, id2);
}

TEST_F(SourceTest, SourceIdInvalid)
{
    auto invalid = SourceId::invalid();
    EXPECT_FALSE(invalid.isValid());
    EXPECT_TRUE(invalid.id.isEmpty());
}

TEST_F(SourceTest, SourceIdNoSource)
{
    auto noSource = SourceId::noSource();
    EXPECT_FALSE(noSource.isValid());
    EXPECT_TRUE(noSource.isNoSource());
    EXPECT_EQ(noSource.id, "NO_SOURCE");
}

TEST_F(SourceTest, SourceIdEquality)
{
    SourceId id1{ "test-id-123" };
    SourceId id2{ "test-id-123" };
    SourceId id3{ "test-id-456" };

    EXPECT_EQ(id1, id2);
    EXPECT_NE(id1, id3);
}

// === InstanceId Tests ===

TEST_F(SourceTest, InstanceIdGeneration)
{
    auto id1 = InstanceId::generate();
    auto id2 = InstanceId::generate();

    EXPECT_TRUE(id1.isValid());
    EXPECT_TRUE(id2.isValid());
    EXPECT_NE(id1, id2);
}

TEST_F(SourceTest, InstanceIdInvalid)
{
    auto invalid = InstanceId::invalid();
    EXPECT_FALSE(invalid.isValid());
}

// === Source State Machine Tests ===

TEST_F(SourceTest, DefaultSourceState)
{
    Source source;
    EXPECT_EQ(source.getState(), SourceState::DISCOVERED);
    EXPECT_FALSE(source.isActive());
}

TEST_F(SourceTest, ValidStateTransitions)
{
    Source source;

    // DISCOVERED -> ACTIVE
    EXPECT_TRUE(source.canTransitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source.transitionTo(SourceState::ACTIVE));
    EXPECT_EQ(source.getState(), SourceState::ACTIVE);
    EXPECT_TRUE(source.isActive());

    // ACTIVE -> INACTIVE
    EXPECT_TRUE(source.canTransitionTo(SourceState::INACTIVE));
    EXPECT_TRUE(source.transitionTo(SourceState::INACTIVE));
    EXPECT_EQ(source.getState(), SourceState::INACTIVE);

    // INACTIVE -> ACTIVE
    EXPECT_TRUE(source.canTransitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source.transitionTo(SourceState::ACTIVE));
    EXPECT_EQ(source.getState(), SourceState::ACTIVE);
}

TEST_F(SourceTest, InvalidStateTransitions)
{
    Source source;

    // DISCOVERED cannot transition to INACTIVE directly
    EXPECT_FALSE(source.canTransitionTo(SourceState::INACTIVE));
    EXPECT_FALSE(source.transitionTo(SourceState::INACTIVE));
    EXPECT_EQ(source.getState(), SourceState::DISCOVERED);  // State unchanged

    // DISCOVERED cannot transition to STALE directly
    EXPECT_FALSE(source.canTransitionTo(SourceState::STALE));
    EXPECT_FALSE(source.transitionTo(SourceState::STALE));
}

TEST_F(SourceTest, ActiveToOrphaned)
{
    Source source;
    source.transitionTo(SourceState::ACTIVE);

    EXPECT_TRUE(source.canTransitionTo(SourceState::ORPHANED));
    EXPECT_TRUE(source.transitionTo(SourceState::ORPHANED));
    EXPECT_EQ(source.getState(), SourceState::ORPHANED);
}

TEST_F(SourceTest, OrphanedToActive)
{
    Source source;
    source.transitionTo(SourceState::ACTIVE);
    source.transitionTo(SourceState::ORPHANED);

    // Can recover from ORPHANED to ACTIVE
    EXPECT_TRUE(source.canTransitionTo(SourceState::ACTIVE));
    EXPECT_TRUE(source.transitionTo(SourceState::ACTIVE));
    EXPECT_EQ(source.getState(), SourceState::ACTIVE);
}

// === Source Ownership Tests ===

TEST_F(SourceTest, OwnershipAssignment)
{
    Source source;
    auto instanceId = InstanceId::generate();

    source.setOwningInstanceId(instanceId);
    EXPECT_EQ(source.getOwningInstanceId(), instanceId);
}

TEST_F(SourceTest, BackupInstances)
{
    Source source;
    auto owner = InstanceId::generate();
    auto backup1 = InstanceId::generate();
    auto backup2 = InstanceId::generate();

    source.setOwningInstanceId(owner);
    source.addBackupInstance(backup1);
    source.addBackupInstance(backup2);

    EXPECT_TRUE(source.hasBackupInstances());
    EXPECT_EQ(source.getBackupInstanceIds().size(), 2);

    auto nextBackup = source.getNextBackupInstance();
    EXPECT_TRUE(nextBackup.has_value());
    EXPECT_EQ(nextBackup.value(), backup1);
}

TEST_F(SourceTest, OwnershipTransfer)
{
    Source source;
    auto owner = InstanceId::generate();
    auto backup1 = InstanceId::generate();
    auto backup2 = InstanceId::generate();

    source.setOwningInstanceId(owner);
    source.addBackupInstance(backup1);
    source.addBackupInstance(backup2);

    // Transfer ownership to first backup
    EXPECT_TRUE(source.transferOwnership());
    EXPECT_EQ(source.getOwningInstanceId(), backup1);
    EXPECT_EQ(source.getBackupInstanceIds().size(), 1);
    EXPECT_EQ(source.getBackupInstanceIds()[0], backup2);

    // Transfer again
    EXPECT_TRUE(source.transferOwnership());
    EXPECT_EQ(source.getOwningInstanceId(), backup2);
    EXPECT_FALSE(source.hasBackupInstances());

    // No more backups to transfer to
    EXPECT_FALSE(source.transferOwnership());
}

TEST_F(SourceTest, RemoveBackupInstance)
{
    Source source;
    auto backup1 = InstanceId::generate();
    auto backup2 = InstanceId::generate();

    source.addBackupInstance(backup1);
    source.addBackupInstance(backup2);
    EXPECT_EQ(source.getBackupInstanceIds().size(), 2);

    source.removeBackupInstance(backup1);
    EXPECT_EQ(source.getBackupInstanceIds().size(), 1);
    EXPECT_EQ(source.getBackupInstanceIds()[0], backup2);
}

TEST_F(SourceTest, OwnerNotAddedAsBackup)
{
    Source source;
    auto owner = InstanceId::generate();

    source.setOwningInstanceId(owner);
    source.addBackupInstance(owner);  // Should not add

    EXPECT_FALSE(source.hasBackupInstances());
}

// === Source Serialization Tests ===

TEST_F(SourceTest, SerializationRoundTrip)
{
    Source original;
    original.setDawTrackId("track-123");
    original.setDisplayName("Test Track");
    original.setSampleRate(48000.0f);
    original.setChannelConfig(ChannelConfig::STEREO);
    original.setBufferSize(256);
    original.transitionTo(SourceState::ACTIVE);

    auto owner = InstanceId::generate();
    original.setOwningInstanceId(owner);

    auto backup = InstanceId::generate();
    original.addBackupInstance(backup);

    // Serialize
    auto tree = original.toValueTree();

    // Deserialize
    Source restored(tree);

    EXPECT_EQ(restored.getId(), original.getId());
    EXPECT_EQ(restored.getDawTrackId(), original.getDawTrackId());
    EXPECT_EQ(restored.getDisplayName(), original.getDisplayName());
    EXPECT_EQ(restored.getSampleRate(), original.getSampleRate());
    EXPECT_EQ(restored.getChannelConfig(), original.getChannelConfig());
    EXPECT_EQ(restored.getBufferSize(), original.getBufferSize());
    EXPECT_EQ(restored.getState(), original.getState());
    EXPECT_EQ(restored.getOwningInstanceId(), original.getOwningInstanceId());
    EXPECT_EQ(restored.getBackupInstanceIds().size(), original.getBackupInstanceIds().size());
}

// === Display Name Tests ===

TEST_F(SourceTest, DisplayNameTruncation)
{
    Source source;

    // Create a string longer than MAX_DISPLAY_NAME_LENGTH (64)
    juce::String longName;
    for (int i = 0; i < 100; ++i)
        longName += "X";

    source.setDisplayName(longName);

    EXPECT_LE(source.getDisplayName().length(), Source::MAX_DISPLAY_NAME_LENGTH);
}

// === Metrics Tests ===

TEST_F(SourceTest, CorrelationMetrics)
{
    Source source;

    EXPECT_FALSE(source.getCorrelationMetrics().isValid);

    source.updateCorrelationMetrics(0.85f);
    EXPECT_TRUE(source.getCorrelationMetrics().isValid);
    EXPECT_FLOAT_EQ(source.getCorrelationMetrics().correlationCoefficient, 0.85f);

    // Test clamping
    source.updateCorrelationMetrics(2.0f);
    EXPECT_FLOAT_EQ(source.getCorrelationMetrics().correlationCoefficient, 1.0f);

    source.updateCorrelationMetrics(-2.0f);
    EXPECT_FLOAT_EQ(source.getCorrelationMetrics().correlationCoefficient, -1.0f);
}

TEST_F(SourceTest, SignalMetrics)
{
    Source source;

    EXPECT_FALSE(source.getSignalMetrics().hasSignal);

    source.updateSignalMetrics(-10.0f, -5.0f, 0.001f);
    EXPECT_TRUE(source.getSignalMetrics().hasSignal);
    EXPECT_FLOAT_EQ(source.getSignalMetrics().rmsLevel, -10.0f);
    EXPECT_FLOAT_EQ(source.getSignalMetrics().peakLevel, -5.0f);
    EXPECT_FLOAT_EQ(source.getSignalMetrics().dcOffset, 0.001f);

    // Signal below threshold
    source.updateSignalMetrics(-70.0f, -65.0f, 0.0f);
    EXPECT_FALSE(source.getSignalMetrics().hasSignal);
}

// === State Conversion Tests ===

TEST_F(SourceTest, SourceStateStringConversion)
{
    EXPECT_EQ(sourceStateToString(SourceState::DISCOVERED), "DISCOVERED");
    EXPECT_EQ(sourceStateToString(SourceState::ACTIVE), "ACTIVE");
    EXPECT_EQ(sourceStateToString(SourceState::INACTIVE), "INACTIVE");
    EXPECT_EQ(sourceStateToString(SourceState::ORPHANED), "ORPHANED");
    EXPECT_EQ(sourceStateToString(SourceState::STALE), "STALE");

    EXPECT_EQ(stringToSourceState("DISCOVERED"), SourceState::DISCOVERED);
    EXPECT_EQ(stringToSourceState("ACTIVE"), SourceState::ACTIVE);
    EXPECT_EQ(stringToSourceState("INACTIVE"), SourceState::INACTIVE);
    EXPECT_EQ(stringToSourceState("ORPHANED"), SourceState::ORPHANED);
    EXPECT_EQ(stringToSourceState("STALE"), SourceState::STALE);
    EXPECT_EQ(stringToSourceState("INVALID"), SourceState::DISCOVERED);  // Default
}

TEST_F(SourceTest, ChannelConfigStringConversion)
{
    EXPECT_EQ(channelConfigToString(ChannelConfig::MONO), "MONO");
    EXPECT_EQ(channelConfigToString(ChannelConfig::STEREO), "STEREO");

    EXPECT_EQ(stringToChannelConfig("MONO"), ChannelConfig::MONO);
    EXPECT_EQ(stringToChannelConfig("STEREO"), ChannelConfig::STEREO);
    EXPECT_EQ(stringToChannelConfig("INVALID"), ChannelConfig::STEREO);  // Default
}
