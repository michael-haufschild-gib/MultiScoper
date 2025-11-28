/*
    Oscil - Text Field Component Implementation
*/

#include "ui/components/OscilTextField.h"
#include "ui/components/OscilButton.h"

namespace oscil
{

OscilTextField::OscilTextField()
    : focusSpring_(SpringPresets::stiff())
{
    setupComponents();

    theme_ = ThemeManager::getInstance().getCurrentTheme();
    ThemeManager::getInstance().addListener(this);

    focusSpring_.position = 0.0f;
    focusSpring_.target = 0.0f;
}

OscilTextField::OscilTextField(TextFieldVariant variant)
    : OscilTextField()
{
    setVariant(variant);
}

OscilTextField::~OscilTextField()
{
    ThemeManager::getInstance().removeListener(this);
    stopTimer();
}

void OscilTextField::setupComponents()
{
    // Main text editor
    editor_ = std::make_unique<juce::TextEditor>();
    editor_->setMultiLine(false);
    editor_->setReturnKeyStartsNewLine(false);
    editor_->setScrollbarsShown(false);
    editor_->setPopupMenuEnabled(true);
    editor_->setSelectAllWhenFocused(true);

    editor_->onTextChange = [this] {
        validateAndUpdate();
    };

    editor_->onReturnKey = [this] {
        if (onReturnPressed)
            onReturnPressed();
    };

    editor_->onEscapeKey = [this] {
        if (onEscapePressed)
            onEscapePressed();
    };

    addAndMakeVisible(*editor_);

    // Stepper buttons for Number variant (created but not visible by default)
    decrementButton_ = std::make_unique<OscilButton>("-");
    decrementButton_->setVariant(ButtonVariant::Ghost);
    decrementButton_->onClick = [this] { decrementValue(); };
    addChildComponent(*decrementButton_);

    incrementButton_ = std::make_unique<OscilButton>("+");
    incrementButton_->setVariant(ButtonVariant::Ghost);
    incrementButton_->onClick = [this] { incrementValue(); };
    addChildComponent(*incrementButton_);

    // Internal slider for APVTS (hidden)
    internalSlider_.setRange(minValue_, maxValue_, step_);
    internalSlider_.setValue(numValue_);
    internalSlider_.onValueChange = [this] {
        double newValue = internalSlider_.getValue();
        if (std::abs(newValue - numValue_) > 0.0001)
        {
            setNumericValue(newValue, true);
        }
    };

    updateEditorStyle();
}

void OscilTextField::setVariant(TextFieldVariant variant)
{
    if (variant_ == variant) return;

    variant_ = variant;

    // Show/hide stepper buttons
    bool showSteppers = variant == TextFieldVariant::Number;
    decrementButton_->setVisible(showSteppers);
    incrementButton_->setVisible(showSteppers);

    // Adjust editor input restrictions
    if (variant == TextFieldVariant::Number)
    {
        editor_->setInputRestrictions(20, "0123456789.-");
        updateFromNumericValue();
    }
    else
    {
        editor_->setInputRestrictions(0);
    }

    resized();
    repaint();
}

void OscilTextField::setText(const juce::String& text, bool notify)
{
    editor_->setText(text, notify ? juce::sendNotification : juce::dontSendNotification);
}

juce::String OscilTextField::getText() const
{
    return editor_->getText();
}

void OscilTextField::setPlaceholder(const juce::String& placeholder)
{
    placeholder_ = placeholder;
    editor_->setTextToShowWhenEmpty(placeholder, theme_.textSecondary);
}

void OscilTextField::setRange(double min, double max)
{
    minValue_ = min;
    maxValue_ = max;
    internalSlider_.setRange(min, max, step_);

    // Reapply constraints to current value
    double constrained = numValue_;
    applyNumericConstraints(constrained);
    if (std::abs(constrained - numValue_) > 1e-10)
        setNumericValue(constrained, false);
}

void OscilTextField::setDefaultValue(double defaultValue)
{
    defaultValue_ = defaultValue;
}

void OscilTextField::setStep(double step)
{
    step_ = step;
    internalSlider_.setRange(minValue_, maxValue_, step);
}

void OscilTextField::setDecimalPlaces(int places)
{
    decimalPlaces_ = places;
    if (variant_ == TextFieldVariant::Number)
        updateFromNumericValue();
}

void OscilTextField::setSuffix(const juce::String& suffix)
{
    suffix_ = suffix;
    if (variant_ == TextFieldVariant::Number)
    {
        juce::String valueText = juce::String(numValue_, decimalPlaces_);
        if (suffix_.isNotEmpty())
            valueText += " " + suffix_;
        editor_->setText(valueText, juce::dontSendNotification);
    }
}

void OscilTextField::setNumericValue(double value, bool notify)
{
    applyNumericConstraints(value);

    if (std::abs(numValue_ - value) < 0.0001)
        return;

    numValue_ = value;
    internalSlider_.setValue(value, juce::dontSendNotification);
    updateFromNumericValue();

    if (notify)
        notifyValueChanged();
}

void OscilTextField::updateFromNumericValue()
{
    juce::String valueText = juce::String(numValue_, decimalPlaces_);
    if (suffix_.isNotEmpty())
        valueText += " " + suffix_;

    editor_->setText(valueText, juce::dontSendNotification);
}

void OscilTextField::setValidator(Callbacks::ValidationCallback validator)
{
    validator_ = validator;
}

void OscilTextField::setError(const juce::String& errorMessage)
{
    if (errorMessage_ != errorMessage)
    {
        errorMessage_ = errorMessage;
        repaint();
    }
}

void OscilTextField::clearError()
{
    setError({});
}

void OscilTextField::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        editor_->setEnabled(enabled);
        decrementButton_->setEnabled(enabled);
        incrementButton_->setEnabled(enabled);
        repaint();
    }
}

void OscilTextField::attachToParameter(juce::AudioProcessorValueTreeState& apvts,
                                         const juce::String& paramId)
{
    attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, paramId, internalSlider_);
}

void OscilTextField::detachFromParameter()
{
    attachment_.reset();
}

int OscilTextField::getPreferredHeight() const
{
    return ComponentLayout::INPUT_HEIGHT;
}

void OscilTextField::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    paintBackground(g, bounds);

    if (variant_ == TextFieldVariant::Search)
        paintSearchIcon(g, bounds);

    if (editor_->hasKeyboardFocus(true) && enabled_)
        paintFocusRing(g, bounds);
}

void OscilTextField::paintBackground(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // Background
    g.setColour(theme_.backgroundSecondary.withAlpha(opacity));
    g.fillRoundedRectangle(bounds, ComponentLayout::RADIUS_MD);

    // Border
    auto borderColour = hasError() ? theme_.statusError
                                   : (focusAmount_ > 0.01f ? theme_.controlActive
                                                           : theme_.controlBorder);
    borderColour = borderColour.interpolatedWith(theme_.controlActive, focusAmount_);

    g.setColour(borderColour.withAlpha(opacity));
    g.drawRoundedRectangle(bounds.reduced(0.5f), ComponentLayout::RADIUS_MD, 1.0f);

    // Error message below
    if (hasError())
    {
        g.setColour(theme_.statusError.withAlpha(opacity));
        g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
        g.drawText(errorMessage_,
            bounds.translated(0, bounds.getHeight() + 2).withHeight(14),
            juce::Justification::left);
    }
}

void OscilTextField::paintSearchIcon(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // Draw magnifying glass icon
    auto iconBounds = bounds.withWidth(ICON_WIDTH).reduced(8);
    float cx = iconBounds.getCentreX();
    float cy = iconBounds.getCentreY();
    float radius = 5.0f;

    g.setColour(theme_.textSecondary.withAlpha(opacity));
    g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, 1.5f);
    g.drawLine(cx + radius * 0.7f, cy + radius * 0.7f,
               cx + radius * 1.5f, cy + radius * 1.5f, 1.5f);
}

void OscilTextField::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    g.setColour(theme_.controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA * focusAmount_));
    g.drawRoundedRectangle(
        bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET),
        ComponentLayout::RADIUS_MD + ComponentLayout::FOCUS_RING_OFFSET,
        ComponentLayout::FOCUS_RING_WIDTH
    );
}

void OscilTextField::resized()
{
    auto bounds = getLocalBounds();

    // Calculate editor bounds based on variant
    auto editorBounds = bounds.reduced(2);

    if (variant_ == TextFieldVariant::Search)
    {
        editorBounds.removeFromLeft(ICON_WIDTH - 4);
    }
    else if (variant_ == TextFieldVariant::Number)
    {
        auto stepperBounds = editorBounds.removeFromRight(STEPPER_WIDTH * 2);
        decrementButton_->setBounds(stepperBounds.removeFromLeft(STEPPER_WIDTH));
        incrementButton_->setBounds(stepperBounds);
    }

    editor_->setBounds(editorBounds);
}

void OscilTextField::mouseDown(const juce::MouseEvent& /*e*/)
{
    // Pass to editor
    editor_->grabKeyboardFocus();
}

void OscilTextField::mouseDoubleClick(const juce::MouseEvent&)
{
    if (variant_ == TextFieldVariant::Number && enabled_)
    {
        // Reset to default value
        setNumericValue(defaultValue_, true);
    }
}

void OscilTextField::mouseWheelMove(const juce::MouseEvent& e,
                                     const juce::MouseWheelDetails& wheel)
{
    if (variant_ != TextFieldVariant::Number || !enabled_)
        return;

    double delta = wheel.deltaY * step_;

    // Fine control with Alt
    if (e.mods.isAltDown())
        delta *= ComponentLayout::FINE_CONTROL_FACTOR;

    // Coarse control with Shift
    if (e.mods.isShiftDown())
        delta *= ComponentLayout::COARSE_CONTROL_FACTOR;

    setNumericValue(numValue_ + delta, true);
}

void OscilTextField::focusGained(FocusChangeType /*cause*/)
{
    if (AnimationSettings::shouldUseSpringAnimations())
    {
        focusSpring_.setTarget(1.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        focusAmount_ = 1.0f;
        repaint();
    }
}

void OscilTextField::focusLost(FocusChangeType /*cause*/)
{
    if (AnimationSettings::shouldUseSpringAnimations())
    {
        focusSpring_.setTarget(0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        focusAmount_ = 0.0f;
        repaint();
    }
}

void OscilTextField::timerCallback()
{
    focusSpring_.update(AnimationTiming::FRAME_DURATION_60FPS);
    focusAmount_ = focusSpring_.position;

    if (focusSpring_.isSettled())
        stopTimer();

    repaint();
}

void OscilTextField::updateEditorStyle()
{
    editor_->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    editor_->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    editor_->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    editor_->setColour(juce::TextEditor::textColourId, theme_.textPrimary);
    editor_->setColour(juce::TextEditor::highlightColourId, theme_.controlActive.withAlpha(0.3f));
    editor_->setColour(juce::CaretComponent::caretColourId, theme_.controlActive);

    editor_->setFont(juce::Font(juce::FontOptions().withHeight(13.0f)));
    editor_->setTextToShowWhenEmpty(placeholder_, theme_.textSecondary);
}

void OscilTextField::validateAndUpdate()
{
    juce::String text = editor_->getText();

    // Run validation if configured
    if (validator_)
    {
        if (!validator_(text))
        {
            // Validator returned false - keep previous error or set generic one
            return;
        }
        else
        {
            clearError();
        }
    }

    // For number variant, parse and validate
    if (variant_ == TextFieldVariant::Number)
    {
        // Strip suffix if present
        juce::String numericText = text;
        if (suffix_.isNotEmpty() && text.endsWith(suffix_))
            numericText = text.dropLastCharacters(suffix_.length()).trim();

        double parsed = numericText.getDoubleValue();
        if (std::abs(parsed - numValue_) > 0.0001)
        {
            numValue_ = parsed;
            applyNumericConstraints(numValue_);
            internalSlider_.setValue(numValue_, juce::dontSendNotification);
            notifyValueChanged();
        }
    }
    else
    {
        notifyTextChanged();
    }
}

void OscilTextField::incrementValue()
{
    setNumericValue(numValue_ + step_, true);
}

void OscilTextField::decrementValue()
{
    setNumericValue(numValue_ - step_, true);
}

void OscilTextField::applyNumericConstraints(double& value)
{
    value = juce::jlimit(minValue_, maxValue_, value);

    // Round to step
    if (step_ > 0)
    {
        double steps = std::round((value - minValue_) / step_);
        value = minValue_ + steps * step_;
    }
}

void OscilTextField::notifyTextChanged()
{
    if (onTextChanged)
        onTextChanged(editor_->getText());
}

void OscilTextField::notifyValueChanged()
{
    if (onValueChanged)
        onValueChanged(numValue_);
}

void OscilTextField::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    updateEditorStyle();
    repaint();
}

std::unique_ptr<juce::AccessibilityHandler> OscilTextField::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this,
        variant_ == TextFieldVariant::Number ? juce::AccessibilityRole::slider
                                              : juce::AccessibilityRole::editableText
    );
}

} // namespace oscil
