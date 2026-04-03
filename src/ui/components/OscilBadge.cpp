/*
    Oscil - Badge Component Implementation
*/

#include "ui/components/OscilBadge.h"

namespace oscil
{

OscilBadge::OscilBadge(IThemeService& themeService) : ThemedComponent(themeService) {}

OscilBadge::OscilBadge(IThemeService& themeService, const juce::String& text) : OscilBadge(themeService)
{
    text_ = text;
}

OscilBadge::OscilBadge(IThemeService& themeService, const juce::String& text, BadgeColor color)
    : OscilBadge(themeService, text)
{
    color_ = color;
}

OscilBadge::OscilBadge(IThemeService& themeService, const juce::String& text, BadgeColor color,
                       const juce::String& testId)
    : OscilBadge(themeService, text, color)
{
    setTestId(testId);
}

void OscilBadge::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

OscilBadge::~OscilBadge() {}

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
    int const textWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
    int const iconWidth = icon_.isValid() ? ICON_SIZE + 4 : 0;
    int const padding = compact_ ? COMPACT_PADDING_H : PADDING_H;

    return textWidth + iconWidth + (padding * 2);
}

int OscilBadge::getPreferredHeight() const
{
    return compact_ ? ComponentLayout::BADGE_COMPACT_HEIGHT - 4 : ComponentLayout::BADGE_HEIGHT;
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
        // Outline variant: Use higher alpha for better contrast (WCAG AA)
        g.setColour(bgColour.withAlpha(0.2f));
        g.fillRoundedRectangle(bounds, bounds.getHeight() / 2.0f);

        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds.reduced(0.5f), bounds.getHeight() / 2.0f, 1.0f);
    }

    // Content
    int const padding = compact_ ? COMPACT_PADDING_H : PADDING_H;
    auto contentBounds = bounds.reduced(static_cast<float>(padding), 0);

    if (icon_.isValid())
    {
        float const iconY = (bounds.getHeight() - ICON_SIZE) / 2.0f;
        g.drawImage(icon_, juce::Rectangle<float>(contentBounds.getX(), iconY, ICON_SIZE, ICON_SIZE),
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
            return getTheme().statusActive; // Green
        case BadgeColor::Warning:
            return getTheme().statusWarning; // Yellow
        case BadgeColor::Error:
            return getTheme().statusError; // Red
        case BadgeColor::Info:
            return juce::Colour(0xFF06B6D4); // Cyan
        case BadgeColor::Default:
        default:
            return getTheme().controlActive; // Blue
    }
}

juce::Colour OscilBadge::getTextColour() const
{
    if (variant_ == BadgeVariant::Filled)
    {
        // White text on filled badges
        return juce::Colours::white;
    }

    // Colored text on outline badges
    return getBackgroundColour();
}

juce::Colour OscilBadge::getBorderColour() const { return getBackgroundColour(); }

} // namespace oscil
