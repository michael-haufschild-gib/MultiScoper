/*
    Oscil - Tabs Component Painting
    Rendering, layout calculation, and indicator animation for OscilTabs
*/

#include "ui/components/GlassPainter.h"
#include "ui/components/OscilTabs.h"

#include <utility>

namespace oscil
{

void OscilTabs::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Default variant: borderSubtle bottom line under tab list
    if (variant_ == Variant::Default && orientation_ == Orientation::Horizontal)
    {
        g.setColour(getGlass().borderSubtle);
        g.fillRect(0, bounds.getHeight() - 1, bounds.getWidth(), 1);
    }

    for (int i = 0; std::cmp_less(i, tabs_.size()); ++i)
    {
        auto tabBounds = getTabBounds(i);
        paintTab(g, i, tabBounds);
    }

    paintIndicator(g);

    // Focus ring around active tab
    if (hasFocus_)
    {
        auto selectedBounds = getTabBounds(selectedIndex_).toFloat();
        GlassPainter::paintFocusRing(g, selectedBounds, ComponentLayout::RADIUS_SM, getGlass().accent);
    }
}

void OscilTabs::paintTab(juce::Graphics& g, int index, juce::Rectangle<int> bounds)
{
    const auto& tab = tabs_[static_cast<size_t>(index)];
    bool const isSelected = (index == selectedIndex_);
    bool const isHovered = (index == hoveredIndex_);
    float const opacity = tab.enabled ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    // Tab hover: bgHover background
    if (isHovered && !isSelected && tab.enabled)
    {
        g.setColour(getGlass().bgHover);
        g.fillRoundedRectangle(bounds.reduced(2).toFloat(), ComponentLayout::RADIUS_SM);
    }

    auto contentBounds = bounds.reduced(TAB_PADDING_H, 0);
    int contentWidth = 0;

    auto font = juce::Font(juce::FontOptions().withHeight(TAB_FONT_SIZE));
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, tab.label, 0, 0);
    int const labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
    contentWidth += labelWidth;

    if (tab.icon.isValid())
        contentWidth += ICON_SIZE + 8;

    if (tab.badgeCount > 0)
        contentWidth += BADGE_SIZE + 4;

    int startX = contentBounds.getX() + ((contentBounds.getWidth() - contentWidth) / 2);
    int const centerY = bounds.getCentreY();

    if (tab.icon.isValid())
    {
        g.setOpacity(opacity);
        g.drawImage(tab.icon,
                    juce::Rectangle<float>(static_cast<float>(startX),
                                           static_cast<float>(centerY) - (static_cast<float>(ICON_SIZE) / 2.0f),
                                           ICON_SIZE, ICON_SIZE),
                    juce::RectanglePlacement::centred);
        startX += ICON_SIZE + 8;
    }

    // Text color: accent when active, textSecondary default
    auto textColour = isSelected                 ? getGlass().accent
                      : isHovered && tab.enabled ? getTheme().textPrimary
                                                 : getTheme().textSecondary;

    g.setColour(textColour.withAlpha(opacity));
    g.setFont(font);

    auto labelBounds = juce::Rectangle<int>(startX, 0, labelWidth, bounds.getHeight());
    g.drawText(tab.label, labelBounds, juce::Justification::centred);
    startX += labelWidth;

    if (tab.badgeCount > 0)
    {
        auto badgeBounds = juce::Rectangle<int>(startX + 4, centerY - (BADGE_SIZE / 2), BADGE_SIZE, BADGE_SIZE);
        paintBadge(g, badgeBounds, tab.badgeCount);
    }
}

void OscilTabs::paintIndicator(juce::Graphics& g)
{
    if (tabs_.empty())
        return;

    auto indicatorBounds = getIndicatorBounds();

    switch (variant_)
    {
        case Variant::Default:
            // Animated accent underline that slides between tabs
            g.setColour(getGlass().accent);
            if (orientation_ == Orientation::Horizontal)
            {
                g.fillRoundedRectangle(indicatorBounds.getX(), static_cast<float>(getHeight() - INDICATOR_HEIGHT),
                                       indicatorBounds.getWidth(), INDICATOR_HEIGHT, INDICATOR_HEIGHT / 2.0f);
            }
            else
            {
                g.fillRoundedRectangle(0, indicatorBounds.getY(), INDICATOR_HEIGHT, indicatorBounds.getHeight(),
                                       INDICATOR_HEIGHT / 2.0f);
            }
            break;

        case Variant::Pills:
            // accentSubtle bg + accentMuted border
            g.setColour(getGlass().accentSubtle);
            g.fillRoundedRectangle(indicatorBounds.reduced(2), ComponentLayout::RADIUS_SM);
            g.setColour(getGlass().accentMuted);
            g.drawRoundedRectangle(indicatorBounds.reduced(2), ComponentLayout::RADIUS_SM, 1.0f);
            break;

        case Variant::Bordered:
            g.setColour(getGlass().borderDefault);
            g.drawRoundedRectangle(indicatorBounds.reduced(1), ComponentLayout::RADIUS_SM, 1.0f);
            break;
    }
}

void OscilTabs::paintBadge(juce::Graphics& g, juce::Rectangle<int> bounds, int count)
{
    g.setColour(getTheme().statusError);
    g.fillEllipse(bounds.toFloat());

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions().withHeight(BADGE_FONT_SIZE)).boldened());

    juce::String const text = count > 99 ? "99+" : juce::String(count);
    g.drawText(text, bounds, juce::Justification::centred);
}

juce::Rectangle<int> OscilTabs::getTabBounds(int index) const
{
    if (index < 0 || std::cmp_greater_equal(index, cachedTabBounds_.size()))
        return {};

    return cachedTabBounds_[static_cast<size_t>(index)];
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void OscilTabs::updateLayoutCache()
{
    cachedTabBounds_.clear();

    if (tabs_.empty())
        return;

    auto bounds = getLocalBounds();
    cachedTabBounds_.reserve(tabs_.size());

    if (orientation_ == Orientation::Horizontal)
    {
        int const height = tabHeight_ > 0 ? tabHeight_ : bounds.getHeight();

        if (stretchTabs_)
        {
            int const numTabs = static_cast<int>(tabs_.size());
            int const tabWidth = bounds.getWidth() / numTabs;
            for (int i = 0; i < numTabs; ++i)
            {
                // Last tab gets remaining width to avoid rounding gap
                int const w = (i == numTabs - 1) ? (bounds.getWidth() - i * tabWidth) : tabWidth;
                cachedTabBounds_.emplace_back(i * tabWidth, 0, w, height);
            }
        }
        else if (tabWidth_ > 0)
        {
            for (int i = 0; std::cmp_less(i, tabs_.size()); ++i)
            {
                cachedTabBounds_.emplace_back(i * tabWidth_, 0, tabWidth_, height);
            }
        }
        else
        {
            int x = 0;
            auto font = juce::Font(juce::FontOptions().withHeight(TAB_FONT_SIZE));

            for (const auto& tab : tabs_)
            {
                juce::GlyphArrangement glyphs;
                glyphs.addLineOfText(font, tab.label, 0, 0);
                int const labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
                int const iconWidth = tab.icon.isValid() ? ICON_SIZE + 8 : 0;
                int const badgeWidth = tab.badgeCount > 0 ? BADGE_SIZE + 4 : 0;
                int const width = labelWidth + iconWidth + badgeWidth + (TAB_PADDING_H * 2);

                cachedTabBounds_.emplace_back(x, 0, width, height);
                x += width;
            }
        }
    }
    else
    {
        int const width = tabWidth_ > 0 ? tabWidth_ : bounds.getWidth();
        int const height = tabHeight_ > 0 ? tabHeight_ : DEFAULT_TAB_HEIGHT;

        for (int i = 0; std::cmp_less(i, tabs_.size()); ++i)
        {
            cachedTabBounds_.emplace_back(0, i * height, width, height);
        }
    }
}

juce::Rectangle<float> OscilTabs::getIndicatorBounds() const
{
    if (orientation_ == Orientation::Horizontal)
    {
        int const height = tabHeight_ > 0 ? tabHeight_ : getHeight();
        return {indicatorXSpring_.position, 0, indicatorWidthSpring_.position, static_cast<float>(height)};
    }

    int const width = tabWidth_ > 0 ? tabWidth_ : getWidth();
    return {0, indicatorXSpring_.position, static_cast<float>(width), indicatorWidthSpring_.position};
}

} // namespace oscil
