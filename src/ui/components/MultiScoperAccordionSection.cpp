/*
    MultiScoper - Accordion Section Implementation
    Individual collapsible section with header, chevron animation, and content
*/

#include "ui/components/MultiScoperAccordion.h"
#include "ui/components/SurfacePainter.h"

#include <utility>

namespace multiscoper
{
//==============================================================================

MultiScoperAccordionSection::MultiScoperAccordionSection(IThemeService& themeService, juce::String title)
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

MultiScoperAccordionSection::MultiScoperAccordionSection(IThemeService& themeService, juce::String title,
                                                         const juce::String& testId)
    : MultiScoperAccordionSection(themeService, std::move(title))
{
    setTestId(testId);
}

void MultiScoperAccordionSection::registerTestId() { MULTISCOPER_REGISTER_TEST_ID(testId_); }

MultiScoperAccordionSection::~MultiScoperAccordionSection() { stopTimer(); }

void MultiScoperAccordionSection::setTitle(const juce::String& title)
{
    if (title_ != title)
    {
        title_ = title;
        repaint();
    }
}

void MultiScoperAccordionSection::setIcon(const juce::Image& icon)
{
    icon_ = icon;
    repaint();
}

void MultiScoperAccordionSection::clearIcon()
{
    icon_ = {};
    repaint();
}

void MultiScoperAccordionSection::setContent(juce::Component* content)
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

void MultiScoperAccordionSection::startExpandAnimation(bool expandTarget)
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

void MultiScoperAccordionSection::applyExpandedImmediate(bool expandTarget)
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

void MultiScoperAccordionSection::setExpanded(bool expanded, bool animate)
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

void MultiScoperAccordionSection::toggle()
{
    if (isEnabled())
        setExpanded(!expanded_);
}

void MultiScoperAccordionSection::enablementChanged() { repaint(); }

int MultiScoperAccordionSection::getContentHeight() const
{
    if (!content_)
        return 0;

    if (dynamicContent_)
        return dynamicContent_->getPreferredHeight();

    return content_->getHeight();
}

int MultiScoperAccordionSection::getPreferredHeight() const
{
    int const contentHeight = getContentHeight();
    float const expandAmount = expandSpring_.position;

    return HEADER_HEIGHT + static_cast<int>(static_cast<float>(contentHeight) * expandAmount);
}

void MultiScoperAccordionSection::contentHeightChanged()
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

void MultiScoperAccordionSection::paint(juce::Graphics& g)
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

void MultiScoperAccordionSection::paintHeader(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float const opacity = isEnabled() ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float const hoverAmount = hoverSpring_.position;
    const auto& glass = getSurface();

    // Background — blend toward bgHover on hover
    auto bgColour = getTheme().backgroundSecondary;
    if (hoverAmount > 0.01f && isEnabled())
        bgColour = bgColour.interpolatedWith(glass.bgHover, hoverAmount);

    // bgColour is interpolated with glass.bgHover (which carries a designed
    // 0.08 alpha), so the blend may already be semi-transparent. Multiply
    // rather than replace so the disabled-state dim scales the existing
    // alpha instead of forcing the header fill to full-opacity textPrimary.
    g.setColour(bgColour.withMultipliedAlpha(opacity));
    g.fillRect(bounds);

    // Bottom border — glass borderSubtle (multiply; already alpha-tinted)
    g.setColour(glass.borderSubtle.withMultipliedAlpha(opacity));
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
        SurfacePainter::paintFocusRing(g, bounds.toFloat(), ComponentLayout::RADIUS_SM, glass.accent);
    }
}

void MultiScoperAccordionSection::paintChevron(juce::Graphics& g, juce::Rectangle<float> bounds)
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

void MultiScoperAccordionSection::resized()
{
    if (content_)
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(HEADER_HEIGHT);
        content_->setBounds(bounds);
    }
}

void MultiScoperAccordionSection::mouseDown(const juce::MouseEvent& e)
{
    // Track if mouseDown started in header area for proper click detection
    // This prevents trackpad gestures from triggering false toggles
    mouseDownInHeader_ = e.mods.isLeftButtonDown() && !e.mods.isPopupMenu() && e.y < HEADER_HEIGHT && isEnabled();
}

void MultiScoperAccordionSection::mouseUp(const juce::MouseEvent& e)
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

void MultiScoperAccordionSection::mouseEnter(const juce::MouseEvent& /*event*/)
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

void MultiScoperAccordionSection::mouseExit(const juce::MouseEvent& /*event*/)
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

void MultiScoperAccordionSection::focusGained(FocusChangeType /*cause*/)
{
    hasFocus_ = true;
    repaint();
}

void MultiScoperAccordionSection::focusLost(FocusChangeType /*cause*/)
{
    hasFocus_ = false;
    repaint();
}

void MultiScoperAccordionSection::timerCallback()
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

void MultiScoperAccordionSection::updateAnimations()
{
    float const dt = AnimationTiming::FRAME_DURATION_60FPS;
    expandSpring_.update(dt);
    hoverSpring_.update(dt);
    chevronSpring_.update(dt);
}

//==============================================================================
// MultiScoperAccordion Implementation
//==============================================================================

} // namespace multiscoper
