/*
    Oscil - Pane Tests
*/

#include "ui/layout/Pane.h"

#include <gtest/gtest.h>

using namespace oscil;

class PaneTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Pane ID generation
TEST_F(PaneTest, IdGeneration)
{
    Pane pane1;
    Pane pane2;

    EXPECT_TRUE(pane1.getId().isValid());
    EXPECT_TRUE(pane2.getId().isValid());
    EXPECT_NE(pane1.getId(), pane2.getId());
}

// Test: Pane default values
TEST_F(PaneTest, DefaultValues)
{
    Pane pane;

    EXPECT_FALSE(pane.isCollapsed());
    EXPECT_EQ(pane.getOrderIndex(), 0);
}

// Test: Pane serialization
TEST_F(PaneTest, SerializationRoundTrip)
{
    Pane original;
    original.setOrderIndex(5);
    original.setCollapsed(true);
    original.setName("Test Pane");

    auto valueTree = original.toValueTree();

    Pane restored(valueTree);

    EXPECT_EQ(restored.getOrderIndex(), original.getOrderIndex());
    EXPECT_EQ(restored.isCollapsed(), original.isCollapsed());
    EXPECT_EQ(restored.getName(), original.getName());
}

// Test: Height ratio clamping
TEST_F(PaneTest, HeightRatioClampingLow)
{
    Pane pane;
    pane.setHeightRatio(0.0f);
    EXPECT_EQ(pane.getHeightRatio(), Pane::MIN_HEIGHT_RATIO);
}

TEST_F(PaneTest, HeightRatioClampingHigh)
{
    Pane pane;
    pane.setHeightRatio(2.0f);
    EXPECT_EQ(pane.getHeightRatio(), Pane::MAX_HEIGHT_RATIO);
}

TEST_F(PaneTest, HeightRatioNegative)
{
    Pane pane;
    pane.setHeightRatio(-1.0f);
    EXPECT_EQ(pane.getHeightRatio(), Pane::MIN_HEIGHT_RATIO);
}

TEST_F(PaneTest, HeightRatioDefault)
{
    Pane pane;
    EXPECT_EQ(pane.getHeightRatio(), Pane::DEFAULT_HEIGHT_RATIO);
}

// Test: Column index
TEST_F(PaneTest, ColumnIndexDefault)
{
    Pane pane;
    EXPECT_EQ(pane.getColumnIndex(), 0);
}

TEST_F(PaneTest, ColumnIndexSetter)
{
    Pane pane;
    pane.setColumnIndex(2);
    EXPECT_EQ(pane.getColumnIndex(), 2);
}

// Test: PaneId handling
TEST_F(PaneTest, InvalidPaneId)
{
    PaneId invalid = PaneId::invalid();
    EXPECT_FALSE(invalid.isValid());
    EXPECT_TRUE(invalid.id.isEmpty());
}

TEST_F(PaneTest, PaneIdEquality)
{
    PaneId id1 = PaneId::generate();
    PaneId id2 = PaneId::generate();
    PaneId id1Copy;
    id1Copy.id = id1.id;

    EXPECT_NE(id1, id2);
    EXPECT_EQ(id1, id1Copy);
}

// Test: Pane deserialization with out of range values
TEST_F(PaneTest, DeserializeWithOutOfRangeHeightRatio)
{
    juce::ValueTree state("Pane");
    state.setProperty("heightRatio", 5.0f, nullptr);

    Pane pane(state);
    EXPECT_EQ(pane.getHeightRatio(), Pane::MAX_HEIGHT_RATIO);
}

TEST_F(PaneTest, DeserializeWrongType)
{
    juce::ValueTree wrongType("WrongType");
    wrongType.setProperty("id", "test-id", nullptr);

    Pane pane;
    PaneId originalId = pane.getId();
    pane.fromValueTree(wrongType);

    // Should not load from wrong type
    EXPECT_EQ(pane.getId(), originalId);
}

TEST_F(PaneTest, PaneIdHash)
{
    PaneIdHash hasher;
    PaneId id1 = PaneId::generate();
    PaneId id2 = PaneId::generate();

    size_t hash1 = hasher(id1);
    size_t hash2 = hasher(id2);

    // Hashes should be different (with very high probability)
    EXPECT_NE(hash1, hash2);

    // Same ID should produce same hash
    PaneId id1Copy;
    id1Copy.id = id1.id;
    EXPECT_EQ(hasher(id1), hasher(id1Copy));
}

// ============================================================================
// PaneDragState Tests
// ============================================================================

class PaneDragStateTest : public ::testing::Test
{
protected:
    PaneDragState dragState;
};

TEST_F(PaneDragStateTest, InitialState)
{
    EXPECT_FALSE(dragState.isDragging);
    EXPECT_FALSE(dragState.draggedPaneId.isValid());
    EXPECT_EQ(dragState.dragStartIndex, -1);
}

TEST_F(PaneDragStateTest, StartDrag)
{
    PaneId paneId = PaneId::generate();
    dragState.startDrag(paneId, 2, 1, {100, 200});

    EXPECT_TRUE(dragState.isDragging);
    EXPECT_EQ(dragState.draggedPaneId, paneId);
    EXPECT_EQ(dragState.dragStartIndex, 2);
    EXPECT_EQ(dragState.dragStartColumn, 1);
    EXPECT_EQ(dragState.dragStartPosition, juce::Point<int>(100, 200));
    EXPECT_EQ(dragState.currentPosition, juce::Point<int>(100, 200));
    EXPECT_FALSE(dragState.isValidDropTarget);
}

TEST_F(PaneDragStateTest, UpdateDrag)
{
    PaneId paneId = PaneId::generate();
    dragState.startDrag(paneId, 0, 0, {0, 0});
    dragState.updateDrag({150, 300});

    EXPECT_EQ(dragState.currentPosition, juce::Point<int>(150, 300));
}

TEST_F(PaneDragStateTest, SetDropTarget)
{
    PaneId dragId = PaneId::generate();
    PaneId targetId = PaneId::generate();

    dragState.startDrag(dragId, 0, 0, {0, 0});
    dragState.setDropTarget(targetId, 2, 1, true);

    EXPECT_EQ(dragState.dropTargetPaneId, targetId);
    EXPECT_EQ(dragState.dropTargetIndex, 2);
    EXPECT_EQ(dragState.dropTargetColumn, 1);
    EXPECT_TRUE(dragState.isValidDropTarget);
}

TEST_F(PaneDragStateTest, EndDrag)
{
    PaneId paneId = PaneId::generate();
    dragState.startDrag(paneId, 1, 0, {50, 50});
    dragState.endDrag();

    EXPECT_FALSE(dragState.isDragging);
    EXPECT_FALSE(dragState.draggedPaneId.isValid());
    EXPECT_EQ(dragState.dragStartIndex, -1);
    EXPECT_EQ(dragState.dropTargetIndex, -1);
}

TEST_F(PaneDragStateTest, CancelDrag)
{
    PaneId paneId = PaneId::generate();
    dragState.startDrag(paneId, 1, 0, {50, 50});
    dragState.cancelDrag();

    EXPECT_FALSE(dragState.isDragging);
}

TEST_F(PaneDragStateTest, IsSamePositionDrop)
{
    PaneId paneId = PaneId::generate();
    dragState.startDrag(paneId, 2, 1, {0, 0});
    dragState.setDropTarget(PaneId::generate(), 2, 1, true);

    EXPECT_TRUE(dragState.isSamePositionDrop());
}

TEST_F(PaneDragStateTest, IsDifferentPositionDrop)
{
    PaneId paneId = PaneId::generate();
    dragState.startDrag(paneId, 2, 1, {0, 0});
    dragState.setDropTarget(PaneId::generate(), 3, 1, true);

    EXPECT_FALSE(dragState.isSamePositionDrop());
}
