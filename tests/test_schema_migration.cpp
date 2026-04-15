/*
    Oscil - SchemaMigration Unit Tests
    Validates each registered migration in isolation plus rejection paths.
*/

#include "core/OscilState.h"
#include "core/SchemaMigration.h"

#include <juce_data_structures/juce_data_structures.h>

#include <gtest/gtest.h>

namespace
{

juce::ValueTree makeBareOscilState(int version)
{
    juce::ValueTree tree(oscil::StateIds::OscilState);
    if (version >= 0)
        tree.setProperty(oscil::StateIds::Version, version, nullptr);
    tree.appendChild(juce::ValueTree(oscil::StateIds::Oscillators), nullptr);
    return tree;
}

} // namespace

TEST(SchemaMigration, V0ToV1StampsVersionOne)
{
    auto tree = makeBareOscilState(0);
    const auto result = oscil::migration::migrateOscilState(tree, 0, 1);
    EXPECT_EQ(result, oscil::migration::MigrationResult::Success);
    EXPECT_EQ(static_cast<int>(tree.getProperty(oscil::StateIds::Version, -1)), 1);
    EXPECT_TRUE(tree.getChildWithName(oscil::StateIds::Oscillators).isValid());
}

TEST(SchemaMigration, V1ToV2StampsVersionTwoPreservingChildren)
{
    auto tree = makeBareOscilState(1);
    tree.appendChild(juce::ValueTree(oscil::StateIds::Theme), nullptr);
    const auto result = oscil::migration::migrateOscilState(tree, 1, 2);
    EXPECT_EQ(result, oscil::migration::MigrationResult::Success);
    EXPECT_EQ(static_cast<int>(tree.getProperty(oscil::StateIds::Version, -1)), 2);
    EXPECT_TRUE(tree.getChildWithName(oscil::StateIds::Theme).isValid());
}

TEST(SchemaMigration, IdempotentWhenFromEqualsToStampsVersion)
{
    auto tree = makeBareOscilState(-1); // no version attr
    const auto result = oscil::migration::migrateOscilState(tree, 2, 2);
    EXPECT_EQ(result, oscil::migration::MigrationResult::Success);
    EXPECT_EQ(static_cast<int>(tree.getProperty(oscil::StateIds::Version, -1)), 2);
}

TEST(SchemaMigration, DowngradeIsRejected)
{
    auto tree = makeBareOscilState(2);
    const auto result = oscil::migration::migrateOscilState(tree, 2, 1);
    EXPECT_EQ(result, oscil::migration::MigrationResult::DowngradeUnsupported);
    // Version was not stamped down.
    EXPECT_EQ(static_cast<int>(tree.getProperty(oscil::StateIds::Version, -1)), 2);
}

TEST(SchemaMigration, FutureFromVersionGreaterThanToIsRejectedAsDowngrade)
{
    auto tree = makeBareOscilState(99);
    const auto result = oscil::migration::migrateOscilState(tree, 99, oscil::OscilState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(result, oscil::migration::MigrationResult::DowngradeUnsupported);
}

TEST(SchemaMigration, UnknownFutureToVersionIsRejected)
{
    auto tree = makeBareOscilState(1);
    const int future = oscil::migration::highestSupportedToVersion() + 5;
    const auto result = oscil::migration::migrateOscilState(tree, 1, future);
    EXPECT_EQ(result, oscil::migration::MigrationResult::UnsupportedToVersion);
}

TEST(SchemaMigration, FromVersionBelowLowestKnownIsRejected)
{
    auto tree = makeBareOscilState(-3);
    const int belowLowest = oscil::migration::lowestSupportedFromVersion() - 1;
    const auto result =
        oscil::migration::migrateOscilState(tree, belowLowest, oscil::OscilState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(result, oscil::migration::MigrationResult::UnsupportedFromVersion);
}

TEST(SchemaMigration, ChainMigrationV0ToV2AppliesBothSteps)
{
    auto tree = makeBareOscilState(0);
    const auto result = oscil::migration::migrateOscilState(tree, 0, 2);
    EXPECT_EQ(result, oscil::migration::MigrationResult::Success);
    // Final version is the target, proving both v0->v1 and v1->v2 fired.
    EXPECT_EQ(static_cast<int>(tree.getProperty(oscil::StateIds::Version, -1)), 2);
}

TEST(SchemaMigration, ResultStringsAreStable)
{
    EXPECT_STREQ(oscil::migration::migrationResultToString(oscil::migration::MigrationResult::Success), "Success");
    EXPECT_STREQ(oscil::migration::migrationResultToString(oscil::migration::MigrationResult::DowngradeUnsupported),
                 "DowngradeUnsupported");
    EXPECT_STREQ(oscil::migration::migrationResultToString(oscil::migration::MigrationResult::UnsupportedFromVersion),
                 "UnsupportedFromVersion");
    EXPECT_STREQ(oscil::migration::migrationResultToString(oscil::migration::MigrationResult::UnsupportedToVersion),
                 "UnsupportedToVersion");
}
