/*
    Oscil - Oscillator List Component Events
    Item listener forwarding, filter-mode changes, drag-and-drop source
    and target handling. Split from OscillatorListComponent.cpp which
    owns construction, layout, and list rebuild/paint.
*/

#include "ui/panels/OscillatorListComponent.h"

namespace oscil
{

void OscillatorListComponent::oscillatorSelected(const OscillatorId& id)
{
    setSelectedOscillator(id);
    listeners_.call([id](Listener& l) { l.oscillatorSelected(id); });
}

void OscillatorListComponent::oscillatorVisibilityChanged(const OscillatorId& id, bool visible)
{
    listeners_.call([id, visible](Listener& l) { l.oscillatorVisibilityChanged(id, visible); });
}

void OscillatorListComponent::oscillatorModeChanged(const OscillatorId& id, ProcessingMode mode)
{
    listeners_.call([id, mode](Listener& l) { l.oscillatorModeChanged(id, mode); });
}

void OscillatorListComponent::oscillatorConfigRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorConfigRequested(id); });
}

void OscillatorListComponent::oscillatorColorConfigRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorColorConfigRequested(id); });
}

void OscillatorListComponent::oscillatorDeleteRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorDeleteRequested(id); });
}

void OscillatorListComponent::oscillatorDragStarted(const OscillatorId& id)
{
    // Find the component for this ID
    OscillatorListItemComponent* sourceComponent = nullptr;
    int sourceIndex = -1;

    for (size_t i = 0; i < items_.size(); ++i)
    {
        if (items_[i]->getOscillatorId() == id)
        {
            sourceComponent = items_[i].get();
            sourceIndex = static_cast<int>(i);
            break;
        }
    }

    if (sourceComponent)
    {
        // Create drag description using DynamicObject properly
        auto* dragObj = new juce::DynamicObject();
        dragObj->setProperty("type", "oscillator");
        dragObj->setProperty("id", id.id);
        dragObj->setProperty("index", sourceIndex);
        juce::var const dragDescription(dragObj);

        // Create snapshot for drag image
        juce::Image const dragImage = sourceComponent->createComponentSnapshot(sourceComponent->getLocalBounds());

        // Start JUCE drag operation (1.0f = scale factor for the drag image)
        startDragging(dragDescription, sourceComponent, juce::ScaledImage(dragImage), true);
    }
}

bool OscillatorListComponent::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    return dragSourceDetails.description.isObject() && dragSourceDetails.description.hasProperty("type") &&
           dragSourceDetails.description.getProperty("type", "").toString() == "oscillator";
}

void OscillatorListComponent::itemDragEnter(const SourceDetails& dragSourceDetails) { itemDragMove(dragSourceDetails); }

void OscillatorListComponent::itemDragMove(const SourceDetails& dragSourceDetails)
{
    auto mousePos = dragSourceDetails.localPosition;

    // Convert mouse Y to container coordinates to check against items
    int const containerY = mousePos.y - viewport_->getY() + viewport_->getViewPositionY();

    int const newTarget = getItemIndexAtY(containerY);
    if (newTarget != dragTargetIndex_)
    {
        updateDragIndicator(newTarget);
    }
}

void OscillatorListComponent::itemDragExit(const SourceDetails& /*dragSourceDetails*/) { updateDragIndicator(-1); }

void OscillatorListComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
    int const sourceIndex = static_cast<int>(dragSourceDetails.description.getProperty("index", -1));
    int targetIndex = dragTargetIndex_;

    updateDragIndicator(-1);

    if (sourceIndex != -1 && targetIndex != -1 && sourceIndex != targetIndex)
    {
        // Filtered indices don't map 1:1 to global orderIndex — reject reorder
        // while a filter is active to avoid corrupting oscillator order.
        if (currentFilterMode_ != OscillatorFilterMode::All)
            return;

        if (sourceIndex < targetIndex)
        {
            targetIndex--;
        }

        listeners_.call([sourceIndex, targetIndex](Listener& l) { l.oscillatorsReordered(sourceIndex, targetIndex); });
    }
}

void OscillatorListComponent::oscillatorMoveRequested(const OscillatorId& id, int direction)
{
    // Reject moves while a filter is active — same rationale as itemDropped.
    if (currentFilterMode_ != OscillatorFilterMode::All)
        return;

    // Find current index
    int currentIndex = -1;
    for (size_t i = 0; i < items_.size(); ++i)
    {
        if (items_[i]->getOscillatorId() == id)
        {
            currentIndex = static_cast<int>(i);
            break;
        }
    }

    if (currentIndex == -1)
        return;

    int const newIndex = currentIndex + direction;
    if (newIndex >= 0 && std::cmp_less(newIndex, items_.size()))
    {
        listeners_.call([currentIndex, newIndex](Listener& l) { l.oscillatorsReordered(currentIndex, newIndex); });
    }
}

void OscillatorListComponent::oscillatorPaneSelectionRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorPaneSelectionRequested(id); });
}

void OscillatorListComponent::oscillatorNameChanged(const OscillatorId& id, const juce::String& newName)
{
    listeners_.call([id, newName](Listener& l) { l.oscillatorNameChanged(id, newName); });
}

int OscillatorListComponent::getItemIndexAtY(int y) const
{
    if (items_.empty())
        return 0;

    int currentY = 0;
    for (size_t i = 0; i < items_.size(); ++i)
    {
        int const height = items_[i]->getHeight();
        if (y < currentY + (height / 2))
        {
            return static_cast<int>(i);
        }
        currentY += height;
    }

    return static_cast<int>(items_.size());
}

void OscillatorListComponent::updateDragIndicator(int targetIndex)
{
    if (dragTargetIndex_ != targetIndex)
    {
        dragTargetIndex_ = targetIndex;
        repaint();
    }
}

void OscillatorListComponent::filterModeChanged(OscillatorFilterMode mode)
{
    currentFilterMode_ = mode;
    refreshList(allOscillators_);
}

void OscillatorListComponent::addListener(Listener* listener) { listeners_.add(listener); }

void OscillatorListComponent::removeListener(Listener* listener) { listeners_.remove(listener); }

} // namespace oscil
