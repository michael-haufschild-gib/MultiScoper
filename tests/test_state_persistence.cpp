/*
    Oscil - State Persistence Tests
*/

#include <gtest/gtest.h>
#include "core/OscilState.h"
#include "core/Oscillator.h"
#include "core/Pane.h"

using namespace oscil;

class StatePersistenceTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default state initialization
TEST_F(StatePersistenceTest, DefaultStateInitialization)
{
    OscilState state;

    EXPECT_EQ(state.getSchemaVersion(), OscilState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(state.getThemeName(), "Dark Professional");
    EXPECT_EQ(state.getColumnLayout(), ColumnLayout::Single);
}

// Test: Add oscillator
TEST_F(StatePersistenceTest, AddOscillator)
{
    OscilState state;

    Oscillator osc;
    osc.setName("Test Oscillator");
    osc.setProcessingMode(ProcessingMode::Mid);

    state.addOscillator(osc);

    auto oscillators = state.getOscillators();
    EXPECT_EQ(oscillators.size(), 1);
    EXPECT_EQ(oscillators[0].getName(), "Test Oscillator");
    EXPECT_EQ(oscillators[0].getProcessingMode(), ProcessingMode::Mid);
}

// Test: Remove oscillator
TEST_F(StatePersistenceTest, RemoveOscillator)
{
    OscilState state;

    Oscillator osc1, osc2;
    osc1.setName("Osc 1");
    osc2.setName("Osc 2");

    state.addOscillator(osc1);
    state.addOscillator(osc2);

    EXPECT_EQ(state.getOscillators().size(), 2);

    state.removeOscillator(osc1.getId());

    auto oscillators = state.getOscillators();
    EXPECT_EQ(oscillators.size(), 1);
    EXPECT_EQ(oscillators[0].getName(), "Osc 2");
}

// Test: Update oscillator
TEST_F(StatePersistenceTest, UpdateOscillator)
{
    OscilState state;

    Oscillator osc;
    osc.setName("Original Name");
    state.addOscillator(osc);

    // Update the oscillator
    osc.setName("Updated Name");
    osc.setProcessingMode(ProcessingMode::Side);
    state.updateOscillator(osc);

    auto retrieved = state.getOscillator(osc.getId());
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getName(), "Updated Name");
    EXPECT_EQ(retrieved->getProcessingMode(), ProcessingMode::Side);
}

// Test: Get oscillator by ID
TEST_F(StatePersistenceTest, GetOscillatorById)
{
    OscilState state;

    Oscillator osc;
    osc.setName("Target Oscillator");
    state.addOscillator(osc);

    auto retrieved = state.getOscillator(osc.getId());
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getName(), "Target Oscillator");

    // Nonexistent ID
    auto notFound = state.getOscillator(OscillatorId::generate());
    EXPECT_FALSE(notFound.has_value());
}

// Test: Theme name persistence
TEST_F(StatePersistenceTest, ThemeNamePersistence)
{
    OscilState state;

    state.setThemeName("Classic Green");
    EXPECT_EQ(state.getThemeName(), "Classic Green");
}

// Test: Column layout persistence
TEST_F(StatePersistenceTest, ColumnLayoutPersistence)
{
    OscilState state;

    state.setColumnLayout(ColumnLayout::Triple);
    EXPECT_EQ(state.getColumnLayout(), ColumnLayout::Triple);
}

// Test: XML serialization round-trip
TEST_F(StatePersistenceTest, XmlSerializationRoundTrip)
{
    OscilState original;

    // Add oscillators
    Oscillator osc1, osc2;
    osc1.setName("Oscillator 1");
    osc1.setProcessingMode(ProcessingMode::Mid);
    osc1.setColour(juce::Colour(0xFF112233));

    osc2.setName("Oscillator 2");
    osc2.setProcessingMode(ProcessingMode::Side);
    osc2.setColour(juce::Colour(0xFF445566));

    original.addOscillator(osc1);
    original.addOscillator(osc2);

    // Set other properties
    original.setThemeName("High Contrast");
    original.setColumnLayout(ColumnLayout::Double);

    // Serialize to XML
    juce::String xmlString = original.toXmlString();
    EXPECT_FALSE(xmlString.isEmpty());

    // Restore from XML
    OscilState restored;
    EXPECT_TRUE(restored.fromXmlString(xmlString));

    // Verify restoration
    EXPECT_EQ(restored.getThemeName(), "High Contrast");
    EXPECT_EQ(restored.getColumnLayout(), ColumnLayout::Double);

    auto oscillators = restored.getOscillators();
    EXPECT_EQ(oscillators.size(), 2);

    // Find oscillators by name (order may vary)
    bool foundOsc1 = false, foundOsc2 = false;
    for (const auto& osc : oscillators)
    {
        if (osc.getName() == "Oscillator 1")
        {
            foundOsc1 = true;
            EXPECT_EQ(osc.getProcessingMode(), ProcessingMode::Mid);
            EXPECT_EQ(osc.getColour().getARGB(), 0xFF112233);
        }
        else if (osc.getName() == "Oscillator 2")
        {
            foundOsc2 = true;
            EXPECT_EQ(osc.getProcessingMode(), ProcessingMode::Side);
            EXPECT_EQ(osc.getColour().getARGB(), 0xFF445566);
        }
    }

    EXPECT_TRUE(foundOsc1);
    EXPECT_TRUE(foundOsc2);
}

// Test: Empty XML handling
TEST_F(StatePersistenceTest, EmptyXmlHandling)
{
    OscilState state;

    EXPECT_FALSE(state.fromXmlString(""));
    EXPECT_FALSE(state.fromXmlString("invalid xml"));
}

// Test: Invalid XML handling
TEST_F(StatePersistenceTest, InvalidXmlHandling)
{
    OscilState state;

    // Valid XML but wrong structure
    EXPECT_FALSE(state.fromXmlString("<WrongRoot></WrongRoot>"));
}

// Test: Migration detection
TEST_F(StatePersistenceTest, MigrationDetection)
{
    OscilState state;

    // Fresh state should not need migration
    EXPECT_FALSE(state.needsMigration());
}

// Test: Pane layout manager integration
TEST_F(StatePersistenceTest, PaneLayoutManagerIntegration)
{
    OscilState state;

    auto& layoutManager = state.getLayoutManager();

    Pane pane1, pane2;
    pane1.setName("Pane 1");
    pane1.setOrderIndex(0);
    pane2.setName("Pane 2");
    pane2.setOrderIndex(1);

    layoutManager.addPane(pane1);
    layoutManager.addPane(pane2);

    EXPECT_EQ(layoutManager.getPaneCount(), 2);

    // Serialize and restore
    juce::String xmlString = state.toXmlString();

    OscilState restored;
    restored.fromXmlString(xmlString);

    // Note: Pane layout manager serialization is handled separately
    // This tests that the state can hold pane data
}

// Test: Multiple oscillators with same source
TEST_F(StatePersistenceTest, MultipleOscillatorsSameSource)
{
    OscilState state;

    SourceId sharedSource = SourceId::generate();

    Oscillator osc1, osc2, osc3;
    osc1.setSourceId(sharedSource);
    osc1.setProcessingMode(ProcessingMode::Mid);
    osc1.setName("Mid View");

    osc2.setSourceId(sharedSource);
    osc2.setProcessingMode(ProcessingMode::Side);
    osc2.setName("Side View");

    osc3.setSourceId(sharedSource);
    osc3.setProcessingMode(ProcessingMode::Mono);
    osc3.setName("Mono View");

    state.addOscillator(osc1);
    state.addOscillator(osc2);
    state.addOscillator(osc3);

    auto oscillators = state.getOscillators();
    EXPECT_EQ(oscillators.size(), 3);

    // All should reference the same source
    for (const auto& osc : oscillators)
    {
        EXPECT_EQ(osc.getSourceId(), sharedSource);
    }

    // But have different processing modes
    std::set<ProcessingMode> modes;
    for (const auto& osc : oscillators)
    {
        modes.insert(osc.getProcessingMode());
    }
    EXPECT_EQ(modes.size(), 3);
}

// Test: Schema version in serialized state
TEST_F(StatePersistenceTest, SchemaVersionInSerializedState)
{
    OscilState state;
    juce::String xmlString = state.toXmlString();

    // Version should be present
    EXPECT_TRUE(xmlString.contains("version"));
}
