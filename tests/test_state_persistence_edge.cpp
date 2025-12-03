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
