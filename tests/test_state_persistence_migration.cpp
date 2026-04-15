/*
    Oscil - State Persistence Tests: Schema Migration
    Tests for version migrations and backward compatibility
*/

#include "core/OscilState.h"

#include "helpers/Fixtures.h"

#include <gtest/gtest.h>

using namespace oscil;
using namespace oscil::test;

class StatePersistenceMigrationTest : public StateTestFixture
{
};

// Test: Missing schema version (pre-versioned v0 fixture) migrates forward.
// SchemaMigration treats a missing "version" attribute as v0 and chain-migrates
// v0 -> v1 -> CURRENT_SCHEMA_VERSION. Payload is preserved.
TEST_F(StatePersistenceMigrationTest, MissingSchemaVersion)
{
    juce::String oldXml = R"(
        <OscilState>
            <Oscillators/>
            <Theme themeName="Classic Green"/>
        </OscilState>
    )";
    EXPECT_TRUE(state->fromXmlString(oldXml));

    // Post-migration the version is stamped to current.
    EXPECT_EQ(state->getSchemaVersion(), OscilState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(state->getThemeName(), "Classic Green");
}

// Test: Old schema version v1 migrates forward to current and preserves payload.
TEST_F(StatePersistenceMigrationTest, OldSchemaVersion)
{
    juce::String v1Xml = R"(
        <OscilState version="1">
            <Oscillators/>
            <Theme themeName="Old Theme"/>
        </OscilState>
    )";
    EXPECT_TRUE(state->fromXmlString(v1Xml));

    EXPECT_EQ(state->getSchemaVersion(), OscilState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(state->getThemeName(), "Old Theme");
}

// Test: Future schema version is rejected (fail-closed). State is preserved.
TEST_F(StatePersistenceMigrationTest, FutureSchemaVersion)
{
    auto const themeBefore = state->getThemeName();
    juce::String futureXml = R"(
        <OscilState version="999">
            <Oscillators/>
            <Theme themeName="Future Theme"/>
        </OscilState>
    )";
    EXPECT_FALSE(state->fromXmlString(futureXml));

    EXPECT_EQ(state->getSchemaVersion(), OscilState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(state->getThemeName(), themeBefore);
    EXPECT_NE(state->getThemeName(), "Future Theme");
}

// Test: Malformed XML recovery
TEST_F(StatePersistenceMigrationTest, MalformedXmlRecovery)
{
    // Malformed XML (unclosed tag)
    EXPECT_FALSE(state->fromXmlString("<OscilState><broken"));

    // State should remain usable with defaults
    EXPECT_EQ(state->getSchemaVersion(), OscilState::CURRENT_SCHEMA_VERSION);
}

// Test: Truncated XML recovery
TEST_F(StatePersistenceMigrationTest, TruncatedXmlRecovery)
{
    auto original = StateBuilder().withThemeName("Test Theme").buildUnique();

    juce::String xmlString = original->toXmlString();

    // Truncate the XML
    juce::String truncated = xmlString.substring(0, xmlString.length() / 2);

    EXPECT_FALSE(state->fromXmlString(truncated));
}

// Test: XML with wrong root type
TEST_F(StatePersistenceMigrationTest, XmlWithWrongRootType)
{
    // Valid XML but wrong root type
    EXPECT_FALSE(state->fromXmlString("<SomeOtherRoot><Oscillators/></SomeOtherRoot>"));
}
