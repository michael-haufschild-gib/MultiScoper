/*
    MultiScoper - State Persistence Tests: Schema Migration
    Tests for version migrations and backward compatibility
*/

#include "core/MultiScoperState.h"

#include "helpers/Fixtures.h"

#include <gtest/gtest.h>

using namespace multiscoper;
using namespace multiscoper::test;

class StatePersistenceMigrationTest : public StateTestFixture
{
};

// Test: Missing schema version (no "version" attribute) is rejected —
// pre-v3 saves are no longer supported.
TEST_F(StatePersistenceMigrationTest, MissingSchemaVersionRejected)
{
    auto const themeBefore = state->getThemeName();
    juce::String oldXml = R"(
        <MultiScoperState>
            <Oscillators/>
            <Theme themeName="Classic Green"/>
        </MultiScoperState>
    )";
    EXPECT_FALSE(state->fromXmlString(oldXml));
    EXPECT_EQ(state->getThemeName(), themeBefore);
}

// Test: Pre-current schema version (e.g. v1) is rejected.
TEST_F(StatePersistenceMigrationTest, OldSchemaVersionRejected)
{
    auto const themeBefore = state->getThemeName();
    juce::String v1Xml = R"(
        <MultiScoperState version="1">
            <Oscillators/>
            <Theme themeName="Old Theme"/>
        </MultiScoperState>
    )";
    EXPECT_FALSE(state->fromXmlString(v1Xml));
    EXPECT_EQ(state->getThemeName(), themeBefore);
}

// Test: Future schema version is rejected (fail-closed). State is preserved.
TEST_F(StatePersistenceMigrationTest, FutureSchemaVersion)
{
    auto const themeBefore = state->getThemeName();
    juce::String futureXml = R"(
        <MultiScoperState version="999">
            <Oscillators/>
            <Theme themeName="Future Theme"/>
        </MultiScoperState>
    )";
    EXPECT_FALSE(state->fromXmlString(futureXml));

    EXPECT_EQ(state->getSchemaVersion(), MultiScoperState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(state->getThemeName(), themeBefore);
    EXPECT_NE(state->getThemeName(), "Future Theme");
}

// Test: Malformed XML recovery
TEST_F(StatePersistenceMigrationTest, MalformedXmlRecovery)
{
    // Malformed XML (unclosed tag)
    EXPECT_FALSE(state->fromXmlString("<MultiScoperState><broken"));

    // State should remain usable with defaults
    EXPECT_EQ(state->getSchemaVersion(), MultiScoperState::CURRENT_SCHEMA_VERSION);
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
