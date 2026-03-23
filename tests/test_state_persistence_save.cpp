/*
    Oscil - State Persistence Tests: Save Operations
    Tests for state serialization, XML output, and save operations
*/

#include "core/OscilState.h"
#include "core/Oscillator.h"
#include "core/Pane.h"

#include "helpers/Fixtures.h"
#include "helpers/OscillatorBuilder.h"
#include "helpers/StateBuilder.h"

#include <gtest/gtest.h>

using namespace oscil;
using namespace oscil::test;

class StatePersistenceSaveTest : public StateTestFixture
{
};

// Test: Default state initialization
TEST_F(StatePersistenceSaveTest, DefaultStateInitialization)
{
    EXPECT_EQ(state->getSchemaVersion(), OscilState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(state->getThemeName(), "Dark Professional");
    EXPECT_EQ(state->getColumnLayout(), ColumnLayout::Single);
}

// Test: Add oscillator
TEST_F(StatePersistenceSaveTest, AddOscillator)
{
    auto osc = OscillatorBuilder().withName("Test Oscillator").asMid().build();

    state->addOscillator(osc);

    auto oscillators = state->getOscillators();
    EXPECT_EQ(oscillators.size(), 1);
    EXPECT_EQ(oscillators[0].getName(), "Test Oscillator");
    EXPECT_EQ(oscillators[0].getProcessingMode(), ProcessingMode::Mid);
}

// Test: Remove oscillator
TEST_F(StatePersistenceSaveTest, RemoveOscillator)
{
    auto osc1 = OscillatorBuilder().withName("Osc 1").build();
    auto osc2 = OscillatorBuilder().withName("Osc 2").build();

    state->addOscillator(osc1);
    state->addOscillator(osc2);

    EXPECT_EQ(state->getOscillators().size(), 2);

    state->removeOscillator(osc1.getId());

    auto oscillators = state->getOscillators();
    EXPECT_EQ(oscillators.size(), 1);
    EXPECT_EQ(oscillators[0].getName(), "Osc 2");
}

// Test: Update oscillator
TEST_F(StatePersistenceSaveTest, UpdateOscillator)
{
    auto osc = OscillatorBuilder().withName("Original Name").build();
    state->addOscillator(osc);

    // Update the oscillator
    osc.setName("Updated Name");
    osc.setProcessingMode(ProcessingMode::Side);
    state->updateOscillator(osc);

    auto retrieved = state->getOscillator(osc.getId());
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getName(), "Updated Name");
    EXPECT_EQ(retrieved->getProcessingMode(), ProcessingMode::Side);
}

// Test: Get oscillator by ID
TEST_F(StatePersistenceSaveTest, GetOscillatorById)
{
    auto osc = OscillatorBuilder().withName("Target Oscillator").build();
    state->addOscillator(osc);

    auto retrieved = state->getOscillator(osc.getId());
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getName(), "Target Oscillator");

    // Nonexistent ID
    auto notFound = state->getOscillator(OscillatorId::generate());
    EXPECT_FALSE(notFound.has_value());
}

// Test: Theme name persistence
TEST_F(StatePersistenceSaveTest, ThemeNamePersistence)
{
    state->setThemeName("Classic Green");
    EXPECT_EQ(state->getThemeName(), "Classic Green");
}

// Test: Column layout persistence
TEST_F(StatePersistenceSaveTest, ColumnLayoutPersistence)
{
    state->setColumnLayout(ColumnLayout::Triple);
    EXPECT_EQ(state->getColumnLayout(), ColumnLayout::Triple);
}

// Test: XML serialization round-trip
TEST_F(StatePersistenceSaveTest, XmlSerializationRoundTrip)
{
    auto state =
        StateBuilder()
            .withThemeName("High Contrast")
            .withDoubleColumn()
            .withOscillator(
                OscillatorBuilder().withName("Oscillator 1").asMid().withColour(juce::Colour(0xFF112233)).build())
            .withOscillator(
                OscillatorBuilder().withName("Oscillator 2").asSide().withColour(juce::Colour(0xFF445566)).build())
            .buildUnique();

    // Serialize to XML
    juce::String xmlString = state->toXmlString();
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

// Test: Pane layout manager integration
TEST_F(StatePersistenceSaveTest, PaneLayoutManagerIntegration)
{
    auto& layoutManager = state->getLayoutManager();

    Pane pane1, pane2;
    pane1.setName("Pane 1");
    pane1.setOrderIndex(0);
    pane2.setName("Pane 2");
    pane2.setOrderIndex(1);

    layoutManager.addPane(pane1);
    layoutManager.addPane(pane2);

    EXPECT_EQ(layoutManager.getPaneCount(), 2);

    // Serialize and restore
    juce::String xmlString = state->toXmlString();

    OscilState restored;
    (void) restored.fromXmlString(xmlString);

    // Note: Pane layout manager serialization is handled separately
    // This tests that the state can hold pane data
}

// Test: Multiple oscillators with same source
TEST_F(StatePersistenceSaveTest, MultipleOscillatorsSameSource)
{
    SourceId sharedSource = SourceId::generate();

    auto state =
        StateBuilder()
            .withOscillator(OscillatorBuilder().withSourceId(sharedSource).asMid().withName("Mid View").build())
            .withOscillator(OscillatorBuilder().withSourceId(sharedSource).asSide().withName("Side View").build())
            .withOscillator(OscillatorBuilder().withSourceId(sharedSource).asMono().withName("Mono View").build())
            .buildUnique();

    auto oscillators = state->getOscillators();
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
TEST_F(StatePersistenceSaveTest, SchemaVersionInSerializedState)
{
    juce::String xmlString = state->toXmlString();

    // Version should be present
    EXPECT_TRUE(xmlString.contains("version"));
}

// Test: Display options persistence
TEST_F(StatePersistenceSaveTest, DisplayOptionsPersistence)
{
    auto original = StateBuilder().withoutGrid().withoutAutoScaling().withGainDb(-12.0f).buildUnique();

    juce::String xml = original->toXmlString();

    OscilState restored;
    EXPECT_TRUE(restored.fromXmlString(xml));

    EXPECT_FALSE(restored.isShowGridEnabled());
    EXPECT_FALSE(restored.isAutoScaleEnabled());
    EXPECT_FLOAT_EQ(restored.getGainDb(), -12.0f);
}

// Test: Sidebar persistence
TEST_F(StatePersistenceSaveTest, SidebarPersistence)
{
    auto original = StateBuilder().withSidebarWidth(400).withCollapsedSidebar().withoutStatusBar().buildUnique();

    juce::String xml = original->toXmlString();

    OscilState restored;
    EXPECT_TRUE(restored.fromXmlString(xml));

    EXPECT_EQ(restored.getSidebarWidth(), 400);
    EXPECT_TRUE(restored.isSidebarCollapsed());
    EXPECT_FALSE(restored.isStatusBarVisible());
}

// Test: Many oscillators
TEST_F(StatePersistenceSaveTest, ManyOscillators)
{
    auto state = createStateWithOscillators(100);

    EXPECT_EQ(state->getOscillators().size(), 100);

    // Serialize and restore
    juce::String xml = state->toXmlString();
    EXPECT_FALSE(xml.isEmpty());

    OscilState restored;
    EXPECT_TRUE(restored.fromXmlString(xml));
    EXPECT_EQ(restored.getOscillators().size(), 100);
}

// Test: Long theme name
TEST_F(StatePersistenceSaveTest, LongThemeName)
{
    // Very long theme name
    juce::String longName;
    for (int i = 0; i < 1000; ++i)
        longName += "X";

    state->setThemeName(longName);
    EXPECT_EQ(state->getThemeName(), longName);

    // Should persist correctly
    juce::String xml = state->toXmlString();
    OscilState restored;
    EXPECT_TRUE(restored.fromXmlString(xml));
    EXPECT_EQ(restored.getThemeName(), longName);
}

// Test: Special characters in theme name
TEST_F(StatePersistenceSaveTest, SpecialCharactersInThemeName)
{
    state->setThemeName("Theme <with> \"special\" & 'chars'");

    juce::String xml = state->toXmlString();
    OscilState restored;
    EXPECT_TRUE(restored.fromXmlString(xml));

    EXPECT_EQ(restored.getThemeName(), "Theme <with> \"special\" & 'chars'");
}

// Test: Listener notifications
class TestListener : public juce::ValueTree::Listener
{
public:
    int propertyChangedCount = 0;
    int childAddedCount = 0;
    int childRemovedCount = 0;

    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override { propertyChangedCount++; }

    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override { childAddedCount++; }

    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override { childRemovedCount++; }
};

TEST_F(StatePersistenceSaveTest, ListenerNotifications)
{
    TestListener listener;

    state->addListener(&listener);

    // Change property
    state->setThemeName("New Theme");
    EXPECT_GT(listener.propertyChangedCount, 0);

    // Add oscillator
    auto osc = OscillatorBuilder().build();
    state->addOscillator(osc);
    EXPECT_GT(listener.childAddedCount, 0);

    // Remove oscillator
    state->removeOscillator(osc.getId());
    EXPECT_GT(listener.childRemovedCount, 0);

    state->removeListener(&listener);
}

TEST_F(StatePersistenceSaveTest, RemoveListenerStopsNotifications)
{
    TestListener listener;

    state->addListener(&listener);
    state->setThemeName("Theme 1");
    int countAfterFirst = listener.propertyChangedCount;

    state->removeListener(&listener);
    state->setThemeName("Theme 2");

    // Count should not have increased
    EXPECT_EQ(listener.propertyChangedCount, countAfterFirst);
}
