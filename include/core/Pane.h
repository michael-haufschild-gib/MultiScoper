/*
    Oscil - Pane Model
    Container for grouping oscillators in the visualization layout
*/

#pragma once

#include <juce_core/juce_core.h>
#include "Oscillator.h"
#include <vector>

namespace oscil
{

/**
 * Pane configuration and state.
 * A pane is a container grouping one or more Oscillators stacked vertically.
 * Panes are ordered and flow into columns based on the layout settings.
 * PRD aligned.
 */
class Pane
{
public:
    // Height constraints
    static constexpr float MIN_HEIGHT_RATIO = 0.1f;  // Minimum 10% of column height
    static constexpr float MAX_HEIGHT_RATIO = 1.0f;  // Maximum full column height
    static constexpr float DEFAULT_HEIGHT_RATIO = 0.33f;  // Default 1/3 of column

    /**
     * Create a new pane with generated ID
     */
    Pane();

    /**
     * Create a pane from serialized ValueTree
     */
    explicit Pane(const juce::ValueTree& state);

    /**
     * Serialize to ValueTree
     */
    juce::ValueTree toValueTree() const;

    /**
     * Load state from ValueTree
     */
    void fromValueTree(const juce::ValueTree& state);

    // Getters
    PaneId getId() const { return id_; }
    int getOrderIndex() const { return orderIndex_; }
    bool isCollapsed() const { return collapsed_; }
    juce::String getName() const { return name_; }
    float getHeightRatio() const { return heightRatio_; }
    int getColumnIndex() const { return columnIndex_; }

    // Setters
    void setOrderIndex(int index) { orderIndex_ = index; }
    void setCollapsed(bool collapsed) { collapsed_ = collapsed; }
    void setName(const juce::String& name) { name_ = name; }
    void setHeightRatio(float ratio) { heightRatio_ = juce::jlimit(MIN_HEIGHT_RATIO, MAX_HEIGHT_RATIO, ratio); }
    void setColumnIndex(int column) { columnIndex_ = column; }

private:
    PaneId id_;
    int orderIndex_ = 0;
    bool collapsed_ = false;
    juce::String name_;
    float heightRatio_ = DEFAULT_HEIGHT_RATIO;
    int columnIndex_ = 0;  // Which column this pane is in (0-2)
};

/**
 * State for pane drag-drop operations (PRD aligned)
 */
struct PaneDragState
{
    bool isDragging = false;
    PaneId draggedPaneId;
    int dragStartIndex = -1;
    int dragStartColumn = -1;
    juce::Point<int> dragStartPosition;
    juce::Point<int> currentPosition;

    // Drop target
    PaneId dropTargetPaneId;
    int dropTargetIndex = -1;
    int dropTargetColumn = -1;
    bool isValidDropTarget = false;

    // Visual feedback
    juce::Rectangle<int> dragPreviewBounds;
    juce::Rectangle<int> dropIndicatorBounds;

    void startDrag(const PaneId& paneId, int index, int column, juce::Point<int> position)
    {
        isDragging = true;
        draggedPaneId = paneId;
        dragStartIndex = index;
        dragStartColumn = column;
        dragStartPosition = position;
        currentPosition = position;
        dropTargetPaneId = PaneId::invalid();
        dropTargetIndex = -1;
        dropTargetColumn = -1;
        isValidDropTarget = false;
    }

    void updateDrag(juce::Point<int> position)
    {
        currentPosition = position;
    }

    void setDropTarget(const PaneId& targetId, int targetIndex, int targetColumn, bool isValid)
    {
        dropTargetPaneId = targetId;
        dropTargetIndex = targetIndex;
        dropTargetColumn = targetColumn;
        isValidDropTarget = isValid;
    }

    void endDrag()
    {
        isDragging = false;
        draggedPaneId = PaneId::invalid();
        dragStartIndex = -1;
        dragStartColumn = -1;
        dropTargetPaneId = PaneId::invalid();
        dropTargetIndex = -1;
        dropTargetColumn = -1;
        isValidDropTarget = false;
    }

    void cancelDrag()
    {
        endDrag();
    }

    bool isSamePositionDrop() const
    {
        return dragStartIndex == dropTargetIndex && dragStartColumn == dropTargetColumn;
    }
};

/**
 * Layout column configuration
 */
enum class ColumnLayout
{
    Single = 1,
    Double = 2,
    Triple = 3
};

/**
 * Manages pane layout and ordering (PRD aligned)
 */
class PaneLayoutManager
{
public:
    PaneLayoutManager() = default;

    /**
     * Set the column layout and redistribute panes
     */
    void setColumnLayout(ColumnLayout layout);

    /**
     * Get the column layout
     */
    ColumnLayout getColumnLayout() const { return columnLayout_; }

    /**
     * Get the number of columns
     */
    int getColumnCount() const { return static_cast<int>(columnLayout_); }

    /**
     * Add a pane
     */
    void addPane(const Pane& pane);

    /**
     * Remove a pane
     */
    void removePane(const PaneId& paneId);

    /**
     * Get all panes in order (const access only - use addPane/removePane/movePane to modify)
     */
    const std::vector<Pane>& getPanes() const { return panes_; }

    /**
     * Get panes for a specific column
     */
    std::vector<Pane*> getPanesInColumn(int column);
    std::vector<const Pane*> getPanesInColumn(int column) const;

    /**
     * Get pane by ID
     */
    Pane* getPane(const PaneId& paneId);
    const Pane* getPane(const PaneId& paneId) const;

    /**
     * Move a pane to a new position within or between columns
     */
    void movePane(const PaneId& paneId, int newIndex);

    /**
     * Move a pane to a specific column and position
     */
    void movePaneToColumn(const PaneId& paneId, int targetColumn, int positionInColumn);

    /**
     * Clear all panes
     */
    void clear() { panes_.clear(); }

    /**
     * Get the number of panes
     */
    size_t getPaneCount() const { return panes_.size(); }

    /**
     * Get the number of panes in a specific column
     */
    int getPaneCountInColumn(int column) const;

    /**
     * Calculate which column a pane should appear in
     * Uses row-major distribution
     */
    int getColumnForPane(int paneIndex) const;

    /**
     * Calculate pane bounds within the available area
     */
    juce::Rectangle<int> getPaneBounds(int paneIndex, juce::Rectangle<int> availableArea) const;

    /**
     * Calculate pane bounds for a pane in a specific column
     */
    juce::Rectangle<int> getPaneBoundsInColumn(const PaneId& paneId, juce::Rectangle<int> columnArea) const;

    /**
     * Redistribute panes across columns after layout change
     * PRD: "round-robin distribution to balance columns"
     */
    void redistributePanes();

    /**
     * Serialize to ValueTree
     */
    juce::ValueTree toValueTree() const;

    /**
     * Load from ValueTree
     */
    void fromValueTree(const juce::ValueTree& state);

    /**
     * Listener interface for layout changes
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void columnLayoutChanged(ColumnLayout /*layout*/) {}
        virtual void paneOrderChanged() {}
        virtual void paneAdded(const PaneId& /*paneId*/) {}
        virtual void paneRemoved(const PaneId& /*paneId*/) {}
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    ColumnLayout columnLayout_ = ColumnLayout::Single;
    std::vector<Pane> panes_;
    juce::ListenerList<Listener> listeners_;

    void sortPanesByOrder();
    void notifyColumnLayoutChanged();
    void notifyPaneOrderChanged();
    void notifyPaneAdded(const PaneId& paneId);
    void notifyPaneRemoved(const PaneId& paneId);
};

// ValueTree identifiers for Pane and PaneLayout
namespace PaneIds
{
    static const juce::Identifier Pane{ "Pane" };
    static const juce::Identifier Panes{ "Panes" };
    static const juce::Identifier Id{ "id" };
    static const juce::Identifier OrderIndex{ "orderIndex" };
    static const juce::Identifier Collapsed{ "collapsed" };
    static const juce::Identifier Name{ "name" };
    static const juce::Identifier HeightRatio{ "heightRatio" };
    static const juce::Identifier ColumnIndex{ "columnIndex" };
    static const juce::Identifier ColumnLayout{ "columnLayout" };
}

} // namespace oscil
