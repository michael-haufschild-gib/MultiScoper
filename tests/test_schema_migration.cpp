/*
    MultiScoper - SchemaMigration Unit Tests
    Validates the rejection paths of the current (empty) migration table.
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

TEST(SchemaMigration, CurrentVersionAcceptedAsNoOp)
{
    auto tree = makeBareMultiScoperState(multiscoper::MultiScoperState::CURRENT_SCHEMA_VERSION);
    const auto result =
        multiscoper::migration::migrateMultiScoperState(tree, multiscoper::MultiScoperState::CURRENT_SCHEMA_VERSION,
                                                        multiscoper::MultiScoperState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(result, multiscoper::migration::MigrationResult::Success);
    EXPECT_EQ(static_cast<int>(tree.getProperty(multiscoper::StateIds::Version, -1)),
              multiscoper::MultiScoperState::CURRENT_SCHEMA_VERSION);
}

TEST(SchemaMigration, DowngradeIsRejected)
{
    auto tree = makeBareMultiScoperState(multiscoper::MultiScoperState::CURRENT_SCHEMA_VERSION);
    const auto result =
        multiscoper::migration::migrateMultiScoperState(tree, multiscoper::MultiScoperState::CURRENT_SCHEMA_VERSION,
                                                        multiscoper::MultiScoperState::CURRENT_SCHEMA_VERSION - 1);
    EXPECT_EQ(result, multiscoper::migration::MigrationResult::DowngradeUnsupported);
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
    auto tree = makeBareMultiScoperState(multiscoper::MultiScoperState::CURRENT_SCHEMA_VERSION);
    const int future = multiscoper::migration::highestSupportedToVersion() + 5;
    const auto result = multiscoper::migration::migrateMultiScoperState(
        tree, multiscoper::MultiScoperState::CURRENT_SCHEMA_VERSION, future);
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
