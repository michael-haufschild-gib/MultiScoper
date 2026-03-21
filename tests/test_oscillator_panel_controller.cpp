/*
    Oscil - Oscillator Panel Controller Tests
*/

#include <gtest/gtest.h>
#include "ui/controllers/OscillatorPanelController.h"
#include "OscilTestFixtures.h"
#include "ui/managers/DisplaySettingsManager.h"
#include "rendering/GpuRenderCoordinator.h"
#include "rendering/PresetManager.h"

namespace oscil
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
        : processor_(instanceRegistry_, themeService_, shaderRegistry_, presetManager_, memoryBudgetManager_)
        , serviceContext_{ instanceRegistry_, themeService_, shaderRegistry_, presetManager_ }
    {
    }

    void SetUp() override
    {
        // Disable GPU
        processor_.getState().setGpuRenderingEnabled(false);

        // Initialize minimal editor
        editor_ = std::make_unique<TestEditor>(processor_);
        
        container_ = std::make_unique<PaneContainerComponent>();
        displaySettings_ = std::make_unique<DisplaySettingsManager>([this]() {
            std::vector<PaneComponent*> snapshot;
            for (auto& pane : paneComponents_)
                snapshot.push_back(pane.get());
            return snapshot;
        });
        gpuCoordinator_ = std::make_unique<GpuRenderCoordinator>(*editor_, statusBar_);
        
        // Create controller
        controller_ = std::make_unique<OscillatorPanelController>(
            processor_, serviceContext_, *container_, *gpuCoordinator_
        );
        
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
    oscil::test::MockInstanceRegistry instanceRegistry_;
    oscil::test::MockThemeService themeService_;
    ShaderRegistry shaderRegistry_;
    PresetManager presetManager_;
    MemoryBudgetManager memoryBudgetManager_;
    OscilPluginProcessor processor_;
    ServiceContext serviceContext_;
    
    // View Components
    StatusBarComponent statusBar_{ themeService_ };
    
    // Deferred Components
    std::unique_ptr<TestEditor> editor_;
    std::unique_ptr<PaneContainerComponent> container_;
    
    std::vector<std::unique_ptr<PaneComponent>> paneComponents_; // Dummy for displaySettings
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

} // namespace oscil
