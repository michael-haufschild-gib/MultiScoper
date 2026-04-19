/*
    MultiScoper - Checkbox Component Implementation
    Flat-surface rendering with spring scale animation.
    (Historical: "glassmorphism rendering" prior to the 2026-Q2 uplift.)
*/

#include "ui/components/MultiScoperCheckbox.h"

#include "ui/components/SurfacePainter.h"

namespace multiscoper
{

MultiScoperCheckbox::MultiScoperCheckbox(IThemeService& themeService)
    : ThemedComponent(themeService)
    , checkSpring_(SpringPresets::medium())
    , hoverSpring_(SpringPresets::fast())
    , scaleSpring_(SpringPresets::springSwitch())
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    checkSpring_.position = 0.0f;
    checkSpring_.target = 0.0f;
    hoverSpring_.position = 0.0f;
    hoverSpring_.target = 0.0f;
    scaleSpring_.position = 1.0f;
    scaleSpring_.target = 1.0f;
}

MultiScoperCheckbox::MultiScoperCheckbox(IThemeService& themeService, const juce::String& label)
    : MultiScoperCheckbox(themeService)
{
    label_ = label;
}

MultiScoperCheckbox::MultiScoperCheckbox(IThemeService& themeService, const juce::String& label,
                                         const juce::String& testId)
    : MultiScoperCheckbox(themeService)
{
    label_ = label;
    setTestId(testId);
}

void MultiScoperCheckbox::registerTestId() { MULTISCOPER_REGISTER_TEST_ID(testId_); }

MultiScoperCheckbox::~MultiScoperCheckbox() { stopTimer(); }

void MultiScoperCheckbox::setChecked(bool checked, bool notify)
{
    setState(checked ? CheckState::Checked : CheckState::Unchecked, notify);
}

void MultiScoperCheckbox::setState(CheckState state, bool notify)
{
    if (state_ == state)
        return;

    state_ = state;

    float const target = (state == CheckState::Checked) ? 1.0f : (state == CheckState::Indeterminate) ? 0.5f : 0.0f;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        checkSpring_.setTarget(target);
        // Trigger scale pop: briefly shrink to 0.85 then spring back to 1.0
        scaleSpring_.position = 0.85f;
        scaleSpring_.velocity = 0.0f;
        scaleSpring_.setTarget(1.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        checkSpring_.snapToTarget();
        checkSpring_.position = target;
        repaint();
    }

    if (notify)
        notifyStateChanged();
}

void MultiScoperCheckbox::toggle()
{
    if (!enabled_)
        return;

    // Cycle: Unchecked -> Checked -> Unchecked (skip Indeterminate in toggle)
    setChecked(state_ != CheckState::Checked);
}

void MultiScoperCheckbox::setLabel(const juce::String& label)
{
    if (label_ != label)
    {
        label_ = label;
        repaint();
    }
}

void MultiScoperCheckbox::setLabelOnRight(bool onRight)
{
    if (labelOnRight_ != onRight)
    {
        labelOnRight_ = onRight;
        repaint();
    }
}

void MultiScoperCheckbox::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        juce::Component::setEnabled(enabled);
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

int MultiScoperCheckbox::getPreferredWidth() const
{
    int const boxWidth = ComponentLayout::CHECKBOX_SIZE;

    if (label_.isNotEmpty())
    {
        auto font = ComponentLayout::defaultFont();
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        int const labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
        return boxWidth + ComponentLayout::SPACING_SM + labelWidth;
    }

    return boxWidth;
}

int MultiScoperCheckbox::getPreferredHeight() const
{
    return std::max(ComponentLayout::CHECKBOX_SIZE, static_cast<int>(ComponentLayout::defaultFont().getHeight()));
}

void MultiScoperCheckbox::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // Calculate box bounds
    juce::Rectangle<float> boxBounds;

    if (label_.isEmpty())
    {
        boxBounds =
            bounds.toFloat().withSizeKeepingCentre(ComponentLayout::CHECKBOX_SIZE, ComponentLayout::CHECKBOX_SIZE);
    }
    else if (labelOnRight_)
    {
        boxBounds =
            juce::Rectangle<float>(0, (static_cast<float>(bounds.getHeight()) - ComponentLayout::CHECKBOX_SIZE) / 2.0f,
                                   ComponentLayout::CHECKBOX_SIZE, ComponentLayout::CHECKBOX_SIZE);

        // Draw label
        auto labelBounds = bounds.toFloat().withLeft(ComponentLayout::CHECKBOX_SIZE + ComponentLayout::SPACING_SM);

        g.setColour(getTheme().textPrimary.withAlpha(opacity));
        g.setFont(ComponentLayout::defaultFont());
        g.drawText(label_, labelBounds, juce::Justification::centredLeft);
    }
    else
    {
        auto font = ComponentLayout::defaultFont();
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        float const labelWidthF = glyphs.getBoundingBox(0, -1, false).getWidth();

        // Draw label on left
        auto labelBounds = juce::Rectangle<float>(0, 0, labelWidthF, static_cast<float>(bounds.getHeight()));

        g.setColour(getTheme().textPrimary.withAlpha(opacity));
        g.setFont(font);
        g.drawText(label_, labelBounds, juce::Justification::centredRight);

        boxBounds =
            juce::Rectangle<float>(labelWidthF + ComponentLayout::SPACING_SM,
                                   (static_cast<float>(bounds.getHeight()) - ComponentLayout::CHECKBOX_SIZE) / 2.0f,
                                   ComponentLayout::CHECKBOX_SIZE, ComponentLayout::CHECKBOX_SIZE);
    }

    // Apply scale animation around box center
    float const scale = scaleSpring_.position;
    if (std::abs(scale - 1.0f) > 0.001f)
    {
        boxBounds = boxBounds.withSizeKeepingCentre(boxBounds.getWidth() * scale, boxBounds.getHeight() * scale);
    }

    paintBox(g, boxBounds);

    if (state_ == CheckState::Checked || checkSpring_.position > 0.5f)
        paintCheckMark(g, boxBounds);
    else if (state_ == CheckState::Indeterminate || (checkSpring_.position > 0.1f && checkSpring_.position <= 0.5f))
        paintIndeterminate(g, boxBounds);

    if (hasFocus_ && enabled_)
        paintFocusRing(g, boxBounds);
}

void MultiScoperCheckbox::paintBox(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float const hoverAmount = hoverSpring_.position;
    const auto& surface = getSurface();

    bool const isCheckedOrTransitioning = (state_ != CheckState::Unchecked || checkSpring_.position > 0.01f);

    if (isCheckedOrTransitioning)
    {
        // Checked state: accent fill
        float const fillAmount = std::min(1.0f, checkSpring_.position * 2.0f);
        auto bgColour = surface.bgGlass.interpolatedWith(surface.accent, fillAmount);

        // Hover on checked: slightly brighter accent
        if (hoverAmount > 0.01f && fillAmount > 0.5f)
            bgColour = bgColour.brighter(0.1f * hoverAmount);

        g.setColour(bgColour.withAlpha(bgColour.getFloatAlpha() * opacity));
        g.fillRect(bounds);

        // Border: accent when checked
        auto borderColour = surface.borderDefault.interpolatedWith(surface.accent, fillAmount);
        g.setColour(borderColour.withAlpha(borderColour.getFloatAlpha() * opacity));
        g.drawRect(bounds, 1.0f);
    }
    else
    {
        // Unchecked state: bgGlass fill + borderDefault border
        g.setColour(surface.bgGlass.withAlpha(surface.bgGlass.getFloatAlpha() * opacity));
        g.fillRect(bounds);

        // Hover: borderStrong instead of borderDefault
        auto borderColour = (hoverAmount > 0.5f) ? surface.borderStrong : surface.borderDefault;
        g.setColour(borderColour.withAlpha(borderColour.getFloatAlpha() * opacity));
        g.drawRect(bounds, 1.0f);
    }
}

void MultiScoperCheckbox::paintCheckMark(juce::Graphics& g, const juce::Rectangle<float>& bounds) const
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float const progress = std::clamp((checkSpring_.position - 0.5f) * 2.0f, 0.0f, 1.0f);

    if (progress < 0.01f)
        return;

    // Checkmark on accent fill: pick contrast-safe colour. White over a
    // bright cyan accent (Dark Pro) fails WCAG AA; this falls back to
    // near-black for bright accents and stays white for dark accents.
    g.setColour(ColorTheme::pickContrastingText(getSurface().accent).withAlpha(opacity * progress));

    // Draw animated checkmark
    float const cx = bounds.getCentreX();
    float const cy = bounds.getCentreY();
    float const size = bounds.getWidth() * 0.5f;

    juce::Path checkPath;
    checkPath.startNewSubPath(cx - (size * 0.35f), cy);
    checkPath.lineTo(cx - (size * 0.05f), cy + (size * 0.3f));
    checkPath.lineTo(cx + (size * 0.35f), cy - (size * 0.25f));

    // Animate drawing the path
    juce::PathStrokeType const stroke(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    g.strokePath(checkPath, stroke);
}

void MultiScoperCheckbox::paintIndeterminate(juce::Graphics& g, const juce::Rectangle<float>& bounds) const
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // See paintCheckMark: contrast-safe colour against the accent fill.
    g.setColour(ColorTheme::pickContrastingText(getSurface().accent).withAlpha(opacity));

    float const lineWidth = bounds.getWidth() * 0.5f;
    float const lineHeight = 2.0f;

    auto lineBounds = juce::Rectangle<float>(bounds.getCentreX() - (lineWidth / 2),
                                             bounds.getCentreY() - (lineHeight / 2), lineWidth, lineHeight);

    g.fillRect(lineBounds);
}

void MultiScoperCheckbox::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    SurfacePainter::paintFocusRing(g, bounds, 0.0f, getSurface().accent);
}

void MultiScoperCheckbox::resized()
{
    // No child components
}

void MultiScoperCheckbox::mouseDown(const juce::MouseEvent& /*event*/)
{
    if (enabled_)
        isPressed_ = true;
}

void MultiScoperCheckbox::mouseUp(const juce::MouseEvent& e)
{
    if (isPressed_ && enabled_ && contains(e.getPosition()))
    {
        toggle();
    }
    isPressed_ = false;
}

void MultiScoperCheckbox::mouseEnter(const juce::MouseEvent& /*event*/)
{
    if (!enabled_)
        return;

    isHovered_ = true;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        hoverSpring_.setTarget(1.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        hoverSpring_.position = 1.0f;
        repaint();
    }
}

void MultiScoperCheckbox::mouseExit(const juce::MouseEvent& /*event*/)
{
    isHovered_ = false;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        hoverSpring_.setTarget(0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        hoverSpring_.position = 0.0f;
        repaint();
    }
}

void MultiScoperCheckbox::focusGained(FocusChangeType /*cause*/)
{
    hasFocus_ = true;
    repaint();
}

void MultiScoperCheckbox::focusLost(FocusChangeType /*cause*/)
{
    hasFocus_ = false;
    repaint();
}

void MultiScoperCheckbox::timerCallback()
{
    updateAnimations();

    if (checkSpring_.isSettled() && hoverSpring_.isSettled() && scaleSpring_.isSettled())
        stopTimer();

    repaint();
}

void MultiScoperCheckbox::updateAnimations()
{
    float const dt = AnimationTiming::FRAME_DURATION_60FPS;
    checkSpring_.update(dt);
    hoverSpring_.update(dt);
    scaleSpring_.update(dt);
}

void MultiScoperCheckbox::notifyStateChanged()
{
    if (onStateChanged)
        onStateChanged(state_);

    if (onCheckedChanged)
        onCheckedChanged(state_ == CheckState::Checked);
}

std::unique_ptr<juce::AccessibilityHandler> MultiScoperCheckbox::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this, juce::AccessibilityRole::toggleButton,
        juce::AccessibilityActions().addAction(juce::AccessibilityActionType::toggle, [this] {
            if (enabled_)
                toggle();
        }));
}

} // namespace multiscoper
