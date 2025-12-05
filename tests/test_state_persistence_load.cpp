/*
    Oscil - State Persistence Tests: Load Operations
    Tests for state deserialization, XML parsing, and load operations
*/

#include <gtest/gtest.h>
#include "helpers/StateBuilder.h"
#include "helpers/Fixtures.h"
#include "core/OscilState.h"

using namespace oscil;
using namespace oscil::test;

class StatePersistenceLoadTest : public StateTestFixture {};

// Test: Empty XML handling
TEST_F(StatePersistenceLoadTest, EmptyXmlHandling)
{
    EXPECT_FALSE(state->fromXmlString(""));
    EXPECT_FALSE(state->fromXmlString("invalid xml"));
}

// Test: Invalid XML handling
TEST_F(StatePersistenceLoadTest, InvalidXmlHandling)
{
    // Valid XML but wrong structure
    EXPECT_FALSE(state->fromXmlString("<WrongRoot></WrongRoot>"));
}

// Test: XML with missing child nodes
TEST_F(StatePersistenceLoadTest, XmlWithMissingChildNodes)
{
    // Valid OscilState but missing all child nodes
    juce::String minimalXml = "<OscilState version=\"2\"/>";
    EXPECT_TRUE(state->fromXmlString(minimalXml));

    // Should have empty oscillators
    EXPECT_TRUE(state->getOscillators().empty());

    // Adding oscillator should still work (creates Oscillators node)
    auto osc = OscillatorBuilder().withName("Test").build();
    state->addOscillator(osc);
    EXPECT_EQ(state->getOscillators().size(), 1);
}

// Test: XML with partial child nodes
TEST_F(StatePersistenceLoadTest, XmlWithPartialChildNodes)
{
    // Has Oscillators but missing Layout and Theme
    juce::String partialXml = R"(
        <OscilState version="2">
            <Oscillators/>
        </OscilState>
    )";
    EXPECT_TRUE(state->fromXmlString(partialXml));

    // Should use defaults for missing nodes
    EXPECT_EQ(state->getThemeName(), "Dark Professional");
}

// Test: Corrupted oscillator data recovery
TEST_F(StatePersistenceLoadTest, CorruptedOscillatorDataRecovery)
{
    // Oscillator with invalid/missing properties
    juce::String xmlWithBadOsc = R"(
        <OscilState version="2">
            <Oscillators>
                <Oscillator/>
                <Oscillator id="valid-id" name="Valid Osc"/>
            </Oscillators>
        </OscilState>
    )";
    EXPECT_TRUE(state->fromXmlString(xmlWithBadOsc));

    // Should load what it can
    auto oscillators = state->getOscillators();
    EXPECT_GE(oscillators.size(), 1);
}

// Test: Construct from valid XML
TEST_F(StatePersistenceLoadTest, ConstructFromValidXml)
{
    juce::String xml = R"(
        <OscilState version="2">
            <Oscillators/>
            <Theme themeName="Constructed Theme"/>
        </OscilState>
    )";

    OscilState state(xml);

    EXPECT_EQ(state.getThemeName(), "Constructed Theme");
}

// Test: Construct from invalid XML
TEST_F(StatePersistenceLoadTest, ConstructFromInvalidXml)
{
    OscilState state("invalid xml content");

    // Should have default state
    EXPECT_EQ(state.getSchemaVersion(), OscilState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(state.getThemeName(), "Dark Professional");
}

// Test: Construct from empty string
TEST_F(StatePersistenceLoadTest, ConstructFromEmptyString)
{
    OscilState state("");

    // Should have default state
    EXPECT_EQ(state.getSchemaVersion(), OscilState::CURRENT_SCHEMA_VERSION);
}

// Test: Display options defaults
TEST_F(StatePersistenceLoadTest, DisplayOptionsDefaults)
{
    // Check defaults
    EXPECT_TRUE(state->isShowGridEnabled());
    EXPECT_TRUE(state->isAutoScaleEnabled());
    EXPECT_FLOAT_EQ(state->getGainDb(), 0.0f);
}

// Test: Sidebar defaults
TEST_F(StatePersistenceLoadTest, SidebarDefaults)
{
    EXPECT_EQ(state->getSidebarWidth(), 300);
    EXPECT_FALSE(state->isSidebarCollapsed());
    EXPECT_TRUE(state->isStatusBarVisible());
}

// Note: GlobalPreferences tests are in test_state_persistence.cpp (GlobalPreferencesTest fixture)
