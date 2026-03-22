/*
    Oscil Test Harness - UI Controller: Simulation Methods & JSON Serialization
*/

#include "TestUIController.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilAccordion.h"

namespace oscil::test
{

// ================== Private Simulation Methods ==================

void TestUIController::simulateMouseClick(juce::Component* component, bool doubleClick,
                                           const juce::ModifierKeys& mods)
{
    if (component == nullptr)
        return;

    // Fast path: if the target is an OscilButton, call triggerClick() directly
    // for reliable test automation. Mouse event simulation can fail due to
    // coordinate transforms, component hierarchy differences, or JUCE internal
    // state that isn't properly set up in synthetic events.
    if (!doubleClick && mods.getRawFlags() == 0)
    {
        if (auto* oscilButton = dynamic_cast<OscilButton*>(component))
        {
            oscilButton->triggerClick();
            return;
        }

        // Accordion sections: toggle() bypasses mouse position checks
        if (auto* accordion = dynamic_cast<oscil::OscilAccordionSection*>(component))
        {
            accordion->toggle();
            return;
        }
    }

    auto bounds = component->getLocalBounds();
    auto center = bounds.getCentre();
    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();

    // Ensure left button modifier is set for a proper left-click simulation
    auto clickMods = mods.getRawFlags() == 0
        ? juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier)
        : mods;

    juce::MouseEvent mouseDown(
        mouseSource,
        center.toFloat(),
        clickMods,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        component, component,
        juce::Time::getCurrentTime(),
        center.toFloat(),
        juce::Time::getCurrentTime(),
        doubleClick ? 2 : 1,
        false
    );

    component->mouseDown(mouseDown);
    component->mouseUp(mouseDown);

    if (doubleClick)
    {
        component->mouseDoubleClick(mouseDown);
    }
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

    juce::MouseEvent mouseDown(
        mouseSource,
        center.toFloat(),
        mods,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        component, component,
        juce::Time::getCurrentTime(),
        center.toFloat(),
        juce::Time::getCurrentTime(),
        1, false
    );

    component->mouseDown(mouseDown);
    component->mouseUp(mouseDown);
}

void TestUIController::simulateMouseDrag(juce::Component* from, juce::Component* to,
                                          const juce::ModifierKeys& mods)
{
    if (from == nullptr || to == nullptr)
        return;

    auto fromCenter = from->getLocalBounds().getCentre();
    auto toCenter = to->getScreenBounds().getCentre() - from->getScreenPosition();
    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();

    juce::MouseEvent mouseDown(
        mouseSource,
        fromCenter.toFloat(),
        mods,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        from, from,
        juce::Time::getCurrentTime(),
        fromCenter.toFloat(),
        juce::Time::getCurrentTime(),
        1, false
    );

    from->mouseDown(mouseDown);

    juce::MouseEvent mouseDrag(
        mouseSource,
        toCenter.toFloat(),
        mods,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        from, from,
        juce::Time::getCurrentTime(),
        fromCenter.toFloat(),
        juce::Time::getCurrentTime(),
        1, true
    );

    from->mouseDrag(mouseDrag);
    from->mouseUp(mouseDrag);
}

void TestUIController::simulateMouseDragOffset(juce::Component* component, int deltaX, int deltaY,
                                                const juce::ModifierKeys& mods)
{
    if (component == nullptr)
        return;

    auto center = component->getLocalBounds().getCentre();
    auto endPoint = center + juce::Point<int>(deltaX, deltaY);
    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();

    juce::MouseEvent mouseDown(
        mouseSource,
        center.toFloat(),
        mods,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        component, component,
        juce::Time::getCurrentTime(),
        center.toFloat(),
        juce::Time::getCurrentTime(),
        1, false
    );

    component->mouseDown(mouseDown);

    juce::MouseEvent mouseDrag(
        mouseSource,
        endPoint.toFloat(),
        mods,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        component, component,
        juce::Time::getCurrentTime(),
        center.toFloat(),
        juce::Time::getCurrentTime(),
        1, true
    );

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

    juce::MouseEvent mouseEvent(
        mouseSource,
        center.toFloat(),
        mods,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        component, component,
        juce::Time::getCurrentTime(),
        center.toFloat(),
        juce::Time::getCurrentTime(),
        1, false
    );

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

    juce::MouseEvent mouseEvent(
        mouseSource,
        center.toFloat(),
        juce::ModifierKeys(),
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        component, component,
        juce::Time::getCurrentTime(),
        center.toFloat(),
        juce::Time::getCurrentTime(),
        0, false
    );

    component->mouseEnter(mouseEvent);
    component->mouseMove(mouseEvent);
}

void TestUIController::simulateKeyPress(juce::Component* component, const juce::KeyPress& key)
{
    if (component == nullptr)
        return;

    component->keyPressed(key);
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

void TestUIController::appendComponentTypeInfo(json& info, juce::Component* component)
{
    if (auto* button = dynamic_cast<juce::Button*>(component))
    {
        info["type"] = "button";
        info["text"] = button->getButtonText().toStdString();
        info["toggled"] = button->getToggleState();
        info["toggleable"] = button->getClickingTogglesState();
    }
    else if (auto* oscilDropdown = dynamic_cast<oscil::OscilDropdown*>(component))
    {
        info["type"] = "combobox";
        info["selectedId"] = oscilDropdown->getSelectedId().toStdString();
        info["selectedText"] = oscilDropdown->getSelectedLabel().toStdString();
        info["numItems"] = oscilDropdown->getNumItems();
        info["items"] = buildDropdownItems(oscilDropdown);
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
        {"x", bounds.getX()}, {"y", bounds.getY()},
        {"width", bounds.getWidth()}, {"height", bounds.getHeight()}
    };

    auto screenBounds = component->getScreenBounds();
    info["screenBounds"] = {
        {"x", screenBounds.getX()}, {"y", screenBounds.getY()},
        {"width", screenBounds.getWidth()}, {"height", screenBounds.getHeight()}
    };

    appendComponentTypeInfo(info, component);
    return info;
}

} // namespace oscil::test
