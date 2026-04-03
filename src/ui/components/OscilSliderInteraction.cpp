/*
    Oscil - Slider Component Interaction Handling
    (Mouse events, keyboard, focus, hit testing, animations, accessibility)
    Property setters are in OscilSlider.cpp, painting in OscilSliderPainting.cpp
*/

#include "ui/components/OscilSlider.h"

#include <cmath>

namespace oscil
{

void OscilSlider::mouseEnter(const juce::MouseEvent&)
{
    if (!enabled_)
        return;

    isHovered_ = true;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        thumbScale_.setTarget(1.1f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        currentThumbScale_ = 1.1f;
    }

    repaint();
}

void OscilSlider::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;

    if (!isDragging_)
    {
        if (AnimationSettings::shouldUseSpringAnimations())
        {
            thumbScale_.setTarget(1.0f);
            startTimerHz(ComponentLayout::ANIMATION_FPS);
        }
        else
        {
            currentThumbScale_ = 1.0f;
        }
    }

    repaint();
}

void OscilSlider::mouseDown(const juce::MouseEvent& e)
{
    if (!enabled_)
        return;

    isDragging_ = true;
    dragStartPoint_ = e.getPosition();

    if (variant_ == SliderVariant::Range)
    {
        // Range sliders are always horizontal — SliderVariant::Range and ::Vertical are mutually exclusive.
        constexpr bool isVertical = false;
        float startThumbPos = getThumbPosition(false);
        float endThumbPos = getThumbPosition(true);

        bool hitStart = hitTestThumb(e.getPosition(), startThumbPos, isVertical);
        bool hitEnd = hitTestThumb(e.getPosition(), endThumbPos, isVertical);

        if (hitEnd && !hitStart)
            draggingRangeEnd_ = true;
        else if (hitStart && !hitEnd)
            draggingRangeEnd_ = false;
        else
        {
            float mousePos = isVertical ? static_cast<float>(e.getPosition().y) : static_cast<float>(e.getPosition().x);
            draggingRangeEnd_ = std::abs(mousePos - endThumbPos) < std::abs(mousePos - startThumbPos);
        }

        dragStartValue_ = draggingRangeEnd_ ? rangeEnd_ : rangeStart_;
    }
    else
    {
        dragStartValue_ = value_;
    }

    if (onDragStart)
        onDragStart();

    repaint();
}

void OscilSlider::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging_ || !enabled_)
        return;

    auto bounds = getLocalBounds();
    bool isVertical = variant_ == SliderVariant::Vertical;

    float proportion;
    if (isVertical)
    {
        float trackHeight = std::max(1.0f, static_cast<float>(bounds.getHeight()) - THUMB_SIZE);
        float relY = static_cast<float>(bounds.getBottom()) - THUMB_SIZE / 2.0f - static_cast<float>(e.getPosition().y);
        proportion = relY / trackHeight;
    }
    else
    {
        float trackWidth = std::max(1.0f, static_cast<float>(bounds.getWidth()) - THUMB_SIZE);
        float relX = static_cast<float>(e.getPosition().x) - THUMB_SIZE / 2.0f;
        proportion = relX / trackWidth;
    }

    if (e.mods.isAltDown())
    {
        float delta = proportion - static_cast<float>(valueToProportionOfLength(dragStartValue_));
        delta *= ComponentLayout::FINE_CONTROL_FACTOR;
        proportion = static_cast<float>(valueToProportionOfLength(dragStartValue_)) + delta;
    }

    proportion = juce::jlimit(0.0f, 1.0f, proportion);
    double newValue = proportionOfLengthToValue(proportion);

    if (!e.mods.isShiftDown())
    {
        bool didSnap = false;
        newValue = snapController_.applySnapping(newValue, minValue_, maxValue_, didSnap);
        if (didSnap)
            triggerSnapFeedback();
    }

    if (variant_ == SliderVariant::Range)
    {
        if (draggingRangeEnd_)
            setRangeValues(rangeStart_, newValue, true);
        else
            setRangeValues(newValue, rangeEnd_, true);
    }
    else
    {
        setValue(newValue, true);
    }
}

void OscilSlider::mouseUp(const juce::MouseEvent&)
{
    if (!isDragging_)
        return;

    isDragging_ = false;

    if (!isHovered_)
    {
        if (AnimationSettings::shouldUseSpringAnimations())
        {
            thumbScale_.setTarget(1.0f);
            startTimerHz(ComponentLayout::ANIMATION_FPS);
        }
        else
        {
            currentThumbScale_ = 1.0f;
        }
    }

    if (onDragEnd)
        onDragEnd();

    repaint();
}

void OscilSlider::mouseDoubleClick(const juce::MouseEvent&)
{
    if (!enabled_)
        return;
    setValue(defaultValue_, true);
}

void OscilSlider::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (!enabled_)
        return;

    double delta = wheel.deltaY * step_;

    if (e.mods.isAltDown())
        delta *= ComponentLayout::FINE_CONTROL_FACTOR;

    if (e.mods.isShiftDown())
        delta *= ComponentLayout::COARSE_CONTROL_FACTOR;

    setValue(value_ + delta, true);
}

bool OscilSlider::keyPressed(const juce::KeyPress& key)
{
    if (!enabled_)
        return false;

    double delta = 0.0;

    if (key == juce::KeyPress::leftKey || key == juce::KeyPress::downKey)
        delta = -step_;
    else if (key == juce::KeyPress::rightKey || key == juce::KeyPress::upKey)
        delta = step_;
    else if (key == juce::KeyPress::homeKey)
    {
        setValue(minValue_, true);
        return true;
    }
    else if (key == juce::KeyPress::endKey)
    {
        setValue(maxValue_, true);
        return true;
    }

    if (std::abs(delta) > 1e-9)
    {
        if (key.getModifiers().isShiftDown())
            delta *= ComponentLayout::COARSE_CONTROL_FACTOR;

        setValue(value_ + delta, true);
        return true;
    }

    return false;
}

void OscilSlider::focusGained(FocusChangeType)
{
    hasFocus_ = true;
    repaint();
}

void OscilSlider::focusLost(FocusChangeType)
{
    hasFocus_ = false;
    repaint();
}

bool OscilSlider::hitTest(int x, int y)
{
    auto bounds = getLocalBounds().toFloat();
    bool isVertical = variant_ == SliderVariant::Vertical;

    float expansion = THUMB_HIT_EXTRA;
    juce::Rectangle<float> hitArea;

    if (isVertical)
        hitArea = bounds.expanded(expansion, 0);
    else
        hitArea = bounds.expanded(0, expansion);

    return hitArea.contains(static_cast<float>(x), static_cast<float>(y));
}

void OscilSlider::timerCallback()
{
    updateAnimations();

    if (thumbScale_.isSettled())
    {
        stopTimer();
        justSnapped_ = false;
    }

    repaint();
}

void OscilSlider::updateAnimations()
{
    float dt = AnimationTiming::FRAME_DURATION_60FPS;
    thumbScale_.update(dt);
    currentThumbScale_ = thumbScale_.position;
}

void OscilSlider::triggerSnapFeedback()
{
    justSnapped_ = true;
    repaint();
}

bool OscilSlider::hitTestThumb(const juce::Point<int>& point, float thumbPosition, bool isVertical) const
{
    float size = THUMB_SIZE + THUMB_HIT_EXTRA * 2;

    if (isVertical)
    {
        float cx = static_cast<float>(getWidth()) / 2.0f;
        return juce::Rectangle<float>(cx - size / 2, thumbPosition - size / 2, size, size).contains(point.toFloat());
    }
    else
    {
        float cy = static_cast<float>(getHeight()) / 2.0f;
        return juce::Rectangle<float>(thumbPosition - size / 2, cy - size / 2, size, size).contains(point.toFloat());
    }
}

// Accessibility handler
class OscilSliderAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit OscilSliderAccessibilityHandler(OscilSlider& slider)
        : juce::AccessibilityHandler(
              slider, juce::AccessibilityRole::slider,
              juce::AccessibilityActions().addAction(juce::AccessibilityActionType::press,
                                                     [&slider] { slider.setValue(slider.getDefaultValue(), true); }))
        , slider_(slider)
    {
    }

    juce::String getTitle() const override
    {
        if (slider_.getLabel().isNotEmpty())
            return slider_.getLabel();
        return "Slider";
    }

    juce::String getDescription() const override
    {
        juce::String text = juce::String(slider_.getValue(), slider_.getDecimalPlaces());
        if (slider_.getSuffix().isNotEmpty())
            text += " " + slider_.getSuffix();

        juce::String range = " (Range: " + juce::String(slider_.getMinimum(), slider_.getDecimalPlaces()) + " to " +
                             juce::String(slider_.getMaximum(), slider_.getDecimalPlaces()) + ")";

        return "Value: " + text + range;
    }

    juce::String getHelp() const override { return "Use arrow keys to adjust. Double-click to reset to default."; }

private:
    OscilSlider& slider_;
};

std::unique_ptr<juce::AccessibilityHandler> OscilSlider::createAccessibilityHandler()
{
    return std::make_unique<OscilSliderAccessibilityHandler>(*this);
}

} // namespace oscil
