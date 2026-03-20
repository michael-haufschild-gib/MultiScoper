/*
    Oscil - Source Ownership Tests
    Tests for Source owner assignment, backup instances, and ownership transfer
*/

#include <gtest/gtest.h>
#include "core/Source.h"

using namespace oscil;

class SourceOwnershipTest : public ::testing::Test
{
protected:
    std::unique_ptr<Source> source;
    SourceId sourceId;
    InstanceId instanceId;

    void SetUp() override
    {
        sourceId = SourceId::generate();
        instanceId = InstanceId::generate();
        source = std::make_unique<Source>(sourceId);
    }

    void TearDown() override
    {
        source.reset();
    }
};

// === Basic Ownership Tests ===

TEST_F(SourceOwnershipTest, OwnershipAssignment)
{
    auto newInstanceId = InstanceId::generate();

    source->setOwningInstanceId(newInstanceId);
    EXPECT_EQ(source->getOwningInstanceId(), newInstanceId);
}

TEST_F(SourceOwnershipTest, BackupInstances)
{
    auto owner = InstanceId::generate();
    auto backup1 = InstanceId::generate();
    auto backup2 = InstanceId::generate();

    source->setOwningInstanceId(owner);
    source->addBackupInstance(backup1);
    source->addBackupInstance(backup2);

    EXPECT_TRUE(source->hasBackupInstances());
    EXPECT_EQ(source->getBackupInstanceIds().size(), 2);

    auto nextBackup = source->getNextBackupInstance();
    EXPECT_TRUE(nextBackup.has_value());
    EXPECT_EQ(nextBackup.value(), backup1);
}

TEST_F(SourceOwnershipTest, RemoveBackupInstance)
{
    auto backup1 = InstanceId::generate();
    auto backup2 = InstanceId::generate();

    source->addBackupInstance(backup1);
    source->addBackupInstance(backup2);
    EXPECT_EQ(source->getBackupInstanceIds().size(), 2);

    source->removeBackupInstance(backup1);
    EXPECT_EQ(source->getBackupInstanceIds().size(), 1);
    EXPECT_EQ(source->getBackupInstanceIds()[0], backup2);
}

TEST_F(SourceOwnershipTest, OwnerNotAddedAsBackup)
{
    auto owner = InstanceId::generate();

    source->setOwningInstanceId(owner);
    source->addBackupInstance(owner);  // Should not add

    EXPECT_FALSE(source->hasBackupInstances());
}

// === Ownership Transfer Tests ===

TEST_F(SourceOwnershipTest, OwnershipTransfer)
{
    auto owner = InstanceId::generate();
    auto backup1 = InstanceId::generate();
    auto backup2 = InstanceId::generate();

    source->setOwningInstanceId(owner);
    source->addBackupInstance(backup1);
    source->addBackupInstance(backup2);

    // Transfer ownership to first backup
    EXPECT_TRUE(source->transferOwnership());
    EXPECT_EQ(source->getOwningInstanceId(), backup1);
    EXPECT_EQ(source->getBackupInstanceIds().size(), 1);
    EXPECT_EQ(source->getBackupInstanceIds()[0], backup2);

    // Transfer again
    EXPECT_TRUE(source->transferOwnership());
    EXPECT_EQ(source->getOwningInstanceId(), backup2);
    EXPECT_FALSE(source->hasBackupInstances());

    // No more backups to transfer to
    EXPECT_FALSE(source->transferOwnership());
}

TEST_F(SourceOwnershipTest, TransferOwnershipWithNoBackups)
{
    auto owner = InstanceId::generate();
    source->setOwningInstanceId(owner);

    EXPECT_FALSE(source->transferOwnership());
    EXPECT_EQ(source->getOwningInstanceId(), owner);  // Owner unchanged
}

// === Edge Case Tests ===

TEST_F(SourceOwnershipTest, AddInvalidBackupInstance)
{
    // Invalid instance ID should not be added
    source->addBackupInstance(InstanceId::invalid());

    EXPECT_FALSE(source->hasBackupInstances());
}

TEST_F(SourceOwnershipTest, AddDuplicateBackupInstance)
{
    auto backup = InstanceId::generate();

    source->addBackupInstance(backup);
    source->addBackupInstance(backup);  // Duplicate
    source->addBackupInstance(backup);  // Duplicate again

    // Should only have one entry
    EXPECT_EQ(source->getBackupInstanceIds().size(), 1);
}

TEST_F(SourceOwnershipTest, RemoveNonExistentBackupInstance)
{
    auto backup1 = InstanceId::generate();
    auto backup2 = InstanceId::generate();

    source->addBackupInstance(backup1);

    // Remove non-existent backup
    source->removeBackupInstance(backup2);

    // Should still have backup1
    EXPECT_EQ(source->getBackupInstanceIds().size(), 1);
    EXPECT_EQ(source->getBackupInstanceIds()[0], backup1);
}

TEST_F(SourceOwnershipTest, SetOwnerRemovesFromBackupList)
{
    auto instance1 = InstanceId::generate();
    auto instance2 = InstanceId::generate();

    // Add both as backups
    source->addBackupInstance(instance1);
    source->addBackupInstance(instance2);
    EXPECT_EQ(source->getBackupInstanceIds().size(), 2);

    // Set instance1 as owner - should be removed from backup list
    source->setOwningInstanceId(instance1);

    EXPECT_EQ(source->getOwningInstanceId(), instance1);
    EXPECT_EQ(source->getBackupInstanceIds().size(), 1);
    EXPECT_EQ(source->getBackupInstanceIds()[0], instance2);
}

TEST_F(SourceOwnershipTest, GetNextBackupInstanceEmpty)
{
    auto next = source->getNextBackupInstance();

    EXPECT_FALSE(next.has_value());
}

// === Serialization Tests for Ownership ===

TEST_F(SourceOwnershipTest, SerializeAndDeserializeBackupInstances)
{
    // Add multiple backup instances
    auto backup1 = InstanceId::generate();
    auto backup2 = InstanceId::generate();
    auto backup3 = InstanceId::generate();

    source->addBackupInstance(backup1);
    source->addBackupInstance(backup2);
    source->addBackupInstance(backup3);

    auto tree = source->toValueTree();
    Source restored(tree);

    EXPECT_EQ(restored.getBackupInstanceIds().size(), 3);
    EXPECT_EQ(restored.getBackupInstanceIds()[0], backup1);
    EXPECT_EQ(restored.getBackupInstanceIds()[1], backup2);
    EXPECT_EQ(restored.getBackupInstanceIds()[2], backup3);
}

TEST_F(SourceOwnershipTest, DeserializeEmptyBackupInstances)
{
    juce::ValueTree tree("Source");
    tree.setProperty("id", "test-id", nullptr);

    // Add empty BackupInstances child
    juce::ValueTree backups("BackupInstances");
    tree.appendChild(backups, nullptr);

    Source testSource(tree);

    EXPECT_FALSE(testSource.hasBackupInstances());
}

TEST_F(SourceOwnershipTest, DeserializeBackupInstancesWithInvalidIds)
{
    juce::ValueTree tree("Source");
    tree.setProperty("id", "test-id", nullptr);

    juce::ValueTree backups("BackupInstances");

    // Add instance with empty ID (should be filtered out)
    juce::ValueTree emptyInstance("Instance");
    emptyInstance.setProperty("id", "", nullptr);
    backups.appendChild(emptyInstance, nullptr);

    // Add valid instance
    juce::ValueTree validInstance("Instance");
    validInstance.setProperty("id", "valid-instance-id", nullptr);
    backups.appendChild(validInstance, nullptr);

    tree.appendChild(backups, nullptr);

    Source testSource(tree);

    // Only valid instance should be added
    EXPECT_EQ(testSource.getBackupInstanceIds().size(), 1);
    EXPECT_EQ(testSource.getBackupInstanceIds()[0].id, "valid-instance-id");
}

TEST_F(SourceOwnershipTest, DeserializeBackupInstancesExcludesOwnerAndDuplicates)
{
    juce::ValueTree tree("Source");
    tree.setProperty("id", "test-id", nullptr);
    tree.setProperty("owningInstanceId", "owner-instance-id", nullptr);

    juce::ValueTree backups("BackupInstances");

    juce::ValueTree ownerAsBackup("Instance");
    ownerAsBackup.setProperty("id", "owner-instance-id", nullptr);
    backups.appendChild(ownerAsBackup, nullptr);

    juce::ValueTree backup("Instance");
    backup.setProperty("id", "backup-instance-id", nullptr);
    backups.appendChild(backup, nullptr);

    juce::ValueTree duplicate("Instance");
    duplicate.setProperty("id", "backup-instance-id", nullptr);
    backups.appendChild(duplicate, nullptr);

    tree.appendChild(backups, nullptr);

    Source testSource(tree);

    ASSERT_EQ(testSource.getOwningInstanceId().id, "owner-instance-id");
    ASSERT_EQ(testSource.getBackupInstanceIds().size(), 1);
    EXPECT_EQ(testSource.getBackupInstanceIds()[0].id, "backup-instance-id");
}

// === Sequential Operations Test ===

TEST_F(SourceOwnershipTest, SequentialBackupInstanceOperations)
{
    std::vector<InstanceId> instances;

    // Generate instance IDs
    for (int i = 0; i < 10; ++i)
    {
        instances.push_back(InstanceId::generate());
    }

    // Sequential add/remove operations work correctly
    for (const auto& inst : instances)
    {
        source->addBackupInstance(inst);
    }
    EXPECT_EQ(source->getBackupInstanceIds().size(), 10);

    for (size_t i = 0; i < 5; ++i)
    {
        source->removeBackupInstance(instances[i]);
    }
    EXPECT_EQ(source->getBackupInstanceIds().size(), 5);
}
