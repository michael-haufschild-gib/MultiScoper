/*
    Oscil - Slider Component Implementation (Property Management)
    Interaction handling is in OscilSliderInteraction.cpp
    Painting methods are in OscilSliderPainting.cpp
*/

#include "ui/components/OscilSlider.h"
#include <cmath>

namespace oscil
{

OscilSlider::OscilSlider(IThemeService& themeService)
    : ThemedComponent(themeService)
    , thumbScale_(SpringPresets::snappy())
    , snapPulse_(SpringPresets::bouncy())
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

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
    snapController_.setMagneticPoints({ minValue_, maxValue_ * 0.25, maxValue_ * 0.5,
                                        maxValue_ * 0.75, maxValue_, defaultValue_ });
}

OscilSlider::OscilSlider(IThemeService& themeService, SliderVariant variant)
    : OscilSlider(themeService)
{
    setVariant(variant);
}

OscilSlider::OscilSlider(IThemeService& themeService, const juce::String& testId)
    : OscilSlider(themeService)
{
    setTestId(testId);
}

OscilSlider::OscilSlider(IThemeService& themeService, SliderVariant variant, const juce::String& testId)
    : OscilSlider(themeService)
{
    setVariant(variant);
    setTestId(testId);
}

void OscilSlider::registerTestId()
{
    OSCIL_REGISTER_TEST_ID(testId_);
}

OscilSlider::~OscilSlider()
{
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

    snapController_.setMagneticPoints({ minValue_, (minValue_ + maxValue_) * 0.25,
                                        (minValue_ + maxValue_) * 0.5,
                                        (minValue_ + maxValue_) * 0.75, maxValue_, defaultValue_ });

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
    snapController_.addMagneticPoint(defaultValue);
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
    snapController_.setEnabled(enabled);
}

void OscilSlider::setMagneticPoints(const std::vector<double>& points)
{
    snapController_.setMagneticPoints(points);
}

void OscilSlider::addMagneticPoint(double point)
{
    snapController_.addMagneticPoint(point);
}

void OscilSlider::clearMagneticPoints()
{
    snapController_.clearMagneticPoints();
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

void OscilSlider::resized()
{
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
    double range = maxValue_ - minValue_;
    if (std::abs(range) < 1e-10)
        return 0.0;

    double proportion = (value - minValue_) / range;

    if (std::abs(skewFactor_ - 1.0) > 1e-9)
        proportion = std::pow(proportion, 1.0 / skewFactor_);

    return proportion;
}

double OscilSlider::proportionOfLengthToValue(double proportion) const
{
    if (std::abs(skewFactor_ - 1.0) > 1e-9)
        proportion = std::pow(proportion, skewFactor_);

    return minValue_ + proportion * (maxValue_ - minValue_);
}

} // namespace oscil
