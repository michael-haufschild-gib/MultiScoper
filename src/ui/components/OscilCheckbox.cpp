/*
    Oscil - Checkbox Component Implementation
*/

#include "ui/components/OscilCheckbox.h"

namespace oscil
{

OscilCheckbox::OscilCheckbox()
    : checkSpring_(SpringPresets::bouncy())
    , hoverSpring_(SpringPresets::stiff())
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    theme_ = ThemeManager::getInstance().getCurrentTheme();
    ThemeManager::getInstance().addListener(this);

    checkSpring_.position = 0.0f;
    checkSpring_.target = 0.0f;
    hoverSpring_.position = 0.0f;
    hoverSpring_.target = 0.0f;
}

OscilCheckbox::OscilCheckbox(const juce::String& label)
    : OscilCheckbox()
{
    label_ = label;
}

OscilCheckbox::~OscilCheckbox()
{
    ThemeManager::getInstance().removeListener(this);
    stopTimer();
}

void OscilCheckbox::setChecked(bool checked, bool notify)
{
    setState(checked ? CheckState::Checked : CheckState::Unchecked, notify);
}

void OscilCheckbox::setState(CheckState state, bool notify)
{
    if (state_ == state) return;

    state_ = state;

    float target = (state == CheckState::Checked) ? 1.0f
                 : (state == CheckState::Indeterminate) ? 0.5f
                 : 0.0f;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        checkSpring_.setTarget(target);
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

void OscilCheckbox::toggle()
{
    if (!enabled_) return;

    // Cycle: Unchecked -> Checked -> Unchecked (skip Indeterminate in toggle)
    setChecked(state_ != CheckState::Checked);
}

void OscilCheckbox::setLabel(const juce::String& label)
{
    if (label_ != label)
    {
        label_ = label;
        repaint();
    }
}

void OscilCheckbox::setLabelOnRight(bool onRight)
{
    if (labelOnRight_ != onRight)
    {
        labelOnRight_ = onRight;
        repaint();
    }
}

void OscilCheckbox::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor
                               : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

int OscilCheckbox::getPreferredWidth() const
{
    int boxWidth = ComponentLayout::CHECKBOX_SIZE;

    if (label_.isNotEmpty())
    {
        auto font = juce::Font(juce::FontOptions().withHeight(13.0f));
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        int labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
        return boxWidth + ComponentLayout::SPACING_SM + labelWidth;
    }

    return boxWidth;
}

int OscilCheckbox::getPreferredHeight() const
{
    return std::max(ComponentLayout::CHECKBOX_SIZE,
                    static_cast<int>(juce::Font(juce::FontOptions().withHeight(13.0f)).getHeight()));
}

void OscilCheckbox::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // Calculate box bounds
    juce::Rectangle<float> boxBounds;

    if (label_.isEmpty())
    {
        boxBounds = bounds.toFloat().withSizeKeepingCentre(
            ComponentLayout::CHECKBOX_SIZE, ComponentLayout::CHECKBOX_SIZE);
    }
    else if (labelOnRight_)
    {
        boxBounds = juce::Rectangle<float>(
            0, (static_cast<float>(bounds.getHeight()) - ComponentLayout::CHECKBOX_SIZE) / 2.0f,
            ComponentLayout::CHECKBOX_SIZE, ComponentLayout::CHECKBOX_SIZE);

        // Draw label
        auto labelBounds = bounds.toFloat()
            .withLeft(ComponentLayout::CHECKBOX_SIZE + ComponentLayout::SPACING_SM);

        g.setColour(theme_.textPrimary.withAlpha(opacity));
        g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f)));
        g.drawText(label_, labelBounds, juce::Justification::centredLeft);
    }
    else
    {
        auto font = juce::Font(juce::FontOptions().withHeight(13.0f));
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        float labelWidthF = glyphs.getBoundingBox(0, -1, false).getWidth();

        // Draw label on left
        auto labelBounds = juce::Rectangle<float>(
            0, 0, labelWidthF, static_cast<float>(bounds.getHeight()));

        g.setColour(theme_.textPrimary.withAlpha(opacity));
        g.setFont(font);
        g.drawText(label_, labelBounds, juce::Justification::centredRight);

        boxBounds = juce::Rectangle<float>(
            labelWidthF + ComponentLayout::SPACING_SM,
            (static_cast<float>(bounds.getHeight()) - ComponentLayout::CHECKBOX_SIZE) / 2.0f,
            ComponentLayout::CHECKBOX_SIZE, ComponentLayout::CHECKBOX_SIZE);
    }

    paintBox(g, boxBounds);

    if (state_ == CheckState::Checked || checkSpring_.position > 0.5f)
        paintCheckMark(g, boxBounds);
    else if (state_ == CheckState::Indeterminate ||
             (checkSpring_.position > 0.1f && checkSpring_.position <= 0.5f))
        paintIndeterminate(g, boxBounds);

    if (hasFocus_ && enabled_)
        paintFocusRing(g, boxBounds);
}

void OscilCheckbox::paintBox(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float hoverAmount = hoverSpring_.position;

    // Background
    auto bgColour = theme_.backgroundSecondary;
    if (state_ != CheckState::Unchecked || checkSpring_.position > 0.01f)
    {
        float fillAmount = std::min(1.0f, checkSpring_.position * 2.0f);
        bgColour = bgColour.interpolatedWith(theme_.controlActive, fillAmount);
    }

    if (hoverAmount > 0.01f)
        bgColour = bgColour.brighter(0.1f * hoverAmount);

    g.setColour(bgColour.withAlpha(opacity));
    g.fillRoundedRectangle(bounds, ComponentLayout::RADIUS_SM);

    // Border
    auto borderColour = state_ != CheckState::Unchecked
        ? theme_.controlActive : theme_.controlBorder;

    g.setColour(borderColour.withAlpha(opacity));
    g.drawRoundedRectangle(bounds.reduced(0.5f), ComponentLayout::RADIUS_SM, 1.0f);
}

void OscilCheckbox::paintCheckMark(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float progress = std::clamp((checkSpring_.position - 0.5f) * 2.0f, 0.0f, 1.0f);

    if (progress < 0.01f) return;

    g.setColour(juce::Colours::white.withAlpha(opacity * progress));

    // Draw animated checkmark
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float size = bounds.getWidth() * 0.5f;

    juce::Path checkPath;
    checkPath.startNewSubPath(cx - size * 0.35f, cy);
    checkPath.lineTo(cx - size * 0.05f, cy + size * 0.3f);
    checkPath.lineTo(cx + size * 0.35f, cy - size * 0.25f);

    // Animate drawing the path
    juce::PathStrokeType stroke(2.0f, juce::PathStrokeType::curved,
                                 juce::PathStrokeType::rounded);

    g.strokePath(checkPath, stroke);
}

void OscilCheckbox::paintIndeterminate(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    g.setColour(juce::Colours::white.withAlpha(opacity));

    float lineWidth = bounds.getWidth() * 0.5f;
    float lineHeight = 2.0f;

    auto lineBounds = juce::Rectangle<float>(
        bounds.getCentreX() - lineWidth / 2,
        bounds.getCentreY() - lineHeight / 2,
        lineWidth, lineHeight);

    g.fillRoundedRectangle(lineBounds, 1.0f);
}

void OscilCheckbox::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    g.setColour(theme_.controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
    g.drawRoundedRectangle(
        bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET),
        ComponentLayout::RADIUS_SM + ComponentLayout::FOCUS_RING_OFFSET,
        ComponentLayout::FOCUS_RING_WIDTH
    );
}

void OscilCheckbox::resized()
{
    // No child components
}

void OscilCheckbox::mouseDown(const juce::MouseEvent&)
{
    if (enabled_)
        isPressed_ = true;
}

void OscilCheckbox::mouseUp(const juce::MouseEvent& e)
{
    if (isPressed_ && enabled_ && contains(e.getPosition()))
    {
        toggle();
    }
    isPressed_ = false;
}

void OscilCheckbox::mouseEnter(const juce::MouseEvent&)
{
    if (!enabled_) return;

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

void OscilCheckbox::mouseExit(const juce::MouseEvent&)
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

bool OscilCheckbox::keyPressed(const juce::KeyPress& key)
{
    if (enabled_ && (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey))
    {
        toggle();
        return true;
    }
    return false;
}

void OscilCheckbox::focusGained(FocusChangeType)
{
    hasFocus_ = true;
    repaint();
}

void OscilCheckbox::focusLost(FocusChangeType)
{
    hasFocus_ = false;
    repaint();
}

void OscilCheckbox::timerCallback()
{
    updateAnimations();

    if (checkSpring_.isSettled() && hoverSpring_.isSettled())
        stopTimer();

    repaint();
}

void OscilCheckbox::updateAnimations()
{
    float dt = AnimationTiming::FRAME_DURATION_60FPS;
    checkSpring_.update(dt);
    hoverSpring_.update(dt);
}

void OscilCheckbox::notifyStateChanged()
{
    if (onStateChanged)
        onStateChanged(state_);

    if (onCheckedChanged)
        onCheckedChanged(state_ == CheckState::Checked);
}

void OscilCheckbox::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    repaint();
}

std::unique_ptr<juce::AccessibilityHandler> OscilCheckbox::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this,
        juce::AccessibilityRole::toggleButton,
        juce::AccessibilityActions()
            .addAction(juce::AccessibilityActionType::toggle,
                [this] { if (enabled_) toggle(); })
    );
}

} // namespace oscil
