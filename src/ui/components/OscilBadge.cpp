/*
    Oscil - Badge Component Implementation
*/

#include "ui/components/OscilBadge.h"

#include "ui/components/SurfacePainter.h"
#include "ui/theme/Typography.h"

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
    auto font = compact_ ? Typography::caption() : Typography::small();
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
    const auto& glass = getSurface();

    auto bgColour = getBackgroundColour();
    auto textColour = getTextColour();
    auto borderColour = getBorderColour();
    float const radius = bounds.getHeight() / 2.0f;

    // Background + border
    if (color_ == BadgeColor::Default)
    {
        // Default variant: glass bg + borderSubtle
        g.setColour(glass.bgGlass);
        g.fillRoundedRectangle(bounds, radius);
        g.setColour(glass.borderSubtle);
        g.drawRoundedRectangle(bounds.reduced(0.5f), radius, 1.0f);
    }
    else if (color_ == BadgeColor::Info)
    {
        // Accent variant: accentSubtle fill + accentMuted border
        g.setColour(glass.accentSubtle);
        g.fillRoundedRectangle(bounds, radius);
        g.setColour(glass.accentMuted);
        g.drawRoundedRectangle(bounds.reduced(0.5f), radius, 1.0f);
    }
    else if (variant_ == BadgeVariant::Filled)
    {
        // Status filled: status color bg
        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds, radius);
    }
    else
    {
        // Status outline: subtle bg + colored border
        g.setColour(bgColour.withAlpha(0.2f));
        g.fillRoundedRectangle(bounds, radius);
        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds.reduced(0.5f), radius, 1.0f);
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
    g.setFont(compact_ ? Typography::caption() : Typography::small());
    g.drawText(text_, contentBounds, juce::Justification::centred);
}

juce::Colour OscilBadge::getBackgroundColour() const
{
    const auto& glass = getSurface();

    switch (color_)
    {
        case BadgeColor::Success:
            return getTheme().statusActive;
        case BadgeColor::Warning:
            return getTheme().statusWarning;
        case BadgeColor::Error:
            return getTheme().statusError;
        case BadgeColor::Info:
            return glass.accent;
        case BadgeColor::Default:
        default:
            return glass.bgGlass;
    }
}

juce::Colour OscilBadge::getTextColour() const
{
    const auto& glass = getSurface();

    if (color_ == BadgeColor::Default)
        return getTheme().textPrimary;

    if (color_ == BadgeColor::Info)
        return glass.accent;

    if (variant_ == BadgeVariant::Filled)
    {
        // Text contrasts with the opaque status fill. Hardcoded white fails
        // AA against bright statuses in dark themes (e.g. #00DD00) and
        // succeeds against dark statuses in light themes (e.g. #006600);
        // compute per theme instead.
        return ColorTheme::pickContrastingText(getBackgroundColour());
    }

    // Colored text on outline badges for status variants
    return getBackgroundColour();
}

juce::Colour OscilBadge::getBorderColour() const
{
    const auto& glass = getSurface();

    switch (color_)
    {
        case BadgeColor::Default:
            return glass.borderSubtle;
        case BadgeColor::Info:
            return glass.accentMuted;
        default:
            return getBackgroundColour();
    }
}

} // namespace oscil
