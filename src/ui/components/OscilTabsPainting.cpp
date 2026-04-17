/*
    Oscil - Tabs Component Painting
    Rendering, layout calculation, and indicator animation for OscilTabs
*/

#include "ui/components/OscilTabs.h"
#include "ui/components/SurfacePainter.h"
#include "ui/theme/Typography.h"

#include <utility>

namespace oscil
{

void OscilTabs::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Default variant: borderSubtle bottom line under tab list
    if (variant_ == Variant::Default && orientation_ == Orientation::Horizontal)
    {
        g.setColour(getSurface().borderSubtle);
        g.fillRect(0, bounds.getHeight() - 1, bounds.getWidth(), 1);
    }

    for (int i = 0; std::cmp_less(i, tabs_.size()); ++i)
    {
        auto tabBounds = getTabBounds(i);
        paintTab(g, i, tabBounds);
    }

    // 1px vertical hairline separators between adjacent tabs (Default horizontal only).
    if (variant_ == Variant::Default && orientation_ == Orientation::Horizontal && tabs_.size() > 1)
    {
        g.setColour(getSurface().borderSubtle);
        for (int i = 0; std::cmp_less(i + 1, tabs_.size()); ++i)
        {
            auto tb = getTabBounds(i);
            g.fillRect(tb.getRight() - 1, tb.getY() + 4, 1, tb.getHeight() - 8);
        }
    }

    paintIndicator(g);

    // Focus ring around active tab
    if (hasFocus_)
    {
        auto selectedBounds = getTabBounds(selectedIndex_).toFloat();
        SurfacePainter::paintFocusRing(g, selectedBounds, ComponentLayout::RADIUS_SM, getSurface().accent);
    }
}

void OscilTabs::paintTab(juce::Graphics& g, int index, juce::Rectangle<int> bounds)
{
    const auto& tab = tabs_[static_cast<size_t>(index)];
    bool const isSelected = (index == selectedIndex_);
    bool const isHovered = (index == hoveredIndex_);
    bool const isDefaultHorizontal = (variant_ == Variant::Default && orientation_ == Orientation::Horizontal);

    if (isSelected)
    {
        g.setColour(getSurface().bgActive);
        g.fillRect(bounds);
    }
    else if (isHovered && tab.enabled)
    {
        g.setColour(getSurface().bgHover);
        g.fillRect(bounds);
    }

    paintTabContent(g, index, bounds, isSelected, isHovered);

    if (isDefaultHorizontal && isHovered && !isSelected && tab.enabled)
    {
        auto accent = getEffectiveTabAccent(index).withAlpha(HOVER_UNDERLINE_ALPHA);
        g.setColour(accent);
        g.fillRect(bounds.getX() + 1, bounds.getBottom() - HOVER_UNDERLINE_HEIGHT, bounds.getWidth() - 2,
                   HOVER_UNDERLINE_HEIGHT);
    }
}

void OscilTabs::paintTabContent(juce::Graphics& g, int index, juce::Rectangle<int> bounds, bool isSelected,
                                bool isHovered)
{
    const auto& tab = tabs_[static_cast<size_t>(index)];
    auto contentBounds = bounds.reduced(TAB_PADDING_H, 0);
    auto font = Typography::body().withHeight(TAB_FONT_SIZE);

    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, tab.label, 0, 0);
    int const labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
    int contentWidth = labelWidth;
    if (tab.icon.isValid())
        contentWidth += ICON_SIZE + 8;
    if (tab.badgeCount > 0)
        contentWidth += BADGE_SIZE + 4;

    int startX = contentBounds.getX() + ((contentBounds.getWidth() - contentWidth) / 2);
    int const centerY = bounds.getCentreY();
    float const opacity = tab.enabled ? 1.0f : ComponentLayout::DISABLED_OPACITY;

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

    juce::Colour textColour;
    if (!tab.enabled)
        textColour = getTheme().textMuted;
    else if (isSelected)
        textColour = getTheme().textPrimary;
    else if (isHovered)
        textColour = getTheme().textHighlight;
    else
        textColour = getTheme().textSecondary;

    g.setColour(textColour.withAlpha(opacity));
    g.setFont(font);
    g.drawText(tab.label, juce::Rectangle<int>(startX, 0, labelWidth, bounds.getHeight()),
               juce::Justification::centred);
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
            // Active tab underline in per-tab accent colour. Flat 2px bar, 1px inset each side.
            g.setColour(getEffectiveTabAccent(selectedIndex_));
            if (orientation_ == Orientation::Horizontal)
            {
                g.fillRect(indicatorBounds.getX() + 1.0f, static_cast<float>(getHeight() - ACTIVE_UNDERLINE_HEIGHT),
                           indicatorBounds.getWidth() - 2.0f, static_cast<float>(ACTIVE_UNDERLINE_HEIGHT));
            }
            else
            {
                g.fillRect(0.0f, indicatorBounds.getY() + 1.0f, static_cast<float>(ACTIVE_UNDERLINE_HEIGHT),
                           indicatorBounds.getHeight() - 2.0f);
            }
            break;

        case Variant::Pills:
            // accentSubtle bg + accentMuted border
            g.setColour(getSurface().accentSubtle);
            g.fillRoundedRectangle(indicatorBounds.reduced(2), ComponentLayout::RADIUS_SM);
            g.setColour(getSurface().accentMuted);
            g.drawRoundedRectangle(indicatorBounds.reduced(2), ComponentLayout::RADIUS_SM, 1.0f);
            break;

        case Variant::Bordered:
            g.setColour(getSurface().borderDefault);
            g.drawRoundedRectangle(indicatorBounds.reduced(1), ComponentLayout::RADIUS_SM, 1.0f);
            break;
    }
}

void OscilTabs::paintBadge(juce::Graphics& g, juce::Rectangle<int> bounds, int count)
{
    g.setColour(getTheme().statusError);
    g.fillEllipse(bounds.toFloat());

    g.setColour(juce::Colours::white);
    // BADGE_FONT_SIZE is 10pt; ellipse sized for this font.
    g.setFont(Typography::captionBold().withHeight(BADGE_FONT_SIZE));

    juce::String const text = count > 99 ? "99+" : juce::String(count);
    g.drawText(text, bounds, juce::Justification::centred);
}

juce::Rectangle<int> OscilTabs::getTabBounds(int index) const
{
    if (index < 0 || std::cmp_greater_equal(index, cachedTabBounds_.size()))
        return {};

    return cachedTabBounds_[static_cast<size_t>(index)];
}

void OscilTabs::updateHorizontalLayoutCache(juce::Rectangle<int> bounds)
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
        return;
    }

    if (tabWidth_ > 0)
    {
        for (int i = 0; std::cmp_less(i, tabs_.size()); ++i)
        {
            cachedTabBounds_.emplace_back(i * tabWidth_, 0, tabWidth_, height);
        }
        return;
    }

    int x = 0;
    auto font = Typography::body().withHeight(TAB_FONT_SIZE);

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

void OscilTabs::updateVerticalLayoutCache(juce::Rectangle<int> bounds)
{
    int const width = tabWidth_ > 0 ? tabWidth_ : bounds.getWidth();
    int const height = tabHeight_ > 0 ? tabHeight_ : DEFAULT_TAB_HEIGHT;

    for (int i = 0; std::cmp_less(i, tabs_.size()); ++i)
    {
        cachedTabBounds_.emplace_back(0, i * height, width, height);
    }
}

void OscilTabs::updateLayoutCache()
{
    cachedTabBounds_.clear();

    if (tabs_.empty())
        return;

    auto bounds = getLocalBounds();
    cachedTabBounds_.reserve(tabs_.size());

    if (orientation_ == Orientation::Horizontal)
        updateHorizontalLayoutCache(bounds);
    else
        updateVerticalLayoutCache(bounds);
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
