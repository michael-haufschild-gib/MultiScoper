/*
    MultiScoper - Dropdown Popup Event Handling
    (Setup and inner widgets are in MultiScoperDropdownPopup.cpp)
*/

#include "ui/components/MultiScoperDropdown.h"

namespace multiscoper
{

void MultiScoperDropdownPopup::mouseDown(const juce::MouseEvent& /*event*/) { dismiss(); }
void MultiScoperDropdownPopup::mouseMove(const juce::MouseEvent& /*event*/) {}
void MultiScoperDropdownPopup::mouseExit(const juce::MouseEvent& /*event*/) {}

void MultiScoperDropdownPopup::focusLost(FocusChangeType /*cause*/)
{
    auto* focusedComp = juce::Component::getCurrentlyFocusedComponent();
    if (focusedComp != this && !isParentOf(focusedComp))
        dismiss();
}

void MultiScoperDropdownPopup::ensureItemVisible(int index)
{
    if (!viewport_ || index < 0)
        return;
    int const itemY = index * ITEM_HEIGHT;
    int const viewY = viewport_->getViewPositionY();
    int const viewH = viewport_->getViewHeight();

    if (itemY < viewY)
        viewport_->setViewPosition(viewport_->getViewPositionX(), itemY);
    else if (itemY + ITEM_HEIGHT > viewY + viewH)
        viewport_->setViewPosition(viewport_->getViewPositionX(), itemY + ITEM_HEIGHT - viewH);
}

namespace
{
bool isFilteredIndexSelectable(const std::vector<int>& filtered, const std::vector<DropdownItem>& items,
                               int filterIndex)
{
    if (filterIndex < 0 || std::cmp_greater_equal(filterIndex, filtered.size()))
        return false;
    const auto& item = items[static_cast<size_t>(filtered[static_cast<size_t>(filterIndex)])];
    return !item.isSeparator && item.enabled;
}
} // namespace

void MultiScoperDropdownPopup::moveFocus(int direction)
{
    const int last = static_cast<int>(filteredIndices_.size()) - 1;
    int next = focusedIndex_ + direction;
    while (next >= 0 && next <= last && !isFilteredIndexSelectable(filteredIndices_, items_, next))
        next += direction;
    if (next >= 0 && next <= last)
        focusedIndex_ = next;
    ensureItemVisible(focusedIndex_);
    if (viewport_ && viewport_->getViewedComponent())
        viewport_->getViewedComponent()->repaint();
}

void MultiScoperDropdownPopup::activateFocusedItem()
{
    if (focusedIndex_ < 0 || std::cmp_greater_equal(focusedIndex_, filteredIndices_.size()))
        return;
    const int itemIndex = filteredIndices_[static_cast<size_t>(focusedIndex_)];
    const auto& item = items_[static_cast<size_t>(itemIndex)];
    if (item.isSeparator || !item.enabled || !onItemClicked)
        return;
    onItemClicked(itemIndex);
    if (!multiSelect_)
        dismiss();
}

bool MultiScoperDropdownPopup::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        dismiss();
        return true;
    }
    if (key == juce::KeyPress::upKey)
    {
        moveFocus(-1);
        return true;
    }
    if (key == juce::KeyPress::downKey)
    {
        moveFocus(+1);
        return true;
    }
    if (key == juce::KeyPress::returnKey)
    {
        activateFocusedItem();
        return true;
    }
    return false;
}

} // namespace multiscoper
