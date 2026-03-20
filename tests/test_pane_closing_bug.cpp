/*
    Oscil - Pane Closing Bug Test
    Regression test for issue where closing a pane affected other panes
*/

#include "OscilTestFixtures.h"
#include "ui/controllers/OscillatorPanelController.h"
#include "ui/layout/PaneComponent.h"
#include "ui/layout/PaneContainerComponent.h"
#include "ui/panels/WaveformComponent.h"

using namespace oscil;
using namespace oscil::test;

class PaneClosingBugTest : public OscilPluginTestFixture
{
protected:
    void SetUp() override
    {
        OscilPluginTestFixture::SetUp();
        
        container = std::make_unique<PaneContainerComponent>();
        // Hacky but we need a coordinator.
        // Actually GpuRenderCoordinator needs a real editor.
        // Let's create the editor properly.
        createEditor();
        auto* statusBar = dynamic_cast<StatusBarComponent*>(editor->findChildWithID("statusBar"));
        if (statusBar)
            gpuCoordinator = std::make_unique<GpuRenderCoordinator>(*editor, *statusBar);
    }
    
    // Helper to find a pane component by ID
    PaneComponent* findPaneComponent(const PaneId& paneId)
    {
        // This is tricky because OscillatorPanelController owns them and doesn't expose map easily
        // But we can iterate children of container if we had access.
        // Better: OscillatorPanelController exposes `getPaneComponents()` for testing in my updated code.
        
        // Wait, I can't easily access the controller inside Editor from here without a getter.
        // I added `getPaneComponents` to PluginEditor.
        
        auto& panes = editor->getPaneComponents();
        for (const auto& pane : panes)
        {
            if (pane && pane->getPaneId() == paneId)
                return pane.get();
        }
        return nullptr;
    }

    std::unique_ptr<PaneContainerComponent> container;
    std::unique_ptr<GpuRenderCoordinator> gpuCoordinator;
};

TEST_F(PaneClosingBugTest, ClosingPaneRemovesItAndHidesOscillator)
{
    auto& state = processor->getState();
    auto& layoutManager = state.getLayoutManager();
    
    // Manually remove all panes from layout
    while (layoutManager.getPaneCount() > 0)
    {
        layoutManager.removePane(layoutManager.getPanes()[0].getId());
    }
    
    // Remove all oscillators
    auto oscillators = state.getOscillators();
    for (const auto& osc : oscillators)
    {
        state.removeOscillator(osc.getId());
    }
    
    // Setup: Create 2 panes, each with 1 oscillator
    Pane pane1; pane1.setName("Pane 1"); layoutManager.addPane(pane1);
    Pane pane2; pane2.setName("Pane 2"); layoutManager.addPane(pane2);
    
    Oscillator osc1; osc1.setName("Osc 1"); osc1.setPaneId(pane1.getId()); state.addOscillator(osc1);
    Oscillator osc2; osc2.setName("Osc 2"); osc2.setPaneId(pane2.getId()); state.addOscillator(osc2);
    
    // Refresh
    editor->refreshPanels();
    
    // Verify initial state
    auto& panes = editor->getPaneComponents();
    ASSERT_EQ(panes.size(), 2); // Use ASSERT to stop if failed
    
    // Close Pane 1 via its component callback
    auto* pane1Comp = findPaneComponent(pane1.getId());
    ASSERT_NE(pane1Comp, nullptr);
    
    // Find header
    PaneHeader* header = nullptr;
    for (auto* child : pane1Comp->getChildren())
    {
        if (auto* h = dynamic_cast<PaneHeader*>(child))
        {
            header = h;
            break;
        }
    }
    ASSERT_NE(header, nullptr);
    
    // Find close button in header
    OscilButton* closeButton = nullptr;
    for (auto* child : header->getChildren())
    {
        if (auto* btn = dynamic_cast<OscilButton*>(child))
        {
            // PaneHeader has only one direct OscilButton child (Close button)
            // Other buttons are inside PaneActionBar
            closeButton = btn;
            break;
        }
    }
    ASSERT_NE(closeButton, nullptr);
    
    closeButton->triggerClick();
    
    // Verify
    // Controller should have run.
    // Pane 1 gone. Osc 1 hidden.
    
    EXPECT_EQ(layoutManager.getPaneCount(), 1);
    EXPECT_EQ(state.getOscillators().size(), 2); // Oscillator still exists
    
    auto updatedOsc1 = state.getOscillator(osc1.getId());
    ASSERT_TRUE(updatedOsc1.has_value());
    EXPECT_FALSE(updatedOsc1->isVisible());
    EXPECT_FALSE(updatedOsc1->getPaneId().isValid());
    
    auto updatedOsc2 = state.getOscillator(osc2.getId());
    ASSERT_TRUE(updatedOsc2.has_value());
    EXPECT_TRUE(updatedOsc2->isVisible());
    EXPECT_EQ(updatedOsc2->getPaneId(), pane2.getId());
}

TEST_F(PaneClosingBugTest, ClosingOnePaneDoesNotAffectOthers)
{
    // Similar setup
    auto& state = processor->getState();
    auto& layoutManager = state.getLayoutManager();
    
    while (layoutManager.getPaneCount() > 0)
    {
        layoutManager.removePane(layoutManager.getPanes()[0].getId());
    }
    
    Pane pane1; pane1.setName("Pane 1"); layoutManager.addPane(pane1);
    Pane pane2; pane2.setName("Pane 2"); layoutManager.addPane(pane2);
    Pane pane3; pane3.setName("Pane 3"); layoutManager.addPane(pane3);
    
    Oscillator osc1; osc1.setPaneId(pane1.getId()); state.addOscillator(osc1);
    Oscillator osc2; osc2.setPaneId(pane2.getId()); state.addOscillator(osc2);
    Oscillator osc3; osc3.setPaneId(pane3.getId()); state.addOscillator(osc3);
    
    editor->refreshPanels();
    
    // Close middle pane (Pane 2)
    auto* pane2Comp = findPaneComponent(pane2.getId());
    ASSERT_NE(pane2Comp, nullptr);
    
    PaneHeader* header = nullptr;
    for (auto* child : pane2Comp->getChildren())
    {
        if (auto* h = dynamic_cast<PaneHeader*>(child))
        {
            header = h;
            break;
        }
    }
    ASSERT_NE(header, nullptr);
    
    OscilButton* closeButton = nullptr;
    for (auto* child : header->getChildren())
    {
        if (auto* btn = dynamic_cast<OscilButton*>(child))
        {
            closeButton = btn;
            break;
        }
    }
    ASSERT_NE(closeButton, nullptr);
    
    closeButton->triggerClick();
    
    // Verify
    EXPECT_EQ(layoutManager.getPaneCount(), 2);
    
    // Pane 1 and 3 should remain
    EXPECT_NE(layoutManager.getPane(pane1.getId()), nullptr);
    EXPECT_EQ(layoutManager.getPane(pane2.getId()), nullptr);
    EXPECT_NE(layoutManager.getPane(pane3.getId()), nullptr);
    
    // Osc 2 hidden
    auto updatedOsc2 = state.getOscillator(osc2.getId());
    EXPECT_FALSE(updatedOsc2->isVisible());
    
    // Osc 1 and 3 visible and assigned
    auto updatedOsc1 = state.getOscillator(osc1.getId());
    EXPECT_TRUE(updatedOsc1->isVisible());
    EXPECT_EQ(updatedOsc1->getPaneId(), pane1.getId());
    
    auto updatedOsc3 = state.getOscillator(osc3.getId());
    EXPECT_TRUE(updatedOsc3->isVisible());
    EXPECT_EQ(updatedOsc3->getPaneId(), pane3.getId());
}