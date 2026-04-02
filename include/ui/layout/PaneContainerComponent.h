/*
    Oscil - Pane Container Component
    Custom content component that acts as a DragAndDropTarget proxy.
*/

#pragma once

#include "ui/layout/PaneComponent.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace oscil
{

/**
 * Custom content component that acts as a DragAndDropTarget proxy.
 * When drops occur, it forwards them to the appropriate child PaneComponent.
 */
class PaneContainerComponent
    : public juce::Component
    , public juce::DragAndDropTarget
{
public:
    using PaneDropCallback = std::function<void(const PaneId& movedPaneId, const PaneId& targetPaneId)>;
    using EmptyColumnDropCallback = std::function<void(const PaneId& movedPaneId, int targetColumn)>;

    PaneContainerComponent();
    ~PaneContainerComponent() override = default;

    void setPaneDropCallback(PaneDropCallback callback);
    void setEmptyColumnDropCallback(EmptyColumnDropCallback callback);
    void setColumnCount(int count);

    /// Accept pane drag-drop for reordering between columns (DragAndDropTarget).
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

private:
    void paint(juce::Graphics& g) override;

    PaneComponent* findPaneAt(juce::Point<int> position);
    void updateDropTarget(const SourceDetails& dragSourceDetails);
    void clearDropHighlight();

    PaneDropCallback paneDropCallback_;
    EmptyColumnDropCallback emptyColumnDropCallback_;
    int columnCount_ = 1;

    // Drop highlight state
    PaneId highlightedPaneId_;
    int highlightedColumn_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PaneContainerComponent)
};

} // namespace oscil
