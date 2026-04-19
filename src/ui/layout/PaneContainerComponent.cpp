/*
    MultiScoper - Pane Container Component Implementation
*/

#include "ui/layout/PaneContainerComponent.h"

#include "ui/theme/ThemeManager.h"

namespace multiscoper
{

PaneContainerComponent::PaneContainerComponent(IThemeService& themeService) : themeService_(themeService) {}

void PaneContainerComponent::setPaneDropCallback(PaneDropCallback callback) { paneDropCallback_ = std::move(callback); }

void PaneContainerComponent::setEmptyColumnDropCallback(EmptyColumnDropCallback callback)
{
    emptyColumnDropCallback_ = std::move(callback);
}

void PaneContainerComponent::setColumnCount(int count) { columnCount_ = count; }

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

void PaneContainerComponent::itemDragExit(const SourceDetails& /*dragSourceDetails*/) { clearDropHighlight(); }

void PaneContainerComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
    clearDropHighlight();

    if (!dragSourceDetails.description.isString())
        return;

    PaneId movedPaneId;
    movedPaneId.id = dragSourceDetails.description.toString();

    // Find which PaneComponent we dropped on
    PaneComponent const* targetPane = findPaneAt(dragSourceDetails.localPosition);
    if (targetPane != nullptr && targetPane->getPaneId().id != movedPaneId.id)
    {
        if (paneDropCallback_)
        {
            paneDropCallback_(movedPaneId, targetPane->getPaneId());
        }
    }
    else if (!targetPane && columnCount_ > 1 && emptyColumnDropCallback_)
    {
        // Dropped on empty column area - calculate target column from X position
        int const componentWidth = getWidth();
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

void PaneContainerComponent::paint(juce::Graphics& g)
{
    auto const highlightColour = themeService_.getCurrentTheme().controlActive;

    // Draw drop highlight overlay when dragging over a target
    if (highlightedPaneId_.isValid())
    {
        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            if (auto* pane = dynamic_cast<PaneComponent*>(getChildComponent(i)))
            {
                if (pane->getPaneId() == highlightedPaneId_)
                {
                    auto bounds = pane->getBounds().toFloat();
                    g.setColour(highlightColour.withAlpha(0.13f));
                    g.fillRect(bounds);
                    g.setColour(highlightColour.withAlpha(0.40f));
                    g.drawRect(bounds.reduced(1.0f), 2.0f);
                    break;
                }
            }
        }
    }
    else if (highlightedColumn_ >= 0 && columnCount_ > 1)
    {
        int const colWidth = getWidth() / columnCount_;
        auto colBounds = juce::Rectangle<float>(static_cast<float>(highlightedColumn_ * colWidth), 0.0f,
                                                static_cast<float>(colWidth), static_cast<float>(getHeight()));
        g.setColour(highlightColour.withAlpha(0.07f));
        g.fillRect(colBounds.reduced(2.0f));
    }
}

void PaneContainerComponent::updateDropTarget(const SourceDetails& dragSourceDetails)
{
    PaneId const previousHighlight = highlightedPaneId_;
    int const previousColumn = highlightedColumn_;

    highlightedPaneId_ = PaneId::invalid();
    highlightedColumn_ = -1;

    if (auto* targetPane = findPaneAt(dragSourceDetails.localPosition))
    {
        if (auto* sourcePane = dynamic_cast<PaneComponent*>(dragSourceDetails.sourceComponent.get()))
        {
            if (targetPane->getPaneId().id != sourcePane->getPaneId().id)
            {
                highlightedPaneId_ = targetPane->getPaneId();
            }
        }
    }
    else if (columnCount_ > 1)
    {
        int const componentWidth = getWidth();
        if (componentWidth > 0)
        {
            highlightedColumn_ = (dragSourceDetails.localPosition.x * columnCount_) / componentWidth;
            highlightedColumn_ = juce::jlimit(0, columnCount_ - 1, highlightedColumn_);
        }
    }

    if (highlightedPaneId_ != previousHighlight || highlightedColumn_ != previousColumn)
        repaint();
}

void PaneContainerComponent::clearDropHighlight()
{
    if (highlightedPaneId_.isValid() || highlightedColumn_ >= 0)
    {
        highlightedPaneId_ = PaneId::invalid();
        highlightedColumn_ = -1;
        repaint();
    }
}

} // namespace multiscoper
