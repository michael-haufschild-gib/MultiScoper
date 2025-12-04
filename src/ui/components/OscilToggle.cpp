/*
    Oscil - Toggle Component Implementation
*/

#include "ui/components/OscilToggle.h"

namespace oscil
{

OscilToggle::OscilToggle(IThemeService& themeService)
    : positionSpring_(SpringPresets::bouncy())
    , celebrationSpring_(SpringPresets::snappy())
    , themeService_(themeService)
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    theme_ = themeService_.getCurrentTheme();
    themeService_.addListener(this);

    positionSpring_.position = 0.0f;
    positionSpring_.target = 0.0f;
    celebrationSpring_.position = 1.0f;
    celebrationSpring_.target = 1.0f;

    // Setup internal toggle button for APVTS (hidden)
    internalButton_.setToggleState(false, juce::dontSendNotification);
    internalButton_.onClick = [this] {
        bool newValue = internalButton_.getToggleState();
        if (newValue != value_)
        {
            setValue(newValue, true);
        }
    };
}



OscilToggle::OscilToggle(IThemeService& themeService, const juce::String& label)
    : OscilToggle(themeService)
{
    label_ = label;
}



OscilToggle::OscilToggle(IThemeService& themeService, const juce::String& label, const juce::String& testId)
    : OscilToggle(themeService)
{
    label_ = label;
    setTestId(testId);
}



void OscilToggle::registerTestId()
{
    OSCIL_REGISTER_TEST_ID(testId_);
}

OscilToggle::~OscilToggle()
{
    themeService_.removeListener(this);
    stopTimer();
}

void OscilToggle::setValue(bool value, bool animate)
{
    if (value_ == value) return;

    bool wasOff = !value_;
    value_ = value;

    // Update internal button for APVTS sync
    internalButton_.setToggleState(value, juce::dontSendNotification);

    if (animate && AnimationSettings::shouldUseSpringAnimations())
    {
        positionSpring_.setTarget(value ? 1.0f : 0.0f);

        // Trigger celebration on activation
        if (value && wasOff)
            triggerCelebration();

        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        positionSpring_.snapToTarget();
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
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor
                               : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void OscilToggle::attachToParameter(juce::AudioProcessorValueTreeState& apvts,
                                     const juce::String& paramId)
{
    attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, paramId, internalButton_);
}

void OscilToggle::detachFromParameter()
{
    attachment_.reset();
}

int OscilToggle::getPreferredWidth() const
{
    int toggleWidth = ComponentLayout::TOGGLE_WIDTH;

    if (label_.isNotEmpty())
    {
        auto font = juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT));
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        int labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
        return toggleWidth + ComponentLayout::SPACING_SM + labelWidth;
    }

    return toggleWidth;
}

int OscilToggle::getPreferredHeight() const
{
    return std::max(ComponentLayout::TOGGLE_HEIGHT,
                    static_cast<int>(juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT)).getHeight()));
}

void OscilToggle::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // Calculate toggle track bounds
    juce::Rectangle<float> trackBounds;

    if (label_.isEmpty())
    {
        trackBounds = bounds.toFloat();
    }
    else if (labelOnRight_)
    {
        trackBounds = juce::Rectangle<float>(
            0, static_cast<float>(bounds.getHeight() - ComponentLayout::TOGGLE_HEIGHT) / 2.0f,
            ComponentLayout::TOGGLE_WIDTH, ComponentLayout::TOGGLE_HEIGHT);

        // Draw label
        auto labelBounds = bounds.toFloat()
            .withLeft(ComponentLayout::TOGGLE_WIDTH + ComponentLayout::SPACING_SM);

        g.setColour(theme_.textPrimary.withAlpha(opacity));
        g.setFont(juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT)));
        g.drawText(label_, labelBounds, juce::Justification::centredLeft);
    }
    else
    {
        auto font = juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT));
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        int labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());

        // Draw label on left
        auto labelBounds = juce::Rectangle<float>(
            0, 0, static_cast<float>(labelWidth), static_cast<float>(bounds.getHeight()));

        g.setColour(theme_.textPrimary.withAlpha(opacity));
        g.setFont(font);
        g.drawText(label_, labelBounds, juce::Justification::centredRight);

        trackBounds = juce::Rectangle<float>(
            static_cast<float>(labelWidth + ComponentLayout::SPACING_SM),
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
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // Interpolate track color based on position
    float progress = positionSpring_.position;
    auto offColor = theme_.controlBorder;
    auto onColor = theme_.statusActive;
    auto trackColor = offColor.interpolatedWith(onColor, progress);

    g.setColour(trackColor.withAlpha(opacity));
    g.fillRoundedRectangle(bounds, bounds.getHeight() / 2.0f);
}

void OscilToggle::paintKnob(juce::Graphics& g, const juce::Rectangle<float>& trackBounds)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float progress = positionSpring_.position;

    // Calculate knob position
    float knobSize = ComponentLayout::TOGGLE_KNOB_SIZE;
    float padding = (trackBounds.getHeight() - knobSize) / 2.0f;
    float minX = trackBounds.getX() + padding;
    float maxX = trackBounds.getRight() - knobSize - padding;
    float knobX = minX + (maxX - minX) * progress;
    float knobY = trackBounds.getY() + padding;

    // Apply celebration scale
    float scale = celebrationSpring_.position;
    float scaledSize = knobSize * scale;
    float offset = (scaledSize - knobSize) / 2.0f;

    auto knobBounds = juce::Rectangle<float>(
        knobX - offset, knobY - offset, scaledSize, scaledSize);

    // Draw knob shadow
    g.setColour(juce::Colours::black.withAlpha(0.2f * opacity));
    g.fillEllipse(knobBounds.translated(0, 1));

    // Draw knob
    g.setColour(juce::Colours::white.withAlpha(opacity));
    g.fillEllipse(knobBounds);
}

void OscilToggle::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    g.setColour(theme_.controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
    g.drawRoundedRectangle(
        bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET),
        bounds.getHeight() / 2.0f + ComponentLayout::FOCUS_RING_OFFSET,
        ComponentLayout::FOCUS_RING_WIDTH
    );
}

void OscilToggle::resized()
{
    // No child components to layout
}

void OscilToggle::mouseDown(const juce::MouseEvent&)
{
    if (enabled_)
    {
        toggle();
    }
}

void OscilToggle::mouseUp(const juce::MouseEvent&)
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

void OscilToggle::focusGained(FocusChangeType)
{
    hasFocus_ = true;
    repaint();
}

void OscilToggle::focusLost(FocusChangeType)
{
    hasFocus_ = false;
    repaint();
}

void OscilToggle::triggerCelebration()
{
    // Respect reduced motion preference
    if (!AnimationSettings::shouldUseSpringAnimations())
        return;

    celebrationSpring_.setTarget(ComponentLayout::CELEBRATION_SCALE, 1.0f);

    // Schedule return to normal scale
    juce::Timer::callAfterDelay(100, [this] {
        celebrationSpring_.setTarget(1.0f);
        if (!isTimerRunning())
            startTimerHz(ComponentLayout::ANIMATION_FPS);
    });
}

void OscilToggle::notifyValueChanged()
{
    if (onValueChanged)
        onValueChanged(value_);
}

void OscilToggle::timerCallback()
{
    updateAnimations();

    if (positionSpring_.isSettled() && celebrationSpring_.isSettled())
        stopTimer();

    repaint();
}

void OscilToggle::updateAnimations()
{
    float dt = AnimationTiming::FRAME_DURATION_60FPS;

    positionSpring_.update(dt);
    celebrationSpring_.update(dt);
}

void OscilToggle::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    repaint();
}

//==============================================================================
// Accessibility Handler for OscilToggle
//==============================================================================
class OscilToggleAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit OscilToggleAccessibilityHandler(OscilToggle& toggle)
        : juce::AccessibilityHandler(toggle, juce::AccessibilityRole::toggleButton,
            juce::AccessibilityActions()
                .addAction(juce::AccessibilityActionType::toggle,
                    [&toggle] { if (toggle.isEnabled()) toggle.toggle(); })
          )
        , toggle_(toggle)
    {
    }

    juce::String getTitle() const override
    {
        return toggle_.getLabel().isNotEmpty() ? toggle_.getLabel() : "Toggle";
    }

    juce::String getDescription() const override
    {
        juce::String state = toggle_.getValue() ? "On" : "Off";
        if (!toggle_.isEnabled())
        {
            state += " (disabled)";
        }
        return state;
    }

    juce::String getHelp() const override
    {
        return "Press Space or Enter to toggle.";
    }

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
