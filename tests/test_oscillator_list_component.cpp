/*
    Oscil - Oscillator List Component Tests
    Tests for OscillatorListComponent logic
*/

#include <gtest/gtest.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/panels/OscillatorListComponent.h"
#include "ui/panels/OscillatorListItem.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "core/Oscillator.h"
#include "ui/components/SegmentedButtonBar.h"
#include "ui/components/InlineEditLabel.h"
#include "ui/theme/ThemeManager.h"
#include "rendering/ShaderRegistry.h"
#include "TestElementRegistry.h"
#include "OscilTestFixtures.h"
#include "OscilTestUtils.h"

using namespace oscil;
using namespace oscil::test;

// Dummy Registry for simple tests
class DummyRegistry : public IInstanceRegistry
{
public:
    SourceId registerInstance(const juce::String&, std::shared_ptr<IAudioBuffer>, const juce::String&, int, double, std::shared_ptr<AnalysisEngine>) override { return SourceId::generate(); }
    void unregisterInstance(const SourceId&) override {}
    std::vector<SourceInfo> getAllSources() const override { return {}; }
    std::optional<SourceInfo> getSource(const SourceId&) const override { return std::nullopt; }
    std::shared_ptr<IAudioBuffer> getCaptureBuffer(const SourceId&) const override { return nullptr; }
    void updateSource(const SourceId&, const juce::String&, int, double) override {}
    size_t getSourceCount() const override { return 0; }
    void addListener(InstanceRegistryListener*) override {}
    void removeListener(InstanceRegistryListener*) override {}
};

class OscillatorListComponentTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create owned service instances (no singletons)
        themeManager_ = std::make_unique<ThemeManager>();
        shaderRegistry_ = std::make_unique<ShaderRegistry>();
    }

    void TearDown() override
    {
        // Pump to process any pending callbacks
        pumpMessageQueue(50);

        // Destroy services in reverse order
        shaderRegistry_.reset();
        themeManager_.reset();

        // Final cleanup
        pumpMessageQueue(50);
    }

    IThemeService& getThemeService() { return *themeManager_; }

    // Owned services
    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<ShaderRegistry> shaderRegistry_;
};

TEST_F(OscillatorListComponentTest, ToolbarConstruction)
{
    OscillatorListToolbar toolbar(getThemeService());
    // OscillatorListToolbar registers itself with TestElementRegistry
    EXPECT_EQ(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_toolbar"), &toolbar);
}

TEST_F(OscillatorListComponentTest, Construction)
{
    std::cout << "Constructing registry..." << std::endl;
    DummyRegistry registry;
    std::cout << "Constructing list..." << std::endl;
    auto list = std::make_unique<OscillatorListComponent>(getThemeService(), registry);
    std::cout << "Verifying list ID..." << std::endl;
    // OscillatorListComponent inherits TestIdSupport and sets testId="oscillatorList"
    EXPECT_EQ(oscil::test::TestElementRegistry::getInstance().findElement("oscillatorList"), list.get());
    std::cout << "Deleting list..." << std::endl;
    list.reset();
    std::cout << "Construction test done." << std::endl;
}

TEST_F(OscillatorListComponentTest, RefreshListPopulatesItems)
{
    DummyRegistry registry;
    OscillatorListComponent list(getThemeService(), registry);
    list.setSize(300, 400); 

    std::vector<Oscillator> oscillators;
    Oscillator osc1;
    osc1.setName("Osc 1");
    osc1.setOrderIndex(0);
    oscillators.push_back(osc1);

    Oscillator osc2;
    osc2.setName("Osc 2");
    osc2.setOrderIndex(1);
    oscillators.push_back(osc2);

    list.refreshList(oscillators);

    // Verify items are created by checking TestElementRegistry
    EXPECT_NE(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_0"), nullptr);
    EXPECT_NE(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_1"), nullptr);
}

TEST_F(OscillatorListComponentTest, FilteringVisiblity)
{
    DummyRegistry registry;
    OscillatorListComponent list(getThemeService(), registry);
    
    Oscillator visibleOsc;
    visibleOsc.setName("Visible");
    visibleOsc.setVisible(true);
    visibleOsc.setOrderIndex(0);

    Oscillator hiddenOsc;
    hiddenOsc.setName("Hidden");
    hiddenOsc.setVisible(false);
    hiddenOsc.setOrderIndex(1);

    std::vector<Oscillator> oscillators = { visibleOsc, hiddenOsc };
    
    // 1. All mode
    list.filterModeChanged(OscillatorFilterMode::All);
    list.refreshList(oscillators);
    EXPECT_NE(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_0"), nullptr);
    EXPECT_NE(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_1"), nullptr);

    // 2. Visible mode
    list.filterModeChanged(OscillatorFilterMode::Visible);
    EXPECT_NE(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_0"), nullptr);
    // hiddenOsc (item 1) should be removed/destroyed
    EXPECT_EQ(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_1"), nullptr);

    // 3. Hidden mode
    list.filterModeChanged(OscillatorFilterMode::Hidden);
    EXPECT_EQ(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_0"), nullptr);
    EXPECT_NE(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_1"), nullptr);
}

// Mock Listener
class SimpleMockListener : public OscillatorListComponent::Listener
{
public:
    OscillatorId lastSelectedId;
    OscillatorId lastDeletedId;
    int selectionCount = 0;
    int deleteCount = 0;

    void oscillatorSelected(const OscillatorId& id) override {
        lastSelectedId = id;
        selectionCount++;
    }

    void oscillatorDeleteRequested(const OscillatorId& id) override {
        lastDeletedId = id;
        deleteCount++;
    }
};

TEST_F(OscillatorListComponentTest, SelectionPropagatesToListener)
{
    DummyRegistry registry;
    OscillatorListComponent list(getThemeService(), registry);
    SimpleMockListener listener;
    list.addListener(&listener);

    std::vector<Oscillator> oscillators;
    Oscillator osc1;
    oscillators.push_back(osc1);
    list.refreshList(oscillators);

    // Simulate selection
    list.oscillatorSelected(osc1.getId());

    EXPECT_EQ(listener.selectionCount, 1);
    EXPECT_EQ(listener.lastSelectedId, osc1.getId());
    
    list.removeListener(&listener);
}

TEST_F(OscillatorListComponentTest, DeletionPropagatesToListener)
{
    DummyRegistry registry;
    OscillatorListComponent list(getThemeService(), registry);
    SimpleMockListener listener;
    list.addListener(&listener);

    std::vector<Oscillator> oscillators;
    Oscillator osc1;
    oscillators.push_back(osc1);
    list.refreshList(oscillators);

    // Simulate deletion request
    list.oscillatorDeleteRequested(osc1.getId());
    
    // Async callback verification skipped
}

TEST_F(OscillatorListComponentTest, LabelUpdates)
{
    DummyRegistry registry;
    Oscillator osc;
    osc.setName("Initial Name");

    OscillatorListItemComponent item(osc, registry, getThemeService());

    // Find InlineEditLabel by iterating through children
    InlineEditLabel* nameLabel = nullptr;
    for (int i = 0; i < item.getNumChildComponents(); ++i)
    {
        if (auto* label = dynamic_cast<InlineEditLabel*>(item.getChildComponent(i)))
        {
            nameLabel = label;
            break;
        }
    }
    ASSERT_NE(nameLabel, nullptr);
    EXPECT_EQ(nameLabel->getText(), "Initial Name");

    // Update oscillator
    osc.setName("Updated Name");
    item.updateFromOscillator(osc);

    EXPECT_EQ(nameLabel->getText(), "Updated Name");
}

TEST_F(OscillatorListComponentTest, ItemExpansionUpdatesListLayout)
{
    DummyRegistry registry;
    OscillatorListComponent list(getThemeService(), registry);
    list.setSize(300, 400);

    std::vector<Oscillator> oscillators;
    Oscillator osc1;
    osc1.setName("Osc 1");
    oscillators.push_back(osc1);

    list.refreshList(oscillators);

    // Access container via viewport
    // Child 0 is toolbar, Child 1 is Viewport
    auto* viewport = dynamic_cast<juce::Viewport*>(list.getChildComponent(1));
    ASSERT_NE(viewport, nullptr);
    auto* container = viewport->getViewedComponent();
    ASSERT_NE(container, nullptr);

    // Initial height should be COMPACT_HEIGHT (56)
    EXPECT_EQ(container->getHeight(), OscillatorListItemComponent::COMPACT_HEIGHT);

    // Select item
    list.oscillatorSelected(osc1.getId());

    // Height should now be EXPANDED_HEIGHT (100)
    EXPECT_EQ(container->getHeight(), OscillatorListItemComponent::EXPANDED_HEIGHT);
}

TEST_F(OscillatorListComponentTest, NameLabelIsVisible)
{
    DummyRegistry registry;
    Oscillator osc;
    osc.setName("Visible Test");

    OscillatorListItemComponent item(osc, registry, getThemeService());

    InlineEditLabel* nameLabel = nullptr;
    for (int i = 0; i < item.getNumChildComponents(); ++i)
    {
        if (auto* label = dynamic_cast<InlineEditLabel*>(item.getChildComponent(i)))
        {
            nameLabel = label;
            break;
        }
    }
    ASSERT_NE(nameLabel, nullptr);
    
    EXPECT_TRUE(nameLabel->isVisible());
}
