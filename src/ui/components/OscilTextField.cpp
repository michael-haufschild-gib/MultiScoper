/*
    Oscil - Text Field Component Implementation
*/

#include "ui/components/OscilTextField.h"

#include "ui/components/OscilButton.h"

namespace oscil
{

OscilTextField::OscilTextField(IThemeService& themeService)
    : ThemedComponent(themeService)
    , focusSpring_(SpringPresets::fast())
    , cachedErrorFont_(juce::FontOptions{})
{
    setupComponents();

    focusSpring_.position = 0.0f;
    focusSpring_.target = 0.0f;
}

OscilTextField::OscilTextField(IThemeService& themeService, TextFieldVariant variant) : OscilTextField(themeService)
{
    setVariant(variant);
}

OscilTextField::OscilTextField(IThemeService& themeService, const juce::String& testId) : OscilTextField(themeService)
{
    setTestId(testId);
}

OscilTextField::OscilTextField(IThemeService& themeService, TextFieldVariant variant, const juce::String& testId)
    : OscilTextField(themeService)
{
    setVariant(variant);
    setTestId(testId);
}

void OscilTextField::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

OscilTextField::~OscilTextField() { stopTimer(); }

void OscilTextField::setupComponents()
{
    // Main text editor
    editor_ = std::make_unique<juce::TextEditor>();
    editor_->setMultiLine(false);
    editor_->setReturnKeyStartsNewLine(false);
    editor_->setScrollbarsShown(false);
    editor_->setPopupMenuEnabled(true);
    editor_->setSelectAllWhenFocused(true);

    editor_->onTextChange = [this] { validateAndUpdate(); };

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
    // Using Primary variant (blue) for clear visibility
    decrementButton_ = std::make_unique<OscilButton>(getThemeService(), "-");
    decrementButton_->setVariant(ButtonVariant::Primary);
    decrementButton_->onClick = [this] { decrementValue(); };
    addChildComponent(*decrementButton_);

    incrementButton_ = std::make_unique<OscilButton>(getThemeService(), "+");
    incrementButton_->setVariant(ButtonVariant::Primary);
    incrementButton_->onClick = [this] { incrementValue(); };
    addChildComponent(*incrementButton_);

    // Internal slider for APVTS (hidden)
    internalSlider_.setRange(minValue_, maxValue_, step_);
    internalSlider_.setValue(numValue_);
    internalSlider_.onValueChange = [this] {
        double const newValue = internalSlider_.getValue();
        if (std::abs(newValue - numValue_) > 0.0001)
        {
            setNumericValue(newValue, true);
        }
    };

    updateEditorStyle();
}

void OscilTextField::setVariant(TextFieldVariant variant)
{
    if (variant_ == variant)
        return;

    variant_ = variant;

    // Show/hide stepper buttons
    bool const showSteppers = variant == TextFieldVariant::Number;
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

void OscilTextField::setText(const juce::String& text, bool notify) { editor_->setText(text, notify); }

juce::String OscilTextField::getText() const { return editor_->getText(); }

void OscilTextField::setPlaceholder(const juce::String& placeholder)
{
    placeholder_ = placeholder;
    editor_->setTextToShowWhenEmpty(placeholder, getTheme().textSecondary);
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

void OscilTextField::setDefaultValue(double defaultValue) { defaultValue_ = defaultValue; }

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
        editor_->setText(valueText, false);
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

    editor_->setText(valueText, false);
}

void OscilTextField::setValidator(Callbacks::ValidationCallback validator) { validator_ = validator; }

void OscilTextField::setError(const juce::String& errorMessage)
{
    if (errorMessage_ != errorMessage)
    {
        errorMessage_ = errorMessage;
        repaint();
    }
}

void OscilTextField::clearError() { setError({}); }

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

void OscilTextField::attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
{
    attachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, internalSlider_);
}

void OscilTextField::detachFromParameter() { attachment_.reset(); }

int OscilTextField::getPreferredHeight() const { return ComponentLayout::INPUT_HEIGHT; }

// paint, paintBackground, paintSearchIcon, paintFocusRing are in OscilTextFieldPainting.cpp

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

// mouseDown, mouseDoubleClick, mouseWheelMove, focusGained, focusLost,
// timerCallback, updateEditorStyle, createAccessibilityHandler are in OscilTextFieldPainting.cpp

void OscilTextField::validateAndUpdate()
{
    juce::String const text = editor_->getText();

    // Run validation if configured
    if (validator_)
    {
        if (!validator_(text))
        {
            // Validator returned false - keep previous error or set generic one
            return;
        }

        clearError();
    }

    // For number variant, parse and validate
    if (variant_ == TextFieldVariant::Number)
    {
        // Strip suffix if present
        juce::String numericText = text;
        if (suffix_.isNotEmpty() && text.endsWith(suffix_))
            numericText = text.dropLastCharacters(suffix_.length()).trim();

        double const parsed = numericText.getDoubleValue();
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

void OscilTextField::incrementValue() { setNumericValue(numValue_ + step_, true); }

void OscilTextField::decrementValue() { setNumericValue(numValue_ - step_, true); }

void OscilTextField::applyNumericConstraints(double& value) const
{
    value = juce::jlimit(minValue_, maxValue_, value);

    // Round to step
    if (step_ > 0)
    {
        double const steps = std::round((value - minValue_) / step_);
        value = minValue_ + (steps * step_);
    }
}

void OscilTextField::notifyTextChanged()
{
    if (onTextChanged)
        onTextChanged(editor_->getText());
}

void OscilTextField::notifyValueChanged() const
{
    if (onValueChanged)
        onValueChanged(numValue_);
}

} // namespace oscil
