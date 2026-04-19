/*
    MultiScoper - Slider Component Implementation (Property Management)
    Interaction handling is in MultiScoperSliderInteraction.cpp
    Painting methods are in MultiScoperSliderPainting.cpp
*/

#include "ui/components/MultiScoperSlider.h"

#include <cmath>

namespace multiscoper
{

MultiScoperSlider::MultiScoperSlider(IThemeService& themeService)
    : ThemedComponent(themeService)
    , thumbScale_(SpringPresets::medium())
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    thumbScale_.position = 1.0f;
    thumbScale_.target = 1.0f;

    // Setup internal slider for APVTS
    internalSlider_.setRange(minValue_, maxValue_, step_);
    internalSlider_.setValue(value_);
    internalSlider_.onValueChange = [this] {
        double const newValue = internalSlider_.getValue();
        if (std::abs(newValue - value_) > 0.0001)
        {
            setValue(newValue, true);
        }
    };

    // Default magnetic points
    snapController_.setMagneticPoints(
        {minValue_, maxValue_ * 0.25, maxValue_ * 0.5, maxValue_ * 0.75, maxValue_, defaultValue_});
}

MultiScoperSlider::MultiScoperSlider(IThemeService& themeService, SliderVariant variant)
    : MultiScoperSlider(themeService)
{
    setVariant(variant);
}

MultiScoperSlider::MultiScoperSlider(IThemeService& themeService, const juce::String& testId)
    : MultiScoperSlider(themeService)
{
    setTestId(testId);
}

MultiScoperSlider::MultiScoperSlider(IThemeService& themeService, SliderVariant variant, const juce::String& testId)
    : MultiScoperSlider(themeService)
{
    setVariant(variant);
    setTestId(testId);
}

void MultiScoperSlider::registerTestId() { MULTISCOPER_REGISTER_TEST_ID(testId_); }

MultiScoperSlider::~MultiScoperSlider() { stopTimer(); }

void MultiScoperSlider::setVariant(SliderVariant variant)
{
    if (variant_ != variant)
    {
        variant_ = variant;
        repaint();
    }
}

void MultiScoperSlider::setValue(double value, bool notify)
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

void MultiScoperSlider::setRangeValues(double start, double end, bool notify)
{
    start = constrainValue(start);
    end = constrainValue(end);

    if (start > end)
        std::swap(start, end);

    if (std::abs(rangeStart_ - start) < 0.0001 && std::abs(rangeEnd_ - end) < 0.0001)
        return;

    rangeStart_ = start;
    rangeEnd_ = end;

    if (notify && onRangeChanged)
        onRangeChanged(rangeStart_, rangeEnd_);

    repaint();
}

void MultiScoperSlider::setRange(double min, double max)
{
    minValue_ = min;
    maxValue_ = max;
    internalSlider_.setRange(min, max, step_);

    snapController_.setMagneticPoints({minValue_, (minValue_ + maxValue_) * 0.25, (minValue_ + maxValue_) * 0.5,
                                       (minValue_ + maxValue_) * 0.75, maxValue_, defaultValue_});

    setValue(constrainValue(value_), false);
}

void MultiScoperSlider::setStep(double step)
{
    step_ = step;
    internalSlider_.setRange(minValue_, maxValue_, step);
}

void MultiScoperSlider::setDefaultValue(double defaultValue)
{
    defaultValue_ = defaultValue;
    snapController_.addMagneticPoint(defaultValue);
}

void MultiScoperSlider::setSkewFactor(double skew)
{
    skewFactor_ = skew;
    repaint();
}

void MultiScoperSlider::setLabel(const juce::String& label)
{
    label_ = label;
    repaint();
}

void MultiScoperSlider::setSuffix(const juce::String& suffix)
{
    suffix_ = suffix;
    repaint();
}

void MultiScoperSlider::setDecimalPlaces(int places)
{
    decimalPlaces_ = places;
    repaint();
}

void MultiScoperSlider::setShowValueOnHover(bool show) { showValueOnHover_ = show; }

void MultiScoperSlider::setShowValue(bool show)
{
    if (showValue_ != show)
    {
        showValue_ = show;
        repaint();
    }
}

void MultiScoperSlider::setBipolar(bool bipolar)
{
    if (bipolar_ != bipolar)
    {
        bipolar_ = bipolar;
        repaint();
    }
}

void MultiScoperSlider::setArcColour(juce::Colour colour)
{
    if (arcColour_ != colour)
    {
        arcColour_ = colour;
        repaint();
    }
}

void MultiScoperSlider::setModAmount(float deltaNormalized)
{
    float const clamped = juce::jlimit(-1.0f, 1.0f, deltaNormalized);
    if (std::abs(modAmount_ - clamped) > 1e-5f)
    {
        modAmount_ = clamped;
        repaint();
    }
}

void MultiScoperSlider::setModColour(juce::Colour colour)
{
    if (modColour_ != colour)
    {
        modColour_ = colour;
        repaint();
    }
}

void MultiScoperSlider::setMagneticSnappingEnabled(bool enabled) { snapController_.setEnabled(enabled); }

void MultiScoperSlider::setMagneticPoints(const std::vector<double>& points)
{
    snapController_.setMagneticPoints(points);
}

void MultiScoperSlider::addMagneticPoint(double point) { snapController_.addMagneticPoint(point); }

void MultiScoperSlider::clearMagneticPoints() { snapController_.clearMagneticPoints(); }

void MultiScoperSlider::setValueFormatter(Callbacks::FormatCallback formatter) { valueFormatter_ = formatter; }

void MultiScoperSlider::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        juce::Component::setEnabled(enabled);
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void MultiScoperSlider::attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
{
    attachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, internalSlider_);
}

void MultiScoperSlider::detachFromParameter() { attachment_.reset(); }

int MultiScoperSlider::getPreferredWidth() const
{
    if (variant_ == SliderVariant::Rotary)
        return ROTARY_KNOB_SIZE;
    return variant_ == SliderVariant::Vertical ? THUMB_SIZE + 8 : 150;
}

int MultiScoperSlider::getPreferredHeight() const
{
    if (variant_ == SliderVariant::Rotary)
    {
        int height = ROTARY_KNOB_SIZE;
        if (label_.isNotEmpty())
            height += ROTARY_LABEL_HEIGHT;
        if (showValue_)
            height += ROTARY_VALUE_HEIGHT;
        return height;
    }
    return variant_ == SliderVariant::Vertical ? 100 : THUMB_SIZE + 8;
}

void MultiScoperSlider::resized() {}

double MultiScoperSlider::constrainValue(double value) const
{
    value = juce::jlimit(minValue_, maxValue_, value);

    if (step_ > 0)
    {
        double const steps = std::round((value - minValue_) / step_);
        value = minValue_ + (steps * step_);
    }

    return value;
}

double MultiScoperSlider::valueToProportionOfLength(double value) const
{
    double const range = maxValue_ - minValue_;
    if (std::abs(range) < 1e-10)
        return 0.0;

    double proportion = (value - minValue_) / range;

    if (std::abs(skewFactor_ - 1.0) > 1e-9)
        proportion = std::pow(proportion, 1.0 / skewFactor_);

    return proportion;
}

double MultiScoperSlider::proportionOfLengthToValue(double proportion) const
{
    if (std::abs(skewFactor_ - 1.0) > 1e-9)
        proportion = std::pow(proportion, skewFactor_);

    return minValue_ + (proportion * (maxValue_ - minValue_));
}

} // namespace multiscoper
