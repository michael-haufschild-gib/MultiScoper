/*
    Oscil - Pane Layout Tests (Edge Cases)
    Listeners, serialization, invalid inputs, stress tests
*/

#include "core/Pane.h"

#include <gtest/gtest.h>

using namespace oscil;

class PaneLayoutManagerEdgeTest : public ::testing::Test
{
protected:
    PaneLayoutManager manager;

    void SetUp() override {}
};

// Test: Listener notifications
class TestPaneListener : public PaneLayoutManager::Listener
{
public:
    int columnLayoutChangedCount = 0;
    int paneOrderChangedCount = 0;
    int paneAddedCount = 0;
    int paneRemovedCount = 0;

    void columnLayoutChanged(ColumnLayout) override { ++columnLayoutChangedCount; }
    void paneOrderChanged() override { ++paneOrderChangedCount; }
    void paneAdded(const PaneId&) override { ++paneAddedCount; }
    void paneRemoved(const PaneId&) override { ++paneRemovedCount; }
};

TEST_F(PaneLayoutManagerEdgeTest, ListenerNotifications)
{
    TestPaneListener listener;
    manager.addListener(&listener);

    // Add pane
    Pane pane;
    manager.addPane(pane);
    EXPECT_EQ(listener.paneAddedCount, 1);

    // Change layout
    manager.setColumnLayout(ColumnLayout::Double);
    EXPECT_EQ(listener.columnLayoutChangedCount, 1);

    // Move pane
    manager.movePane(pane.getId(), 0);
    EXPECT_GE(listener.paneOrderChangedCount, 1);

    // Remove pane
    manager.removePane(pane.getId());
    EXPECT_EQ(listener.paneRemovedCount, 1);

    manager.removeListener(&listener);
}

TEST_F(PaneLayoutManagerEdgeTest, RemoveListenerStopsNotifications)
{
    TestPaneListener listener;
    manager.addListener(&listener);
    manager.removeListener(&listener);

    Pane pane;
    manager.addPane(pane);

    EXPECT_EQ(listener.paneAddedCount, 0);
}

// Test: Serialization with empty panes list
TEST_F(PaneLayoutManagerEdgeTest, SerializeEmptyPanes)
{
    auto valueTree = manager.toValueTree();

    EXPECT_TRUE(valueTree.isValid());
    EXPECT_EQ(valueTree.getNumChildren(), 0);
}

TEST_F(PaneLayoutManagerEdgeTest, DeserializeEmptyPanes)
{
    juce::ValueTree emptyPanes("Panes");
    emptyPanes.setProperty("columnLayout", 2, nullptr);

    manager.fromValueTree(emptyPanes);

    EXPECT_EQ(manager.getPaneCount(), 0);
    EXPECT_EQ(manager.getColumnLayout(), ColumnLayout::Double);
}

TEST_F(PaneLayoutManagerEdgeTest, DeserializeNegativeColumnIndexIsClampedToZero)
{
    juce::ValueTree panes("Panes");
    panes.setProperty("columnLayout", 2, nullptr);

    juce::ValueTree pane("Pane");
    pane.setProperty("id", PaneId::generate().id, nullptr);
    pane.setProperty("orderIndex", 0, nullptr);
    pane.setProperty("columnIndex", -3, nullptr);
    panes.appendChild(pane, nullptr);

    manager.fromValueTree(panes);

    ASSERT_EQ(manager.getPaneCount(), 1);
    const auto& loaded = manager.getPanes()[0];
    EXPECT_EQ(loaded.getColumnIndex(), 0);
    EXPECT_EQ(manager.getPaneCountInColumn(0), 1);
}

// Test: Serialization round-trip
TEST_F(PaneLayoutManagerEdgeTest, SerializationRoundTrip)
{
    manager.setColumnLayout(ColumnLayout::Triple);

    for (int i = 0; i < 5; ++i)
    {
        Pane pane;
        pane.setName(juce::String("Pane ") + juce::String(i));
        pane.setHeightRatio(0.5f);
        manager.addPane(pane);
    }

    auto valueTree = manager.toValueTree();

    PaneLayoutManager restored;
    restored.fromValueTree(valueTree);

    EXPECT_EQ(restored.getColumnLayout(), ColumnLayout::Triple);
    EXPECT_EQ(restored.getPaneCount(), 5);
}

// Test: movePaneToColumn with negative column
TEST_F(PaneLayoutManagerEdgeTest, MovePaneToNegativeColumn)
{
    manager.setColumnLayout(ColumnLayout::Double);

    Pane pane;
    manager.addPane(pane);

    manager.movePaneToColumn(pane.getId(), -5, 0);

    // Should be clamped to column 0
    const Pane* movedPane = manager.getPane(pane.getId());
    EXPECT_GE(movedPane->getColumnIndex(), 0);
}

// Test: Get pane bounds in column with invalid pane
TEST_F(PaneLayoutManagerEdgeTest, GetPaneBoundsInColumnInvalidPane)
{
    Pane pane;
    manager.addPane(pane);

    juce::Rectangle<int> columnArea(0, 0, 400, 600);
    auto bounds = manager.getPaneBoundsInColumn(PaneId::generate(), columnArea);

    EXPECT_TRUE(bounds.isEmpty());
}

// Test: Column count for different layouts
TEST_F(PaneLayoutManagerEdgeTest, ColumnCountForLayouts)
{
    manager.setColumnLayout(ColumnLayout::Single);
    EXPECT_EQ(manager.getColumnCount(), 1);

    manager.setColumnLayout(ColumnLayout::Double);
    EXPECT_EQ(manager.getColumnCount(), 2);

    manager.setColumnLayout(ColumnLayout::Triple);
    EXPECT_EQ(manager.getColumnCount(), 3);
}

// Test: getColumnForPane with out of range index
TEST_F(PaneLayoutManagerEdgeTest, GetColumnForPaneOutOfRange)
{
    manager.setColumnLayout(ColumnLayout::Triple);

    // With no panes, should use fallback row-major
    EXPECT_EQ(manager.getColumnForPane(0), 0);
    EXPECT_EQ(manager.getColumnForPane(1), 1);
    EXPECT_EQ(manager.getColumnForPane(2), 2);
    EXPECT_EQ(manager.getColumnForPane(3), 0);
}
