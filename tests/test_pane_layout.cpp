#include <gtest/gtest.h>
#include "ui/layout/Pane.h"
#include <juce_core/juce_core.h>

using namespace oscil;

class PaneLayoutTest : public ::testing::Test {
protected:
    PaneLayoutManager layoutManager;

    void SetUp() override {
        layoutManager.clear();
    }

    // Helper to add N panes
    void addPanes(int count) {
        for (int i = 0; i < count; ++i) {
            Pane p;
            p.setName("Pane " + juce::String(i));
            p.setOrderIndex(i);
            layoutManager.addPane(p);
        }
    }
};

TEST_F(PaneLayoutTest, Initialization) {
    EXPECT_EQ(layoutManager.getPaneCount(), 0);
    EXPECT_EQ(layoutManager.getColumnLayout(), ColumnLayout::Single);
    EXPECT_EQ(layoutManager.getColumnCount(), 1);
}

TEST_F(PaneLayoutTest, AddPane) {
    Pane p;
    p.setName("Test Pane");
    layoutManager.addPane(p);

    EXPECT_EQ(layoutManager.getPaneCount(), 1);
    EXPECT_EQ(layoutManager.getPanes()[0].getName(), "Test Pane");
    EXPECT_EQ(layoutManager.getPanes()[0].getColumnIndex(), 0); // Default to col 0
}

TEST_F(PaneLayoutTest, RemovePane) {
    Pane p;
    layoutManager.addPane(p);
    EXPECT_EQ(layoutManager.getPaneCount(), 1);

    layoutManager.removePane(p.getId());
    EXPECT_EQ(layoutManager.getPaneCount(), 0);
}

TEST_F(PaneLayoutTest, RedistributePanesSingleColumn) {
    addPanes(3);
    layoutManager.setColumnLayout(ColumnLayout::Single);

    // All panes should be in column 0
    for (const auto& pane : layoutManager.getPanes()) {
        EXPECT_EQ(pane.getColumnIndex(), 0);
    }
    
    EXPECT_EQ(layoutManager.getPaneCountInColumn(0), 3);
}

TEST_F(PaneLayoutTest, RedistributePanesDoubleColumn) {
    addPanes(4);
    layoutManager.setColumnLayout(ColumnLayout::Double);

    // Round-robin distribution expected: 0->0, 1->1, 2->0, 3->1
    const auto& panes = layoutManager.getPanes();
    
    EXPECT_EQ(panes[0].getColumnIndex(), 0);
    EXPECT_EQ(panes[1].getColumnIndex(), 1);
    EXPECT_EQ(panes[2].getColumnIndex(), 0);
    EXPECT_EQ(panes[3].getColumnIndex(), 1);

    EXPECT_EQ(layoutManager.getPaneCountInColumn(0), 2);
    EXPECT_EQ(layoutManager.getPaneCountInColumn(1), 2);
}

TEST_F(PaneLayoutTest, RedistributePanesTripleColumn) {
    addPanes(5);
    layoutManager.setColumnLayout(ColumnLayout::Triple);

    // Round-robin: 0->0, 1->1, 2->2, 3->0, 4->1
    const auto& panes = layoutManager.getPanes();
    
    EXPECT_EQ(panes[0].getColumnIndex(), 0);
    EXPECT_EQ(panes[1].getColumnIndex(), 1);
    EXPECT_EQ(panes[2].getColumnIndex(), 2);
    EXPECT_EQ(panes[3].getColumnIndex(), 0);
    EXPECT_EQ(panes[4].getColumnIndex(), 1);

    EXPECT_EQ(layoutManager.getPaneCountInColumn(0), 2);
    EXPECT_EQ(layoutManager.getPaneCountInColumn(1), 2);
    EXPECT_EQ(layoutManager.getPaneCountInColumn(2), 1);
}

TEST_F(PaneLayoutTest, MovePaneInternalReorder) {
    addPanes(3); // 0, 1, 2
    PaneId id0 = layoutManager.getPanes()[0].getId();
    PaneId id1 = layoutManager.getPanes()[1].getId();
    PaneId id2 = layoutManager.getPanes()[2].getId();

    // Move Pane 0 to end (index 2)
    layoutManager.movePane(id0, 2);

    const auto& panes = layoutManager.getPanes();
    // Expected order: 1, 2, 0
    EXPECT_EQ(panes[0].getId().id, id1.id);
    EXPECT_EQ(panes[1].getId().id, id2.id);
    EXPECT_EQ(panes[2].getId().id, id0.id);
    
    EXPECT_EQ(panes[0].getOrderIndex(), 0);
    EXPECT_EQ(panes[1].getOrderIndex(), 1);
    EXPECT_EQ(panes[2].getOrderIndex(), 2);
}

TEST_F(PaneLayoutTest, MovePaneToColumn) {
    layoutManager.setColumnLayout(ColumnLayout::Double);
    addPanes(2); // Pane 0 (col 0), Pane 1 (col 1)
    
    PaneId id0 = layoutManager.getPanes()[0].getId(); // currently col 0
    
    // Move Pane 0 to Column 1, position 1 (after Pane 1)
    layoutManager.movePaneToColumn(id0, 1, 1);

    // Both should be in col 1
    EXPECT_EQ(layoutManager.getPaneCountInColumn(0), 0);
    EXPECT_EQ(layoutManager.getPaneCountInColumn(1), 2);
    
    const auto* p0 = layoutManager.getPane(id0);
    EXPECT_EQ(p0->getColumnIndex(), 1);
}

TEST_F(PaneLayoutTest, Serialization) {
    layoutManager.setColumnLayout(ColumnLayout::Double);
    addPanes(2);
    
    // Modify one pane
    auto& p0 = const_cast<Pane&>(layoutManager.getPanes()[0]);
    p0.setHeightRatio(0.5f);
    p0.setCollapsed(true);

    // Save
    juce::ValueTree state = layoutManager.toValueTree();

    // Restore to new manager
    PaneLayoutManager newManager;
    newManager.fromValueTree(state);

    EXPECT_EQ(newManager.getColumnLayout(), ColumnLayout::Double);
    EXPECT_EQ(newManager.getPaneCount(), 2);
    
    const auto* newP0 = newManager.getPane(p0.getId());
    ASSERT_NE(newP0, nullptr);
    EXPECT_EQ(newP0->getName(), p0.getName());
    EXPECT_EQ(newP0->getColumnIndex(), p0.getColumnIndex());
    EXPECT_FLOAT_EQ(newP0->getHeightRatio(), 0.5f);
    EXPECT_TRUE(newP0->isCollapsed());
}

TEST_F(PaneLayoutTest, BoundsCalculationSingleColumn) {
    layoutManager.setColumnLayout(ColumnLayout::Single);
    addPanes(2);
    
    juce::Rectangle<int> area(0, 0, 100, 200);
    
    // Both panes have default height ratio (equal) -> 50/50 split
    auto b0 = layoutManager.getPaneBounds(0, area);
    auto b1 = layoutManager.getPaneBounds(1, area);
    
    // Width includes margins
    // margin = 2. width = 100 - 4 = 96.
    EXPECT_EQ(b0.getWidth(), 96);
    EXPECT_EQ(b1.getWidth(), 96);
    
    // Height includes margins. 
    // each pane height (allocated) = 100.
    // rect height = 100 - 4 = 96.
    EXPECT_EQ(b0.getHeight(), 96);
    EXPECT_EQ(b1.getHeight(), 96);
    
    // Top Y position includes margin
    EXPECT_EQ(b0.getY(), 2);
    // Second pane Y includes allocated height of first (100) + margin (2) = 102
    EXPECT_EQ(b1.getY(), 102);
}
