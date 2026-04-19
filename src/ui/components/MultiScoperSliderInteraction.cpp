/*
    MultiScoper - Slider Component Interaction Handling
    (Mouse events, keyboard, focus, hit testing, animations, accessibility)
    Property setters are in MultiScoperSlider.cpp, painting in MultiScoperSliderPainting.cpp
*/

#include "ui/components/MultiScoperSlider.h"

#include <cmath>

namespace multiscoper
{

void MultiScoperSlider::mouseEnter(const juce::MouseEvent& /*event*/)
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

void MultiScoperSlider::mouseExit(const juce::MouseEvent& /*event*/)
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

void MultiScoperSlider::mouseDown(const juce::MouseEvent& e)
{
    if (!enabled_)
        return;

    isDragging_ = true;
    dragStartPoint_ = e.getPosition();

    if (variant_ == SliderVariant::Range)
    {
        // Range sliders are always horizontal — SliderVariant::Range and ::Vertical are mutually exclusive.
        constexpr bool isVertical = false;
        float const startThumbPos = getThumbPosition(false);
        float const endThumbPos = getThumbPosition(true);

        bool const hitStart = hitTestThumb(e.getPosition(), startThumbPos, isVertical);
        bool const hitEnd = hitTestThumb(e.getPosition(), endThumbPos, isVertical);

        if (hitEnd && !hitStart)
        {
            draggingRangeEnd_ = true;
        }
        else if (hitStart && !hitEnd)
        {
            draggingRangeEnd_ = false;
        }
        else
        {
            float const mousePos =
                isVertical ? static_cast<float>(e.getPosition().y) : static_cast<float>(e.getPosition().x);
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

void MultiScoperSlider::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging_ || !enabled_)
        return;

    auto bounds = getLocalBounds();
    bool const isVertical = variant_ == SliderVariant::Vertical;

    float proportion = 0.0f;
    if (variant_ == SliderVariant::Rotary)
    {
        // Vertical pixel delta from the drag-start point drives the knob: up = increase.
        constexpr float PIXELS_PER_FULL_SWEEP = 150.0f;
        float const dy = static_cast<float>(dragStartPoint_.y - e.getPosition().y);
        auto const startProp = static_cast<float>(valueToProportionOfLength(dragStartValue_));
        proportion = startProp + (dy / PIXELS_PER_FULL_SWEEP);
    }
    else if (isVertical)
    {
        float const trackHeight = std::max(1.0f, static_cast<float>(bounds.getHeight()) - THUMB_SIZE);
        float const relY =
            static_cast<float>(bounds.getBottom()) - (THUMB_SIZE / 2.0f) - static_cast<float>(e.getPosition().y);
        proportion = relY / trackHeight;
    }
    else
    {
        float const trackWidth = std::max(1.0f, static_cast<float>(bounds.getWidth()) - THUMB_SIZE);
        float const relX = static_cast<float>(e.getPosition().x) - (THUMB_SIZE / 2.0f);
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

void MultiScoperSlider::mouseUp(const juce::MouseEvent& /*event*/)
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

void MultiScoperSlider::mouseDoubleClick(const juce::MouseEvent& /*event*/)
{
    if (!enabled_)
        return;
    setValue(defaultValue_, true);
}

void MultiScoperSlider::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
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

bool MultiScoperSlider::keyPressed(const juce::KeyPress& key)
{
    if (!enabled_)
        return false;

    double delta = 0.0;

    if (key == juce::KeyPress::leftKey || key == juce::KeyPress::downKey)
    {
        delta = -step_;
    }
    else if (key == juce::KeyPress::rightKey || key == juce::KeyPress::upKey)
    {
        delta = step_;
    }
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

void MultiScoperSlider::focusGained(FocusChangeType /*cause*/)
{
    hasFocus_ = true;
    repaint();
}

void MultiScoperSlider::focusLost(FocusChangeType /*cause*/)
{
    hasFocus_ = false;
    repaint();
}

bool MultiScoperSlider::hitTest(int x, int y)
{
    auto bounds = getLocalBounds().toFloat();
    bool const isVertical = variant_ == SliderVariant::Vertical;

    float const expansion = THUMB_HIT_EXTRA;
    juce::Rectangle<float> hitArea;

    if (isVertical)
        hitArea = bounds.expanded(expansion, 0);
    else
        hitArea = bounds.expanded(0, expansion);

    return hitArea.contains(static_cast<float>(x), static_cast<float>(y));
}

void MultiScoperSlider::timerCallback()
{
    updateAnimations();

    if (thumbScale_.isSettled())
    {
        stopTimer();
        justSnapped_ = false;
    }

    repaint();
}

void MultiScoperSlider::updateAnimations()
{
    float const dt = AnimationTiming::FRAME_DURATION_60FPS;
    thumbScale_.update(dt);
    currentThumbScale_ = thumbScale_.position;
}

void MultiScoperSlider::triggerSnapFeedback()
{
    justSnapped_ = true;
    repaint();
}

bool MultiScoperSlider::hitTestThumb(const juce::Point<int>& point, float thumbPosition, bool isVertical) const
{
    float const size = THUMB_SIZE + (THUMB_HIT_EXTRA * 2);

    if (isVertical)
    {
        float const cx = static_cast<float>(getWidth()) / 2.0f;
        return juce::Rectangle<float>(cx - (size / 2), thumbPosition - (size / 2), size, size)
            .contains(point.toFloat());
    }

    float const cy = static_cast<float>(getHeight()) / 2.0f;
    return juce::Rectangle<float>(thumbPosition - size / 2, cy - size / 2, size, size).contains(point.toFloat());
}

// Accessibility handler
class MultiScoperSliderAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit MultiScoperSliderAccessibilityHandler(MultiScoperSlider& slider)
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

        juce::String const range = " (Range: " + juce::String(slider_.getMinimum(), slider_.getDecimalPlaces()) +
                                   " to " + juce::String(slider_.getMaximum(), slider_.getDecimalPlaces()) + ")";

        return "Value: " + text + range;
    }

    juce::String getHelp() const override { return "Use arrow keys to adjust. Double-click to reset to default."; }

private:
    MultiScoperSlider& slider_;
};

std::unique_ptr<juce::AccessibilityHandler> MultiScoperSlider::createAccessibilityHandler()
{
    return std::make_unique<MultiScoperSliderAccessibilityHandler>(*this);
}

} // namespace multiscoper
