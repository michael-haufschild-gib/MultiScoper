/*
    Oscil - Accordion Section Implementation
    Individual collapsible section with header, chevron animation, and content
*/

#include "ui/components/GlassPainter.h"
#include "ui/components/OscilAccordion.h"

#include <utility>

namespace oscil
{
//==============================================================================

OscilAccordionSection::OscilAccordionSection(IThemeService& themeService, juce::String title)
    : ThemedComponent(themeService)
    , title_(std::move(title))
    , expandSpring_(SpringPresets::medium())
    , hoverSpring_(SpringPresets::fast())
    , chevronSpring_(SpringPresets::medium())
{
    setWantsKeyboardFocus(true);

    expandSpring_.position = 0.0f;
    hoverSpring_.position = 0.0f;
    chevronSpring_.position = 0.0f;
}

OscilAccordionSection::OscilAccordionSection(IThemeService& themeService, juce::String title,
                                             const juce::String& testId)
    : OscilAccordionSection(themeService, std::move(title))
{
    setTestId(testId);
}

void OscilAccordionSection::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

OscilAccordionSection::~OscilAccordionSection() { stopTimer(); }

void OscilAccordionSection::setTitle(const juce::String& title)
{
    if (title_ != title)
    {
        title_ = title;
        repaint();
    }
}

void OscilAccordionSection::setIcon(const juce::Image& icon)
{
    icon_ = icon;
    repaint();
}

void OscilAccordionSection::clearIcon()
{
    icon_ = {};
    repaint();
}

void OscilAccordionSection::setContent(juce::Component* content)
{
    if (content_ != content)
    {
        // Clear previous callback
        if (dynamicContent_ != nullptr)
        {
            dynamicContent_->onPreferredHeightChanged = nullptr;
        }

        if (content_)
            removeChildComponent(content_);

        content_ = content;
        dynamicContent_ = nullptr;

        if (content_)
        {
            addAndMakeVisible(content_);
            content_->setVisible(expanded_ || expandSpring_.position > 0.01f);

            // Check for dynamic height support
            dynamicContent_ = dynamic_cast<DynamicHeightContent*>(content_);
            if (dynamicContent_)
            {
                dynamicContent_->onPreferredHeightChanged = [this] { contentHeightChanged(); };
            }
        }

        resized();
    }
}

void OscilAccordionSection::startExpandAnimation(bool expandTarget)
{
    float const target = expandTarget ? 1.0f : 0.0f;
    expandSpring_.setTarget(target);
    chevronSpring_.setTarget(target);
    startTimerHz(ComponentLayout::ANIMATION_FPS);

    // Immediately hide content on collapse so visibility state is
    // consistent before the animation completes. Expand reveals
    // content via the timer callback once the spring crosses 0.01.
    if (!expandTarget && content_)
        content_->setVisible(false);
}

void OscilAccordionSection::applyExpandedImmediate(bool expandTarget)
{
    // When skipping animation, set BOTH position AND target to prevent
    // the spring from animating toward the wrong value when timer runs.
    float const value = expandTarget ? 1.0f : 0.0f;
    expandSpring_.position = value;
    expandSpring_.target = value;
    chevronSpring_.position = value;
    chevronSpring_.target = value;

    if (content_)
        content_->setVisible(expandTarget);

    if (auto* parent = getParentComponent())
        parent->resized();

    repaint();
}

void OscilAccordionSection::setExpanded(bool expanded, bool animate)
{
    if (expanded_ == expanded)
        return;

    expanded_ = expanded;

    if (animate && AnimationSettings::shouldUseSpringAnimations())
        startExpandAnimation(expanded);
    else
        applyExpandedImmediate(expanded);

    if (onExpandedChanged)
        onExpandedChanged(expanded_);
}

void OscilAccordionSection::toggle()
{
    if (isEnabled())
        setExpanded(!expanded_);
}

void OscilAccordionSection::enablementChanged() { repaint(); }

int OscilAccordionSection::getContentHeight() const
{
    if (!content_)
        return 0;

    if (dynamicContent_)
        return dynamicContent_->getPreferredHeight();

    return content_->getHeight();
}

int OscilAccordionSection::getPreferredHeight() const
{
    int const contentHeight = getContentHeight();
    float const expandAmount = expandSpring_.position;

    return HEADER_HEIGHT + static_cast<int>(static_cast<float>(contentHeight) * expandAmount);
}

void OscilAccordionSection::contentHeightChanged()
{
    // If expanded, we need to update layout
    if (expanded_ || expandSpring_.position > 0.01f)
    {
        if (onHeightChanged)
            onHeightChanged();
        else if (auto* parent = getParentComponent())
            parent->resized();
    }
}

void OscilAccordionSection::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Header
    auto headerBounds = bounds.removeFromTop(HEADER_HEIGHT);
    paintHeader(g, headerBounds);

    // Content area (clipped) — no special background, inherits parent
    if (content_ && expandSpring_.position > 0.01f)
    {
        g.reduceClipRegion(bounds);
    }
}

void OscilAccordionSection::paintHeader(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float const opacity = isEnabled() ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float const hoverAmount = hoverSpring_.position;
    const auto& glass = getGlass();

    // Background — blend toward bgHover on hover
    auto bgColour = getTheme().backgroundSecondary;
    if (hoverAmount > 0.01f && isEnabled())
        bgColour = bgColour.interpolatedWith(glass.bgHover, hoverAmount);

    g.setColour(bgColour.withAlpha(opacity));
    g.fillRect(bounds);

    // Bottom border — glass borderSubtle
    g.setColour(glass.borderSubtle.withAlpha(opacity));
    g.fillRect(bounds.getX(), bounds.getBottom() - 1, bounds.getWidth(), 1);

    auto contentBounds = bounds.reduced(PADDING_H, 0);

    // Chevron
    auto chevronBounds =
        contentBounds.removeFromLeft(CHEVRON_SIZE + 8).toFloat().withSizeKeepingCentre(CHEVRON_SIZE, CHEVRON_SIZE);
    paintChevron(g, chevronBounds);

    // Icon
    if (icon_.isValid())
    {
        auto iconBounds =
            contentBounds.removeFromLeft(ICON_SIZE + 8).toFloat().withSizeKeepingCentre(ICON_SIZE, ICON_SIZE);

        g.setOpacity(opacity);
        g.drawImage(icon_, iconBounds, juce::RectanglePlacement::centred);
    }

    // Title
    g.setColour(getTheme().textPrimary.withAlpha(opacity));
    g.setFont(ComponentLayout::defaultFont().boldened());
    g.drawText(title_, contentBounds, juce::Justification::centredLeft);

    // Focus ring
    if (hasFocus_ && isEnabled())
    {
        GlassPainter::paintFocusRing(g, bounds.toFloat(), ComponentLayout::RADIUS_SM, glass.accent);
    }
}

void OscilAccordionSection::paintChevron(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    float const opacity = isEnabled() ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float const rotation = chevronSpring_.position * juce::MathConstants<float>::halfPi;

    g.setColour(getTheme().textSecondary.withAlpha(opacity));

    juce::Path chevron;
    float const size = bounds.getWidth() * 0.35f;
    float const cx = bounds.getCentreX();
    float const cy = bounds.getCentreY();

    // Right-pointing chevron that rotates to down
    chevron.startNewSubPath(cx - (size * 0.3f), cy - size);
    chevron.lineTo(cx + (size * 0.3f), cy);
    chevron.lineTo(cx - (size * 0.3f), cy + size);

    chevron.applyTransform(juce::AffineTransform::rotation(rotation, cx, cy));

    g.strokePath(chevron, juce::PathStrokeType(ComponentLayout::BORDER_MEDIUM, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

void OscilAccordionSection::resized()
{
    if (content_)
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(HEADER_HEIGHT);
        content_->setBounds(bounds);
    }
}

void OscilAccordionSection::mouseDown(const juce::MouseEvent& e)
{
    // Track if mouseDown started in header area for proper click detection
    // This prevents trackpad gestures from triggering false toggles
    mouseDownInHeader_ = e.mods.isLeftButtonDown() && !e.mods.isPopupMenu() && e.y < HEADER_HEIGHT && isEnabled();
}

void OscilAccordionSection::mouseUp(const juce::MouseEvent& e)
{
    // Only toggle if:
    // 1. MouseDown started in header (mouseDownInHeader_)
    // 2. MouseUp is still in header area
    // 3. User didn't drag (not a scroll/pan gesture)
    if (mouseDownInHeader_ && e.y < HEADER_HEIGHT && !e.mouseWasDraggedSinceMouseDown())
    {
        toggle();
    }
    mouseDownInHeader_ = false;
}

void OscilAccordionSection::mouseEnter(const juce::MouseEvent& /*event*/)
{
    if (!isEnabled())
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

void OscilAccordionSection::mouseExit(const juce::MouseEvent& /*event*/)
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

bool OscilAccordionSection::keyPressed(const juce::KeyPress& key)
{
    if (isEnabled() && (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey))
    {
        toggle();
        return true;
    }
    return false;
}

void OscilAccordionSection::focusGained(FocusChangeType /*cause*/)
{
    hasFocus_ = true;
    repaint();
}

void OscilAccordionSection::focusLost(FocusChangeType /*cause*/)
{
    hasFocus_ = false;
    repaint();
}

void OscilAccordionSection::timerCallback()
{
    updateAnimations();

    // Show/hide content based on animation state
    if (content_)
    {
        bool const shouldBeVisible = expandSpring_.position > 0.01f;
        if (content_->isVisible() != shouldBeVisible)
            content_->setVisible(shouldBeVisible);
    }

    // Only notify parent when height actually changed to avoid expensive layout recalculations
    int const currentHeight = getPreferredHeight();
    if (currentHeight != lastReportedHeight_)
    {
        lastReportedHeight_ = currentHeight;
        if (onHeightChanged)
            onHeightChanged();
        else if (auto* parent = getParentComponent())
            parent->resized();
    }

    if (expandSpring_.isSettled() && hoverSpring_.isSettled() && chevronSpring_.isSettled())
        stopTimer();

    repaint();
}

void OscilAccordionSection::updateAnimations()
{
    float const dt = AnimationTiming::FRAME_DURATION_60FPS;
    expandSpring_.update(dt);
    hoverSpring_.update(dt);
    chevronSpring_.update(dt);
}

//==============================================================================
// OscilAccordion Implementation
//==============================================================================

} // namespace oscil
