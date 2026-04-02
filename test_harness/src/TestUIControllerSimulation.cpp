/*
    Oscil Test Harness - UI Controller: Simulation Methods & JSON Serialization
*/

#include "ui/components/InlineEditLabel.h"
#include "ui/components/OscilAccordion.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/OscilSlider.h"
#include "ui/components/OscilTextField.h"
#include "ui/components/OscilToggle.h"
#include "ui/layout/SidebarComponent.h"
#include "ui/panels/OscillatorListComponent.h"
#include "ui/panels/OscillatorListItem.h"

#include "TestUIController.h"

namespace oscil::test
{

// ================== Private Simulation Methods ==================

bool TestUIController::tryClickFastPath(juce::Component* component)
{
    if (auto* oscilButton = dynamic_cast<OscilButton*>(component))
    {
        oscilButton->triggerClick();
        return true;
    }
    if (auto* toggle = dynamic_cast<oscil::OscilToggle*>(component))
    {
        toggle->toggle();
        return true;
    }
    if (auto* accordion = dynamic_cast<oscil::OscilAccordionSection*>(component))
    {
        accordion->toggle();
        return true;
    }
    return false;
}

void TestUIController::simulateMouseClick(juce::Component* component, bool doubleClick, const juce::ModifierKeys& mods)
{
    if (component == nullptr)
        return;

    // Fast path: call widget APIs directly for reliable test automation
    if (!doubleClick && mods.getRawFlags() == 0 && tryClickFastPath(component))
        return;

    auto center = component->getLocalBounds().getCentre();
    auto mouseSource = juce::Desktop::getInstance().getMainMouseSource();
    auto clickMods = mods.getRawFlags() == 0 ? juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier) : mods;

    juce::MouseEvent mouseDown(mouseSource, center.toFloat(), clickMods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, component,
                               component, juce::Time::getCurrentTime(), center.toFloat(), juce::Time::getCurrentTime(),
                               doubleClick ? 2 : 1, false);

    component->mouseDown(mouseDown);
    component->mouseUp(mouseDown);

    if (doubleClick)
        component->mouseDoubleClick(mouseDown);
}

void TestUIController::simulateMouseRightClick(juce::Component* component)
{
    if (component == nullptr)
        return;

    auto bounds = component->getLocalBounds();
    auto center = bounds.getCentre();
    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();

    juce::ModifierKeys mods(juce::ModifierKeys::popupMenuClickModifier);

    juce::MouseEvent mouseDown(mouseSource, center.toFloat(), mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, component, component,
                               juce::Time::getCurrentTime(), center.toFloat(), juce::Time::getCurrentTime(), 1, false);

    component->mouseDown(mouseDown);
    component->mouseUp(mouseDown);
}

bool TestUIController::tryDragFastPathListItems(juce::Component* from, juce::Component* to)
{
    auto* fromItem = dynamic_cast<oscil::OscillatorListItemComponent*>(from);
    auto* toItem = dynamic_cast<oscil::OscillatorListItemComponent*>(to);
    if (!fromItem || !toItem)
        return false;

    for (auto* p = from->getParentComponent(); p != nullptr; p = p->getParentComponent())
    {
        if (auto* list = dynamic_cast<oscil::OscillatorListComponent*>(p))
        {
            int direction = (to->getY() > from->getY()) ? 1 : -1;
            list->oscillatorMoveRequested(fromItem->getOscillatorId(), direction);
            return true;
        }
    }
    return false;
}

void TestUIController::simulateMouseDrag(juce::Component* from, juce::Component* to, const juce::ModifierKeys& mods)
{
    if (from == nullptr || to == nullptr)
        return;

    // Fast path: JUCE DragAndDropContainer needs OS-level events — use API directly
    if (tryDragFastPathListItems(from, to))
        return;

    auto fromCenter = from->getLocalBounds().getCentre();
    auto toCenter = to->getScreenBounds().getCentre() - from->getScreenPosition();
    auto mouseSource = juce::Desktop::getInstance().getMainMouseSource();

    juce::MouseEvent mouseDown(mouseSource, fromCenter.toFloat(), mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, from, from,
                               juce::Time::getCurrentTime(), fromCenter.toFloat(), juce::Time::getCurrentTime(), 1,
                               false);
    from->mouseDown(mouseDown);

    juce::MouseEvent mouseDrag(mouseSource, toCenter.toFloat(), mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, from, from,
                               juce::Time::getCurrentTime(), fromCenter.toFloat(), juce::Time::getCurrentTime(), 1,
                               true);
    from->mouseDrag(mouseDrag);
    from->mouseUp(mouseDrag);
}

bool TestUIController::tryDragOffsetFastPathSidebar(juce::Component* component, int deltaX)
{
    auto* handle = dynamic_cast<oscil::SidebarResizeHandle*>(component);
    if (!handle)
        return false;

    juce::Logger::writeToLog("[DragOffset] SidebarResizeHandle fast path: deltaX=" + juce::String(deltaX));
    if (handle->onResizeStart)
        handle->onResizeStart();
    if (handle->onResizeDrag)
        handle->onResizeDrag(deltaX);
    if (handle->onResizeEnd)
        handle->onResizeEnd();

    // Snap the sidebar's width spring so getEffectiveWidth() reflects the new width immediately
    for (auto* p = component->getParentComponent(); p != nullptr; p = p->getParentComponent())
    {
        if (auto* sidebar = dynamic_cast<oscil::SidebarComponent*>(p))
        {
            sidebar->snapWidthToTarget();
            break;
        }
    }
    return true;
}

void TestUIController::simulateMouseDragOffset(juce::Component* component, int deltaX, int deltaY,
                                               const juce::ModifierKeys& mods)
{
    if (component == nullptr)
        return;

    if (tryDragOffsetFastPathSidebar(component, deltaX))
        return;

    auto center = component->getLocalBounds().getCentre();
    auto endPoint = center + juce::Point<int>(deltaX, deltaY);
    auto mouseSource = juce::Desktop::getInstance().getMainMouseSource();

    juce::MouseEvent mouseDown(mouseSource, center.toFloat(), mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, component, component,
                               juce::Time::getCurrentTime(), center.toFloat(), juce::Time::getCurrentTime(), 1, false);
    component->mouseDown(mouseDown);

    juce::MouseEvent mouseDrag(mouseSource, endPoint.toFloat(), mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, component,
                               component, juce::Time::getCurrentTime(), center.toFloat(), juce::Time::getCurrentTime(),
                               1, true);
    component->mouseDrag(mouseDrag);
    component->mouseUp(mouseDrag);
}

void TestUIController::simulateMouseWheel(juce::Component* component, float deltaX, float deltaY,
                                          const juce::ModifierKeys& mods)
{
    if (component == nullptr)
        return;

    auto center = component->getLocalBounds().getCentre();
    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();

    juce::MouseEvent mouseEvent(mouseSource, center.toFloat(), mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, component, component,
                                juce::Time::getCurrentTime(), center.toFloat(), juce::Time::getCurrentTime(), 1, false);

    juce::MouseWheelDetails wheel;
    wheel.deltaX = deltaX;
    wheel.deltaY = deltaY;
    wheel.isReversed = false;
    wheel.isSmooth = true;
    wheel.isInertial = false;

    component->mouseWheelMove(mouseEvent, wheel);
}

void TestUIController::simulateMouseHover(juce::Component* component)
{
    if (component == nullptr)
        return;

    auto center = component->getLocalBounds().getCentre();
    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();

    juce::MouseEvent mouseEvent(mouseSource, center.toFloat(), juce::ModifierKeys(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                component, component, juce::Time::getCurrentTime(), center.toFloat(),
                                juce::Time::getCurrentTime(), 0, false);

    component->mouseEnter(mouseEvent);
    component->mouseMove(mouseEvent);
}

void TestUIController::simulateKeyPress(juce::Component* component, const juce::KeyPress& key)
{
    if (component == nullptr)
        return;

    // Propagate up the parent chain, mirroring JUCE's normal key event
    // dispatch: if a component doesn't handle the key, its parent gets
    // a chance.
    for (auto* c = component; c != nullptr; c = c->getParentComponent())
    {
        if (c->keyPressed(key))
            return;
    }
}

// ================== JSON Serialization ==================

namespace
{

json buildDropdownItems(oscil::OscilDropdown* dropdown)
{
    json items = json::array();
    for (int i = 0; i < dropdown->getNumItems(); ++i)
    {
        const auto& item = dropdown->getItem(i);
        items.push_back({{"id", item.id.toStdString()}, {"text", item.label.toStdString()}});
    }
    return items;
}

json buildComboBoxItems(juce::ComboBox* comboBox)
{
    json items = json::array();
    for (int i = 0; i < comboBox->getNumItems(); ++i)
        items.push_back({{"id", comboBox->getItemId(i)}, {"text", comboBox->getItemText(i).toStdString()}});
    return items;
}

} // anonymous namespace

bool TestUIController::appendOscilTypeInfo(json& info, juce::Component* component)
{
    if (auto* toggle = dynamic_cast<oscil::OscilToggle*>(component))
    {
        info["type"] = "toggle";
        info["toggled"] = toggle->getValue();
        info["value"] = toggle->getValue();
    }
    else if (auto* oscilSlider = dynamic_cast<oscil::OscilSlider*>(component))
    {
        info["type"] = "oscilSlider";
        info["value"] = oscilSlider->getValue();
    }
    else if (auto* oscilText = dynamic_cast<oscil::OscilTextField*>(component))
    {
        info["type"] = "oscilTextField";
        info["text"] = oscilText->getText().toStdString();
        info["value"] = oscilText->getText().toStdString();
    }
    else if (auto* oscilDropdown = dynamic_cast<oscil::OscilDropdown*>(component))
    {
        info["type"] = "combobox";
        info["selectedId"] = oscilDropdown->getSelectedId().toStdString();
        info["selectedText"] = oscilDropdown->getSelectedLabel().toStdString();
        info["numItems"] = oscilDropdown->getNumItems();
        info["items"] = buildDropdownItems(oscilDropdown);
    }
    else if (auto* accordion = dynamic_cast<oscil::OscilAccordionSection*>(component))
    {
        info["type"] = "accordion";
        info["expanded"] = accordion->isExpanded();
    }
    else if (auto* inlineLabel = dynamic_cast<oscil::InlineEditLabel*>(component))
    {
        info["type"] = "inlineEditLabel";
        info["text"] = inlineLabel->getText().toStdString();
        info["editable"] = true;
    }
    else
    {
        return false;
    }
    return true;
}

void TestUIController::appendComponentTypeInfo(json& info, juce::Component* component)
{
    if (appendOscilTypeInfo(info, component))
        return;

    if (auto* button = dynamic_cast<juce::Button*>(component))
    {
        info["type"] = "button";
        info["text"] = button->getButtonText().toStdString();
        info["toggled"] = button->getToggleState();
        info["toggleable"] = button->getClickingTogglesState();
    }
    else if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
    {
        info["type"] = "combobox";
        info["selectedId"] = comboBox->getSelectedId();
        info["selectedText"] = comboBox->getText().toStdString();
        info["numItems"] = comboBox->getNumItems();
        info["items"] = buildComboBoxItems(comboBox);
    }
    else if (auto* slider = dynamic_cast<juce::Slider*>(component))
    {
        info["type"] = "slider";
        info["value"] = slider->getValue();
        info["minimum"] = slider->getMinimum();
        info["maximum"] = slider->getMaximum();
        info["interval"] = slider->getInterval();
        info["skewFactor"] = slider->getSkewFactor();
    }
    else if (auto* label = dynamic_cast<juce::Label*>(component))
    {
        info["type"] = "label";
        info["text"] = label->getText().toStdString();
        info["editable"] = label->isEditable();
    }
    else if (auto* textEditor = dynamic_cast<juce::TextEditor*>(component))
    {
        info["type"] = "textEditor";
        info["text"] = textEditor->getText().toStdString();
        info["readOnly"] = textEditor->isReadOnly();
        info["multiLine"] = textEditor->isMultiLine();
    }
    else
    {
        info["type"] = "component";
        info["componentName"] = component->getName().toStdString();
        info["componentId"] = component->getComponentID().toStdString();
    }
}

json TestUIController::componentToJson(juce::Component* component, const juce::String& testId)
{
    json info;
    info["testId"] = testId.toStdString();
    info["visible"] = component->isVisible();
    info["showing"] = component->isShowing();
    info["enabled"] = component->isEnabled();
    info["hasFocus"] = component->hasKeyboardFocus(false);
    info["focusable"] = component->getWantsKeyboardFocus();

    auto bounds = component->getBounds();
    info["bounds"] = {
        {"x", bounds.getX()}, {"y", bounds.getY()}, {"width", bounds.getWidth()}, {"height", bounds.getHeight()}};

    auto screenBounds = component->getScreenBounds();
    info["screenBounds"] = {{"x", screenBounds.getX()},
                            {"y", screenBounds.getY()},
                            {"width", screenBounds.getWidth()},
                            {"height", screenBounds.getHeight()}};

    appendComponentTypeInfo(info, component);
    return info;
}

} // namespace oscil::test
