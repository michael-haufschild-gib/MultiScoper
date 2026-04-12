/*
    Oscil - Oscillator List Component Tests
    Tests for OscillatorListComponent logic
*/

#include "core/Oscillator.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "ui/components/InlineEditLabel.h"
#include "ui/components/SegmentedButtonBar.h"
#include "ui/panels/OscillatorListComponent.h"
#include "ui/panels/OscillatorListItem.h"
#include "ui/panels/OscillatorListToolbar.h"
#include "ui/theme/ThemeManager.h"

#include "OscilTestFixtures.h"
#include "OscilTestUtils.h"
#include "TestElementRegistry.h"
#include "rendering/ShaderRegistry.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <gtest/gtest.h>

namespace oscil
{

using namespace oscil::test;

namespace
{

// Find the first child component of the given type in parent. Returns nullptr if none.
template <typename T>
T* findChildOfType(juce::Component& parent)
{
    for (int i = 0; i < parent.getNumChildComponents(); ++i)
    {
        if (auto* typed = dynamic_cast<T*>(parent.getChildComponent(i)))
            return typed;
    }
    return nullptr;
}

// Locate the scrolling container inside an OscillatorListComponent by its stable componentID.
juce::Component* findListContainer(OscillatorListComponent& list)
{
    auto* viewport = dynamic_cast<juce::Viewport*>(list.findChildWithID("oscillatorListViewport"));
    return viewport != nullptr ? viewport->getViewedComponent() : nullptr;
}

juce::Viewport* findListViewport(OscillatorListComponent& list)
{
    return dynamic_cast<juce::Viewport*>(list.findChildWithID("oscillatorListViewport"));
}

// Mock listener that records every callback and its arguments.
class RecordingListener : public OscillatorListComponent::Listener
{
public:
    OscillatorId lastSelectedId, lastDeletedId, lastConfigRequestedId, lastColorConfigRequestedId;
    OscillatorId lastPaneSelectionRequestedId, lastNameChangedId, lastVisibilityChangedId, lastModeChangedId;
    juce::String lastNewName;
    bool lastVisible = false;
    ProcessingMode lastMode = ProcessingMode::FullStereo;
    int lastReorderFromIndex = -1, lastReorderToIndex = -1;
    int selectionCount = 0, deleteCount = 0, configRequestedCount = 0, colorConfigRequestedCount = 0;
    int paneSelectionRequestedCount = 0, nameChangedCount = 0, visibilityChangedCount = 0;
    int modeChangedCount = 0, reorderedCount = 0;

    void oscillatorSelected(const OscillatorId& id) override
    {
        lastSelectedId = id;
        ++selectionCount;
    }
    void oscillatorDeleteRequested(const OscillatorId& id) override
    {
        lastDeletedId = id;
        ++deleteCount;
    }
    void oscillatorConfigRequested(const OscillatorId& id) override
    {
        lastConfigRequestedId = id;
        ++configRequestedCount;
    }
    void oscillatorColorConfigRequested(const OscillatorId& id) override
    {
        lastColorConfigRequestedId = id;
        ++colorConfigRequestedCount;
    }
    void oscillatorPaneSelectionRequested(const OscillatorId& id) override
    {
        lastPaneSelectionRequestedId = id;
        ++paneSelectionRequestedCount;
    }
    void oscillatorNameChanged(const OscillatorId& id, const juce::String& newName) override
    {
        lastNameChangedId = id;
        lastNewName = newName;
        ++nameChangedCount;
    }
    void oscillatorVisibilityChanged(const OscillatorId& id, bool visible) override
    {
        lastVisibilityChangedId = id;
        lastVisible = visible;
        ++visibilityChangedCount;
    }
    void oscillatorModeChanged(const OscillatorId& id, ProcessingMode mode) override
    {
        lastModeChangedId = id;
        lastMode = mode;
        ++modeChangedCount;
    }
    void oscillatorsReordered(int fromIndex, int toIndex) override
    {
        lastReorderFromIndex = fromIndex;
        lastReorderToIndex = toIndex;
        ++reorderedCount;
    }
};

} // anonymous namespace

class OscillatorListComponentTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create owned service instances (no singletons)
        themeManager_ = std::make_unique<ThemeManager>();
        shaderRegistry_ = std::make_unique<ShaderRegistry>();
        mockRegistry_ = std::make_unique<MockInstanceRegistry>();
    }

    void TearDown() override
    {
        // Pump to process any pending callbacks
        pumpMessageQueue(50);

        // Destroy services in reverse order
        mockRegistry_.reset();
        shaderRegistry_.reset();
        themeManager_.reset();

        // Final cleanup
        pumpMessageQueue(50);
    }

    IThemeService& getThemeService() { return *themeManager_; }
    IInstanceRegistry& getRegistry() { return *mockRegistry_; }

    // Owned services
    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<ShaderRegistry> shaderRegistry_;
    std::unique_ptr<MockInstanceRegistry> mockRegistry_;
};

TEST_F(OscillatorListComponentTest, ToolbarConstruction)
{
    OscillatorListToolbar toolbar(getThemeService());
    // OscillatorListToolbar registers itself with TestElementRegistry
    EXPECT_EQ(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_toolbar"), &toolbar);
}

TEST_F(OscillatorListComponentTest, Construction)
{
    OscillatorListComponent list(getThemeService(), getRegistry());
    // OscillatorListComponent inherits TestIdSupport and sets testId="oscillatorList"
    EXPECT_EQ(oscil::test::TestElementRegistry::getInstance().findElement("oscillatorList"), &list);
    EXPECT_EQ(list.getDisplayedItemCount(), 0u);
}

TEST_F(OscillatorListComponentTest, RefreshListPopulatesItems)
{
    OscillatorListComponent list(getThemeService(), getRegistry());
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
    auto* item0 = oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_0");
    auto* item1 = oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_1");
    EXPECT_NE(item0, nullptr);
    EXPECT_NE(item1, nullptr);

    // Verify the list's displayed-item count matches the provided data.
    EXPECT_EQ(list.getDisplayedItemCount(), 2u);

    // Verify the scroll container actually holds the item widgets.
    auto* container = findListContainer(list);
    ASSERT_NE(container, nullptr);
    EXPECT_EQ(container->getNumChildComponents(), 2);
}

TEST_F(OscillatorListComponentTest, FilteringVisibility)
{
    OscillatorListComponent list(getThemeService(), getRegistry());

    Oscillator visibleOsc;
    visibleOsc.setName("Visible");
    visibleOsc.setVisible(true);
    visibleOsc.setOrderIndex(0);

    Oscillator hiddenOsc;
    hiddenOsc.setName("Hidden");
    hiddenOsc.setVisible(false);
    hiddenOsc.setOrderIndex(1);

    std::vector<Oscillator> oscillators = {visibleOsc, hiddenOsc};

    // 1. All mode — both items displayed.
    list.filterModeChanged(OscillatorFilterMode::All);
    list.refreshList(oscillators);
    EXPECT_EQ(list.getDisplayedItemCount(), 2u);
    EXPECT_NE(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_0"), nullptr);
    EXPECT_NE(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_1"), nullptr);

    // 2. Visible mode — hidden osc is removed.
    list.filterModeChanged(OscillatorFilterMode::Visible);
    EXPECT_EQ(list.getDisplayedItemCount(), 1u);
    EXPECT_NE(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_0"), nullptr);
    EXPECT_EQ(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_1"), nullptr);

    // 3. Hidden mode — only the hidden osc is shown, re-indexed to position 0.
    list.filterModeChanged(OscillatorFilterMode::Hidden);
    EXPECT_EQ(list.getDisplayedItemCount(), 1u);
    EXPECT_NE(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_0"), nullptr);
    EXPECT_EQ(oscil::test::TestElementRegistry::getInstance().findElement("sidebar_oscillators_item_1"), nullptr);
}

TEST_F(OscillatorListComponentTest, SelectionPropagatesToListener)
{
    OscillatorListComponent list(getThemeService(), getRegistry());
    RecordingListener listener;
    list.addListener(&listener);

    std::vector<Oscillator> oscillators;
    Oscillator osc1;
    oscillators.push_back(osc1);
    list.refreshList(oscillators);

    // Simulate selection via the item-listener interface implemented by the list.
    list.oscillatorSelected(osc1.getId());

    EXPECT_EQ(listener.selectionCount, 1);
    EXPECT_EQ(listener.lastSelectedId, osc1.getId());

    list.removeListener(&listener);
}

TEST_F(OscillatorListComponentTest, DeletionPropagatesToListener)
{
    OscillatorListComponent list(getThemeService(), getRegistry());
    RecordingListener listener;
    list.addListener(&listener);

    std::vector<Oscillator> oscillators;
    Oscillator osc1;
    oscillators.push_back(osc1);
    list.refreshList(oscillators);

    auto oscId = osc1.getId();
    list.oscillatorDeleteRequested(oscId);

    EXPECT_EQ(listener.deleteCount, 1);
    EXPECT_EQ(listener.lastDeletedId, oscId);

    list.removeListener(&listener);
}

TEST_F(OscillatorListComponentTest, ConfigAndColorRequestsPropagate)
{
    OscillatorListComponent list(getThemeService(), getRegistry());
    RecordingListener listener;
    list.addListener(&listener);

    Oscillator osc;
    list.refreshList({osc});

    list.oscillatorConfigRequested(osc.getId());
    list.oscillatorColorConfigRequested(osc.getId());

    EXPECT_EQ(listener.configRequestedCount, 1);
    EXPECT_EQ(listener.lastConfigRequestedId, osc.getId());
    EXPECT_EQ(listener.colorConfigRequestedCount, 1);
    EXPECT_EQ(listener.lastColorConfigRequestedId, osc.getId());

    list.removeListener(&listener);
}

TEST_F(OscillatorListComponentTest, VisibilityAndModeChangesPropagate)
{
    OscillatorListComponent list(getThemeService(), getRegistry());
    RecordingListener listener;
    list.addListener(&listener);

    Oscillator osc;
    list.refreshList({osc});

    list.oscillatorVisibilityChanged(osc.getId(), false);
    list.oscillatorModeChanged(osc.getId(), ProcessingMode::Side);

    EXPECT_EQ(listener.visibilityChangedCount, 1);
    EXPECT_EQ(listener.lastVisibilityChangedId, osc.getId());
    EXPECT_FALSE(listener.lastVisible);

    EXPECT_EQ(listener.modeChangedCount, 1);
    EXPECT_EQ(listener.lastModeChangedId, osc.getId());
    EXPECT_EQ(listener.lastMode, ProcessingMode::Side);

    list.removeListener(&listener);
}

TEST_F(OscillatorListComponentTest, NameChangePropagates)
{
    OscillatorListComponent list(getThemeService(), getRegistry());
    RecordingListener listener;
    list.addListener(&listener);

    Oscillator osc;
    osc.setName("Original");
    list.refreshList({osc});

    list.oscillatorNameChanged(osc.getId(), "Renamed");

    EXPECT_EQ(listener.nameChangedCount, 1);
    EXPECT_EQ(listener.lastNameChangedId, osc.getId());
    EXPECT_EQ(listener.lastNewName, "Renamed");

    list.removeListener(&listener);
}

TEST_F(OscillatorListComponentTest, PaneSelectionRequestPropagates)
{
    OscillatorListComponent list(getThemeService(), getRegistry());
    RecordingListener listener;
    list.addListener(&listener);

    Oscillator osc;
    list.refreshList({osc});

    list.oscillatorPaneSelectionRequested(osc.getId());

    EXPECT_EQ(listener.paneSelectionRequestedCount, 1);
    EXPECT_EQ(listener.lastPaneSelectionRequestedId, osc.getId());

    list.removeListener(&listener);
}

TEST_F(OscillatorListComponentTest, MoveRequestEmitsReorderWithinBounds)
{
    OscillatorListComponent list(getThemeService(), getRegistry());
    RecordingListener listener;
    list.addListener(&listener);

    Oscillator osc0;
    osc0.setOrderIndex(0);
    Oscillator osc1;
    osc1.setOrderIndex(1);
    Oscillator osc2;
    osc2.setOrderIndex(2);

    list.refreshList({osc0, osc1, osc2});

    // Move middle item up — should emit reorder(1, 0).
    list.oscillatorMoveRequested(osc1.getId(), -1);
    EXPECT_EQ(listener.reorderedCount, 1);
    EXPECT_EQ(listener.lastReorderFromIndex, 1);
    EXPECT_EQ(listener.lastReorderToIndex, 0);

    // Move last item down — out of bounds, no emission.
    list.oscillatorMoveRequested(osc2.getId(), 1);
    EXPECT_EQ(listener.reorderedCount, 1);

    // Move first item up — out of bounds, no emission.
    list.oscillatorMoveRequested(osc0.getId(), -1);
    EXPECT_EQ(listener.reorderedCount, 1);

    list.removeListener(&listener);
}

TEST_F(OscillatorListComponentTest, ToolbarCountBadgeReflectsVisibleTotals)
{
    OscillatorListComponent list(getThemeService(), getRegistry());

    Oscillator visible0;
    visible0.setVisible(true);
    visible0.setOrderIndex(0);
    Oscillator hidden1;
    hidden1.setVisible(false);
    hidden1.setOrderIndex(1);
    Oscillator visible2;
    visible2.setVisible(true);
    visible2.setOrderIndex(2);

    list.refreshList({visible0, hidden1, visible2});

    // Locate toolbar as child; it's the first OscillatorListToolbar inside list.
    auto* toolbar = findChildOfType<OscillatorListToolbar>(list);
    ASSERT_NE(toolbar, nullptr);
    EXPECT_EQ(toolbar->getTotalCount(), 3);
    EXPECT_EQ(toolbar->getVisibleCount(), 2);

    // Refresh with an empty set — counts collapse to 0.
    list.refreshList({});
    EXPECT_EQ(toolbar->getTotalCount(), 0);
    EXPECT_EQ(toolbar->getVisibleCount(), 0);
}

TEST_F(OscillatorListComponentTest, EmptyStateHidesViewportWhenNoItems)
{
    OscillatorListComponent list(getThemeService(), getRegistry());
    list.setSize(300, 400);

    auto* viewport = findListViewport(list);
    ASSERT_NE(viewport, nullptr);

    // Empty list — viewport hidden (empty-state label shown).
    list.refreshList({});
    EXPECT_FALSE(viewport->isVisible());
    EXPECT_EQ(list.getDisplayedItemCount(), 0u);

    // Populate — viewport visible.
    Oscillator osc;
    list.refreshList({osc});
    EXPECT_TRUE(viewport->isVisible());
    EXPECT_EQ(list.getDisplayedItemCount(), 1u);

    // Clear again — viewport hidden.
    list.refreshList({});
    EXPECT_FALSE(viewport->isVisible());
}

TEST_F(OscillatorListComponentTest, RefreshReusesExistingItemsForSameIds)
{
    OscillatorListComponent list(getThemeService(), getRegistry());
    list.setSize(300, 400);

    Oscillator osc;
    osc.setName("Original");
    list.refreshList({osc});

    auto* container = findListContainer(list);
    ASSERT_NE(container, nullptr);
    ASSERT_EQ(container->getNumChildComponents(), 1);

    // Capture the item widget identity.
    juce::Component* originalItem = container->getChildComponent(0);
    ASSERT_NE(originalItem, nullptr);

    // Refresh with the same oscillator (same ID) but renamed.
    osc.setName("Renamed");
    list.refreshList({osc});

    container = findListContainer(list);
    ASSERT_NE(container, nullptr);
    ASSERT_EQ(container->getNumChildComponents(), 1);

    // Same widget instance should be reused — not destroyed and recreated.
    EXPECT_EQ(container->getChildComponent(0), originalItem);
}

TEST_F(OscillatorListComponentTest, ItemExpansionUpdatesListLayout)
{
    OscillatorListComponent list(getThemeService(), getRegistry());
    list.setSize(300, 400);

    std::vector<Oscillator> oscillators;
    Oscillator osc1;
    osc1.setName("Osc 1");
    oscillators.push_back(osc1);

    list.refreshList(oscillators);

    auto* container = findListContainer(list);
    ASSERT_NE(container, nullptr);

    // Initial height should be COMPACT_HEIGHT (56)
    EXPECT_EQ(container->getHeight(), OscillatorListItemComponent::COMPACT_HEIGHT);

    // Select item
    list.oscillatorSelected(osc1.getId());

    // Height should now be EXPANDED_HEIGHT (100)
    EXPECT_EQ(container->getHeight(), OscillatorListItemComponent::EXPANDED_HEIGHT);
}

} // namespace oscil
