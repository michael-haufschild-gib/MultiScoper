/*
    Oscil - Tooltip Component Implementation
*/

#include "ui/components/OscilTooltip.h"

namespace oscil
{

OscilTooltip::OscilTooltip()
    : opacitySpring_(SpringPresets::stiff())
{
    setAlwaysOnTop(true);
    setVisible(false);
    setInterceptsMouseClicks(false, false);

    theme_ = ThemeManager::getInstance().getCurrentTheme();
    ThemeManager::getInstance().addListener(this);

    opacitySpring_.position = 0.0f;
    opacitySpring_.target = 0.0f;
}

OscilTooltip::~OscilTooltip()
{
    ThemeManager::getInstance().removeListener(this);
    stopTimer();
}

void OscilTooltip::setContent(const RichTooltipContent& content)
{
    content_ = content;
    repaint();
}

void OscilTooltip::setContent(const juce::String& title, const juce::String& value)
{
    content_.title = title;
    content_.value = value;
    repaint();
}

void OscilTooltip::setTitle(const juce::String& title)
{
    content_.title = title;
    repaint();
}

void OscilTooltip::setValue(const juce::String& value)
{
    content_.value = value;
    repaint();
}

void OscilTooltip::setShortcut(const juce::String& shortcut)
{
    content_.shortcut = shortcut;
    repaint();
}

void OscilTooltip::setHint(const juce::String& hint)
{
    content_.hint = hint;
    repaint();
}

void OscilTooltip::setColourPreview(juce::Colour colour)
{
    content_.previewColour = colour;
    content_.showColourPreview = true;
    repaint();
}

void OscilTooltip::clearColourPreview()
{
    content_.showColourPreview = false;
    repaint();
}

void OscilTooltip::showAt(juce::Point<int> position, TooltipPosition preferredPosition)
{
    if (content_.title.isEmpty() && content_.value.isEmpty())
        return;

    auto screenBounds = juce::Desktop::getInstance().getDisplays()
        .getDisplayForPoint(position)->userArea;

    TooltipPosition actualPosition = preferredPosition;
    if (preferredPosition == TooltipPosition::Auto)
        actualPosition = calculateBestPosition(position, screenBounds);

    auto bounds = calculateBounds(position, actualPosition);
    setBounds(bounds);

    hiding_ = false;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        opacitySpring_.setTarget(1.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        currentOpacity_ = 1.0f;
    }

    setVisible(true);
    repaint();
}

void OscilTooltip::showNear(juce::Component* target, TooltipPosition preferredPosition)
{
    if (target == nullptr) return;

    auto targetBounds = target->getScreenBounds();
    auto center = targetBounds.getCentre();

    showAt(center, preferredPosition);
}

void OscilTooltip::hide()
{
    if (!isVisible()) return;

    hiding_ = true;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        opacitySpring_.setTarget(0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        currentOpacity_ = 0.0f;
        setVisible(false);
    }
}

juce::Rectangle<int> OscilTooltip::calculateBounds(juce::Point<int> targetPoint,
                                                    TooltipPosition position) const
{
    int width = calculateContentWidth();
    int height = calculateContentHeight();

    int x = 0, y = 0;

    switch (position)
    {
        case TooltipPosition::Above:
            x = targetPoint.x - width / 2;
            y = targetPoint.y - height - ARROW_SIZE - 4;
            break;

        case TooltipPosition::Below:
            x = targetPoint.x - width / 2;
            y = targetPoint.y + ARROW_SIZE + 4;
            break;

        case TooltipPosition::Left:
            x = targetPoint.x - width - ARROW_SIZE - 4;
            y = targetPoint.y - height / 2;
            break;

        case TooltipPosition::Right:
            x = targetPoint.x + ARROW_SIZE + 4;
            y = targetPoint.y - height / 2;
            break;

        case TooltipPosition::Auto:
        default:
            x = targetPoint.x - width / 2;
            y = targetPoint.y - height - ARROW_SIZE - 4;
            break;
    }

    return { x, y, width, height };
}

TooltipPosition OscilTooltip::calculateBestPosition(juce::Point<int> target,
                                                     juce::Rectangle<int> screenBounds) const
{
    int height = calculateContentHeight();
    int width = calculateContentWidth();

    // Prefer above, but check if it fits
    if (target.y - height - ARROW_SIZE - 4 >= screenBounds.getY())
        return TooltipPosition::Above;

    // Try below
    if (target.y + height + ARROW_SIZE + 4 <= screenBounds.getBottom())
        return TooltipPosition::Below;

    // Try right
    if (target.x + width + ARROW_SIZE + 4 <= screenBounds.getRight())
        return TooltipPosition::Right;

    // Fall back to left
    return TooltipPosition::Left;
}

int OscilTooltip::calculateContentWidth() const
{
    auto titleFont = juce::Font(13.0f, juce::Font::bold);
    auto normalFont = juce::Font(12.0f);
    auto smallFont = juce::Font(11.0f);

    int maxWidth = 0;

    if (content_.title.isNotEmpty())
        maxWidth = std::max(maxWidth, titleFont.getStringWidth(content_.title));

    if (content_.value.isNotEmpty())
        maxWidth = std::max(maxWidth, normalFont.getStringWidth(content_.value));

    if (content_.shortcut.isNotEmpty())
        maxWidth = std::max(maxWidth, smallFont.getStringWidth(content_.shortcut));

    if (content_.hint.isNotEmpty())
        maxWidth = std::max(maxWidth, smallFont.getStringWidth(content_.hint));

    // Add color preview width if showing
    if (content_.showColourPreview)
        maxWidth = std::max(maxWidth, COLOUR_PREVIEW_SIZE);

    return std::min(maxWidth + PADDING * 2, ComponentLayout::TOOLTIP_MAX_WIDTH);
}

int OscilTooltip::calculateContentHeight() const
{
    int height = PADDING * 2;
    int lines = 0;

    if (content_.title.isNotEmpty())
    {
        height += 16; // Title line height
        lines++;
    }

    if (content_.value.isNotEmpty())
    {
        height += 14; // Value line height
        lines++;
    }

    if (content_.shortcut.isNotEmpty())
    {
        height += 14; // Shortcut line height
        lines++;
    }

    if (content_.hint.isNotEmpty())
    {
        height += 14; // Hint line height
        lines++;
    }

    if (content_.showColourPreview)
    {
        height += COLOUR_PREVIEW_SIZE + LINE_SPACING;
    }

    // Add spacing between lines
    if (lines > 1)
        height += (lines - 1) * LINE_SPACING;

    return height;
}

void OscilTooltip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float opacity = currentOpacity_;

    // Background with shadow
    auto shadowColour = juce::Colours::black.withAlpha(0.3f * opacity);
    g.setColour(shadowColour);
    g.fillRoundedRectangle(bounds.translated(0, 2), ComponentLayout::RADIUS_MD);

    // Main background
    g.setColour(theme_.backgroundPane.withAlpha(opacity));
    g.fillRoundedRectangle(bounds, ComponentLayout::RADIUS_MD);

    // Border
    g.setColour(theme_.controlBorder.withAlpha(opacity * 0.5f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), ComponentLayout::RADIUS_MD, 1.0f);

    // Content
    int y = PADDING;

    // Title
    if (content_.title.isNotEmpty())
    {
        g.setColour(theme_.textPrimary.withAlpha(opacity));
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(content_.title,
            PADDING, y, getWidth() - PADDING * 2, 16,
            juce::Justification::left);
        y += 16 + LINE_SPACING;
    }

    // Value
    if (content_.value.isNotEmpty())
    {
        g.setColour(theme_.textPrimary.withAlpha(opacity));
        g.setFont(juce::Font(12.0f));
        g.drawText(content_.value,
            PADDING, y, getWidth() - PADDING * 2, 14,
            juce::Justification::left);
        y += 14 + LINE_SPACING;
    }

    // Color preview
    if (content_.showColourPreview)
    {
        auto previewBounds = juce::Rectangle<float>(
            PADDING, static_cast<float>(y), COLOUR_PREVIEW_SIZE, COLOUR_PREVIEW_SIZE);

        g.setColour(content_.previewColour.withAlpha(opacity));
        g.fillRoundedRectangle(previewBounds, 3.0f);

        g.setColour(theme_.controlBorder.withAlpha(opacity));
        g.drawRoundedRectangle(previewBounds, 3.0f, 1.0f);

        y += COLOUR_PREVIEW_SIZE + LINE_SPACING;
    }

    // Shortcut
    if (content_.shortcut.isNotEmpty())
    {
        g.setColour(theme_.textSecondary.withAlpha(opacity));
        g.setFont(juce::Font(11.0f));
        g.drawText(content_.shortcut,
            PADDING, y, getWidth() - PADDING * 2, 14,
            juce::Justification::left);
        y += 14 + LINE_SPACING;
    }

    // Hint
    if (content_.hint.isNotEmpty())
    {
        g.setColour(theme_.textSecondary.withAlpha(opacity * 0.8f));
        g.setFont(juce::Font(11.0f, juce::Font::italic));
        g.drawText(content_.hint,
            PADDING, y, getWidth() - PADDING * 2, 14,
            juce::Justification::left);
    }
}

void OscilTooltip::timerCallback()
{
    float dt = AnimationTiming::FRAME_DURATION_60FPS;
    opacitySpring_.update(dt);
    currentOpacity_ = std::clamp(opacitySpring_.position, 0.0f, 1.0f);

    if (opacitySpring_.isSettled())
    {
        stopTimer();
        if (hiding_ && currentOpacity_ < 0.01f)
            setVisible(false);
    }

    repaint();
}

void OscilTooltip::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    repaint();
}

void OscilTooltip::showSimple(juce::Component* target, const juce::String& text)
{
    if (globalTooltip_ == nullptr)
    {
        globalTooltip_ = new OscilTooltip();
        if (auto* desktop = juce::Desktop::getInstance().getComponent(0))
            desktop->addChildComponent(globalTooltip_.getComponent());
    }

    globalTooltip_->setContent(text);
    globalTooltip_->showNear(target);
}

void OscilTooltip::hideAll()
{
    if (globalTooltip_ != nullptr)
        globalTooltip_->hide();
}

// TooltipClient implementation

void TooltipClient::setRichTooltip(const RichTooltipContent& content)
{
    tooltipContent_ = content;
}

void TooltipClient::setSimpleTooltip(const juce::String& text)
{
    tooltipContent_.title = text;
    tooltipContent_.value.clear();
    tooltipContent_.shortcut.clear();
    tooltipContent_.hint.clear();
    tooltipContent_.showColourPreview = false;
}

void TooltipClient::tooltipMouseEnter()
{
    if (tooltipContent_.title.isEmpty())
        return;

    tooltipScheduled_ = true;
    startTimer(ComponentLayout::TOOLTIP_DELAY_MS);
}

void TooltipClient::tooltipMouseExit()
{
    tooltipScheduled_ = false;
    stopTimer();
    OscilTooltip::hideAll();
}

void TooltipClient::tooltipMouseMove(const juce::Point<int>& position)
{
    lastMousePosition_ = position;
}

void TooltipClient::timerCallback()
{
    stopTimer();

    if (!tooltipScheduled_)
        return;

    if (auto* target = getTooltipTarget())
    {
        // Create tooltip if needed
        static OscilTooltip tooltip;
        tooltip.setContent(tooltipContent_);
        tooltip.showNear(target);
    }
}

} // namespace oscil
