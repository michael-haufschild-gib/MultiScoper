/*
    MultiScoper - SchemaMigration Unit Tests
    Validates each registered migration in isolation plus rejection paths.
*/

#include "core/MultiScoperState.h"
#include "core/SchemaMigration.h"

#include <juce_data_structures/juce_data_structures.h>

#include <gtest/gtest.h>

namespace
{

juce::ValueTree makeBareMultiScoperState(int version)
{
    juce::ValueTree tree(multiscoper::StateIds::MultiScoperState);
    if (version >= 0)
        tree.setProperty(multiscoper::StateIds::Version, version, nullptr);
    tree.appendChild(juce::ValueTree(multiscoper::StateIds::Oscillators), nullptr);
    return tree;
}

} // namespace

TEST(SchemaMigration, V0ToV1StampsVersionOne)
{
    auto tree = makeBareMultiScoperState(0);
    const auto result = multiscoper::migration::migrateMultiScoperState(tree, 0, 1);
    EXPECT_EQ(result, multiscoper::migration::MigrationResult::Success);
    EXPECT_EQ(static_cast<int>(tree.getProperty(multiscoper::StateIds::Version, -1)), 1);
    EXPECT_TRUE(tree.getChildWithName(multiscoper::StateIds::Oscillators).isValid());
}

TEST(SchemaMigration, V1ToV2StampsVersionTwoPreservingChildren)
{
    auto tree = makeBareMultiScoperState(1);
    tree.appendChild(juce::ValueTree(multiscoper::StateIds::Theme), nullptr);
    const auto result = multiscoper::migration::migrateMultiScoperState(tree, 1, 2);
    EXPECT_EQ(result, multiscoper::migration::MigrationResult::Success);
    EXPECT_EQ(static_cast<int>(tree.getProperty(multiscoper::StateIds::Version, -1)), 2);
    EXPECT_TRUE(tree.getChildWithName(multiscoper::StateIds::Theme).isValid());
}

TEST(SchemaMigration, IdempotentWhenFromEqualsToStampsVersion)
{
    auto tree = makeBareMultiScoperState(-1); // no version attr
    const auto result = multiscoper::migration::migrateMultiScoperState(tree, 2, 2);
    EXPECT_EQ(result, multiscoper::migration::MigrationResult::Success);
    EXPECT_EQ(static_cast<int>(tree.getProperty(multiscoper::StateIds::Version, -1)), 2);
}

TEST(SchemaMigration, DowngradeIsRejected)
{
    auto tree = makeBareMultiScoperState(2);
    const auto result = multiscoper::migration::migrateMultiScoperState(tree, 2, 1);
    EXPECT_EQ(result, multiscoper::migration::MigrationResult::DowngradeUnsupported);
    // Version was not stamped down.
    EXPECT_EQ(static_cast<int>(tree.getProperty(multiscoper::StateIds::Version, -1)), 2);
}

TEST(SchemaMigration, FutureFromVersionGreaterThanToIsRejectedAsDowngrade)
{
    auto tree = makeBareMultiScoperState(99);
    const auto result = multiscoper::migration::migrateMultiScoperState(
        tree, 99, multiscoper::MultiScoperState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(result, multiscoper::migration::MigrationResult::DowngradeUnsupported);
}

TEST(SchemaMigration, UnknownFutureToVersionIsRejected)
{
    auto tree = makeBareMultiScoperState(1);
    const int future = multiscoper::migration::highestSupportedToVersion() + 5;
    const auto result = multiscoper::migration::migrateMultiScoperState(tree, 1, future);
    EXPECT_EQ(result, multiscoper::migration::MigrationResult::UnsupportedToVersion);
}

TEST(SchemaMigration, FromVersionBelowLowestKnownIsRejected)
{
    auto tree = makeBareMultiScoperState(-3);
    const int belowLowest = multiscoper::migration::lowestSupportedFromVersion() - 1;
    const auto result = multiscoper::migration::migrateMultiScoperState(
        tree, belowLowest, multiscoper::MultiScoperState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(result, multiscoper::migration::MigrationResult::UnsupportedFromVersion);
}

TEST(SchemaMigration, ChainMigrationV0ToV2AppliesBothSteps)
{
    auto tree = makeBareMultiScoperState(0);
    const auto result = multiscoper::migration::migrateMultiScoperState(tree, 0, 2);
    EXPECT_EQ(result, multiscoper::migration::MigrationResult::Success);
    // Final version is the target, proving both v0->v1 and v1->v2 fired.
    EXPECT_EQ(static_cast<int>(tree.getProperty(multiscoper::StateIds::Version, -1)), 2);
}

TEST(SchemaMigration, ResultStringsAreStable)
{
    EXPECT_STREQ(multiscoper::migration::migrationResultToString(multiscoper::migration::MigrationResult::Success),
                 "Success");
    EXPECT_STREQ(
        multiscoper::migration::migrationResultToString(multiscoper::migration::MigrationResult::DowngradeUnsupported),
        "DowngradeUnsupported");
    EXPECT_STREQ(multiscoper::migration::migrationResultToString(
                     multiscoper::migration::MigrationResult::UnsupportedFromVersion),
                 "UnsupportedFromVersion");
    EXPECT_STREQ(
        multiscoper::migration::migrationResultToString(multiscoper::migration::MigrationResult::UnsupportedToVersion),
        "UnsupportedToVersion");
}
