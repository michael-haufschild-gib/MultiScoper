/*
    Oscil - State Persistence Tests: Schema Migration
    Tests for version migrations and backward compatibility
*/

#include <gtest/gtest.h>
#include "helpers/Fixtures.h"
#include "core/OscilState.h"

using namespace oscil;
using namespace oscil::test;

class StatePersistenceMigrationTest : public StateTestFixture {};

// Test: Missing schema version
TEST_F(StatePersistenceMigrationTest, MissingSchemaVersion)
{
    // Old state without version property
    juce::String oldXml = R"(
        <OscilState>
            <Oscillators/>
            <Theme themeName="Classic Green"/>
        </OscilState>
    )";
    EXPECT_TRUE(state->fromXmlString(oldXml));

    // getSchemaVersion defaults to 1 for old states
    EXPECT_EQ(state->getSchemaVersion(), 1);
}

// Test: Old schema version
TEST_F(StatePersistenceMigrationTest, OldSchemaVersion)
{
    // Version 1 state
    juce::String v1Xml = R"(
        <OscilState version="1">
            <Oscillators/>
            <Theme themeName="Old Theme"/>
        </OscilState>
    )";
    EXPECT_TRUE(state->fromXmlString(v1Xml));

    EXPECT_EQ(state->getSchemaVersion(), 1);
    EXPECT_EQ(state->getThemeName(), "Old Theme");
}

// Test: Future schema version
TEST_F(StatePersistenceMigrationTest, FutureSchemaVersion)
{
    // Future version (should still load)
    juce::String futureXml = R"(
        <OscilState version="999">
            <Oscillators/>
            <Theme themeName="Future Theme"/>
        </OscilState>
    )";
    EXPECT_TRUE(state->fromXmlString(futureXml));

    EXPECT_EQ(state->getSchemaVersion(), 999);
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
    auto original = StateBuilder()
        .withThemeName("Test Theme")
        .buildUnique();

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
