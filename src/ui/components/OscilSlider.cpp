/*
    Oscil - Slider Component Implementation
*/

#include "ui/components/OscilSlider.h"
#include <cmath>

namespace oscil
{

OscilSlider::OscilSlider()
    : thumbScale_(SpringPresets::snappy())
    , snapPulse_(SpringPresets::bouncy())
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    theme_ = ThemeManager::getInstance().getCurrentTheme();
    ThemeManager::getInstance().addListener(this);

    thumbScale_.position = 1.0f;
    thumbScale_.target = 1.0f;
    snapPulse_.position = 1.0f;
    snapPulse_.target = 1.0f;

    // Setup internal slider for APVTS
    internalSlider_.setRange(minValue_, maxValue_, step_);
    internalSlider_.setValue(value_);
    internalSlider_.onValueChange = [this] {
        double newValue = internalSlider_.getValue();
        if (std::abs(newValue - value_) > 0.0001)
        {
            setValue(newValue, true);
        }
    };

    // Default magnetic points
    magneticPoints_ = { minValue_, maxValue_ * 0.25, maxValue_ * 0.5,
                        maxValue_ * 0.75, maxValue_, defaultValue_ };
}

OscilSlider::OscilSlider(SliderVariant variant)
    : OscilSlider()
{
    setVariant(variant);
}

OscilSlider::~OscilSlider()
{
    ThemeManager::getInstance().removeListener(this);
    stopTimer();
}

void OscilSlider::setVariant(SliderVariant variant)
{
    if (variant_ != variant)
    {
        variant_ = variant;
        repaint();
    }
}

void OscilSlider::setValue(double value, bool notify)
{
    value = constrainValue(value);

    if (std::abs(value_ - value) < 0.0001)
        return;

    value_ = value;
    internalSlider_.setValue(value, juce::dontSendNotification);

    if (notify && onValueChanged)
        onValueChanged(value_);

    repaint();
}

void OscilSlider::setRangeValues(double start, double end, bool notify)
{
    start = constrainValue(start);
    end = constrainValue(end);

    if (start > end)
        std::swap(start, end);

    if (std::abs(rangeStart_ - start) < 0.0001 &&
        std::abs(rangeEnd_ - end) < 0.0001)
        return;

    rangeStart_ = start;
    rangeEnd_ = end;

    if (notify && onRangeChanged)
        onRangeChanged(rangeStart_, rangeEnd_);

    repaint();
}

void OscilSlider::setRange(double min, double max)
{
    minValue_ = min;
    maxValue_ = max;
    internalSlider_.setRange(min, max, step_);

    // Update magnetic points
    magneticPoints_ = { minValue_, (minValue_ + maxValue_) * 0.25,
                        (minValue_ + maxValue_) * 0.5,
                        (minValue_ + maxValue_) * 0.75, maxValue_, defaultValue_ };

    setValue(constrainValue(value_), false);
}

void OscilSlider::setStep(double step)
{
    step_ = step;
    internalSlider_.setRange(minValue_, maxValue_, step);
}

void OscilSlider::setDefaultValue(double defaultValue)
{
    defaultValue_ = defaultValue;
    // Add to magnetic points if not already there
    if (std::find(magneticPoints_.begin(), magneticPoints_.end(), defaultValue)
        == magneticPoints_.end())
    {
        magneticPoints_.push_back(defaultValue);
    }
}

void OscilSlider::setSkewFactor(double skew)
{
    skewFactor_ = skew;
    repaint();
}

void OscilSlider::setLabel(const juce::String& label)
{
    label_ = label;
    repaint();
}

void OscilSlider::setSuffix(const juce::String& suffix)
{
    suffix_ = suffix;
    repaint();
}

void OscilSlider::setDecimalPlaces(int places)
{
    decimalPlaces_ = places;
    repaint();
}

void OscilSlider::setShowValueOnHover(bool show)
{
    showValueOnHover_ = show;
}

void OscilSlider::setMagneticSnappingEnabled(bool enabled)
{
    enableMagneticSnap_ = enabled;
}

void OscilSlider::setMagneticPoints(const std::vector<double>& points)
{
    magneticPoints_ = points;
}

void OscilSlider::addMagneticPoint(double point)
{
    if (std::find(magneticPoints_.begin(), magneticPoints_.end(), point)
        == magneticPoints_.end())
    {
        magneticPoints_.push_back(point);
    }
}

void OscilSlider::clearMagneticPoints()
{
    magneticPoints_.clear();
}

void OscilSlider::setValueFormatter(Callbacks::FormatCallback formatter)
{
    valueFormatter_ = formatter;
}

void OscilSlider::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor
                               : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void OscilSlider::attachToParameter(juce::AudioProcessorValueTreeState& apvts,
                                     const juce::String& paramId)
{
    attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, paramId, internalSlider_);
}

void OscilSlider::detachFromParameter()
{
    attachment_.reset();
}

int OscilSlider::getPreferredWidth() const
{
    return variant_ == SliderVariant::Vertical ? THUMB_SIZE + 8 : 150;
}

int OscilSlider::getPreferredHeight() const
{
    return variant_ == SliderVariant::Vertical ? 100 : THUMB_SIZE + 8;
}

void OscilSlider::paint(juce::Graphics& g)
{
    if (variant_ == SliderVariant::Vertical)
        paintVertical(g);
    else
        paintHorizontal(g);

    if (hasFocus_ && enabled_)
        paintFocusRing(g, getLocalBounds().toFloat());
}

void OscilSlider::paintHorizontal(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float trackY = bounds.getCentreY() - TRACK_HEIGHT / 2.0f;
    auto trackBounds = juce::Rectangle<float>(
        THUMB_SIZE / 2.0f, trackY,
        bounds.getWidth() - THUMB_SIZE, static_cast<float>(TRACK_HEIGHT));

    paintTrack(g, trackBounds, false);

    // Paint thumb(s)
    if (variant_ == SliderVariant::Range)
    {
        paintThumb(g, getThumbPosition(false), false, false);
        paintThumb(g, getThumbPosition(true), false, true);
    }
    else
    {
        paintThumb(g, getThumbPosition(), false);
    }

    // Paint tooltip on hover
    if ((isHovered_ || isDragging_) && showValueOnHover_)
        paintValueTooltip(g, getThumbPosition(), false);
}

void OscilSlider::paintVertical(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float trackX = bounds.getCentreX() - TRACK_HEIGHT / 2.0f;
    auto trackBounds = juce::Rectangle<float>(
        trackX, THUMB_SIZE / 2.0f,
        static_cast<float>(TRACK_HEIGHT), bounds.getHeight() - THUMB_SIZE);

    paintTrack(g, trackBounds, true);
    paintThumb(g, getThumbPosition(), true);

    if ((isHovered_ || isDragging_) && showValueOnHover_)
        paintValueTooltip(g, getThumbPosition(), true);
}

void OscilSlider::paintTrack(juce::Graphics& g, const juce::Rectangle<float>& bounds, bool isVertical)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // Background track
    g.setColour(theme_.backgroundSecondary.withAlpha(opacity));
    g.fillRoundedRectangle(bounds, TRACK_HEIGHT / 2.0f);

    // Filled portion
    float fillProportion = static_cast<float>(valueToProportionOfLength(value_));

    juce::Rectangle<float> filledBounds;
    if (variant_ == SliderVariant::Range)
    {
        float startProp = static_cast<float>(valueToProportionOfLength(rangeStart_));
        float endProp = static_cast<float>(valueToProportionOfLength(rangeEnd_));

        if (isVertical)
        {
            float startY = bounds.getBottom() - bounds.getHeight() * startProp;
            float endY = bounds.getBottom() - bounds.getHeight() * endProp;
            filledBounds = juce::Rectangle<float>(
                bounds.getX(), endY, bounds.getWidth(), startY - endY);
        }
        else
        {
            float startX = bounds.getX() + bounds.getWidth() * startProp;
            float endX = bounds.getX() + bounds.getWidth() * endProp;
            filledBounds = juce::Rectangle<float>(
                startX, bounds.getY(), endX - startX, bounds.getHeight());
        }
    }
    else
    {
        if (isVertical)
        {
            filledBounds = bounds.withTop(bounds.getBottom() - bounds.getHeight() * fillProportion);
        }
        else
        {
            filledBounds = bounds.withWidth(bounds.getWidth() * fillProportion);
        }
    }

    g.setColour(theme_.controlActive.withAlpha(opacity));
    g.fillRoundedRectangle(filledBounds, TRACK_HEIGHT / 2.0f);
}

void OscilSlider::paintThumb(juce::Graphics& g, float position, bool isVertical, bool /*isRangeEnd*/)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    auto bounds = getLocalBounds().toFloat();

    float scale = currentThumbScale_;
    if (justSnapped_)
        scale *= snapPulse_.position;

    float size = THUMB_SIZE * scale;
    float cx, cy;

    if (isVertical)
    {
        cx = bounds.getCentreX();
        cy = position;
    }
    else
    {
        cx = position;
        cy = bounds.getCentreY();
    }

    auto thumbBounds = juce::Rectangle<float>(
        cx - size / 2, cy - size / 2, size, size);

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.2f * opacity));
    g.fillEllipse(thumbBounds.translated(0, 1));

    // Thumb body
    auto thumbColour = isDragging_ ? theme_.controlActive : juce::Colours::white;
    g.setColour(thumbColour.withAlpha(opacity));
    g.fillEllipse(thumbBounds);

    // Thumb border
    g.setColour(theme_.controlBorder.withAlpha(opacity * 0.5f));
    g.drawEllipse(thumbBounds.reduced(0.5f), 1.0f);
}

void OscilSlider::paintValueTooltip(juce::Graphics& g, float thumbPosition, bool isVertical)
{
    juce::String valueText = formatValue(value_);

    auto font = juce::Font(juce::FontOptions().withHeight(12.0f));
    int textWidth = font.getStringWidth(valueText) + TOOLTIP_PADDING * 2;

    juce::Rectangle<float> tooltipBounds;

    if (isVertical)
    {
        tooltipBounds = juce::Rectangle<float>(
            getWidth() + 4.0f, thumbPosition - TOOLTIP_HEIGHT / 2.0f,
            static_cast<float>(textWidth), static_cast<float>(TOOLTIP_HEIGHT));
    }
    else
    {
        tooltipBounds = juce::Rectangle<float>(
            thumbPosition - textWidth / 2.0f, -TOOLTIP_HEIGHT - 4.0f,
            static_cast<float>(textWidth), static_cast<float>(TOOLTIP_HEIGHT));
    }

    // Clamp to bounds
    tooltipBounds = tooltipBounds.constrainedWithin(
        getLocalBounds().toFloat().expanded(50, 30));

    // Background
    g.setColour(theme_.backgroundPane);
    g.fillRoundedRectangle(tooltipBounds, ComponentLayout::RADIUS_SM);

    g.setColour(theme_.controlBorder.withAlpha(0.5f));
    g.drawRoundedRectangle(tooltipBounds.reduced(0.5f), ComponentLayout::RADIUS_SM, 1.0f);

    // Text
    g.setColour(theme_.textPrimary);
    g.setFont(font);
    g.drawText(valueText, tooltipBounds, juce::Justification::centred);
}

void OscilSlider::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    g.setColour(theme_.controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
    g.drawRoundedRectangle(
        bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET),
        ComponentLayout::RADIUS_SM + ComponentLayout::FOCUS_RING_OFFSET,
        ComponentLayout::FOCUS_RING_WIDTH
    );
}

juce::String OscilSlider::formatValue(double value) const
{
    if (valueFormatter_)
        return valueFormatter_(value);

    juce::String text = juce::String(value, decimalPlaces_);
    if (suffix_.isNotEmpty())
        text += " " + suffix_;

    return text;
}

void OscilSlider::resized()
{
    // No child components
}

void OscilSlider::mouseEnter(const juce::MouseEvent&)
{
    if (!enabled_) return;

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
    if (!enabled_) return;

    isDragging_ = true;
    dragStartPoint_ = e.getPosition();

    // Determine which thumb we're dragging (for Range variant)
    if (variant_ == SliderVariant::Range)
    {
        bool isVertical = variant_ == SliderVariant::Vertical;
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
            // Both or neither - pick closest
            float mousePos = isVertical ? e.getPosition().y : e.getPosition().x;
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
    if (!isDragging_ || !enabled_) return;

    auto bounds = getLocalBounds();
    bool isVertical = variant_ == SliderVariant::Vertical;

    // Calculate proportion
    float proportion;
    if (isVertical)
    {
        float trackHeight = bounds.getHeight() - THUMB_SIZE;
        float relY = bounds.getBottom() - THUMB_SIZE / 2 - e.getPosition().y;
        proportion = relY / trackHeight;
    }
    else
    {
        float trackWidth = bounds.getWidth() - THUMB_SIZE;
        float relX = e.getPosition().x - THUMB_SIZE / 2;
        proportion = relX / trackWidth;
    }

    // Apply fine/coarse control
    if (e.mods.isAltDown())
    {
        // Fine control - move from drag start with reduced sensitivity
        float delta = proportion - static_cast<float>(valueToProportionOfLength(dragStartValue_));
        delta *= ComponentLayout::FINE_CONTROL_FACTOR;
        proportion = static_cast<float>(valueToProportionOfLength(dragStartValue_)) + delta;
    }

    proportion = juce::jlimit(0.0f, 1.0f, proportion);
    double newValue = proportionOfLengthToValue(proportion);

    // Apply magnetic snapping (unless Shift held for coarse mode)
    if (enableMagneticSnap_ && !e.mods.isShiftDown())
        applyMagneticSnapping(newValue);

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
    if (!isDragging_) return;

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
    if (!enabled_) return;

    // Reset to default
    setValue(defaultValue_, true);
}

void OscilSlider::mouseWheelMove(const juce::MouseEvent& e,
                                  const juce::MouseWheelDetails& wheel)
{
    if (!enabled_) return;

    double delta = wheel.deltaY * step_;

    if (e.mods.isAltDown())
        delta *= ComponentLayout::FINE_CONTROL_FACTOR;

    if (e.mods.isShiftDown())
        delta *= ComponentLayout::COARSE_CONTROL_FACTOR;

    setValue(value_ + delta, true);
}

bool OscilSlider::keyPressed(const juce::KeyPress& key)
{
    if (!enabled_) return false;

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

    if (delta != 0.0)
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

void OscilSlider::timerCallback()
{
    updateAnimations();

    if (thumbScale_.isSettled() && (!justSnapped_ || snapPulse_.isSettled()))
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

    if (justSnapped_)
        snapPulse_.update(dt);
}

double OscilSlider::constrainValue(double value) const
{
    value = juce::jlimit(minValue_, maxValue_, value);

    if (step_ > 0)
    {
        double steps = std::round((value - minValue_) / step_);
        value = minValue_ + steps * step_;
    }

    return value;
}

double OscilSlider::valueToProportionOfLength(double value) const
{
    double proportion = (value - minValue_) / (maxValue_ - minValue_);

    if (skewFactor_ != 1.0)
        proportion = std::pow(proportion, 1.0 / skewFactor_);

    return proportion;
}

double OscilSlider::proportionOfLengthToValue(double proportion) const
{
    if (skewFactor_ != 1.0)
        proportion = std::pow(proportion, skewFactor_);

    return minValue_ + proportion * (maxValue_ - minValue_);
}

void OscilSlider::applyMagneticSnapping(double& value)
{
    double range = maxValue_ - minValue_;
    double snapThreshold = range * ComponentLayout::MAGNETIC_SNAP_THRESHOLD;

    for (double point : magneticPoints_)
    {
        if (std::abs(value - point) < snapThreshold)
        {
            value = point;
            if (!justSnapped_)
                triggerSnapFeedback();
            return;
        }
    }

    justSnapped_ = false;
}

void OscilSlider::triggerSnapFeedback()
{
    justSnapped_ = true;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        snapPulse_.setTarget(1.15f, 1.0f);

        juce::Timer::callAfterDelay(100, [this] {
            snapPulse_.setTarget(1.0f);
            if (!isTimerRunning())
                startTimerHz(ComponentLayout::ANIMATION_FPS);
        });

        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
}

bool OscilSlider::hitTestThumb(const juce::Point<int>& point, float thumbPosition, bool isVertical) const
{
    float size = THUMB_SIZE + THUMB_HIT_EXTRA * 2;

    if (isVertical)
    {
        float cx = getWidth() / 2.0f;
        return juce::Rectangle<float>(cx - size / 2, thumbPosition - size / 2, size, size)
            .contains(point.toFloat());
    }
    else
    {
        float cy = getHeight() / 2.0f;
        return juce::Rectangle<float>(thumbPosition - size / 2, cy - size / 2, size, size)
            .contains(point.toFloat());
    }
}

float OscilSlider::getThumbPosition(bool isRangeEnd) const
{
    auto bounds = getLocalBounds().toFloat();
    bool isVertical = variant_ == SliderVariant::Vertical;

    double value = (variant_ == SliderVariant::Range)
        ? (isRangeEnd ? rangeEnd_ : rangeStart_)
        : value_;

    float proportion = static_cast<float>(valueToProportionOfLength(value));

    if (isVertical)
    {
        float trackHeight = bounds.getHeight() - THUMB_SIZE;
        return bounds.getBottom() - THUMB_SIZE / 2 - trackHeight * proportion;
    }
    else
    {
        float trackWidth = bounds.getWidth() - THUMB_SIZE;
        return THUMB_SIZE / 2 + trackWidth * proportion;
    }
}

void OscilSlider::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    repaint();
}

std::unique_ptr<juce::AccessibilityHandler> OscilSlider::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this,
        juce::AccessibilityRole::slider,
        juce::AccessibilityActions()
            .addAction(juce::AccessibilityActionType::showMenu,
                [] { /* Show context menu */ })
    );
}

} // namespace oscil
