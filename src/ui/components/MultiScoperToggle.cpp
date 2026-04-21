/*
    MultiScoper - Toggle Component Implementation
    Flat-surface rendering with spring-physics toggle animation.
*/

#include "ui/components/MultiScoperToggle.h"

#include "ui/components/SurfacePainter.h"

namespace multiscoper
{

MultiScoperToggle::MultiScoperToggle(IThemeService& themeService)
    : ThemedComponent(themeService)
    , positionSpring_(SpringPresets::springSwitch())
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    positionSpring_.position = 0.0f;
    positionSpring_.target = 0.0f;

    // Setup internal toggle button for APVTS (hidden)
    internalButton_.setToggleState(false, juce::dontSendNotification);
    internalButton_.onClick = [this] {
        bool const newValue = internalButton_.getToggleState();
        if (newValue != value_)
        {
            setValue(newValue, true);
        }
    };
}

MultiScoperToggle::MultiScoperToggle(IThemeService& themeService, const juce::String& label)
    : MultiScoperToggle(themeService)
{
    label_ = label;
}

MultiScoperToggle::MultiScoperToggle(IThemeService& themeService, const juce::String& label, const juce::String& testId)
    : MultiScoperToggle(themeService)
{
    label_ = label;
    setTestId(testId);
}

void MultiScoperToggle::registerTestId() { MULTISCOPER_REGISTER_TEST_ID(testId_); }

MultiScoperToggle::~MultiScoperToggle() { stopTimer(); }

void MultiScoperToggle::setValue(bool value, bool animate)
{
    if (value_ == value)
        return;

    value_ = value;

    // Update internal button for APVTS sync — sendNotificationSync ensures
    // ButtonAttachment propagates the change to the APVTS parameter.
    // Recursion is prevented by the value_ == value early return above.
    internalButton_.setToggleState(value, juce::sendNotificationSync);

    if (animate && AnimationSettings::shouldUseSpringAnimations())
    {
        positionSpring_.setTarget(value ? 1.0f : 0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        positionSpring_.target = value ? 1.0f : 0.0f;
        positionSpring_.position = positionSpring_.target;
        repaint();
    }

    notifyValueChanged();
}

void MultiScoperToggle::toggle()
{
    if (enabled_)
        setValue(!value_);
}

void MultiScoperToggle::setLabel(const juce::String& label)
{
    if (label_ != label)
    {
        label_ = label;
        repaint();
    }
}

void MultiScoperToggle::setLabelOnRight(bool onRight)
{
    if (labelOnRight_ != onRight)
    {
        labelOnRight_ = onRight;
        repaint();
    }
}

void MultiScoperToggle::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        juce::Component::setEnabled(enabled);
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void MultiScoperToggle::attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
{
    attachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, paramId, internalButton_);
}

void MultiScoperToggle::detachFromParameter() { attachment_.reset(); }

int MultiScoperToggle::getPreferredWidth() const
{
    int const toggleWidth = ComponentLayout::TOGGLE_WIDTH;

    if (label_.isNotEmpty())
    {
        auto font = Typography::headingRegular();
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        int const labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
        return toggleWidth + ComponentLayout::SPACING_SM + labelWidth;
    }

    return toggleWidth;
}

int MultiScoperToggle::getPreferredHeight() const
{
    return std::max(ComponentLayout::TOGGLE_HEIGHT, static_cast<int>(Typography::headingRegular().getHeight()));
}

void MultiScoperToggle::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // Calculate toggle track bounds
    juce::Rectangle<float> trackBounds;

    if (label_.isEmpty())
    {
        trackBounds = bounds.toFloat();
    }
    else if (labelOnRight_)
    {
        trackBounds =
            juce::Rectangle<float>(0, static_cast<float>(bounds.getHeight() - ComponentLayout::TOGGLE_HEIGHT) / 2.0f,
                                   ComponentLayout::TOGGLE_WIDTH, ComponentLayout::TOGGLE_HEIGHT);

        // Draw label
        auto labelBounds = bounds.toFloat().withLeft(ComponentLayout::TOGGLE_WIDTH + ComponentLayout::SPACING_SM);

        g.setColour(getTheme().textPrimary.withAlpha(opacity));
        g.setFont(Typography::headingRegular());
        g.drawText(label_, labelBounds, juce::Justification::centredLeft);
    }
    else
    {
        auto font = Typography::headingRegular();
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        int const labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());

        // Draw label on left
        auto labelBounds =
            juce::Rectangle<float>(0, 0, static_cast<float>(labelWidth), static_cast<float>(bounds.getHeight()));

        g.setColour(getTheme().textPrimary.withAlpha(opacity));
        g.setFont(font);
        g.drawText(label_, labelBounds, juce::Justification::centredRight);

        trackBounds =
            juce::Rectangle<float>(static_cast<float>(labelWidth + ComponentLayout::SPACING_SM),
                                   static_cast<float>(bounds.getHeight() - ComponentLayout::TOGGLE_HEIGHT) / 2.0f,
                                   ComponentLayout::TOGGLE_WIDTH, ComponentLayout::TOGGLE_HEIGHT);
    }

    paintTrack(g, trackBounds);
    paintKnob(g, trackBounds);

    if (hasFocus_ && enabled_)
        paintFocusRing(g, trackBounds);
}

void MultiScoperToggle::paintTrack(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    const auto& glass = getSurface();
    float const progress = std::clamp(positionSpring_.position, 0.0f, 1.0f);

    // Track background: interpolate between OFF (bgGlass) and ON (accent)
    auto offColor = glass.bgGlass;
    auto onColor = glass.accent;
    auto trackColor = offColor.interpolatedWith(onColor, progress);

    // Hover: brighten the OFF track to bgHover
    if (isHovered_ && progress < 0.5f)
        trackColor = trackColor.interpolatedWith(glass.bgHover, (1.0f - progress * 2.0f) * 0.5f);

    g.setColour(trackColor.withAlpha(trackColor.getFloatAlpha() * opacity));
    g.fillRect(bounds);

    // Border: interpolate between borderDefault (OFF) and accent (ON)
    auto offBorder = glass.borderDefault;
    auto onBorder = glass.accent;
    auto borderColor = offBorder.interpolatedWith(onBorder, progress);

    g.setColour(borderColor.withAlpha(borderColor.getFloatAlpha() * opacity));
    g.drawRect(bounds, 1.0f);
}

void MultiScoperToggle::paintKnob(juce::Graphics& g, const juce::Rectangle<float>& trackBounds) const
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float const progress = std::clamp(positionSpring_.position, 0.0f, 1.0f);
    const auto& theme = getTheme();

    // Square thumb, sits inside the rectangular track with a symmetric inset.
    float const knobSize = ComponentLayout::TOGGLE_KNOB_SIZE;
    float const padding = (trackBounds.getHeight() - knobSize) / 2.0f;
    float const minX = trackBounds.getX() + padding;
    float const maxX = trackBounds.getRight() - knobSize - padding;
    float const knobX = minX + ((maxX - minX) * progress);
    float const knobY = trackBounds.getY() + padding;

    auto knobBounds = juce::Rectangle<float>(knobX, knobY, knobSize, knobSize);

    // Thumb shadow (subtle for ON state) — flat rect, not an ellipse.
    if (progress > 0.5f)
    {
        g.setColour(juce::Colours::black.withAlpha(0.15f * opacity));
        g.fillRect(knobBounds.translated(0, 1));
    }

    // Thumb colour: textSecondary (OFF) -> contrast-safe over accent (ON).
    // Hardcoded white fails AA against bright accent themes (Dark Pro cyan).
    // Hover OFF: textPrimary.
    juce::Colour knobColour;
    if (progress > 0.5f)
    {
        knobColour = ColorTheme::pickContrastingText(getSurface().accent);
    }
    else
    {
        knobColour = isHovered_ ? theme.textPrimary : theme.textSecondary;
    }

    g.setColour(knobColour.withAlpha(opacity));
    g.fillRect(knobBounds);
}

void MultiScoperToggle::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    SurfacePainter::paintFocusRing(g, bounds, 0.0f, getSurface().accent);
}

void MultiScoperToggle::resized()
{
    // No child components to layout
}

void MultiScoperToggle::mouseEnter(const juce::MouseEvent& /*event*/)
{
    if (!enabled_)
        return;

    isHovered_ = true;
    repaint();
}

void MultiScoperToggle::mouseExit(const juce::MouseEvent& /*event*/)
{
    isHovered_ = false;
    repaint();
}

void MultiScoperToggle::mouseDown(const juce::MouseEvent& /*event*/)
{
    if (enabled_)
    {
        toggle();
    }
}

void MultiScoperToggle::mouseUp(const juce::MouseEvent& /*event*/)
{
    // Click handled in mouseDown for immediate response
}

void MultiScoperToggle::focusGained(FocusChangeType /*cause*/)
{
    hasFocus_ = true;
    repaint();
}

void MultiScoperToggle::focusLost(FocusChangeType /*cause*/)
{
    hasFocus_ = false;
    repaint();
}

void MultiScoperToggle::notifyValueChanged() const
{
    if (onValueChanged)
        onValueChanged(value_);
}

void MultiScoperToggle::timerCallback()
{
    updateAnimations();

    if (positionSpring_.isSettled())
        stopTimer();

    repaint();
}

void MultiScoperToggle::updateAnimations()
{
    float const dt = AnimationTiming::FRAME_DURATION_60FPS;
    positionSpring_.update(dt);
}

//==============================================================================
// Accessibility Handler for MultiScoperToggle
//==============================================================================
class MultiScoperToggleAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit MultiScoperToggleAccessibilityHandler(MultiScoperToggle& toggle)
        : juce::AccessibilityHandler(toggle, juce::AccessibilityRole::toggleButton,
                                     juce::AccessibilityActions().addAction(juce::AccessibilityActionType::toggle,
                                                                            [&toggle] {
                                                                                if (toggle.isEnabled())
                                                                                    toggle.toggle();
                                                                            }))
        , toggle_(toggle)
    {
    }

    juce::String getTitle() const override { return toggle_.getLabel().isNotEmpty() ? toggle_.getLabel() : "Toggle"; }

    juce::String getDescription() const override
    {
        juce::String state = toggle_.getValue() ? "On" : "Off";
        if (!toggle_.isEnabled())
        {
            state += " (disabled)";
        }
        return state;
    }

    juce::String getHelp() const override { return "Press Space or Enter to toggle."; }

    juce::AccessibleState getCurrentState() const override
    {
        auto state = AccessibilityHandler::getCurrentState();
        if (toggle_.getValue())
            state = state.withChecked();
        return state;
    }

private:
    MultiScoperToggle& toggle_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiScoperToggleAccessibilityHandler)
};

std::unique_ptr<juce::AccessibilityHandler> MultiScoperToggle::createAccessibilityHandler()
{
    return std::make_unique<MultiScoperToggleAccessibilityHandler>(*this);
}

} // namespace multiscoper
