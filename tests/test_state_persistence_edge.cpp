/*
    Oscil - State Persistence Tests: Edge Cases
    Tests for edge cases, boundary conditions, and error handling
*/

#include <gtest/gtest.h>
#include "helpers/StateBuilder.h"
#include "helpers/OscillatorBuilder.h"
#include "helpers/Fixtures.h"
#include "core/OscilState.h"

using namespace oscil;
using namespace oscil::test;

class StatePersistenceEdgeTest : public StateTestFixture {};

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

// Test: Reorder empty list
TEST_F(StatePersistenceEdgeTest, ReorderEmptyList)
{
    // Reorder on empty list should not crash
    state->reorderOscillators(0, 1);
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

// Test: Gain dB extreme values are clamped to valid range [-60, 12]
TEST_F(StatePersistenceEdgeTest, GainDbExtremeValues)
{
    // Values above max should be clamped to 12dB
    state->setGainDb(100.0f);
    EXPECT_FLOAT_EQ(state->getGainDb(), 12.0f);

    // Values below min should be clamped to -60dB
    state->setGainDb(-100.0f);
    EXPECT_FLOAT_EQ(state->getGainDb(), -60.0f);

    // Valid values within range should be stored as-is
    state->setGainDb(0.0f);
    EXPECT_FLOAT_EQ(state->getGainDb(), 0.0f);

    state->setGainDb(-30.0f);
    EXPECT_FLOAT_EQ(state->getGainDb(), -30.0f);

    state->setGainDb(12.0f);
    EXPECT_FLOAT_EQ(state->getGainDb(), 12.0f);
}

// Test: Sidebar width extreme values are clamped to valid range [50, 500]
TEST_F(StatePersistenceEdgeTest, SidebarWidthExtremeValues)
{
    // Values below min should be clamped to 50
    state->setSidebarWidth(0);
    EXPECT_EQ(state->getSidebarWidth(), 50);

    // Values above max should be clamped to 500
    state->setSidebarWidth(10000);
    EXPECT_EQ(state->getSidebarWidth(), 500);

    // Negative values should be clamped to min
    state->setSidebarWidth(-100);
    EXPECT_EQ(state->getSidebarWidth(), 50);

    // Valid values within range should be stored as-is
    state->setSidebarWidth(300);
    EXPECT_EQ(state->getSidebarWidth(), 300);

    state->setSidebarWidth(500);
    EXPECT_EQ(state->getSidebarWidth(), 500);
}

// =============================================================================
// PANE CLOSE TESTS - Oscillator state updates when pane is removed
// =============================================================================

TEST_F(StatePersistenceEdgeTest, PaneCloseUpdatesOscillatorState)
{
    auto& layoutManager = state->getLayoutManager();

    // Create two panes
    Pane pane1, pane2;
    pane1.setName("Pane 1");
    pane1.setOrderIndex(0);
    pane2.setName("Pane 2");
    pane2.setOrderIndex(1);
    layoutManager.addPane(pane1);
    layoutManager.addPane(pane2);

    // Create oscillators assigned to pane1 (default is visible=true)
    auto osc1 = OscillatorBuilder()
        .withName("Osc 1 in Pane 1")
        .withPaneId(pane1.getId())
        .build();

    auto osc2 = OscillatorBuilder()
        .withName("Osc 2 in Pane 1")
        .withPaneId(pane1.getId())
        .build();

    // osc3 is in pane2 - should not be affected
    auto osc3 = OscillatorBuilder()
        .withName("Osc 3 in Pane 2")
        .withPaneId(pane2.getId())
        .build();

    state->addOscillator(osc1);
    state->addOscillator(osc2);
    state->addOscillator(osc3);

    // Simulate pane close: update oscillators in pane1 to have invalid paneId and invisible
    auto oscillators = state->getOscillators();
    for (auto& osc : oscillators)
    {
        if (osc.getPaneId() == pane1.getId())
        {
            osc.setPaneId(PaneId::invalid());
            osc.setVisible(false);
            state->updateOscillator(osc);
        }
    }

    // Remove the pane
    layoutManager.removePane(pane1.getId());

    // Verify: pane1 should be removed
    EXPECT_EQ(layoutManager.getPaneCount(), 1);
    EXPECT_NE(layoutManager.getPane(pane2.getId()), nullptr);
    EXPECT_EQ(layoutManager.getPane(pane1.getId()), nullptr);

    // Verify: oscillators in pane1 should have invalid paneId and be invisible
    oscillators = state->getOscillators();
    EXPECT_EQ(oscillators.size(), 3);

    for (const auto& osc : oscillators)
    {
        if (osc.getName() == "Osc 1 in Pane 1" || osc.getName() == "Osc 2 in Pane 1")
        {
            EXPECT_FALSE(osc.getPaneId().isValid()) << "Oscillator " << osc.getName() << " should have invalid paneId";
            EXPECT_FALSE(osc.isVisible()) << "Oscillator " << osc.getName() << " should be invisible";
        }
        else if (osc.getName() == "Osc 3 in Pane 2")
        {
            EXPECT_TRUE(osc.getPaneId().isValid()) << "Osc 3 should still have valid paneId";
            EXPECT_EQ(osc.getPaneId(), pane2.getId()) << "Osc 3 should still be in pane2";
            EXPECT_TRUE(osc.isVisible()) << "Osc 3 should still be visible";
        }
    }
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
