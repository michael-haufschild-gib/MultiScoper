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
    (void)restored.fromXmlString(xmlString);

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

// =============================================================================
// CORRUPTION RECOVERY TESTS
// =============================================================================

TEST_F(StatePersistenceTest, MalformedXmlRecovery)
{
    OscilState state;

    // Malformed XML (unclosed tag)
    EXPECT_FALSE(state.fromXmlString("<OscilState><broken"));

    // State should remain usable with defaults
    EXPECT_EQ(state.getSchemaVersion(), OscilState::CURRENT_SCHEMA_VERSION);
}

TEST_F(StatePersistenceTest, TruncatedXmlRecovery)
{
    OscilState original;
    original.setThemeName("Test Theme");

    juce::String xmlString = original.toXmlString();

    // Truncate the XML
    juce::String truncated = xmlString.substring(0, xmlString.length() / 2);

    OscilState state;
    EXPECT_FALSE(state.fromXmlString(truncated));
}

TEST_F(StatePersistenceTest, XmlWithWrongRootType)
{
    OscilState state;

    // Valid XML but wrong root type
    EXPECT_FALSE(state.fromXmlString("<SomeOtherRoot><Oscillators/></SomeOtherRoot>"));
}

TEST_F(StatePersistenceTest, XmlWithMissingChildNodes)
{
    OscilState state;

    // Valid OscilState but missing all child nodes
    juce::String minimalXml = "<OscilState version=\"2\"/>";
    EXPECT_TRUE(state.fromXmlString(minimalXml));

    // Should have empty oscillators
    EXPECT_TRUE(state.getOscillators().empty());

    // Adding oscillator should still work (creates Oscillators node)
    Oscillator osc;
    osc.setName("Test");
    state.addOscillator(osc);
    EXPECT_EQ(state.getOscillators().size(), 1);
}

TEST_F(StatePersistenceTest, XmlWithPartialChildNodes)
{
    OscilState state;

    // Has Oscillators but missing Layout and Theme
    juce::String partialXml = R"(
        <OscilState version="2">
            <Oscillators/>
        </OscilState>
    )";
    EXPECT_TRUE(state.fromXmlString(partialXml));

    // Should use defaults for missing nodes
    EXPECT_EQ(state.getThemeName(), "Dark Professional");
}

TEST_F(StatePersistenceTest, CorruptedOscillatorDataRecovery)
{
    OscilState state;

    // Oscillator with invalid/missing properties
    juce::String xmlWithBadOsc = R"(
        <OscilState version="2">
            <Oscillators>
                <Oscillator/>
                <Oscillator id="valid-id" name="Valid Osc"/>
            </Oscillators>
        </OscilState>
    )";
    EXPECT_TRUE(state.fromXmlString(xmlWithBadOsc));

    // Should load what it can
    auto oscillators = state.getOscillators();
    EXPECT_GE(oscillators.size(), 1);
}

// =============================================================================
// SCHEMA MIGRATION TESTS
// =============================================================================

TEST_F(StatePersistenceTest, MissingSchemaVersion)
{
    OscilState state;

    // Old state without version property
    juce::String oldXml = R"(
        <OscilState>
            <Oscillators/>
            <Theme themeName="Classic Green"/>
        </OscilState>
    )";
    EXPECT_TRUE(state.fromXmlString(oldXml));

    // getSchemaVersion defaults to 1 for old states
    EXPECT_EQ(state.getSchemaVersion(), 1);
}

TEST_F(StatePersistenceTest, OldSchemaVersion)
{
    OscilState state;

    // Version 1 state
    juce::String v1Xml = R"(
        <OscilState version="1">
            <Oscillators/>
            <Theme themeName="Old Theme"/>
        </OscilState>
    )";
    EXPECT_TRUE(state.fromXmlString(v1Xml));

    EXPECT_EQ(state.getSchemaVersion(), 1);
    EXPECT_EQ(state.getThemeName(), "Old Theme");
}

TEST_F(StatePersistenceTest, FutureSchemaVersion)
{
    OscilState state;

    // Future version (should still load)
    juce::String futureXml = R"(
        <OscilState version="999">
            <Oscillators/>
            <Theme themeName="Future Theme"/>
        </OscilState>
    )";
    EXPECT_TRUE(state.fromXmlString(futureXml));

    EXPECT_EQ(state.getSchemaVersion(), 999);
}

// =============================================================================
// EDGE CASE TESTS - Oscillator Operations
// =============================================================================

TEST_F(StatePersistenceTest, RemoveNonExistentOscillator)
{
    OscilState state;

    Oscillator osc;
    state.addOscillator(osc);
    EXPECT_EQ(state.getOscillators().size(), 1);

    // Remove with non-existent ID
    state.removeOscillator(OscillatorId::generate());

    // Should still have original oscillator
    EXPECT_EQ(state.getOscillators().size(), 1);
}

TEST_F(StatePersistenceTest, UpdateNonExistentOscillator)
{
    OscilState state;

    Oscillator osc;
    osc.setName("Test");
    state.addOscillator(osc);

    // Create new oscillator with different ID and try to update
    Oscillator nonExistent;
    nonExistent.setName("Non-existent");
    state.updateOscillator(nonExistent);

    // Original should be unchanged
    auto oscillators = state.getOscillators();
    EXPECT_EQ(oscillators.size(), 1);
    EXPECT_EQ(oscillators[0].getName(), "Test");
}

TEST_F(StatePersistenceTest, ReorderWithInvalidOldIndex)
{
    OscilState state;

    Oscillator osc1, osc2;
    osc1.setOrderIndex(0);
    osc2.setOrderIndex(1);
    state.addOscillator(osc1);
    state.addOscillator(osc2);

    // Invalid oldIndex
    state.reorderOscillators(-1, 0);
    state.reorderOscillators(10, 0);

    // Should be unchanged
    EXPECT_EQ(state.getOscillators().size(), 2);
}

TEST_F(StatePersistenceTest, ReorderWithInvalidNewIndex)
{
    OscilState state;

    Oscillator osc1, osc2;
    osc1.setOrderIndex(0);
    osc2.setOrderIndex(1);
    state.addOscillator(osc1);
    state.addOscillator(osc2);

    // Invalid newIndex
    state.reorderOscillators(0, -1);
    state.reorderOscillators(0, 10);

    // Should be unchanged
    EXPECT_EQ(state.getOscillators().size(), 2);
}

TEST_F(StatePersistenceTest, ReorderSameIndex)
{
    OscilState state;

    Oscillator osc;
    osc.setOrderIndex(0);
    state.addOscillator(osc);

    // Same index should be no-op
    state.reorderOscillators(0, 0);

    EXPECT_EQ(state.getOscillators().size(), 1);
}

TEST_F(StatePersistenceTest, ReorderEmptyList)
{
    OscilState state;

    // Reorder on empty list should not crash
    state.reorderOscillators(0, 1);
}

// =============================================================================
// EDGE CASE TESTS - Column Layout
// =============================================================================

TEST_F(StatePersistenceTest, ColumnLayoutClampingLow)
{
    OscilState state;

    // Set via direct property manipulation (bypassing setter)
    auto& tree = state.getState();
    auto layout = tree.getChildWithName(StateIds::Layout);
    layout.setProperty(StateIds::Columns, -1, nullptr);

    // getColumnLayout should clamp
    EXPECT_EQ(static_cast<int>(state.getColumnLayout()), 1);
}

TEST_F(StatePersistenceTest, ColumnLayoutClampingHigh)
{
    OscilState state;

    // Set via direct property manipulation (bypassing setter)
    auto& tree = state.getState();
    auto layout = tree.getChildWithName(StateIds::Layout);
    layout.setProperty(StateIds::Columns, 100, nullptr);

    // getColumnLayout should clamp to 3
    EXPECT_EQ(static_cast<int>(state.getColumnLayout()), 3);
}

// =============================================================================
// EDGE CASE TESTS - Display Options
// =============================================================================

TEST_F(StatePersistenceTest, DisplayOptionsPersistence)
{
    OscilState original;

    original.setShowGridEnabled(false);
    original.setAutoScaleEnabled(false);
    original.setHoldDisplayEnabled(true);
    original.setGainDb(-12.0f);

    juce::String xml = original.toXmlString();

    OscilState restored;
    EXPECT_TRUE(restored.fromXmlString(xml));

    EXPECT_FALSE(restored.isShowGridEnabled());
    EXPECT_FALSE(restored.isAutoScaleEnabled());
    EXPECT_TRUE(restored.isHoldDisplayEnabled());
    EXPECT_FLOAT_EQ(restored.getGainDb(), -12.0f);
}

TEST_F(StatePersistenceTest, GainDbExtremeValues)
{
    OscilState state;

    state.setGainDb(100.0f);
    EXPECT_FLOAT_EQ(state.getGainDb(), 100.0f);

    state.setGainDb(-100.0f);
    EXPECT_FLOAT_EQ(state.getGainDb(), -100.0f);
}

// =============================================================================
// EDGE CASE TESTS - Sidebar Configuration
// =============================================================================

TEST_F(StatePersistenceTest, SidebarPersistence)
{
    OscilState original;

    original.setSidebarWidth(400);
    original.setSidebarCollapsed(true);
    original.setStatusBarVisible(false);

    juce::String xml = original.toXmlString();

    OscilState restored;
    EXPECT_TRUE(restored.fromXmlString(xml));

    EXPECT_EQ(restored.getSidebarWidth(), 400);
    EXPECT_TRUE(restored.isSidebarCollapsed());
    EXPECT_FALSE(restored.isStatusBarVisible());
}

TEST_F(StatePersistenceTest, SidebarWidthExtremeValues)
{
    OscilState state;

    state.setSidebarWidth(0);
    EXPECT_EQ(state.getSidebarWidth(), 0);

    state.setSidebarWidth(10000);
    EXPECT_EQ(state.getSidebarWidth(), 10000);

    state.setSidebarWidth(-100);
    EXPECT_EQ(state.getSidebarWidth(), -100);
}

// =============================================================================
// LARGE STATE HANDLING TESTS
// =============================================================================

TEST_F(StatePersistenceTest, ManyOscillators)
{
    OscilState state;

    // Add many oscillators
    const int count = 100;
    for (int i = 0; i < count; ++i)
    {
        Oscillator osc;
        osc.setName(juce::String("Oscillator ") + juce::String(i));
        osc.setOrderIndex(i);
        state.addOscillator(osc);
    }

    EXPECT_EQ(state.getOscillators().size(), count);

    // Serialize and restore
    juce::String xml = state.toXmlString();
    EXPECT_FALSE(xml.isEmpty());

    OscilState restored;
    EXPECT_TRUE(restored.fromXmlString(xml));
    EXPECT_EQ(restored.getOscillators().size(), count);
}

TEST_F(StatePersistenceTest, LongThemeName)
{
    OscilState state;

    // Very long theme name
    juce::String longName;
    for (int i = 0; i < 1000; ++i)
        longName += "X";

    state.setThemeName(longName);
    EXPECT_EQ(state.getThemeName(), longName);

    // Should persist correctly
    juce::String xml = state.toXmlString();
    OscilState restored;
    EXPECT_TRUE(restored.fromXmlString(xml));
    EXPECT_EQ(restored.getThemeName(), longName);
}

TEST_F(StatePersistenceTest, SpecialCharactersInThemeName)
{
    OscilState state;

    state.setThemeName("Theme <with> \"special\" & 'chars'");

    juce::String xml = state.toXmlString();
    OscilState restored;
    EXPECT_TRUE(restored.fromXmlString(xml));

    EXPECT_EQ(restored.getThemeName(), "Theme <with> \"special\" & 'chars'");
}

// =============================================================================
// LISTENER TESTS
// =============================================================================

class TestListener : public juce::ValueTree::Listener
{
public:
    int propertyChangedCount = 0;
    int childAddedCount = 0;
    int childRemovedCount = 0;

    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override
    {
        propertyChangedCount++;
    }

    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override
    {
        childAddedCount++;
    }

    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override
    {
        childRemovedCount++;
    }
};

TEST_F(StatePersistenceTest, ListenerNotifications)
{
    OscilState state;
    TestListener listener;

    state.addListener(&listener);

    // Change property
    state.setThemeName("New Theme");
    EXPECT_GT(listener.propertyChangedCount, 0);

    // Add oscillator
    Oscillator osc;
    state.addOscillator(osc);
    EXPECT_GT(listener.childAddedCount, 0);

    // Remove oscillator
    state.removeOscillator(osc.getId());
    EXPECT_GT(listener.childRemovedCount, 0);

    state.removeListener(&listener);
}

TEST_F(StatePersistenceTest, RemoveListenerStopsNotifications)
{
    OscilState state;
    TestListener listener;

    state.addListener(&listener);
    state.setThemeName("Theme 1");
    int countAfterFirst = listener.propertyChangedCount;

    state.removeListener(&listener);
    state.setThemeName("Theme 2");

    // Count should not have increased
    EXPECT_EQ(listener.propertyChangedCount, countAfterFirst);
}

// =============================================================================
// CONSTRUCTOR TESTS
// =============================================================================

TEST_F(StatePersistenceTest, ConstructFromValidXml)
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

TEST_F(StatePersistenceTest, ConstructFromInvalidXml)
{
    OscilState state("invalid xml content");

    // Should have default state
    EXPECT_EQ(state.getSchemaVersion(), OscilState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(state.getThemeName(), "Dark Professional");
}

TEST_F(StatePersistenceTest, ConstructFromEmptyString)
{
    OscilState state("");

    // Should have default state
    EXPECT_EQ(state.getSchemaVersion(), OscilState::CURRENT_SCHEMA_VERSION);
}

// =============================================================================
// GLOBAL PREFERENCES TESTS
// =============================================================================

class GlobalPreferencesTest : public ::testing::Test
{
protected:
    std::unique_ptr<GlobalPreferences> prefs_;

    void SetUp() override
    {
        prefs_ = std::make_unique<GlobalPreferences>();
    }

    void TearDown() override
    {
        prefs_.reset();
    }
};

TEST_F(GlobalPreferencesTest, Defaults)
{
    // Check that defaults are reasonable
    EXPECT_EQ(prefs_->getDefaultTheme(), "Dark Professional");
    EXPECT_EQ(prefs_->getDefaultColumnLayout(), 1);
    EXPECT_TRUE(prefs_->getShowStatusBar());
    EXPECT_FALSE(prefs_->getReducedMotion());
    EXPECT_FALSE(prefs_->getUIAudioFeedback());
    EXPECT_TRUE(prefs_->getTooltipsEnabled());
    EXPECT_EQ(prefs_->getDefaultSidebarWidth(), 280);
}

TEST_F(GlobalPreferencesTest, SettersAndGetters)
{
    // Test setters
    prefs_->setDefaultTheme("Test Theme");
    EXPECT_EQ(prefs_->getDefaultTheme(), "Test Theme");

    prefs_->setDefaultColumnLayout(2);
    EXPECT_EQ(prefs_->getDefaultColumnLayout(), 2);

    prefs_->setShowStatusBar(false);
    EXPECT_FALSE(prefs_->getShowStatusBar());

    prefs_->setReducedMotion(true);
    EXPECT_TRUE(prefs_->getReducedMotion());

    prefs_->setUIAudioFeedback(true);
    EXPECT_TRUE(prefs_->getUIAudioFeedback());

    prefs_->setTooltipsEnabled(false);
    EXPECT_FALSE(prefs_->getTooltipsEnabled());

    prefs_->setDefaultSidebarWidth(350);
    EXPECT_EQ(prefs_->getDefaultSidebarWidth(), 350);
}

TEST_F(GlobalPreferencesTest, ReferenceAccess)
{
    // Verify that modifying through a reference updates the instance
    GlobalPreferences& prefsRef = *prefs_;
    prefsRef.setDefaultColumnLayout(3);
    EXPECT_EQ(prefs_->getDefaultColumnLayout(), 3);
}
