/*
    Oscil - Toggle Component Implementation
    Flat-surface rendering with spring-physics toggle animation.
    (Historical: "glassmorphism rendering" prior to the 2026-Q2 uplift.)
*/

#include "ui/components/OscilToggle.h"

#include "ui/components/SurfacePainter.h"

namespace oscil
{

OscilToggle::OscilToggle(IThemeService& themeService)
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

OscilToggle::OscilToggle(IThemeService& themeService, const juce::String& label) : OscilToggle(themeService)
{
    label_ = label;
}

OscilToggle::OscilToggle(IThemeService& themeService, const juce::String& label, const juce::String& testId)
    : OscilToggle(themeService)
{
    label_ = label;
    setTestId(testId);
}

void OscilToggle::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

OscilToggle::~OscilToggle() { stopTimer(); }

void OscilToggle::setValue(bool value, bool animate)
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

void OscilToggle::toggle()
{
    if (enabled_)
        setValue(!value_);
}

void OscilToggle::setLabel(const juce::String& label)
{
    if (label_ != label)
    {
        label_ = label;
        repaint();
    }
}

void OscilToggle::setLabelOnRight(bool onRight)
{
    if (labelOnRight_ != onRight)
    {
        labelOnRight_ = onRight;
        repaint();
    }
}

void OscilToggle::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        juce::Component::setEnabled(enabled);
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void OscilToggle::attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
{
    attachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, paramId, internalButton_);
}

void OscilToggle::detachFromParameter() { attachment_.reset(); }

int OscilToggle::getPreferredWidth() const
{
    int const toggleWidth = ComponentLayout::TOGGLE_WIDTH;

    if (label_.isNotEmpty())
    {
        auto font = ComponentLayout::defaultFont();
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        int const labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
        return toggleWidth + ComponentLayout::SPACING_SM + labelWidth;
    }

    return toggleWidth;
}

int OscilToggle::getPreferredHeight() const
{
    return std::max(ComponentLayout::TOGGLE_HEIGHT, static_cast<int>(ComponentLayout::defaultFont().getHeight()));
}

void OscilToggle::paint(juce::Graphics& g)
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
        g.setFont(ComponentLayout::defaultFont());
        g.drawText(label_, labelBounds, juce::Justification::centredLeft);
    }
    else
    {
        auto font = ComponentLayout::defaultFont();
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

void OscilToggle::paintTrack(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    const auto& glass = getSurface();
    float const progress = std::clamp(positionSpring_.position, 0.0f, 1.0f);

    float const cornerRadius = bounds.getHeight() / 2.0f;

    // Track background: interpolate between OFF (bgGlass) and ON (accent)
    auto offColor = glass.bgGlass;
    auto onColor = glass.accent;
    auto trackColor = offColor.interpolatedWith(onColor, progress);

    // Hover: brighten the OFF track to bgHover
    if (isHovered_ && progress < 0.5f)
        trackColor = trackColor.interpolatedWith(glass.bgHover, (1.0f - progress * 2.0f) * 0.5f);

    g.setColour(trackColor.withAlpha(trackColor.getFloatAlpha() * opacity));
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Border: interpolate between borderDefault (OFF) and accent (ON)
    auto offBorder = glass.borderDefault;
    auto onBorder = glass.accent;
    auto borderColor = offBorder.interpolatedWith(onBorder, progress);

    g.setColour(borderColor.withAlpha(borderColor.getFloatAlpha() * opacity));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);
}

void OscilToggle::paintKnob(juce::Graphics& g, const juce::Rectangle<float>& trackBounds) const
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float const progress = std::clamp(positionSpring_.position, 0.0f, 1.0f);
    const auto& theme = getTheme();

    // Calculate knob position
    float const knobSize = ComponentLayout::TOGGLE_KNOB_SIZE;
    float const padding = (trackBounds.getHeight() - knobSize) / 2.0f;
    float const minX = trackBounds.getX() + padding;
    float const maxX = trackBounds.getRight() - knobSize - padding;
    float const knobX = minX + ((maxX - minX) * progress);
    float const knobY = trackBounds.getY() + padding;

    auto knobBounds = juce::Rectangle<float>(knobX, knobY, knobSize, knobSize);

    // Knob shadow (subtle for ON state)
    if (progress > 0.5f)
    {
        g.setColour(juce::Colours::black.withAlpha(0.15f * opacity));
        g.fillEllipse(knobBounds.translated(0, 1));
    }

    // Knob color: textSecondary (OFF) -> contrast-safe over accent (ON).
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
    g.fillEllipse(knobBounds);
}

void OscilToggle::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float const cornerRadius = bounds.getHeight() / 2.0f;
    SurfacePainter::paintFocusRing(g, bounds, cornerRadius, getSurface().accent);
}

void OscilToggle::resized()
{
    // No child components to layout
}

void OscilToggle::mouseEnter(const juce::MouseEvent& /*event*/)
{
    if (!enabled_)
        return;

    isHovered_ = true;
    repaint();
}

void OscilToggle::mouseExit(const juce::MouseEvent& /*event*/)
{
    isHovered_ = false;
    repaint();
}

void OscilToggle::mouseDown(const juce::MouseEvent& /*event*/)
{
    if (enabled_)
    {
        toggle();
    }
}

void OscilToggle::mouseUp(const juce::MouseEvent& /*event*/)
{
    // Click handled in mouseDown for immediate response
}

bool OscilToggle::keyPressed(const juce::KeyPress& key)
{
    if (enabled_ && (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey))
    {
        toggle();
        return true;
    }
    return false;
}

void OscilToggle::focusGained(FocusChangeType /*cause*/)
{
    hasFocus_ = true;
    repaint();
}

void OscilToggle::focusLost(FocusChangeType /*cause*/)
{
    hasFocus_ = false;
    repaint();
}

void OscilToggle::notifyValueChanged() const
{
    if (onValueChanged)
        onValueChanged(value_);
}

void OscilToggle::timerCallback()
{
    updateAnimations();

    if (positionSpring_.isSettled())
        stopTimer();

    repaint();
}

void OscilToggle::updateAnimations()
{
    float const dt = AnimationTiming::FRAME_DURATION_60FPS;
    positionSpring_.update(dt);
}

//==============================================================================
// Accessibility Handler for OscilToggle
//==============================================================================
class OscilToggleAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit OscilToggleAccessibilityHandler(OscilToggle& toggle)
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
    OscilToggle& toggle_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilToggleAccessibilityHandler)
};

std::unique_ptr<juce::AccessibilityHandler> OscilToggle::createAccessibilityHandler()
{
    return std::make_unique<OscilToggleAccessibilityHandler>(*this);
}

} // namespace oscil
