/*
    MultiScoper - Text Field Component Implementation
*/

#include "ui/components/MultiScoperTextField.h"

#include "ui/components/MultiScoperButton.h"

namespace multiscoper
{

MultiScoperTextField::MultiScoperTextField(IThemeService& themeService)
    : ThemedComponent(themeService)
    , focusSpring_(SpringPresets::fast())
    , shakeSpring_(600.0f, 12.0f, 1.0f)
    , cachedErrorFont_(juce::FontOptions{})
{
    // Make the wrapper focusable so screen readers and the e2e harness
    // discover this element via Tab / focus enumeration. The wrapper is
    // a transparent focus target — its focusGained override (in the
    // Painting TU) redirects grabKeyboardFocus() to the inner TextEditor
    // so the actual keyboard state lives on the editor. Without this,
    // keyboard users could only reach the field via mouse click.
    setWantsKeyboardFocus(true);

    setupComponents();

    focusSpring_.position = 0.0f;
    focusSpring_.target = 0.0f;

    shakeSpring_.setMode(SpringMode::Spring);
    shakeSpring_.position = 0.0f;
    shakeSpring_.target = 0.0f;
}

MultiScoperTextField::MultiScoperTextField(IThemeService& themeService, TextFieldVariant variant)
    : MultiScoperTextField(themeService)
{
    setVariant(variant);
}

MultiScoperTextField::MultiScoperTextField(IThemeService& themeService, const juce::String& testId)
    : MultiScoperTextField(themeService)
{
    setTestId(testId);
}

MultiScoperTextField::MultiScoperTextField(IThemeService& themeService, TextFieldVariant variant,
                                           const juce::String& testId)
    : MultiScoperTextField(themeService)
{
    setVariant(variant);
    setTestId(testId);
}

void MultiScoperTextField::registerTestId() { MULTISCOPER_REGISTER_TEST_ID(testId_); }

MultiScoperTextField::~MultiScoperTextField() { stopTimer(); }

void MultiScoperTextField::setupComponents()
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
    decrementButton_ = std::make_unique<MultiScoperButton>(getThemeService(), "-");
    decrementButton_->setVariant(ButtonVariant::Primary);
    decrementButton_->onClick = [this] { decrementValue(); };
    addChildComponent(*decrementButton_);

    incrementButton_ = std::make_unique<MultiScoperButton>(getThemeService(), "+");
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

void MultiScoperTextField::setVariant(TextFieldVariant variant)
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

void MultiScoperTextField::setText(const juce::String& text, bool notify) { editor_->setText(text, notify); }

juce::String MultiScoperTextField::getText() const { return editor_->getText(); }

void MultiScoperTextField::grabInnerFocus()
{
    if (editor_ != nullptr)
        editor_->grabKeyboardFocus();
}

void MultiScoperTextField::setPlaceholder(const juce::String& placeholder)
{
    placeholder_ = placeholder;
    editor_->setTextToShowWhenEmpty(placeholder, getTheme().textSecondary);
}

void MultiScoperTextField::setRange(double min, double max)
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

void MultiScoperTextField::setDefaultValue(double defaultValue) { defaultValue_ = defaultValue; }

void MultiScoperTextField::setStep(double step)
{
    step_ = step;
    internalSlider_.setRange(minValue_, maxValue_, step);
}

void MultiScoperTextField::setDecimalPlaces(int places)
{
    decimalPlaces_ = places;
    if (variant_ == TextFieldVariant::Number)
        updateFromNumericValue();
}

void MultiScoperTextField::setSuffix(const juce::String& suffix)
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

void MultiScoperTextField::setNumericValue(double value, bool notify)
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

void MultiScoperTextField::updateFromNumericValue()
{
    juce::String valueText = juce::String(numValue_, decimalPlaces_);
    if (suffix_.isNotEmpty())
        valueText += " " + suffix_;

    editor_->setText(valueText, false);
}

void MultiScoperTextField::setValidator(Callbacks::ValidationCallback validator) { validator_ = validator; }

void MultiScoperTextField::setError(const juce::String& errorMessage)
{
    if (errorMessage_ != errorMessage)
    {
        errorMessage_ = errorMessage;

        // Trigger shake animation on new error
        if (errorMessage.isNotEmpty() && AnimationSettings::shouldUseSpringAnimations())
        {
            shakeSpring_.setTarget(0.0f, 3.0f);
            startTimerHz(ComponentLayout::ANIMATION_FPS);
        }

        repaint();
    }
}

void MultiScoperTextField::clearError() { setError({}); }

void MultiScoperTextField::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        juce::Component::setEnabled(enabled);
        editor_->setEnabled(enabled);
        decrementButton_->setEnabled(enabled);
        incrementButton_->setEnabled(enabled);
        repaint();
    }
}

void MultiScoperTextField::attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
{
    attachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, internalSlider_);
}

void MultiScoperTextField::detachFromParameter() { attachment_.reset(); }

int MultiScoperTextField::getPreferredHeight() const { return ComponentLayout::INPUT_HEIGHT; }

// paint, paintBackground, paintSearchIcon, paintFocusRing are in MultiScoperTextFieldPainting.cpp

void MultiScoperTextField::onThemeChanged(const ColorTheme& /*newTheme*/)
{
    // The wrapped juce::TextEditor caches its colour palette and font when
    // setColour() is called. Without re-applying the editor style here, the
    // text colour stays frozen at construction time and won't follow theme
    // changes (visible in the BPM number field after switching themes).
    updateEditorStyle();
}

void MultiScoperTextField::resized()
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
        constexpr int kStepperGap = 1; // 1px breathing room between - and +
        auto stepperBounds = editorBounds.removeFromRight((STEPPER_WIDTH * 2) + kStepperGap);
        decrementButton_->setBounds(stepperBounds.removeFromLeft(STEPPER_WIDTH));
        stepperBounds.removeFromLeft(kStepperGap);
        incrementButton_->setBounds(stepperBounds);
    }

    editor_->setBounds(editorBounds);
}

// mouseDown, mouseDoubleClick, mouseWheelMove, focusGained, focusLost,
// timerCallback, updateEditorStyle, createAccessibilityHandler are in MultiScoperTextFieldPainting.cpp

void MultiScoperTextField::validateAndUpdate()
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
            updateFromNumericValue();
            notifyValueChanged();
        }
    }
    else
    {
        notifyTextChanged();
    }
}

void MultiScoperTextField::incrementValue() { setNumericValue(numValue_ + step_, true); }

void MultiScoperTextField::decrementValue() { setNumericValue(numValue_ - step_, true); }

void MultiScoperTextField::applyNumericConstraints(double& value) const
{
    value = juce::jlimit(minValue_, maxValue_, value);

    // Round to step
    if (step_ > 0)
    {
        double const steps = std::round((value - minValue_) / step_);
        value = minValue_ + (steps * step_);
    }
}

void MultiScoperTextField::notifyTextChanged()
{
    if (onTextChanged)
        onTextChanged(editor_->getText());
}

void MultiScoperTextField::notifyValueChanged() const
{
    if (onValueChanged)
        onValueChanged(numValue_);
}

} // namespace multiscoper
