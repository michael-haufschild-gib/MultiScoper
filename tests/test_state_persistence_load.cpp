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
    EXPECT_FALSE(state->isHoldDisplayEnabled());
    EXPECT_FLOAT_EQ(state->getGainDb(), 0.0f);
}

// Test: Sidebar defaults
TEST_F(StatePersistenceLoadTest, SidebarDefaults)
{
    EXPECT_EQ(state->getSidebarWidth(), 300);
    EXPECT_FALSE(state->isSidebarCollapsed());
    EXPECT_TRUE(state->isStatusBarVisible());
}

// Test: Global preferences defaults
TEST_F(StatePersistenceLoadTest, GlobalPreferencesDefaults)
{
    auto& prefs = GlobalPreferences::getInstance();

    // Check that defaults are reasonable
    EXPECT_EQ(prefs.getDefaultTheme(), "Dark Professional");
    EXPECT_EQ(prefs.getDefaultColumnLayout(), 1);
    EXPECT_TRUE(prefs.getShowStatusBar());
    EXPECT_FALSE(prefs.getReducedMotion());
    EXPECT_FALSE(prefs.getUIAudioFeedback());
    EXPECT_TRUE(prefs.getTooltipsEnabled());
    EXPECT_EQ(prefs.getDefaultSidebarWidth(), 280);
}

// Test: Global preferences setters and getters
TEST_F(StatePersistenceLoadTest, GlobalPreferencesSettersAndGetters)
{
    auto& prefs = GlobalPreferences::getInstance();

    // Save original values
    auto originalTheme = prefs.getDefaultTheme();
    auto originalColumns = prefs.getDefaultColumnLayout();
    auto originalSidebarWidth = prefs.getDefaultSidebarWidth();

    // Test setters
    prefs.setDefaultTheme("Test Theme");
    EXPECT_EQ(prefs.getDefaultTheme(), "Test Theme");

    prefs.setDefaultColumnLayout(2);
    EXPECT_EQ(prefs.getDefaultColumnLayout(), 2);

    prefs.setShowStatusBar(false);
    EXPECT_FALSE(prefs.getShowStatusBar());

    prefs.setReducedMotion(true);
    EXPECT_TRUE(prefs.getReducedMotion());

    prefs.setUIAudioFeedback(true);
    EXPECT_TRUE(prefs.getUIAudioFeedback());

    prefs.setTooltipsEnabled(false);
    EXPECT_FALSE(prefs.getTooltipsEnabled());

    prefs.setDefaultSidebarWidth(350);
    EXPECT_EQ(prefs.getDefaultSidebarWidth(), 350);

    // Restore original values
    prefs.setDefaultTheme(originalTheme);
    prefs.setDefaultColumnLayout(originalColumns);
    prefs.setShowStatusBar(true);
    prefs.setReducedMotion(false);
    prefs.setUIAudioFeedback(false);
    prefs.setTooltipsEnabled(true);
    prefs.setDefaultSidebarWidth(originalSidebarWidth);
}

// Test: Global preferences singleton
TEST_F(StatePersistenceLoadTest, GlobalPreferencesSingleton)
{
    auto& prefs1 = GlobalPreferences::getInstance();
    auto& prefs2 = GlobalPreferences::getInstance();

    // Should be the same instance
    EXPECT_EQ(&prefs1, &prefs2);
}
