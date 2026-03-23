/*
    Oscil - State Persistence Tests: Edge Cases
    Tests for edge cases, boundary conditions, and error handling
*/

#include "core/OscilState.h"

#include "helpers/Fixtures.h"
#include "helpers/OscillatorBuilder.h"
#include "helpers/StateBuilder.h"

#include <gtest/gtest.h>

using namespace oscil;
using namespace oscil::test;

class StatePersistenceEdgeTest : public StateTestFixture
{
};

// Test: Remove non-existent oscillator
TEST_F(StatePersistenceEdgeTest, RemoveNonExistentOscillator)
{
    auto osc = OscillatorBuilder().build();
    state->addOscillator(osc);
    EXPECT_EQ(state->getOscillators().size(), 1);

    // Remove with non-existent ID
    state->removeOscillator(OscillatorId::generate());

    // Should still have original oscillator
    EXPECT_EQ(state->getOscillators().size(), 1);
}

// Test: Update non-existent oscillator
TEST_F(StatePersistenceEdgeTest, UpdateNonExistentOscillator)
{
    auto osc = OscillatorBuilder().withName("Test").build();
    state->addOscillator(osc);

    // Create new oscillator with different ID and try to update
    auto nonExistent = OscillatorBuilder().withName("Non-existent").build();
    state->updateOscillator(nonExistent);

    // Original should be unchanged
    auto oscillators = state->getOscillators();
    EXPECT_EQ(oscillators.size(), 1);
    EXPECT_EQ(oscillators[0].getName(), "Test");
}

// Test: Reorder with invalid old index
TEST_F(StatePersistenceEdgeTest, ReorderWithInvalidOldIndex)
{
    auto osc1 = OscillatorBuilder().withOrderIndex(0).build();
    auto osc2 = OscillatorBuilder().withOrderIndex(1).build();
    state->addOscillator(osc1);
    state->addOscillator(osc2);

    // Invalid oldIndex
    state->reorderOscillators(-1, 0);
    state->reorderOscillators(10, 0);

    // Should be unchanged
    EXPECT_EQ(state->getOscillators().size(), 2);
}

// Test: Reorder with invalid new index
TEST_F(StatePersistenceEdgeTest, ReorderWithInvalidNewIndex)
{
    auto osc1 = OscillatorBuilder().withOrderIndex(0).build();
    auto osc2 = OscillatorBuilder().withOrderIndex(1).build();
    state->addOscillator(osc1);
    state->addOscillator(osc2);

    // Invalid newIndex
    state->reorderOscillators(0, -1);
    state->reorderOscillators(0, 10);

    // Should be unchanged
    EXPECT_EQ(state->getOscillators().size(), 2);
}

// Test: Reorder same index
TEST_F(StatePersistenceEdgeTest, ReorderSameIndex)
{
    auto osc = OscillatorBuilder().withOrderIndex(0).build();
    state->addOscillator(osc);

    // Same index should be no-op
    state->reorderOscillators(0, 0);

    EXPECT_EQ(state->getOscillators().size(), 1);
}

// Test: Reorder empty list leaves state unchanged
TEST_F(StatePersistenceEdgeTest, ReorderEmptyList)
{
    EXPECT_EQ(state->getOscillators().size(), 0);

    // Reorder on empty list should not crash and list should remain empty
    state->reorderOscillators(0, 1);

    EXPECT_EQ(state->getOscillators().size(), 0);
}

// Test: Column layout clamping low
TEST_F(StatePersistenceEdgeTest, ColumnLayoutClampingLow)
{
    // Create a new OscilState for this test to avoid fixture issues
    OscilState testState;

    // Set via direct property manipulation (bypassing setter)
    auto& tree = testState.getState();
    auto layout = tree.getChildWithName(StateIds::Layout);
    layout.setProperty(StateIds::Columns, -1, nullptr);

    // getColumnLayout should clamp
    EXPECT_EQ(static_cast<int>(testState.getColumnLayout()), 1);
}

// Test: Column layout clamping high
TEST_F(StatePersistenceEdgeTest, ColumnLayoutClampingHigh)
{
    // Create a new OscilState for this test to avoid fixture issues
    OscilState testState;

    // Set via direct property manipulation (bypassing setter)
    auto& tree = testState.getState();
    auto layout = tree.getChildWithName(StateIds::Layout);
    layout.setProperty(StateIds::Columns, 100, nullptr);

    // getColumnLayout should clamp to 3
    EXPECT_EQ(static_cast<int>(testState.getColumnLayout()), 3);
}

// Test: Gain dB extreme values
TEST_F(StatePersistenceEdgeTest, GainDbExtremeValues)
{
    state->setGainDb(100.0f);
    EXPECT_FLOAT_EQ(state->getGainDb(), 100.0f);

    state->setGainDb(-100.0f);
    EXPECT_FLOAT_EQ(state->getGainDb(), -100.0f);
}

// Test: Sidebar width extreme values
TEST_F(StatePersistenceEdgeTest, SidebarWidthExtremeValues)
{
    state->setSidebarWidth(0);
    EXPECT_EQ(state->getSidebarWidth(), 0);

    state->setSidebarWidth(10000);
    EXPECT_EQ(state->getSidebarWidth(), 10000);

    state->setSidebarWidth(-100);
    EXPECT_EQ(state->getSidebarWidth(), -100);
}

// =============================================================================
// PANE CLOSE TESTS - Oscillator state updates when pane is removed
// =============================================================================

namespace
{

// Simulate pane close: detach oscillators from pane and remove the pane
void simulatePaneClose(OscilState& state, const PaneId& paneId)
{
    auto oscillators = state.getOscillators();
    for (auto& osc : oscillators)
    {
        if (osc.getPaneId() == paneId)
        {
            osc.setPaneId(PaneId::invalid());
            osc.setVisible(false);
            state.updateOscillator(osc);
        }
    }
    state.getLayoutManager().removePane(paneId);
}

// Verify oscillator is detached (invisible, invalid pane)
void verifyOscillatorDetached(const OscilState& state, const OscillatorId& oscId, const juce::String& label)
{
    auto osc = state.getOscillator(oscId);
    ASSERT_TRUE(osc.has_value()) << label;
    EXPECT_FALSE(osc->getPaneId().isValid()) << label << " should have invalid paneId";
    EXPECT_FALSE(osc->isVisible()) << label << " should be invisible";
}

// Verify oscillator is still attached to a pane
void verifyOscillatorAttached(const OscilState& state, const OscillatorId& oscId, const PaneId& expectedPaneId,
                              const juce::String& label)
{
    auto osc = state.getOscillator(oscId);
    ASSERT_TRUE(osc.has_value()) << label;
    EXPECT_TRUE(osc->getPaneId().isValid()) << label << " should have valid paneId";
    EXPECT_EQ(osc->getPaneId(), expectedPaneId) << label << " wrong pane";
    EXPECT_TRUE(osc->isVisible()) << label << " should be visible";
}

} // anonymous namespace

TEST_F(StatePersistenceEdgeTest, PaneCloseUpdatesOscillatorState)
{
    auto& layoutManager = state->getLayoutManager();

    Pane pane1, pane2;
    pane1.setName("Pane 1");
    pane1.setOrderIndex(0);
    pane2.setName("Pane 2");
    pane2.setOrderIndex(1);
    layoutManager.addPane(pane1);
    layoutManager.addPane(pane2);

    auto osc1 = OscillatorBuilder().withName("Osc 1").withPaneId(pane1.getId()).build();
    auto osc2 = OscillatorBuilder().withName("Osc 2").withPaneId(pane1.getId()).build();
    auto osc3 = OscillatorBuilder().withName("Osc 3").withPaneId(pane2.getId()).build();
    state->addOscillator(osc1);
    state->addOscillator(osc2);
    state->addOscillator(osc3);

    simulatePaneClose(*state, pane1.getId());

    EXPECT_EQ(layoutManager.getPaneCount(), 1);
    EXPECT_EQ(layoutManager.getPane(pane1.getId()), nullptr);
    EXPECT_NE(layoutManager.getPane(pane2.getId()), nullptr);
    EXPECT_EQ(state->getOscillators().size(), 3);

    verifyOscillatorDetached(*state, osc1.getId(), "Osc 1");
    verifyOscillatorDetached(*state, osc2.getId(), "Osc 2");
    verifyOscillatorAttached(*state, osc3.getId(), pane2.getId(), "Osc 3");
}

TEST_F(StatePersistenceEdgeTest, PaneCloseWithNoOscillatorsInPane)
{
    auto& layoutManager = state->getLayoutManager();

    // Create a pane with no oscillators
    Pane pane;
    pane.setName("Empty Pane");
    layoutManager.addPane(pane);

    EXPECT_EQ(layoutManager.getPaneCount(), 1);

    // Remove the pane
    layoutManager.removePane(pane.getId());

    EXPECT_EQ(layoutManager.getPaneCount(), 0);
    EXPECT_TRUE(state->getOscillators().empty());
}
