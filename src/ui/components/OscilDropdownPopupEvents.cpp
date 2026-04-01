/*
    Oscil - Dropdown Popup Event Handling
    (Setup and inner widgets are in OscilDropdownPopup.cpp)
*/

#include "ui/components/OscilDropdown.h"

namespace oscil
{

void OscilDropdownPopup::mouseDown(const juce::MouseEvent&) { dismiss(); }
void OscilDropdownPopup::mouseMove(const juce::MouseEvent&) {}
void OscilDropdownPopup::mouseExit(const juce::MouseEvent&) {}

void OscilDropdownPopup::focusLost(FocusChangeType)
{
    auto* focusedComp = juce::Component::getCurrentlyFocusedComponent();
    if (focusedComp != this && !isParentOf(focusedComp))
        dismiss();
}

void OscilDropdownPopup::ensureItemVisible(int index)
{
    if (!viewport_ || index < 0)
        return;
    int itemY = index * ITEM_HEIGHT;
    int viewY = viewport_->getViewPositionY();
    int viewH = viewport_->getViewHeight();

    if (itemY < viewY)
        viewport_->setViewPosition(viewport_->getViewPositionX(), itemY);
    else if (itemY + ITEM_HEIGHT > viewY + viewH)
        viewport_->setViewPosition(viewport_->getViewPositionX(), itemY + ITEM_HEIGHT - viewH);
}

} // namespace oscil
