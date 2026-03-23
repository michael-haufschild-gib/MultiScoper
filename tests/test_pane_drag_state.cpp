/*
    Oscil - PaneDragState Edge Case Tests
    Tests for edge cases in the drag-drop state machine.
    Basic operations covered in test_pane.cpp.

    Bug targets:
    - startDrag not clearing previous drop target state from prior gesture
    - isSamePositionDrop returning true when column differs
    - updateDrag overwriting dragStartPosition
    - double endDrag corrupting state
*/

#include <gtest/gtest.h>
#include "core/Pane.h"

using namespace oscil;

class PaneDragStateEdgeTest : public ::testing::Test
{
protected:
    PaneDragState state;
};

TEST_F(PaneDragStateEdgeTest, StartDragClearsPreviousDropTargetFromPriorGesture)
{
    // Bug caught: starting a new drag while a previous drag's drop target
    // state is still set causes the new drag to inherit stale state
    state.startDrag(PaneId::generate(), 0, 0, {0, 0});
    state.setDropTarget(PaneId::generate(), 3, 1, true);

    // Start a new drag - should clear old drop target
    state.startDrag(PaneId::generate(), 1, 0, {50, 50});

    EXPECT_FALSE(state.isValidDropTarget);
    EXPECT_FALSE(state.dropTargetPaneId.isValid());
    EXPECT_EQ(state.dropTargetIndex, -1);
    EXPECT_EQ(state.dropTargetColumn, -1);
}

TEST_F(PaneDragStateEdgeTest, UpdateDragPreservesDragStartPosition)
{
    state.startDrag(PaneId::generate(), 0, 0, {10, 20});

    state.updateDrag({100, 200});
    state.updateDrag({300, 400});

    EXPECT_EQ(state.currentPosition, juce::Point<int>(300, 400));
    // Start position must never change during drag
    EXPECT_EQ(state.dragStartPosition, juce::Point<int>(10, 20));
}

TEST_F(PaneDragStateEdgeTest, IsSamePositionDropReturnsFalseWhenColumnDiffers)
{
    // Bug caught: isSamePositionDrop only checks index, not column,
    // so dragging to same index in different column is treated as no-op
    state.startDrag(PaneId::generate(), 2, 0, {0, 0});
    state.setDropTarget(PaneId::generate(), 2, 1, true);

    EXPECT_FALSE(state.isSamePositionDrop());
}

TEST_F(PaneDragStateEdgeTest, DoubleEndDragIsIdempotent)
{
    state.startDrag(PaneId::generate(), 0, 0, {0, 0});
    state.endDrag();
    state.endDrag();

    EXPECT_FALSE(state.isDragging);
    EXPECT_FALSE(state.draggedPaneId.isValid());
}

TEST_F(PaneDragStateEdgeTest, CancelDragAfterEndDragIsIdempotent)
{
    state.startDrag(PaneId::generate(), 0, 0, {0, 0});
    state.endDrag();
    state.cancelDrag();

    EXPECT_FALSE(state.isDragging);
}

TEST_F(PaneDragStateEdgeTest, SetDropTargetOverwritesPreviousTarget)
{
    state.startDrag(PaneId::generate(), 0, 0, {0, 0});

    PaneId target1 = PaneId::generate();
    PaneId target2 = PaneId::generate();

    state.setDropTarget(target1, 1, 0, true);
    EXPECT_EQ(state.dropTargetPaneId, target1);
    EXPECT_TRUE(state.isValidDropTarget);

    state.setDropTarget(target2, 2, 1, false);
    EXPECT_EQ(state.dropTargetPaneId, target2);
    EXPECT_FALSE(state.isValidDropTarget);
}
