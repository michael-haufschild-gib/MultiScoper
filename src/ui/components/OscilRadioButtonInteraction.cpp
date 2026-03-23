/*
    Oscil - Radio Button Component Interaction & Events
    (Core setup is in OscilRadioButton.cpp, painting is in OscilRadioButtonPainting.cpp)
*/

#include "ui/components/OscilRadioButton.h"

namespace oscil
{

void OscilRadioButton::mouseDown(const juce::MouseEvent&)
{
    if (enabled_)
        isPressed_ = true;
}

void OscilRadioButton::mouseUp(const juce::MouseEvent& e)
{
    if (isPressed_ && enabled_ && contains(e.getPosition()))
    {
        if (!selected_)
        {
            if (parentGroup_)
            {
                // Let the group handle selection
                int myIndex = -1;
                for (int i = 0; i < parentGroup_->getNumOptions(); ++i)
                {
                    if (parentGroup_->getButton(i) == this)
                    {
                        myIndex = i;
                        break;
                    }
                }
                if (myIndex >= 0)
                    parentGroup_->setSelectedIndex(myIndex);
            }
            else
            {
                setSelected(true);
            }
        }
    }
    isPressed_ = false;
}

void OscilRadioButton::mouseEnter(const juce::MouseEvent&)
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

void OscilRadioButton::mouseExit(const juce::MouseEvent&)
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

bool OscilRadioButton::keyPressed(const juce::KeyPress& key)
{
    if (enabled_ && (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey))
    {
        if (!selected_)
        {
            if (parentGroup_)
            {
                int myIndex = -1;
                for (int i = 0; i < parentGroup_->getNumOptions(); ++i)
                {
                    if (parentGroup_->getButton(i) == this)
                    {
                        myIndex = i;
                        break;
                    }
                }
                if (myIndex >= 0)
                    parentGroup_->setSelectedIndex(myIndex);
            }
            else
            {
                setSelected(true);
            }
        }
        return true;
    }
    return false;
}

void OscilRadioButton::focusGained(FocusChangeType)
{
    hasFocus_ = true;
    repaint();
}

void OscilRadioButton::focusLost(FocusChangeType)
{
    hasFocus_ = false;
    repaint();
}

void OscilRadioButton::timerCallback()
{
    updateAnimations();

    if (selectionSpring_.isSettled() && hoverSpring_.isSettled())
        stopTimer();

    repaint();
}

void OscilRadioButton::updateAnimations()
{
    float dt = AnimationTiming::FRAME_DURATION_60FPS;
    selectionSpring_.update(dt);
    hoverSpring_.update(dt);
}

std::unique_ptr<juce::AccessibilityHandler> OscilRadioButton::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this, juce::AccessibilityRole::radioButton,
        juce::AccessibilityActions().addAction(juce::AccessibilityActionType::press, [this] {
            if (enabled_ && !selected_)
                setSelected(true);
        }));
}

} // namespace oscil
