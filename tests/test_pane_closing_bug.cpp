/*
    Oscil - Pane Closing Bug Test

    Tests the behavior when closing a pane:
    - Oscillators assigned to the pane should have their paneId invalidated
    - Oscillators should be marked as not visible
    - The pane should be removed from layout manager
*/

#include <gtest/gtest.h>
#include "OscilTestFixtures.h"
#include "core/OscilState.h"

using namespace oscil;
using namespace oscil::test;

/**
 * Test fixture for pane closing behavior.
 * Uses OscilPluginTestFixture because OscilPluginEditor requires real singletons
 * (many UI components bypass DI and access ThemeManager::getInstance() directly).
 */
class PaneClosingBugTest : public OscilPluginTestFixture
{
protected:
    void SetUp() override
    {
        OscilPluginTestFixture::SetUp();
        // Create the editor for this test
        createEditor();
    }
};

TEST_F(PaneClosingBugTest, ClosingPaneRemovesItAndHidesOscillator)
{
    auto& state = processor->getState();
    auto& layoutManager = state.getLayoutManager();

    // Get initial state - editor should have created default pane/oscillator
    // If not, create them manually
    if (layoutManager.getPaneCount() == 0)
    {
        Pane pane;
        pane.setName("Pane 1");
        layoutManager.addPane(pane);

        Oscillator osc;
        osc.setPaneId(pane.getId());
        osc.setVisible(true);
        state.addOscillator(osc);

        // Pump message queue to let state listeners process
        pumpMessageQueue(100);
    }

    ASSERT_GE(layoutManager.getPaneCount(), 1) << "Expected at least one pane";
    ASSERT_GE(state.getOscillators().size(), 1u) << "Expected at least one oscillator";

    auto paneId = layoutManager.getPanes()[0].getId();
    auto initialOscCount = state.getOscillators().size();

    // Find oscillators in this pane
    std::vector<OscillatorId> oscillatorsInPane;
    for (const auto& osc : state.getOscillators())
    {
        if (osc.getPaneId() == paneId)
            oscillatorsInPane.push_back(osc.getId());
    }

    // Simulate the logic from handlePaneClose:
    // 1. Update oscillators assigned to this pane
    for (auto& osc : state.getOscillators())
    {
        if (osc.getPaneId() == paneId)
        {
            Oscillator updated = osc;
            updated.setPaneId(PaneId::invalid());
            updated.setVisible(false);
            state.updateOscillator(updated);
        }
    }

    // 2. Remove the pane
    layoutManager.removePane(paneId);

    // Pump message queue to let async state updates process
    pumpMessageQueue(100);

    // Verify: Pane should be removed
    EXPECT_EQ(layoutManager.getPaneCount(), 0) << "Pane should be removed";

    // Verify: Oscillators should still exist but be unassigned and hidden
    auto updatedOscillators = state.getOscillators();
    EXPECT_EQ(updatedOscillators.size(), initialOscCount) << "Oscillator count should not change";

    for (const auto& oscId : oscillatorsInPane)
    {
        auto it = std::find_if(updatedOscillators.begin(), updatedOscillators.end(),
                               [&oscId](const Oscillator& o) { return o.getId() == oscId; });

        ASSERT_NE(it, updatedOscillators.end()) << "Oscillator should still exist";
        EXPECT_FALSE(it->getPaneId().isValid()) << "Oscillator's paneId should be invalid";
        EXPECT_FALSE(it->isVisible()) << "Oscillator should not be visible";
    }
}

TEST_F(PaneClosingBugTest, ClosingOnePaneDoesNotAffectOthers)
{
    auto& state = processor->getState();
    auto& layoutManager = state.getLayoutManager();

    // Clear existing state
    while (layoutManager.getPaneCount() > 0)
        layoutManager.removePane(layoutManager.getPanes()[0].getId());
    while (!state.getOscillators().empty())
        state.removeOscillator(state.getOscillators()[0].getId());

    pumpMessageQueue(50);

    // Create two panes
    Pane pane1, pane2;
    pane1.setName("Pane 1");
    pane2.setName("Pane 2");
    layoutManager.addPane(pane1);
    layoutManager.addPane(pane2);

    // Create oscillators for each pane
    Oscillator osc1, osc2;
    osc1.setName("Osc 1");
    osc1.setPaneId(pane1.getId());
    osc1.setVisible(true);

    osc2.setName("Osc 2");
    osc2.setPaneId(pane2.getId());
    osc2.setVisible(true);

    state.addOscillator(osc1);
    state.addOscillator(osc2);

    pumpMessageQueue(50);

    // Close pane 1
    for (auto& osc : state.getOscillators())
    {
        if (osc.getPaneId() == pane1.getId())
        {
            Oscillator updated = osc;
            updated.setPaneId(PaneId::invalid());
            updated.setVisible(false);
            state.updateOscillator(updated);
        }
    }
    layoutManager.removePane(pane1.getId());

    pumpMessageQueue(50);

    // Verify pane 2 and its oscillator are unaffected
    EXPECT_EQ(layoutManager.getPaneCount(), 1);
    EXPECT_EQ(layoutManager.getPanes()[0].getId(), pane2.getId());

    auto oscillators = state.getOscillators();
    EXPECT_EQ(oscillators.size(), 2u);

    // Find osc2 and verify it's still assigned and visible
    auto it = std::find_if(oscillators.begin(), oscillators.end(),
                           [&osc2](const Oscillator& o) { return o.getId() == osc2.getId(); });

    ASSERT_NE(it, oscillators.end());
    EXPECT_EQ(it->getPaneId(), pane2.getId()) << "Osc2 should still be assigned to Pane 2";
    EXPECT_TRUE(it->isVisible()) << "Osc2 should still be visible";
}
