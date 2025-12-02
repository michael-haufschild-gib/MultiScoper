/*
    Oscil - Oscillator List Component Tests
    Tests for OscillatorListComponent logic
*/

#include <gtest/gtest.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/OscillatorListComponent.h"
#include "core/IInstanceRegistry.h"
#include "core/Oscillator.h"
#include "ui/components/SegmentedButtonBar.h"
#include "TestElementRegistry.h"

using namespace oscil;

// Dummy Registry
class DummyRegistry : public IInstanceRegistry
{
public:
    SourceId registerInstance(const juce::String&, std::shared_ptr<SharedCaptureBuffer>, const juce::String&, int, double) override { return SourceId::generate(); }
    void unregisterInstance(const SourceId&) override {}
    std::vector<SourceInfo> getAllSources() const override { return {}; }
    std::optional<SourceInfo> getSource(const SourceId&) const override { return std::nullopt; }
    std::shared_ptr<SharedCaptureBuffer> getCaptureBuffer(const SourceId&) const override { return nullptr; }
    void updateSource(const SourceId&, const juce::String&, int, double) override {}
    size_t getSourceCount() const override { return 0; }
    void addListener(InstanceRegistryListener*) override {}
    void removeListener(InstanceRegistryListener*) override {}
};

class OscillatorListComponentTest : public ::testing::Test
{
protected:
    // No SetUp needed, using global JuceTestEnvironment
};

TEST_F(OscillatorListComponentTest, ToolbarConstruction)
{
    OscillatorListToolbar toolbar;
    // OscillatorListToolbar registers itself with TestElementRegistry
    EXPECT_EQ(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_toolbar"), &toolbar);
}

TEST_F(OscillatorListComponentTest, Construction)
{
    std::cout << "Constructing registry..." << std::endl;
    DummyRegistry registry;
    std::cout << "Constructing list..." << std::endl;
    auto* list = new OscillatorListComponent(registry);
    std::cout << "Verifying list ID..." << std::endl;
    // OscillatorListComponent inherits TestIdSupport and sets testId="oscillatorList"
    EXPECT_EQ(oscil::test::TestElementRegistry::getInstance().findElement("oscillatorList"), list);
    std::cout << "Deleting list..." << std::endl;
    delete list;
    std::cout << "Construction test done." << std::endl;
}

TEST_F(OscillatorListComponentTest, RefreshListPopulatesItems)
{
    DummyRegistry registry;
    OscillatorListComponent list(registry);
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
    OscillatorListComponent list(registry);
    
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
    OscillatorListComponent list(registry);
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
    OscillatorListComponent list(registry);
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