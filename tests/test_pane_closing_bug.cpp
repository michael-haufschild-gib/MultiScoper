/*
    Oscil - Pane Closing Bug Test
    Regression test for issue where closing a pane affected other panes
*/

#include "OscilTestFixtures.h"
#include "ui/controllers/GpuRenderCoordinator.h"
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

    // Clear all panes and oscillators from state
    void clearAllPanesAndOscillators()
    {
        auto& state = processor->getState();
        auto& layoutManager = state.getLayoutManager();

        while (layoutManager.getPaneCount() > 0)
            layoutManager.removePane(layoutManager.getPanes()[0].getId());

        auto oscillators = state.getOscillators();
        for (const auto& osc : oscillators)
            state.removeOscillator(osc.getId());
    }

    // Find the close button for a pane (the direct OscilButton child of PaneHeader)
    OscilButton* findCloseButton(const PaneId& paneId)
    {
        auto* paneComp = findPaneComponent(paneId);
        if (paneComp == nullptr)
            return nullptr;

        PaneHeader* header = nullptr;
        for (auto* child : paneComp->getChildren())
        {
            if (auto* h = dynamic_cast<PaneHeader*>(child))
            {
                header = h;
                break;
            }
        }
        if (header == nullptr)
            return nullptr;

        for (auto* child : header->getChildren())
        {
            if (auto* btn = dynamic_cast<OscilButton*>(child))
                return btn;
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

    clearAllPanesAndOscillators();

    // Setup: Create 2 panes, each with 1 oscillator
    Pane pane1; pane1.setName("Pane 1"); layoutManager.addPane(pane1);
    Pane pane2; pane2.setName("Pane 2"); layoutManager.addPane(pane2);

    Oscillator osc1; osc1.setName("Osc 1"); osc1.setPaneId(pane1.getId()); state.addOscillator(osc1);
    Oscillator osc2; osc2.setName("Osc 2"); osc2.setPaneId(pane2.getId()); state.addOscillator(osc2);

    editor->refreshPanels();
    ASSERT_EQ(editor->getPaneComponents().size(), 2);

    auto* closeBtn = findCloseButton(pane1.getId());
    ASSERT_NE(closeBtn, nullptr);
    closeBtn->triggerClick();

    // Pane 1 removed, Osc 1 hidden, Osc 2 unaffected
    EXPECT_EQ(layoutManager.getPaneCount(), 1);
    EXPECT_EQ(state.getOscillators().size(), 2);

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
    auto& state = processor->getState();
    auto& layoutManager = state.getLayoutManager();

    clearAllPanesAndOscillators();

    Pane pane1; pane1.setName("Pane 1"); layoutManager.addPane(pane1);
    Pane pane2; pane2.setName("Pane 2"); layoutManager.addPane(pane2);
    Pane pane3; pane3.setName("Pane 3"); layoutManager.addPane(pane3);

    Oscillator osc1; osc1.setPaneId(pane1.getId()); state.addOscillator(osc1);
    Oscillator osc2; osc2.setPaneId(pane2.getId()); state.addOscillator(osc2);
    Oscillator osc3; osc3.setPaneId(pane3.getId()); state.addOscillator(osc3);

    editor->refreshPanels();

    auto* closeBtn = findCloseButton(pane2.getId());
    ASSERT_NE(closeBtn, nullptr);
    closeBtn->triggerClick();

    EXPECT_EQ(layoutManager.getPaneCount(), 2);
    EXPECT_NE(layoutManager.getPane(pane1.getId()), nullptr);
    EXPECT_EQ(layoutManager.getPane(pane2.getId()), nullptr);
    EXPECT_NE(layoutManager.getPane(pane3.getId()), nullptr);

    EXPECT_FALSE(state.getOscillator(osc2.getId())->isVisible());

    auto updatedOsc1 = state.getOscillator(osc1.getId());
    EXPECT_TRUE(updatedOsc1->isVisible());
    EXPECT_EQ(updatedOsc1->getPaneId(), pane1.getId());

    auto updatedOsc3 = state.getOscillator(osc3.getId());
    EXPECT_TRUE(updatedOsc3->isVisible());
    EXPECT_EQ(updatedOsc3->getPaneId(), pane3.getId());
}