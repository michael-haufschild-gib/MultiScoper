/*
    Oscil - Badge Component Implementation
*/

#include "ui/components/OscilBadge.h"

namespace oscil
{

OscilBadge::OscilBadge()
{
    theme_ = ThemeManager::getInstance().getCurrentTheme();
    ThemeManager::getInstance().addListener(this);
}

OscilBadge::OscilBadge(const juce::String& text)
    : OscilBadge()
{
    text_ = text;
}

OscilBadge::OscilBadge(const juce::String& text, BadgeColor color)
    : OscilBadge(text)
{
    color_ = color;
}

OscilBadge::~OscilBadge()
{
    ThemeManager::getInstance().removeListener(this);
}

void OscilBadge::setText(const juce::String& text)
{
    if (text_ != text)
    {
        text_ = text;
        repaint();
    }
}

void OscilBadge::setIcon(const juce::Image& icon)
{
    icon_ = icon;
    repaint();
}

void OscilBadge::clearIcon()
{
    icon_ = {};
    repaint();
}

void OscilBadge::setColor(BadgeColor color)
{
    if (color_ != color)
    {
        color_ = color;
        repaint();
    }
}

void OscilBadge::setVariant(BadgeVariant variant)
{
    if (variant_ != variant)
    {
        variant_ = variant;
        repaint();
    }
}

void OscilBadge::setCompact(bool compact)
{
    if (compact_ != compact)
    {
        compact_ = compact;
        repaint();
    }
}

int OscilBadge::getPreferredWidth() const
{
    auto font = juce::Font(juce::FontOptions().withHeight(compact_ ? 11.0f : 12.0f));
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, text_, 0, 0);
    int textWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
    int iconWidth = icon_.isValid() ? ICON_SIZE + 4 : 0;
    int padding = compact_ ? COMPACT_PADDING_H : PADDING_H;

    return textWidth + iconWidth + padding * 2;
}

int OscilBadge::getPreferredHeight() const
{
    return compact_ ? ComponentLayout::BADGE_COMPACT_HEIGHT - 4
                    : ComponentLayout::BADGE_HEIGHT;
}

void OscilBadge::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    auto bgColour = getBackgroundColour();
    auto textColour = getTextColour();
    auto borderColour = getBorderColour();

    // Background
    if (variant_ == BadgeVariant::Filled)
    {
        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds, bounds.getHeight() / 2.0f);
    }
    else
    {
        g.setColour(bgColour.withAlpha(0.1f));
        g.fillRoundedRectangle(bounds, bounds.getHeight() / 2.0f);

        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds.reduced(0.5f), bounds.getHeight() / 2.0f, 1.0f);
    }

    // Content
    int padding = compact_ ? COMPACT_PADDING_H : PADDING_H;
    auto contentBounds = bounds.reduced(static_cast<float>(padding), 0);

    if (icon_.isValid())
    {
        float iconY = (bounds.getHeight() - ICON_SIZE) / 2.0f;
        g.drawImage(icon_,
            juce::Rectangle<float>(contentBounds.getX(), iconY, ICON_SIZE, ICON_SIZE),
            juce::RectanglePlacement::centred);

        contentBounds.removeFromLeft(ICON_SIZE + 4);
    }

    g.setColour(textColour);
    g.setFont(juce::Font(juce::FontOptions().withHeight(compact_ ? 11.0f : 12.0f)));
    g.drawText(text_, contentBounds, juce::Justification::centred);
}

juce::Colour OscilBadge::getBackgroundColour() const
{
    switch (color_)
    {
        case BadgeColor::Success:
            return theme_.statusActive;  // Green
        case BadgeColor::Warning:
            return theme_.statusWarning; // Yellow
        case BadgeColor::Error:
            return theme_.statusError;   // Red
        case BadgeColor::Info:
            return juce::Colour(0xFF06B6D4); // Cyan
        case BadgeColor::Default:
        default:
            return theme_.controlActive; // Blue
    }
}

juce::Colour OscilBadge::getTextColour() const
{
    if (variant_ == BadgeVariant::Filled)
    {
        // White text on filled badges
        return juce::Colours::white;
    }
    else
    {
        // Colored text on outline badges
        return getBackgroundColour();
    }
}

juce::Colour OscilBadge::getBorderColour() const
{
    return getBackgroundColour();
}

void OscilBadge::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    repaint();
}

} // namespace oscil
