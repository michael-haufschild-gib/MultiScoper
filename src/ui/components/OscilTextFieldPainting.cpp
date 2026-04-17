/*
    Oscil - Text Field Component Painting & Events
    (Core setup and logic are in OscilTextField.cpp)
*/

#include "ui/components/OscilButton.h"
#include "ui/components/OscilTextField.h"
#include "ui/components/SurfacePainter.h"

namespace oscil
{

void OscilTextField::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Apply shake offset for error animation
    if (shakeSpring_.needsUpdate() || std::abs(shakeSpring_.position) > 0.01f)
        bounds = bounds.translated(shakeSpring_.position, 0.0f);

    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    if (!enabled_)
        g.setOpacity(opacity);

    paintBackground(g, bounds);

    if (variant_ == TextFieldVariant::Search)
        paintSearchIcon(g, bounds);

    // Focus ring via SurfacePainter when focused
    if (hasFocus_ && enabled_ && !hasError())
        SurfacePainter::paintFocusRing(g, bounds, ComponentLayout::RADIUS_MD, getSurface().accent);

    if (!enabled_)
        g.setOpacity(1.0f);
}

void OscilTextField::paintBackground(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    // Use SurfacePainter::paintInput for all states
    SurfacePainter::paintInput(g, bounds, getSurface(), ComponentLayout::RADIUS_MD, hasFocus_, isHovered_, hasError(),
                               getTheme().statusError);

    // Error message below
    if (hasError())
    {
        // Restore full graphics opacity so error text isn't double-dimmed
        // when the parent paint() already applied DISABLED_OPACITY
        g.setOpacity(1.0f);
        float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
        g.setColour(getTheme().statusError.withAlpha(opacity));
        g.setFont(cachedErrorFont_);
        g.drawText(errorMessage_, bounds.translated(0, bounds.getHeight() + 2).withHeight(14),
                   juce::Justification::left);
    }
}

void OscilTextField::paintSearchIcon(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // Draw magnifying glass icon
    auto iconBounds = bounds.withWidth(ICON_WIDTH).reduced(8);
    float const cx = iconBounds.getCentreX();
    float const cy = iconBounds.getCentreY();
    float const radius = 5.0f;

    g.setColour(getTheme().textSecondary.withAlpha(opacity));
    g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, ComponentLayout::BORDER_MEDIUM);
    g.drawLine(cx + (radius * 0.7f), cy + (radius * 0.7f), cx + (radius * 1.5f), cy + (radius * 1.5f),
               ComponentLayout::BORDER_MEDIUM);
}

void OscilTextField::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    SurfacePainter::paintFocusRing(g, bounds, ComponentLayout::RADIUS_MD, getSurface().accent);
}

void OscilTextField::mouseDown(const juce::MouseEvent& /*e*/)
{
    // Pass to editor
    editor_->grabKeyboardFocus();
}

void OscilTextField::mouseEnter(const juce::MouseEvent& /*event*/)
{
    isHovered_ = true;
    repaint();
}

void OscilTextField::mouseExit(const juce::MouseEvent& /*event*/)
{
    isHovered_ = false;
    repaint();
}

void OscilTextField::mouseDoubleClick(const juce::MouseEvent& /*event*/)
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
    hasFocus_ = true;

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
    hasFocus_ = false;

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
    shakeSpring_.update(AnimationTiming::FRAME_DURATION_60FPS);
    focusAmount_ = focusSpring_.position;

    if (focusSpring_.isSettled() && shakeSpring_.isSettled())
        stopTimer();

    repaint();
}

void OscilTextField::updateEditorStyle()
{
    editor_->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    editor_->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    editor_->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    editor_->setColour(juce::TextEditor::textColourId, getTheme().textPrimary);
    editor_->setColour(juce::TextEditor::highlightColourId, getSurface().accent.withAlpha(0.3f));
    editor_->setColour(juce::CaretComponent::caretColourId, getSurface().accent);

    editor_->setFont(ComponentLayout::defaultFont());
    editor_->setTextToShowWhenEmpty(placeholder_, getTheme().textSecondary);

    cachedErrorFont_ = ComponentLayout::captionFont();
}

std::unique_ptr<juce::AccessibilityHandler> OscilTextField::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(*this, variant_ == TextFieldVariant::Number
                                                                   ? juce::AccessibilityRole::slider
                                                                   : juce::AccessibilityRole::editableText);
}

} // namespace oscil
