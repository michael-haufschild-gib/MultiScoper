/*
    Oscil - Tabs Component Painting
    Rendering, layout calculation, and indicator animation for OscilTabs
*/

#include "ui/components/OscilTabs.h"

#include <utility>

namespace oscil
{

static constexpr float kTabFontSize = 13.0f;
static constexpr float kBadgeFontSize = 10.0f;

void OscilTabs::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    if (variant_ == Variant::Default && orientation_ == Orientation::Horizontal)
    {
        g.setColour(getTheme().controlBorder.withAlpha(0.3f));
        g.fillRect(0, bounds.getHeight() - 1, bounds.getWidth(), 1);
    }

    for (int i = 0; std::cmp_less(i, tabs_.size()); ++i)
    {
        auto tabBounds = getTabBounds(i);
        paintTab(g, i, tabBounds);
    }

    paintIndicator(g);

    if (hasFocus_)
    {
        auto selectedBounds = getTabBounds(selectedIndex_).toFloat();
        g.setColour(getTheme().controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
        g.drawRoundedRectangle(selectedBounds.reduced(-ComponentLayout::FOCUS_RING_OFFSET), ComponentLayout::RADIUS_SM,
                               ComponentLayout::FOCUS_RING_WIDTH);
    }
}

void OscilTabs::paintTab(juce::Graphics& g, int index, juce::Rectangle<int> bounds)
{
    const auto& tab = tabs_[static_cast<size_t>(index)];
    bool const isSelected = (index == selectedIndex_);
    bool const isHovered = (index == hoveredIndex_);
    float const opacity = tab.enabled ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    if (variant_ == Variant::Pills && isHovered && !isSelected && tab.enabled)
    {
        g.setColour(getTheme().backgroundSecondary.withAlpha(0.5f));
        g.fillRoundedRectangle(bounds.reduced(2).toFloat(), ComponentLayout::RADIUS_SM);
    }

    auto contentBounds = bounds.reduced(TAB_PADDING_H, 0);
    int contentWidth = 0;

    auto font = juce::Font(juce::FontOptions().withHeight(kTabFontSize));
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
                    juce::Rectangle<float>(static_cast<float>(startX), static_cast<float>(centerY - (ICON_SIZE / 2)),
                                           ICON_SIZE, ICON_SIZE),
                    juce::RectanglePlacement::centred);
        startX += ICON_SIZE + 8;
    }

    auto textColour = isSelected                 ? getTheme().controlActive
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
            g.setColour(getTheme().controlActive);
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
            g.setColour(getTheme().controlActive.withAlpha(0.15f));
            g.fillRoundedRectangle(indicatorBounds.reduced(2), ComponentLayout::RADIUS_SM);
            g.setColour(getTheme().controlActive);
            g.drawRoundedRectangle(indicatorBounds.reduced(2), ComponentLayout::RADIUS_SM, 1.0f);
            break;

        case Variant::Bordered:
            g.setColour(getTheme().controlBorder);
            g.drawRoundedRectangle(indicatorBounds.reduced(1), ComponentLayout::RADIUS_SM, 1.0f);
            break;
    }
}

void OscilTabs::paintBadge(juce::Graphics& g, juce::Rectangle<int> bounds, int count)
{
    g.setColour(getTheme().statusError);
    g.fillEllipse(bounds.toFloat());

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions().withHeight(kBadgeFontSize)).boldened());

    juce::String const text = count > 99 ? "99+" : juce::String(count);
    g.drawText(text, bounds, juce::Justification::centred);
}

juce::Rectangle<int> OscilTabs::getTabBounds(int index) const
{
    if (index < 0 || std::cmp_greater_equal(index, cachedTabBounds_.size()))
        return {};

    return cachedTabBounds_[static_cast<size_t>(index)];
}

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
            int const tabWidth = bounds.getWidth() / static_cast<int>(tabs_.size());
            for (int i = 0; std::cmp_less(i, tabs_.size()); ++i)
            {
                cachedTabBounds_.emplace_back(i * tabWidth, 0, tabWidth, height);
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
            auto font = juce::Font(juce::FontOptions().withHeight(kTabFontSize));

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

    int width = tabWidth_ > 0 ? tabWidth_ : getWidth();
    return juce::Rectangle<float>(0, indicatorXSpring_.position, static_cast<float>(width),
                                  indicatorWidthSpring_.position);
}

} // namespace oscil
