/*
    Oscil - Pane Layout Tests
*/

#include "core/Pane.h"

#include <gtest/gtest.h>

using namespace oscil;

class PaneLayoutManagerTest : public ::testing::Test
{
protected:
    PaneLayoutManager manager;

    void SetUp() override {}
};

// Test: Add and remove panes
TEST_F(PaneLayoutManagerTest, AddRemovePanes)
{
    EXPECT_EQ(manager.getPaneCount(), 0);

    Pane pane1;
    pane1.setOrderIndex(0);
    manager.addPane(pane1);
    EXPECT_EQ(manager.getPaneCount(), 1);

    Pane pane2;
    pane2.setOrderIndex(1);
    manager.addPane(pane2);
    EXPECT_EQ(manager.getPaneCount(), 2);

    manager.removePane(pane1.getId());
    EXPECT_EQ(manager.getPaneCount(), 1);
}

// Test: Column layout
TEST_F(PaneLayoutManagerTest, ColumnLayout)
{
    EXPECT_EQ(manager.getColumnLayout(), ColumnLayout::Single);
    EXPECT_EQ(manager.getColumnCount(), 1);

    manager.setColumnLayout(ColumnLayout::Double);
    EXPECT_EQ(manager.getColumnLayout(), ColumnLayout::Double);
    EXPECT_EQ(manager.getColumnCount(), 2);

    manager.setColumnLayout(ColumnLayout::Triple);
    EXPECT_EQ(manager.getColumnLayout(), ColumnLayout::Triple);
    EXPECT_EQ(manager.getColumnCount(), 3);
}

// Test: Pane column assignment (row-major)
TEST_F(PaneLayoutManagerTest, PaneColumnAssignment)
{
    manager.setColumnLayout(ColumnLayout::Triple);

    // Row-major: 0->col0, 1->col1, 2->col2, 3->col0, etc.
    EXPECT_EQ(manager.getColumnForPane(0), 0);
    EXPECT_EQ(manager.getColumnForPane(1), 1);
    EXPECT_EQ(manager.getColumnForPane(2), 2);
    EXPECT_EQ(manager.getColumnForPane(3), 0);
    EXPECT_EQ(manager.getColumnForPane(4), 1);
    EXPECT_EQ(manager.getColumnForPane(5), 2);
}

// Test: Pane bounds calculation
TEST_F(PaneLayoutManagerTest, PaneBoundsCalculation)
{
    manager.setColumnLayout(ColumnLayout::Double);

    for (int i = 0; i < 4; ++i)
    {
        Pane pane;
        pane.setOrderIndex(i);
        manager.addPane(pane);
    }

    juce::Rectangle<int> availableArea(0, 0, 800, 600);

    // 4 panes in 2 columns = 2 rows
    auto bounds0 = manager.getPaneBounds(0, availableArea);
    auto bounds1 = manager.getPaneBounds(1, availableArea);
    auto bounds2 = manager.getPaneBounds(2, availableArea);
    auto bounds3 = manager.getPaneBounds(3, availableArea);

    // First row: panes 0 and 1
    EXPECT_LT(bounds0.getX(), bounds1.getX());
    EXPECT_EQ(bounds0.getY(), bounds1.getY());

    // Second row: panes 2 and 3
    EXPECT_LT(bounds2.getX(), bounds3.getX());
    EXPECT_EQ(bounds2.getY(), bounds3.getY());

    // Row separation
    EXPECT_GT(bounds2.getY(), bounds0.getY());
}

// Test: Move pane
TEST_F(PaneLayoutManagerTest, MovePane)
{
    Pane pane0, pane1, pane2;
    pane0.setOrderIndex(0);
    pane0.setName("A");
    pane1.setOrderIndex(1);
    pane1.setName("B");
    pane2.setOrderIndex(2);
    pane2.setName("C");

    manager.addPane(pane0);
    manager.addPane(pane1);
    manager.addPane(pane2);

    // Move first pane to end
    manager.movePane(pane0.getId(), 2);

    const auto& panes = manager.getPanes();
    EXPECT_EQ(panes[0].getName(), "B");
    EXPECT_EQ(panes[1].getName(), "C");
    EXPECT_EQ(panes[2].getName(), "A");
}

// Test: Single column pane width - should use full available width
TEST_F(PaneLayoutManagerTest, SingleColumnPaneWidth)
{
    manager.setColumnLayout(ColumnLayout::Single);

    Pane pane;
    pane.setOrderIndex(0);
    manager.addPane(pane);

    juce::Rectangle<int> availableArea(0, 0, 900, 600);
    auto bounds = manager.getPaneBounds(0, availableArea);

    // In single column, pane width should be close to full width (minus 2*margin=4)
    const int margin = 2;
    int expectedWidth = availableArea.getWidth() - 2 * margin;
    EXPECT_EQ(bounds.getWidth(), expectedWidth);
    EXPECT_EQ(bounds.getX(), margin);
}

// Test: Double column pane width - each pane should be half width
TEST_F(PaneLayoutManagerTest, DoubleColumnPaneWidth)
{
    manager.setColumnLayout(ColumnLayout::Double);

    // Add 2 panes (one per column after redistribution)
    Pane pane0, pane1;
    pane0.setOrderIndex(0);
    pane1.setOrderIndex(1);
    manager.addPane(pane0);
    manager.addPane(pane1);

    juce::Rectangle<int> availableArea(0, 0, 900, 600);
    auto bounds0 = manager.getPaneBounds(0, availableArea);
    auto bounds1 = manager.getPaneBounds(1, availableArea);

    // In double column, each pane width should be half (minus margins)
    const int margin = 2;
    int columnWidth = availableArea.getWidth() / 2;
    int expectedPaneWidth = columnWidth - 2 * margin;

    EXPECT_EQ(bounds0.getWidth(), expectedPaneWidth);
    EXPECT_EQ(bounds1.getWidth(), expectedPaneWidth);

    // First pane in first column, second pane in second column
    EXPECT_EQ(bounds0.getX(), margin);
    EXPECT_EQ(bounds1.getX(), columnWidth + margin);
}

// Test: Triple column pane width - each pane should be one-third width
TEST_F(PaneLayoutManagerTest, TripleColumnPaneWidth)
{
    manager.setColumnLayout(ColumnLayout::Triple);

    // Add 3 panes (one per column after redistribution)
    Pane pane0, pane1, pane2;
    pane0.setOrderIndex(0);
    pane1.setOrderIndex(1);
    pane2.setOrderIndex(2);
    manager.addPane(pane0);
    manager.addPane(pane1);
    manager.addPane(pane2);

    juce::Rectangle<int> availableArea(0, 0, 900, 600);
    auto bounds0 = manager.getPaneBounds(0, availableArea);
    auto bounds1 = manager.getPaneBounds(1, availableArea);
    auto bounds2 = manager.getPaneBounds(2, availableArea);

    // In triple column, each pane width should be one-third (minus margins)
    const int margin = 2;
    int columnWidth = availableArea.getWidth() / 3; // 300 pixels each
    int expectedPaneWidth = columnWidth - 2 * margin;

    EXPECT_EQ(bounds0.getWidth(), expectedPaneWidth);
    EXPECT_EQ(bounds1.getWidth(), expectedPaneWidth);
    EXPECT_EQ(bounds2.getWidth(), expectedPaneWidth);

    // Each pane should start at correct column position
    EXPECT_EQ(bounds0.getX(), margin);
    EXPECT_EQ(bounds1.getX(), columnWidth + margin);
    EXPECT_EQ(bounds2.getX(), 2 * columnWidth + margin);
}

// Test: Move pane between columns
TEST_F(PaneLayoutManagerTest, MovePaneBetweenColumns)
{
    manager.setColumnLayout(ColumnLayout::Double);

    Pane pane0, pane1;
    pane0.setOrderIndex(0);
    pane0.setName("A");
    pane1.setOrderIndex(1);
    pane1.setName("B");
    manager.addPane(pane0);
    manager.addPane(pane1);

    // Initially pane0 should be in column 0, pane1 in column 1
    auto* p0 = manager.getPane(pane0.getId());
    auto* p1 = manager.getPane(pane1.getId());
    EXPECT_EQ(p0->getColumnIndex(), 0);
    EXPECT_EQ(p1->getColumnIndex(), 1);

    // Move pane0 to column 1
    manager.movePaneToColumn(pane0.getId(), 1, 0);

    // Now pane0 should be in column 1
    EXPECT_EQ(p0->getColumnIndex(), 1);
}

// Test: Redistribute panes when column layout changes
TEST_F(PaneLayoutManagerTest, RedistributePanesOnLayoutChange)
{
    // Start with 4 panes in single column
    manager.setColumnLayout(ColumnLayout::Single);

    for (int i = 0; i < 4; ++i)
    {
        Pane pane;
        pane.setOrderIndex(i);
        manager.addPane(pane);
    }

    // All panes should be in column 0
    for (const auto& pane : manager.getPanes())
    {
        EXPECT_EQ(pane.getColumnIndex(), 0);
    }

    // Change to double column
    manager.setColumnLayout(ColumnLayout::Double);

    // Panes should be redistributed round-robin: 0,2 in col0, 1,3 in col1
    const auto& panes = manager.getPanes();
    EXPECT_EQ(panes[0].getColumnIndex(), 0);
    EXPECT_EQ(panes[1].getColumnIndex(), 1);
    EXPECT_EQ(panes[2].getColumnIndex(), 0);
    EXPECT_EQ(panes[3].getColumnIndex(), 1);

    // Change to triple column
    manager.setColumnLayout(ColumnLayout::Triple);

    // Panes should be redistributed: 0,3 in col0, 1 in col1, 2 in col2
    EXPECT_EQ(panes[0].getColumnIndex(), 0);
    EXPECT_EQ(panes[1].getColumnIndex(), 1);
    EXPECT_EQ(panes[2].getColumnIndex(), 2);
    EXPECT_EQ(panes[3].getColumnIndex(), 0);
}

// Test: Pane count per column
TEST_F(PaneLayoutManagerTest, PaneCountPerColumn)
{
    manager.setColumnLayout(ColumnLayout::Triple);

    // Add 5 panes
    for (int i = 0; i < 5; ++i)
    {
        Pane pane;
        pane.setOrderIndex(i);
        manager.addPane(pane);
    }

    // Distribution: col0=2, col1=2, col2=1 (round-robin: 0->col0, 1->col1, 2->col2, 3->col0, 4->col1)
    EXPECT_EQ(manager.getPaneCountInColumn(0), 2);
    EXPECT_EQ(manager.getPaneCountInColumn(1), 2);
    EXPECT_EQ(manager.getPaneCountInColumn(2), 1);
}

// Test: Get panes in specific column
TEST_F(PaneLayoutManagerTest, GetPanesInColumn)
{
    manager.setColumnLayout(ColumnLayout::Double);

    Pane pane0, pane1, pane2, pane3;
    pane0.setOrderIndex(0);
    pane0.setName("A");
    pane1.setOrderIndex(1);
    pane1.setName("B");
    pane2.setOrderIndex(2);
    pane2.setName("C");
    pane3.setOrderIndex(3);
    pane3.setName("D");

    manager.addPane(pane0);
    manager.addPane(pane1);
    manager.addPane(pane2);
    manager.addPane(pane3);

    // Column 0: A, C (indices 0, 2)
    // Column 1: B, D (indices 1, 3)
    auto col0Panes = manager.getPanesInColumn(0);
    auto col1Panes = manager.getPanesInColumn(1);

    ASSERT_EQ(col0Panes.size(), 2);
    ASSERT_EQ(col1Panes.size(), 2);

    EXPECT_EQ(col0Panes[0]->getName(), "A");
    EXPECT_EQ(col0Panes[1]->getName(), "C");
    EXPECT_EQ(col1Panes[0]->getName(), "B");
    EXPECT_EQ(col1Panes[1]->getName(), "D");
}

// Test: Pane height distribution within a column
TEST_F(PaneLayoutManagerTest, PaneHeightDistribution)
{
    manager.setColumnLayout(ColumnLayout::Single);

    // Add 2 panes with equal height ratio
    Pane pane0, pane1;
    pane0.setOrderIndex(0);
    pane0.setHeightRatio(0.5f);
    pane1.setOrderIndex(1);
    pane1.setHeightRatio(0.5f);
    manager.addPane(pane0);
    manager.addPane(pane1);

    juce::Rectangle<int> availableArea(0, 0, 800, 600);
    auto bounds0 = manager.getPaneBounds(0, availableArea);
    auto bounds1 = manager.getPaneBounds(1, availableArea);

    // Each pane should get approximately half the height
    // Note: there are margins, so heights won't be exactly 300
    EXPECT_GT(bounds0.getHeight(), 250);
    EXPECT_GT(bounds1.getHeight(), 250);

    // Pane1 should be below pane0
    EXPECT_GT(bounds1.getY(), bounds0.getY());
}

// Test: Remove non-existent pane
TEST_F(PaneLayoutManagerTest, RemoveNonExistentPane)
{
    Pane pane;
    manager.addPane(pane);
    EXPECT_EQ(manager.getPaneCount(), 1);

    // Try to remove a pane that doesn't exist
    manager.removePane(PaneId::generate());

    // Should still have 1 pane
    EXPECT_EQ(manager.getPaneCount(), 1);
}

// Test: Move non-existent pane
TEST_F(PaneLayoutManagerTest, MoveNonExistentPane)
{
    Pane pane;
    manager.addPane(pane);

    // Try to move a pane that doesn't exist - should not crash
    manager.movePane(PaneId::generate(), 0);

    // Original pane should be unaffected
    EXPECT_EQ(manager.getPaneCount(), 1);
}

// Test: Move pane to invalid column
TEST_F(PaneLayoutManagerTest, MovePaneToInvalidColumn)
{
    manager.setColumnLayout(ColumnLayout::Double);

    Pane pane;
    manager.addPane(pane);

    // Try to move to column 10 (only 0-1 are valid)
    manager.movePaneToColumn(pane.getId(), 10, 0);

    // Should be clamped to valid column range (column 1 max for Double)
    const Pane* movedPane = manager.getPane(pane.getId());
    EXPECT_LE(movedPane->getColumnIndex(), 1);
}

// Test: Get pane bounds with empty manager
TEST_F(PaneLayoutManagerTest, GetPaneBoundsEmpty)
{
    juce::Rectangle<int> availableArea(0, 0, 800, 600);
    auto bounds = manager.getPaneBounds(0, availableArea);

    EXPECT_TRUE(bounds.isEmpty());
}

// Test: Get pane bounds with invalid index
TEST_F(PaneLayoutManagerTest, GetPaneBoundsInvalidIndexNegative)
{
    Pane pane;
    manager.addPane(pane);

    juce::Rectangle<int> availableArea(0, 0, 800, 600);
    auto bounds = manager.getPaneBounds(-1, availableArea);

    EXPECT_TRUE(bounds.isEmpty());
}

TEST_F(PaneLayoutManagerTest, GetPaneBoundsInvalidIndexTooLarge)
{
    Pane pane;
    manager.addPane(pane);

    juce::Rectangle<int> availableArea(0, 0, 800, 600);
    auto bounds = manager.getPaneBounds(100, availableArea);

    EXPECT_TRUE(bounds.isEmpty());
}

// Test: Get pane bounds with 0-size area
TEST_F(PaneLayoutManagerTest, GetPaneBoundsZeroSizeArea)
{
    Pane pane;
    manager.addPane(pane);

    juce::Rectangle<int> zeroArea(0, 0, 0, 0);
    auto bounds = manager.getPaneBounds(0, zeroArea);

    // Should return something (even if small due to margins)
    // The bounds will have negative dimensions due to margin subtraction
    EXPECT_LE(bounds.getWidth(), 0);
}

// Test: Get panes in non-existent column
TEST_F(PaneLayoutManagerTest, GetPanesInNonExistentColumn)
{
    manager.setColumnLayout(ColumnLayout::Single);

    Pane pane;
    manager.addPane(pane);

    // Column 5 doesn't exist
    auto panes = manager.getPanesInColumn(5);
    EXPECT_TRUE(panes.empty());
}

// Test: Get pane by invalid ID
TEST_F(PaneLayoutManagerTest, GetPaneInvalidId)
{
    Pane pane;
    manager.addPane(pane);

    Pane* found = manager.getPane(PaneId::generate());
    EXPECT_EQ(found, nullptr);
}

// Test: Many panes stress test
TEST_F(PaneLayoutManagerTest, ManyPanes)
{
    const int count = 100;

    for (int i = 0; i < count; ++i)
    {
        Pane pane;
        pane.setOrderIndex(i);
        manager.addPane(pane);
    }

    EXPECT_EQ(manager.getPaneCount(), count);

    // All panes should be accessible
    juce::Rectangle<int> availableArea(0, 0, 800, 600);
    for (int i = 0; i < count; ++i)
    {
        auto bounds = manager.getPaneBounds(i, availableArea);
        // Bounds might be very small but should exist
        EXPECT_TRUE(bounds.getWidth() >= -10); // Allow for margin issues
    }
}

// Test: Clear all panes
TEST_F(PaneLayoutManagerTest, ClearPanes)
{
    for (int i = 0; i < 5; ++i)
    {
        Pane pane;
        manager.addPane(pane);
    }

    EXPECT_EQ(manager.getPaneCount(), 5);

    manager.clear();

    EXPECT_EQ(manager.getPaneCount(), 0);
}

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

TEST_F(PaneLayoutManagerTest, ListenerNotifications)
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

TEST_F(PaneLayoutManagerTest, RemoveListenerStopsNotifications)
{
    TestPaneListener listener;
    manager.addListener(&listener);
    manager.removeListener(&listener);

    Pane pane;
    manager.addPane(pane);

    EXPECT_EQ(listener.paneAddedCount, 0);
}

// Test: Serialization with empty panes list
TEST_F(PaneLayoutManagerTest, SerializeEmptyPanes)
{
    auto valueTree = manager.toValueTree();

    EXPECT_TRUE(valueTree.isValid());
    EXPECT_EQ(valueTree.getNumChildren(), 0);
}

TEST_F(PaneLayoutManagerTest, DeserializeEmptyPanes)
{
    juce::ValueTree emptyPanes("Panes");
    emptyPanes.setProperty("columnLayout", 2, nullptr);

    manager.fromValueTree(emptyPanes);

    EXPECT_EQ(manager.getPaneCount(), 0);
    EXPECT_EQ(manager.getColumnLayout(), ColumnLayout::Double);
}

TEST_F(PaneLayoutManagerTest, DeserializeNegativeColumnIndexIsClampedToZero)
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
TEST_F(PaneLayoutManagerTest, SerializationRoundTrip)
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
TEST_F(PaneLayoutManagerTest, MovePaneToNegativeColumn)
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
TEST_F(PaneLayoutManagerTest, GetPaneBoundsInColumnInvalidPane)
{
    Pane pane;
    manager.addPane(pane);

    juce::Rectangle<int> columnArea(0, 0, 400, 600);
    auto bounds = manager.getPaneBoundsInColumn(PaneId::generate(), columnArea);

    EXPECT_TRUE(bounds.isEmpty());
}

// Test: Column count for different layouts
TEST_F(PaneLayoutManagerTest, ColumnCountForLayouts)
{
    manager.setColumnLayout(ColumnLayout::Single);
    EXPECT_EQ(manager.getColumnCount(), 1);

    manager.setColumnLayout(ColumnLayout::Double);
    EXPECT_EQ(manager.getColumnCount(), 2);

    manager.setColumnLayout(ColumnLayout::Triple);
    EXPECT_EQ(manager.getColumnCount(), 3);
}

// Test: getColumnForPane with out of range index
TEST_F(PaneLayoutManagerTest, GetColumnForPaneOutOfRange)
{
    manager.setColumnLayout(ColumnLayout::Triple);

    // With no panes, should use fallback row-major
    EXPECT_EQ(manager.getColumnForPane(0), 0);
    EXPECT_EQ(manager.getColumnForPane(1), 1);
    EXPECT_EQ(manager.getColumnForPane(2), 2);
    EXPECT_EQ(manager.getColumnForPane(3), 0);
}
