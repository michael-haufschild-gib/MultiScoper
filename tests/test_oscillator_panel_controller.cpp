/*
    MultiScoper - Oscillator Panel Controller Tests
*/

#include "ui/controllers/GpuRenderCoordinator.h"
#include "ui/controllers/OscillatorPanelController.h"
#include "ui/managers/DisplaySettingsManager.h"

#include "MultiScoperTestFixtures.h"
#include "rendering/PresetManager.h"

#include <gtest/gtest.h>

namespace multiscoper
{

// Minimal editor for testing
class TestEditor : public juce::AudioProcessorEditor
{
public:
    TestEditor(juce::AudioProcessor& p) : AudioProcessorEditor(p) {}
};

class OscillatorPanelControllerTest : public ::testing::Test
{
protected:
    OscillatorPanelControllerTest()
        : processor_(PluginProcessorConfig{.instanceRegistry = instanceRegistry_,
                                           .themeService = themeService_,
                                           .shaderRegistry = shaderRegistry_,
                                           .presetManager = presetManager_,
                                           .memoryBudgetManager = memoryBudgetManager_})
        , serviceContext_{instanceRegistry_, themeService_, shaderRegistry_, presetManager_}
    {
    }

    void SetUp() override
    {
        // Disable GPU
        processor_.getState().setGpuRenderingEnabled(false);

        // Initialize minimal editor
        editor_ = std::make_unique<TestEditor>(processor_);

        container_ = std::make_unique<PaneContainerComponent>(themeService_);
        displaySettings_ = std::make_unique<DisplaySettingsManager>([this]() {
            std::vector<PaneComponent*> snapshot;
            if (controller_)
            {
                for (auto& pane : controller_->getPaneComponents())
                    snapshot.push_back(pane.get());
            }
            return snapshot;
        });
        gpuCoordinator_ = std::make_unique<GpuRenderCoordinator>(*editor_, statusBar_);

        // Create controller
        controller_ =
            std::make_unique<OscillatorPanelController>(processor_, serviceContext_, *container_, *gpuCoordinator_);

        controller_->initialize(nullptr, nullptr, displaySettings_.get());
    }

    void TearDown() override
    {
        controller_.reset();
        gpuCoordinator_.reset();
        displaySettings_.reset();
        container_.reset();
        editor_.reset();
    }

    // Mocks & Dependencies
    multiscoper::test::MockInstanceRegistry instanceRegistry_;
    multiscoper::test::MockThemeService themeService_;
    ShaderRegistry shaderRegistry_;
    PresetManager presetManager_;
    MemoryBudgetManager memoryBudgetManager_;
    MultiScoperPluginProcessor processor_;
    ServiceContext serviceContext_;

    // View Components
    StatusBarComponent statusBar_{themeService_};

    // Deferred Components
    std::unique_ptr<TestEditor> editor_;
    std::unique_ptr<PaneContainerComponent> container_;

    std::unique_ptr<DisplaySettingsManager> displaySettings_;
    std::unique_ptr<GpuRenderCoordinator> gpuCoordinator_;

    std::unique_ptr<OscillatorPanelController> controller_;
};

TEST_F(OscillatorPanelControllerTest, InitializesSuccessfully)
{
    EXPECT_TRUE(controller_->getPaneComponents().empty());
}

TEST_F(OscillatorPanelControllerTest, CreateDefaultOscillatorAddsPane)
{
    auto& state = processor_.getState();

    // Ensure empty state
    state.getOscillators().clear();
    state.getLayoutManager().clear();

    controller_->createDefaultOscillatorIfNeeded();

    // Should have added to State
    EXPECT_FALSE(state.getOscillators().empty());
    EXPECT_GT(state.getLayoutManager().getPaneCount(), 0);
}

TEST_F(OscillatorPanelControllerTest, RefreshPanelsCreatesComponents)
{
    auto& state = processor_.getState();

    // Add some state manually
    Pane pane;
    pane.setName("Test Pane");
    state.getLayoutManager().addPane(pane);

    Oscillator osc;
    osc.setPaneId(pane.getId());
    state.addOscillator(osc);

    // Force refresh
    controller_->refreshPanels();

    // Check components created
    EXPECT_EQ(controller_->getPaneComponents().size(), 1);
}

TEST_F(OscillatorPanelControllerTest, RefreshPanelsProcessesQueuedRefreshRequestedDuringUpdate)
{
    auto& state = processor_.getState();

    Pane initialPane;
    initialPane.setName("Initial Pane");
    state.getLayoutManager().addPane(initialPane);

    Oscillator initialOscillator;
    initialOscillator.setPaneId(initialPane.getId());
    state.addOscillator(initialOscillator);

    bool queuedRefreshRequested = false;
    controller_->setLayoutNeededCallback([this, &state, &queuedRefreshRequested]() {
        if (queuedRefreshRequested)
            return;

        queuedRefreshRequested = true;

        Pane queuedPane;
        queuedPane.setName("Queued Pane");
        state.getLayoutManager().addPane(queuedPane);

        Oscillator queuedOscillator;
        queuedOscillator.setPaneId(queuedPane.getId());
        state.addOscillator(queuedOscillator);

        controller_->refreshPanels();
    });

    controller_->refreshPanels();

    EXPECT_EQ(controller_->getPaneComponents().size(), 2);
}

TEST_F(OscillatorPanelControllerTest, ClosingPaneRemovesIt)
{
    auto& state = processor_.getState();

    // Setup one pane
    Pane pane;
    state.getLayoutManager().addPane(pane);
    controller_->refreshPanels();
    ASSERT_EQ(controller_->getPaneComponents().size(), 1);

    // Close it
    controller_->handlePaneClose(pane.getId());

    // Verify
    EXPECT_EQ(state.getLayoutManager().getPaneCount(), 0);
    EXPECT_EQ(controller_->getPaneComponents().size(), 0);
}

TEST_F(OscillatorPanelControllerTest, ReorderPaneUpdatesLayout)
{
    auto& state = processor_.getState();
    auto& layout = state.getLayoutManager();

    // Create 2 panes in same column
    Pane p1, p2;
    layout.addPane(p1);
    layout.addPane(p2);

    // Initial state: p1=0, p2=1
    EXPECT_EQ(layout.getPane(p1.getId())->getOrderIndex(), 0);
    EXPECT_EQ(layout.getPane(p2.getId())->getOrderIndex(), 1);

    // Swap them
    controller_->handlePaneReordered(p1.getId(), p2.getId());

    // Verify swap in state (p1 should move to p2's index)
    EXPECT_EQ(layout.getPane(p1.getId())->getOrderIndex(), 1);
    EXPECT_EQ(layout.getPane(p2.getId())->getOrderIndex(), 0);
}

TEST_F(OscillatorPanelControllerTest, OscillatorNameChangedPersistsToState)
{
    auto& state = processor_.getState();

    Pane pane;
    state.getLayoutManager().addPane(pane);

    Oscillator osc;
    osc.setName("Old Name");
    osc.setPaneId(pane.getId());
    state.addOscillator(osc);

    // Trigger the SidebarComponent::Listener override on the controller.
    controller_->oscillatorNameChanged(osc.getId(), "New Name");

    // The controller should have written the new name back to state.
    auto updated = state.getOscillators();
    auto it =
        std::find_if(updated.begin(), updated.end(), [&](const Oscillator& o) { return o.getId() == osc.getId(); });
    ASSERT_NE(it, updated.end());
    EXPECT_EQ(it->getName(), "New Name");
}

TEST_F(OscillatorPanelControllerTest, OscillatorNameChangedRejectsEmptyName)
{
    auto& state = processor_.getState();

    Pane pane;
    state.getLayoutManager().addPane(pane);

    Oscillator osc;
    osc.setName("Keep Me");
    osc.setPaneId(pane.getId());
    state.addOscillator(osc);

    controller_->oscillatorNameChanged(osc.getId(), "");

    auto updated = state.getOscillators();
    auto it =
        std::find_if(updated.begin(), updated.end(), [&](const Oscillator& o) { return o.getId() == osc.getId(); });
    ASSERT_NE(it, updated.end());
    EXPECT_EQ(it->getName(), "Keep Me"); // unchanged
}

TEST_F(OscillatorPanelControllerTest, OscillatorNameChangedIgnoresUnknownId)
{
    // Calling with an unknown id should not crash or mutate state.
    auto& state = processor_.getState();
    const auto oscCountBefore = state.getOscillators().size();

    controller_->oscillatorNameChanged(OscillatorId::generate(), "Ghost");

    EXPECT_EQ(state.getOscillators().size(), oscCountBefore);
}

TEST_F(OscillatorPanelControllerTest, OscillatorsReorderedPersistsNewOrderToState)
{
    // Drag-to-reorder regression: this override must forward to state so
    // that dragging in the sidebar actually updates the stored order.
    auto& state = processor_.getState();
    state.getOscillators().clear();
    state.getLayoutManager().clear();

    Pane pane;
    state.getLayoutManager().addPane(pane);

    OscillatorId ids[3];
    for (int i = 0; i < 3; ++i)
    {
        Oscillator osc;
        osc.setName("Osc_" + juce::String(i));
        osc.setOrderIndex(i);
        osc.setPaneId(pane.getId());
        state.addOscillator(osc);
        ids[i] = osc.getId();
    }

    // Move the first oscillator to the end.
    controller_->oscillatorsReordered(0, 2);

    auto oscillators = state.getOscillators();
    ASSERT_EQ(oscillators.size(), 3u);

    // After state.reorderOscillators(0, 2), Osc_0 should now have the
    // highest orderIndex and the other two should shift up.
    const auto findOrderFor = [&](const OscillatorId& id) {
        for (const auto& o : oscillators)
            if (o.getId() == id)
                return o.getOrderIndex();
        return -1;
    };

    EXPECT_EQ(findOrderFor(ids[0]), 2);
    EXPECT_EQ(findOrderFor(ids[1]), 0);
    EXPECT_EQ(findOrderFor(ids[2]), 1);
}

} // namespace multiscoper
