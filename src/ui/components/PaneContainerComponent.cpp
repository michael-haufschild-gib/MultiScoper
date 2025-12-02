/*
    Oscil - Pane Container Component Implementation
*/

#include "ui/components/PaneContainerComponent.h"

namespace oscil
{

PaneContainerComponent::PaneContainerComponent()
{
}

void PaneContainerComponent::setPaneDropCallback(PaneDropCallback callback)
{
    paneDropCallback_ = std::move(callback);
}

void PaneContainerComponent::setEmptyColumnDropCallback(EmptyColumnDropCallback callback)
{
    emptyColumnDropCallback_ = std::move(callback);
}

void PaneContainerComponent::setColumnCount(int count)
{
    columnCount_ = count;
}

bool PaneContainerComponent::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    // Accept drags from PaneComponents
    if (auto* sourcePane = dynamic_cast<PaneComponent*>(dragSourceDetails.sourceComponent.get()))
    {
        juce::ignoreUnused(sourcePane);
        return true;
    }
    return false;
}

void PaneContainerComponent::itemDragEnter(const SourceDetails& dragSourceDetails)
{
    updateDropTarget(dragSourceDetails);
}

void PaneContainerComponent::itemDragMove(const SourceDetails& dragSourceDetails)
{
    updateDropTarget(dragSourceDetails);
}

void PaneContainerComponent::itemDragExit(const SourceDetails& /*dragSourceDetails*/)
{
    clearDropHighlight();
}

void PaneContainerComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
    clearDropHighlight();

    if (!dragSourceDetails.description.isString())
        return;

    PaneId movedPaneId;
    movedPaneId.id = dragSourceDetails.description.toString();

    // Find which PaneComponent we dropped on
    PaneComponent* targetPane = findPaneAt(dragSourceDetails.localPosition);
    if (targetPane && targetPane->getPaneId().id != movedPaneId.id)
    {
        if (paneDropCallback_)
        {
            paneDropCallback_(movedPaneId, targetPane->getPaneId());
        }
    }
    else if (!targetPane && columnCount_ > 1 && emptyColumnDropCallback_)
    {
        // Dropped on empty column area - calculate target column from X position
        int componentWidth = getWidth();
        if (componentWidth > 0)
        {
            int targetColumn = (dragSourceDetails.localPosition.x * columnCount_) / componentWidth;
            targetColumn = juce::jlimit(0, columnCount_ - 1, targetColumn);
            emptyColumnDropCallback_(movedPaneId, targetColumn);
        }
    }
}

PaneComponent* PaneContainerComponent::findPaneAt(juce::Point<int> position)
{
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        if (auto* pane = dynamic_cast<PaneComponent*>(getChildComponent(i)))
        {
            if (pane->getBounds().contains(position))
            {
                return pane;
            }
        }
    }
    return nullptr;
}

void PaneContainerComponent::updateDropTarget(const SourceDetails& dragSourceDetails)
{
    // Find target pane and highlight it
    if (auto* targetPane = findPaneAt(dragSourceDetails.localPosition))
    {
        if (auto* sourcePane = dynamic_cast<PaneComponent*>(dragSourceDetails.sourceComponent.get()))
        {
            if (targetPane->getPaneId().id != sourcePane->getPaneId().id)
            {
                // Would trigger itemDragEnter on the target pane
                // For now just let the visual feedback come from the target itself
            }
        }
    }
}

void PaneContainerComponent::clearDropHighlight()
{
    // Clear any drop highlighting
}

} // namespace oscil
