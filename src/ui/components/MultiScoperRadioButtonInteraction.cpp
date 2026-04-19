/*
    MultiScoper - Radio Button Component Interaction & Events
    (Core setup is in MultiScoperRadioButton.cpp, painting is in MultiScoperRadioButtonPainting.cpp)
*/

#include "ui/components/MultiScoperRadioButton.h"

namespace multiscoper
{

void MultiScoperRadioButton::triggerSelection()
{
    if (!enabled_ || selected_)
        return;

    if (parentGroup_)
    {
        int const myIndex = findOwnIndexInGroup();
        if (myIndex >= 0)
            parentGroup_->setSelectedIndex(myIndex);
    }
    else
    {
        setSelected(true);
    }
}

void MultiScoperRadioButton::mouseDown(const juce::MouseEvent& /*event*/)
{
    if (enabled_)
        isPressed_ = true;
}

void MultiScoperRadioButton::mouseUp(const juce::MouseEvent& e)
{
    if (isPressed_ && enabled_ && contains(e.getPosition()))
        triggerSelection();
    isPressed_ = false;
}

void MultiScoperRadioButton::mouseEnter(const juce::MouseEvent& /*event*/)
{
    if (!enabled_)
        return;

    isHovered_ = true;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        hoverSpring_.setTarget(1.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        hoverSpring_.position = 1.0f;
        repaint();
    }
}

void MultiScoperRadioButton::mouseExit(const juce::MouseEvent& /*event*/)
{
    isHovered_ = false;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        hoverSpring_.setTarget(0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        hoverSpring_.position = 0.0f;
        repaint();
    }
}

int MultiScoperRadioButton::findOwnIndexInGroup() const
{
    if (!parentGroup_)
        return -1;

    for (int i = 0; i < parentGroup_->getNumOptions(); ++i)
    {
        if (parentGroup_->getButton(i) == this)
            return i;
    }
    return -1;
}

void MultiScoperRadioButton::focusGained(FocusChangeType /*cause*/)
{
    hasFocus_ = true;
    repaint();
}

void MultiScoperRadioButton::focusLost(FocusChangeType /*cause*/)
{
    hasFocus_ = false;
    repaint();
}

void MultiScoperRadioButton::timerCallback()
{
    updateAnimations();

    if (selectionSpring_.isSettled() && hoverSpring_.isSettled() && scaleSpring_.isSettled())
        stopTimer();

    repaint();
}

void MultiScoperRadioButton::updateAnimations()
{
    float const dt = AnimationTiming::FRAME_DURATION_60FPS;
    selectionSpring_.update(dt);
    hoverSpring_.update(dt);
    scaleSpring_.update(dt);
}

std::unique_ptr<juce::AccessibilityHandler> MultiScoperRadioButton::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this, juce::AccessibilityRole::radioButton,
        juce::AccessibilityActions().addAction(juce::AccessibilityActionType::press, [this] { triggerSelection(); }));
}

} // namespace multiscoper
