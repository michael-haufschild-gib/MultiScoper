/*
    Oscil - Tabs Painter Implementation
*/

#include "ui/components/painters/TabsPainter.h"
#include "ui/components/OscilTabs.h"

namespace oscil
{

// Constants replicated from OscilTabs.cpp/h
static constexpr float TAB_FONT_SIZE = 13.0f;
static constexpr float BADGE_FONT_SIZE = 10.0f;
static constexpr int ICON_SIZE = 16;
static constexpr int BADGE_SIZE = 18;
static constexpr int TAB_PADDING_H = 16;
static constexpr int INDICATOR_HEIGHT = 3;

void TabsPainter::paint(juce::Graphics& g, OscilTabs& tabs)
{
    auto bounds = tabs.getLocalBounds();

    // Background line for default variant
    if (tabs.variant_ == OscilTabs::Variant::Default && tabs.orientation_ == OscilTabs::Orientation::Horizontal)
    {
        g.setColour(tabs.getTheme().controlBorder.withAlpha(0.3f));
        g.fillRect(0, bounds.getHeight() - 1, bounds.getWidth(), 1);
    }

    // Draw tabs
    for (int i = 0; i < static_cast<int>(tabs.tabs_.size()); ++i)
    {
        auto tabBounds = tabs.getTabBounds(i);
        paintTab(g, tabs, i, tabBounds);
    }

    // Draw indicator
    paintIndicator(g, tabs);

    // Focus ring
    if (tabs.hasFocus_)
    {
        auto selectedBounds = tabs.getTabBounds(tabs.selectedIndex_).toFloat();
        g.setColour(tabs.getTheme().controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
        g.drawRoundedRectangle(
            selectedBounds.reduced(-ComponentLayout::FOCUS_RING_OFFSET),
            ComponentLayout::RADIUS_SM,
            ComponentLayout::FOCUS_RING_WIDTH
        );
    }
}

void TabsPainter::paintTab(juce::Graphics& g, OscilTabs& tabs, int index, juce::Rectangle<int> bounds)
{
    const auto& tab = tabs.tabs_[static_cast<size_t>(index)];
    bool isSelected = (index == tabs.selectedIndex_);
    bool isHovered = (index == tabs.hoveredIndex_);
    float opacity = tab.enabled ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // Hover background for pills variant
    if (tabs.variant_ == OscilTabs::Variant::Pills && isHovered && !isSelected && tab.enabled)
    {
        g.setColour(tabs.getTheme().backgroundSecondary.withAlpha(0.5f));
        g.fillRoundedRectangle(bounds.reduced(2).toFloat(), ComponentLayout::RADIUS_SM);
    }

    // Content bounds
    auto contentBounds = bounds.reduced(TAB_PADDING_H, 0);
    int contentWidth = 0;

    // Calculate total content width for centering
    auto font = juce::Font(juce::FontOptions().withHeight(TAB_FONT_SIZE));
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, tab.label, 0, 0);
    int labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
    contentWidth += labelWidth;

    if (tab.icon.isValid())
        contentWidth += ICON_SIZE + 8;

    if (tab.badgeCount > 0)
        contentWidth += BADGE_SIZE + 4;

    int startX = contentBounds.getX() + (contentBounds.getWidth() - contentWidth) / 2;
    int centerY = bounds.getCentreY();

    // Icon
    if (tab.icon.isValid())
    {
        g.setOpacity(opacity);
        g.drawImage(tab.icon,
            juce::Rectangle<float>(static_cast<float>(startX),
                                   static_cast<float>(centerY - ICON_SIZE / 2),
                                   ICON_SIZE, ICON_SIZE),
            juce::RectanglePlacement::centred);
        startX += ICON_SIZE + 8;
    }

    // Label
    auto textColour = isSelected ? tabs.getTheme().controlActive
                    : isHovered && tab.enabled ? tabs.getTheme().textPrimary
                    : tabs.getTheme().textSecondary;

    g.setColour(textColour.withAlpha(opacity));
    g.setFont(font);

    auto labelBounds = juce::Rectangle<int>(startX, 0, labelWidth, bounds.getHeight());
    g.drawText(tab.label, labelBounds, juce::Justification::centred);
    startX += labelWidth;

    // Badge
    if (tab.badgeCount > 0)
    {
        auto badgeBounds = juce::Rectangle<int>(
            startX + 4, centerY - BADGE_SIZE / 2, BADGE_SIZE, BADGE_SIZE);
        paintBadge(g, tabs, badgeBounds, tab.badgeCount);
    }
}

void TabsPainter::paintIndicator(juce::Graphics& g, OscilTabs& tabs)
{
    if (tabs.tabs_.empty())
        return;

    auto indicatorBounds = tabs.getIndicatorBounds();

    switch (tabs.variant_)
    {
        case OscilTabs::Variant::Default:
            // Underline indicator
            g.setColour(tabs.getTheme().controlActive);
            if (tabs.orientation_ == OscilTabs::Orientation::Horizontal)
            {
                g.fillRoundedRectangle(
                    indicatorBounds.getX(),
                    static_cast<float>(tabs.getHeight() - INDICATOR_HEIGHT),
                    indicatorBounds.getWidth(),
                    INDICATOR_HEIGHT,
                    INDICATOR_HEIGHT / 2.0f
                );
            }
            else
            {
                g.fillRoundedRectangle(
                    0,
                    indicatorBounds.getY(),
                    INDICATOR_HEIGHT,
                    indicatorBounds.getHeight(),
                    INDICATOR_HEIGHT / 2.0f
                );
            }
            break;

        case OscilTabs::Variant::Pills:
            // Filled background
            g.setColour(tabs.getTheme().controlActive.withAlpha(0.15f));
            g.fillRoundedRectangle(indicatorBounds.reduced(2), ComponentLayout::RADIUS_SM);
            g.setColour(tabs.getTheme().controlActive);
            g.drawRoundedRectangle(indicatorBounds.reduced(2), ComponentLayout::RADIUS_SM, 1.0f);
            break;

        case OscilTabs::Variant::Bordered:
            // Border around tab
            g.setColour(tabs.getTheme().controlBorder);
            g.drawRoundedRectangle(indicatorBounds.reduced(1), ComponentLayout::RADIUS_SM, 1.0f);
            break;
    }
}

void TabsPainter::paintBadge(juce::Graphics& g, OscilTabs& tabs, juce::Rectangle<int> bounds, int count)
{
    g.setColour(tabs.getTheme().statusError);
    g.fillEllipse(bounds.toFloat());

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions().withHeight(BADGE_FONT_SIZE)).boldened());

    juce::String text = count > 99 ? "99+" : juce::String(count);
    g.drawText(text, bounds, juce::Justification::centred);
}

} // namespace oscil
