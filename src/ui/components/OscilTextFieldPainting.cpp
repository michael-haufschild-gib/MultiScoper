/*
    Oscil - Text Field Component Painting & Events
    (Core setup and logic are in OscilTextField.cpp)
*/

#include "ui/components/OscilButton.h"
#include "ui/components/OscilTextField.h"

namespace oscil
{

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
    g.setColour(getTheme().backgroundSecondary.withAlpha(opacity));
    g.fillRoundedRectangle(bounds, ComponentLayout::RADIUS_MD);

    // Border
    auto borderColour = hasError() ? getTheme().statusError
                                   : (focusAmount_ > 0.01f ? getTheme().controlActive : getTheme().controlBorder);
    borderColour = borderColour.interpolatedWith(getTheme().controlActive, focusAmount_);

    g.setColour(borderColour.withAlpha(opacity));
    g.drawRoundedRectangle(bounds.reduced(0.5f), ComponentLayout::RADIUS_MD, 1.0f);

    // Error message below
    if (hasError())
    {
        g.setColour(getTheme().statusError.withAlpha(opacity));
        g.setFont(cachedErrorFont_);
        g.drawText(errorMessage_, bounds.translated(0, bounds.getHeight() + 2).withHeight(14),
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

    g.setColour(getTheme().textSecondary.withAlpha(opacity));
    g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, ComponentLayout::BORDER_MEDIUM);
    g.drawLine(cx + radius * 0.7f, cy + radius * 0.7f, cx + radius * 1.5f, cy + radius * 1.5f,
               ComponentLayout::BORDER_MEDIUM);
}

void OscilTextField::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    g.setColour(getTheme().controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA * focusAmount_));
    g.drawRoundedRectangle(bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET),
                           ComponentLayout::RADIUS_MD + ComponentLayout::FOCUS_RING_OFFSET,
                           ComponentLayout::FOCUS_RING_WIDTH);
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

void OscilTextField::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
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
    editor_->setColour(juce::TextEditor::textColourId, getTheme().textPrimary);
    editor_->setColour(juce::TextEditor::highlightColourId, getTheme().controlActive.withAlpha(0.3f));
    editor_->setColour(juce::CaretComponent::caretColourId, getTheme().controlActive);

    editor_->setFont(juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT)));
    editor_->setTextToShowWhenEmpty(placeholder_, getTheme().textSecondary);

    cachedErrorFont_ = juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_CAPTION));
}

std::unique_ptr<juce::AccessibilityHandler> OscilTextField::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(*this, variant_ == TextFieldVariant::Number
                                                                   ? juce::AccessibilityRole::slider
                                                                   : juce::AccessibilityRole::editableText);
}

} // namespace oscil
