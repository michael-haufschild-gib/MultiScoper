/*
    Oscil - Collapsible Section Component Implementation
    Generic wrapper that adds accordion-style collapse/expand behavior to any section
*/

#include "ui/sections/CollapsibleSection.h"

namespace oscil
{

CollapsibleSection::CollapsibleSection(const juce::String& title, const juce::String& testIdPrefix)
    : title_(title)
    , testIdPrefix_(testIdPrefix)
{
    OSCIL_REGISTER_TEST_ID(testIdPrefix_);
    ThemeManager::getInstance().addListener(this);
    themeChanged(ThemeManager::getInstance().getCurrentTheme());
}

CollapsibleSection::~CollapsibleSection()
{
    // Clear the callback to prevent dangling pointer if content outlives us
    if (dynamicContent_ != nullptr)
    {
        dynamicContent_->onPreferredHeightChanged = nullptr;
    }

    ThemeManager::getInstance().removeListener(this);
}

void CollapsibleSection::paint(juce::Graphics& g)
{
    auto headerBounds = getHeaderBounds();

    // Draw header background
    g.setColour(headerBackground_);
    g.fillRect(headerBounds);

    // Draw bottom border of header
    g.setColour(borderColour_);
    g.drawHorizontalLine(headerBounds.getBottom() - 1,
                         static_cast<float>(headerBounds.getX()),
                         static_cast<float>(headerBounds.getRight()));

    // Draw title text
    g.setColour(headerText_);
    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));

    constexpr int textPaddingLeft = 12;
    constexpr int caretWidth = 20;

    auto textBounds = headerBounds.reduced(textPaddingLeft, 0)
                                   .withTrimmedRight(caretWidth);
    g.drawText(title_, textBounds, juce::Justification::centredLeft);

    // Draw caret indicator
    constexpr float caretPaddingRight = 12.0f;
    constexpr float caretSize = 8.0f;

    auto caretBounds = juce::Rectangle<float>(
        static_cast<float>(headerBounds.getRight()) - caretPaddingRight - caretSize,
        static_cast<float>(headerBounds.getCentreY()) - caretSize / 2.0f,
        caretSize,
        caretSize
    );

    drawCaret(g, caretBounds);
}

void CollapsibleSection::drawCaret(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    g.setColour(caretColour_);

    juce::Path caretPath;

    if (collapsed_)
    {
        // Triangle pointing right (>)
        caretPath.addTriangle(
            bounds.getX(), bounds.getY(),
            bounds.getRight(), bounds.getCentreY(),
            bounds.getX(), bounds.getBottom()
        );
    }
    else
    {
        // Triangle pointing down (v)
        caretPath.addTriangle(
            bounds.getX(), bounds.getY(),
            bounds.getRight(), bounds.getY(),
            bounds.getCentreX(), bounds.getBottom()
        );
    }

    g.fillPath(caretPath);
}

void CollapsibleSection::resized()
{
    if (content_ != nullptr)
    {
        auto contentBounds = getLocalBounds().withTrimmedTop(HEADER_HEIGHT);
        content_->setBounds(contentBounds);
        content_->setVisible(!collapsed_);
    }
}

void CollapsibleSection::mouseUp(const juce::MouseEvent& e)
{
    // Only toggle if click was in the header area
    if (getHeaderBounds().contains(e.getPosition()))
    {
        toggleCollapsed();
    }
}

void CollapsibleSection::themeChanged(const ColorTheme& newTheme)
{
    headerBackground_ = newTheme.backgroundPane;
    headerText_ = newTheme.textSecondary;
    caretColour_ = newTheme.textSecondary;
    borderColour_ = newTheme.controlBorder;

    repaint();
}

void CollapsibleSection::setContent(juce::Component* content)
{
    // Clear previous content's callback if it was dynamic
    if (dynamicContent_ != nullptr)
    {
        dynamicContent_->onPreferredHeightChanged = nullptr;
    }

    if (content_ != nullptr)
    {
        removeChildComponent(content_);
    }

    content_ = content;
    dynamicContent_ = nullptr;

    if (content_ != nullptr)
    {
        addAndMakeVisible(*content_);
        content_->setVisible(!collapsed_);

        // Check if content implements DynamicHeightContent interface
        dynamicContent_ = dynamic_cast<DynamicHeightContent*>(content_);
        if (dynamicContent_ != nullptr)
        {
            // Wire up the callback so content can notify us when its height changes
            dynamicContent_->onPreferredHeightChanged = [this]()
            {
                contentHeightChanged();
            };
        }

        resized();
    }
}

void CollapsibleSection::setCollapsed(bool collapsed)
{
    if (collapsed_ != collapsed)
    {
        collapsed_ = collapsed;

        if (content_ != nullptr)
        {
            content_->setVisible(!collapsed_);
        }

        if (onCollapseStateChanged)
        {
            onCollapseStateChanged(collapsed_);
        }

        if (onLayoutNeeded)
        {
            onLayoutNeeded();
        }

        repaint();
    }
}

void CollapsibleSection::toggleCollapsed()
{
    setCollapsed(!collapsed_);
}

int CollapsibleSection::getPreferredHeight() const
{
    if (collapsed_)
    {
        return HEADER_HEIGHT;
    }

    // When expanded, return header height + content's preferred height
    int contentHeight = 0;

    if (content_ != nullptr)
    {
        // If content implements DynamicHeightContent, use its preferred height
        if (dynamicContent_ != nullptr)
        {
            contentHeight = dynamicContent_->getPreferredHeight();
        }
        else
        {
            // Fall back to current component height for static content
            contentHeight = content_->getHeight();

            // If content height is 0, use a minimum fallback
            if (contentHeight == 0)
            {
                contentHeight = 100;
            }
        }
    }

    return HEADER_HEIGHT + contentHeight;
}

void CollapsibleSection::contentHeightChanged()
{
    if (!collapsed_ && onLayoutNeeded)
    {
        onLayoutNeeded();
    }
}

juce::Rectangle<int> CollapsibleSection::getHeaderBounds() const
{
    return getLocalBounds().withHeight(HEADER_HEIGHT);
}

} // namespace oscil
