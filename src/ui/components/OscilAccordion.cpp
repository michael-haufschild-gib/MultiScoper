/*
    Oscil - Accordion Component Implementation
*/

#include "ui/components/OscilAccordion.h"
#include "ui/animation/OscilAnimationService.h"

namespace oscil
{

//==============================================================================
// OscilAccordionSection Implementation
//==============================================================================

OscilAccordionSection::OscilAccordionSection(IThemeService& themeService, const juce::String& title)
    : ThemedComponent(themeService)
    , title_(title)
{
    setWantsKeyboardFocus(true);

    expandProgress_ = 0.0f;
    hoverProgress_ = 0.0f;
    chevronRotation_ = 0.0f;
}

OscilAccordionSection::OscilAccordionSection(IThemeService& themeService, const juce::String& title, const juce::String& testId)
    : OscilAccordionSection(themeService, title)
{
    setTestId(testId);
}

void OscilAccordionSection::registerTestId()
{
    OSCIL_REGISTER_TEST_ID(testId_);
}

OscilAccordionSection::~OscilAccordionSection()
{
    // ScopedAnimators handle completion on destruction
}

void OscilAccordionSection::parentHierarchyChanged()
{
    ThemedComponent::parentHierarchyChanged();
    if (auto* service = findAnimationService(this))
    {
        animService_ = service;
    }
}

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
            content_->setVisible(expanded_ || expandProgress_ > 0.01f);

            // Check for dynamic height support
            dynamicContent_ = dynamic_cast<DynamicHeightContent*>(content_);
            if (dynamicContent_)
            {
                dynamicContent_->onPreferredHeightChanged = [this] {
                    contentHeightChanged();
                };
            }
        }

        resized();
    }
}

void OscilAccordionSection::setExpanded(bool expanded, bool animate)
{
    if (expanded_ == expanded)
        return;

    expanded_ = expanded;

    float targetExpand = expanded ? 1.0f : 0.0f;

    if (animate && animService_ && OscilAnimationService::shouldAnimate())
    {
        float startExpand = expandProgress_;
        float startChevron = chevronRotation_;
        auto safeThis = juce::Component::SafePointer<OscilAccordionSection>(this);

        expandAnimator_.set(animService_->createExpandAnimation(
            [safeThis, startExpand, targetExpand](float v) {
                if (!safeThis)
                    return;

                safeThis->expandProgress_ = juce::jmap(v, startExpand, targetExpand);

                // Show/hide content based on animation state
                if (safeThis->content_)
                {
                    bool shouldBeVisible = safeThis->expandProgress_ > 0.01f;
                    if (safeThis->content_->isVisible() != shouldBeVisible)
                        safeThis->content_->setVisible(shouldBeVisible);
                }

                // Only notify parent when height actually changed
                int currentHeight = safeThis->getPreferredHeight();
                if (currentHeight != safeThis->lastReportedHeight_)
                {
                    safeThis->lastReportedHeight_ = currentHeight;
                    if (safeThis->onHeightChanged)
                        safeThis->onHeightChanged();
                    else if (auto* parent = safeThis->getParentComponent())
                        parent->resized();
                }

                safeThis->repaint();
            }));
        expandAnimator_.start();

        chevronAnimator_.set(animService_->createExpandAnimation(
            [safeThis, startChevron, targetExpand](float v) {
                if (safeThis)
                {
                    safeThis->chevronRotation_ = juce::jmap(v, startChevron, targetExpand);
                    safeThis->repaint();
                }
            }));
        chevronAnimator_.start();
    }
    else
    {
        expandProgress_ = targetExpand;
        chevronRotation_ = targetExpand;

        if (content_)
            content_->setVisible(expanded);

        if (auto* parent = getParentComponent())
            parent->resized();

        repaint();
    }

    if (onExpandedChanged)
        onExpandedChanged(expanded_);
}

void OscilAccordionSection::toggle()
{
    if (enabled_)
        setExpanded(!expanded_);
}

void OscilAccordionSection::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        repaint();
    }
}

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
    int contentHeight = getContentHeight();
    float expandAmount = expandProgress_;

    return HEADER_HEIGHT + static_cast<int>(static_cast<float>(contentHeight) * expandAmount);
}

void OscilAccordionSection::contentHeightChanged()
{
    // If expanded, we need to update layout
    if (expanded_ || expandProgress_ > 0.01f)
    {
        if (onHeightChanged)
            onHeightChanged();
        else if (auto* parent = getParentComponent())
            parent->resized();
    }
}

void OscilAccordionSection::paint(juce::Graphics& g)
{
    // Guard against zero-size painting
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    auto bounds = getLocalBounds();

    // Header
    auto headerBounds = bounds.removeFromTop(HEADER_HEIGHT);
    paintHeader(g, headerBounds);

    // Content area (clipped)
    if (content_ && expandProgress_ > 0.01f)
    {
        auto contentBounds = bounds;
        g.reduceClipRegion(contentBounds);

        // Optional: subtle background for content
        g.setColour(getTheme().backgroundPrimary.withAlpha(0.3f));
        g.fillRect(contentBounds);
    }
}

void OscilAccordionSection::paintHeader(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float hoverAmount = hoverProgress_;

    // Background
    auto bgColour = getTheme().backgroundSecondary;
    if (hoverAmount > 0.01f && enabled_)
        bgColour = bgColour.brighter(0.05f * hoverAmount);

    g.setColour(bgColour.withAlpha(opacity));
    g.fillRect(bounds);

    // Bottom border
    g.setColour(getTheme().controlBorder.withAlpha(opacity * 0.3f));
    g.fillRect(bounds.getX(), bounds.getBottom() - 1, bounds.getWidth(), 1);

    auto contentBounds = bounds.reduced(PADDING_H, 0);

    // Chevron
    auto chevronBounds = contentBounds.removeFromLeft(CHEVRON_SIZE + 8).toFloat()
        .withSizeKeepingCentre(CHEVRON_SIZE, CHEVRON_SIZE);
    paintChevron(g, chevronBounds);

    // Icon
    if (icon_.isValid())
    {
        auto iconBounds = contentBounds.removeFromLeft(ICON_SIZE + 8).toFloat()
            .withSizeKeepingCentre(ICON_SIZE, ICON_SIZE);

        g.setOpacity(opacity);
        g.drawImage(icon_, iconBounds, juce::RectanglePlacement::centred);
    }

    // Title
    g.setColour(getTheme().textPrimary.withAlpha(opacity));
    g.setFont(juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT)).boldened());
    g.drawText(title_, contentBounds, juce::Justification::centredLeft);

    // Focus ring
    if (hasFocus_ && enabled_)
    {
        g.setColour(getTheme().controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
        g.drawRect(bounds.reduced(2), 2);
    }
}

void OscilAccordionSection::paintChevron(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float rotation = chevronRotation_ * juce::MathConstants<float>::halfPi;

    g.setColour(getTheme().textSecondary.withAlpha(opacity));

    juce::Path chevron;
    float size = bounds.getWidth() * 0.35f;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    // Right-pointing chevron that rotates to down
    chevron.startNewSubPath(cx - size * 0.3f, cy - size);
    chevron.lineTo(cx + size * 0.3f, cy);
    chevron.lineTo(cx - size * 0.3f, cy + size);

    chevron.applyTransform(juce::AffineTransform::rotation(rotation, cx, cy));

    g.strokePath(chevron, juce::PathStrokeType(ComponentLayout::BORDER_MEDIUM, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
}

void OscilAccordionSection::resized()
{
    // Guard against zero or negative dimensions
    if (getWidth() <= 0 || getHeight() <= 0)
    {
        juce::Logger::writeToLog("OscilAccordionSection::resized() - Invalid dimensions: "
            + juce::String(getWidth()) + "x" + juce::String(getHeight()));
        return;
    }

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
    mouseDownInHeader_ = e.mods.isLeftButtonDown() && !e.mods.isPopupMenu() &&
                         e.y < HEADER_HEIGHT && enabled_;

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

void OscilAccordionSection::mouseEnter(const juce::MouseEvent&)
{
    if (!enabled_) return;

    isHovered_ = true;

    if (animService_ && OscilAnimationService::shouldAnimate())
    {
        float startValue = hoverProgress_;
        hoverAnimator_.set(animService_->createHoverAnimation(
            [safeThis = juce::Component::SafePointer<OscilAccordionSection>(this), startValue](float v) {
                if (auto* self = safeThis.getComponent())
                {
                    // Interpolate from start value to target (1.0 for enter)
                    self->hoverProgress_ = startValue + (1.0f - startValue) * v;
                    self->repaint();
                }
            }));
        hoverAnimator_.start();
    }
    else
    {
        hoverProgress_ = 1.0f;
        repaint();
    }
}

void OscilAccordionSection::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;

    if (animService_ && OscilAnimationService::shouldAnimate())
    {
        float startValue = hoverProgress_;
        hoverAnimator_.set(animService_->createHoverAnimation(
            [safeThis = juce::Component::SafePointer<OscilAccordionSection>(this), startValue](float v) {
                if (auto* self = safeThis.getComponent())
                {
                    // Interpolate from start value to target (0.0 for exit)
                    self->hoverProgress_ = startValue * (1.0f - v);
                    self->repaint();
                }
            }));
        hoverAnimator_.start();
    }
    else
    {
        hoverProgress_ = 0.0f;
        repaint();
    }
}

bool OscilAccordionSection::keyPressed(const juce::KeyPress& key)
{
    if (enabled_ && (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey))
    {
        toggle();
        return true;
    }
    return false;
}

void OscilAccordionSection::focusGained(FocusChangeType)
{
    hasFocus_ = true;
    repaint();
}

void OscilAccordionSection::focusLost(FocusChangeType)
{
    hasFocus_ = false;
    repaint();
}

//==============================================================================
// OscilAccordion Implementation
//==============================================================================

OscilAccordion::OscilAccordion(IThemeService& themeService)
    : ThemedComponent(themeService)
{
}

OscilAccordion::~OscilAccordion()
{
}

OscilAccordionSection* OscilAccordion::addSection(const juce::String& title)
{
    auto section = std::make_unique<OscilAccordionSection>(getThemeService(), title);
    auto* sectionPtr = section.get();

    int index = static_cast<int>(sections_.size());
    section->onExpandedChanged = [this, index](bool expanded) {
        handleSectionExpanded(index, expanded);
    };
    section->onHeightChanged = [this]() {
        updateContentHeight();
    };

    addAndMakeVisible(*section);
    sections_.push_back(std::move(section));

    resized();
    return sectionPtr;
}

OscilAccordionSection* OscilAccordion::addSection(const juce::String& title, juce::Component* content)
{
    auto* section = addSection(title);
    section->setContent(content);
    return section;
}

void OscilAccordion::removeSection(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < sections_.size())
    {
        sections_.erase(sections_.begin() + index);
        resized();
    }
}

void OscilAccordion::clearSections()
{
    sections_.clear();
    resized();
}

OscilAccordionSection* OscilAccordion::getSection(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < sections_.size())
        return sections_[static_cast<size_t>(index)].get();
    return nullptr;
}

void OscilAccordion::setAllowMultiExpand(bool allow)
{
    if (allowMultiExpand_ != allow)
    {
        allowMultiExpand_ = allow;

        // If disabling multi-expand and multiple sections are open,
        // keep only the first expanded one
        if (!allowMultiExpand_)
        {
            bool foundExpanded = false;
            for (auto& section : sections_)
            {
                if (section->isExpanded())
                {
                    if (foundExpanded)
                        section->setExpanded(false);
                    else
                        foundExpanded = true;
                }
            }
        }
    }
}

void OscilAccordion::expandSection(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < sections_.size())
        sections_[static_cast<size_t>(index)]->setExpanded(true);
}

void OscilAccordion::collapseSection(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < sections_.size())
        sections_[static_cast<size_t>(index)]->setExpanded(false);
}

void OscilAccordion::expandAll()
{
    if (!allowMultiExpand_)
        return;

    for (auto& section : sections_)
        section->setExpanded(true);
}

void OscilAccordion::collapseAll()
{
    for (auto& section : sections_)
        section->setExpanded(false);
}

void OscilAccordion::setSpacing(int spacing)
{
    if (spacing_ != spacing)
    {
        spacing_ = spacing;
        resized();
    }
}

int OscilAccordion::getPreferredHeight() const
{
    int totalHeight = 0;

    for (size_t i = 0; i < sections_.size(); ++i)
    {
        totalHeight += sections_[i]->getPreferredHeight();
        if (i > 0)
            totalHeight += spacing_;
    }

    return totalHeight;
}

void OscilAccordion::updateContentHeight()
{
    int preferredHeight = getPreferredHeight();
    if (getHeight() != preferredHeight)
        setSize(getWidth(), preferredHeight);
    
    layoutSections();
}

void OscilAccordion::resized()
{
    // Guard against zero or negative dimensions
    if (getWidth() <= 0 || getHeight() <= 0)
    {
        juce::Logger::writeToLog("OscilAccordion::resized() - Invalid dimensions: "
            + juce::String(getWidth()) + "x" + juce::String(getHeight()));
        return;
    }

    layoutSections();
}

void OscilAccordion::handleSectionExpanded(int index, bool expanded)
{
    // If not allowing multi-expand and a section was just expanded,
    // collapse all other sections
    if (!allowMultiExpand_ && expanded)
    {
        for (size_t i = 0; i < sections_.size(); ++i)
        {
            if (static_cast<int>(i) != index && sections_[i]->isExpanded())
                sections_[i]->setExpanded(false);
        }
    }
}

void OscilAccordion::layoutSections()
{
    auto bounds = getLocalBounds();
    int y = 0;

    for (size_t i = 0; i < sections_.size(); ++i)
    {
        int height = sections_[i]->getPreferredHeight();
        sections_[i]->setBounds(0, y, bounds.getWidth(), height);

        y += height + spacing_;
    }
}

} // namespace oscil
