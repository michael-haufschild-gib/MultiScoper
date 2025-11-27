/*
    Oscil - Badge Component
    Small status indicators with multiple colors and variants
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/ThemeManager.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"

namespace oscil
{

/**
 * A small badge component for status indicators
 *
 * Features:
 * - 5 colors: Default, Success, Warning, Error, Info
 * - 2 variants: Filled, Outline
 * - Optional icon
 * - Compact mode for smaller spaces
 */
class OscilBadge : public juce::Component,
                   public ThemeManagerListener
{
public:
    OscilBadge();
    explicit OscilBadge(const juce::String& text);
    OscilBadge(const juce::String& text, BadgeColor color);
    ~OscilBadge() override;

    // Content
    void setText(const juce::String& text);
    juce::String getText() const { return text_; }

    void setIcon(const juce::Image& icon);
    void clearIcon();

    // Styling
    void setColor(BadgeColor color);
    BadgeColor getColor() const { return color_; }

    void setVariant(BadgeVariant variant);
    BadgeVariant getVariant() const { return variant_; }

    void setCompact(bool compact);
    bool isCompact() const { return compact_; }

    // Size hints
    int getPreferredWidth() const;
    int getPreferredHeight() const;

    // Component overrides
    void paint(juce::Graphics& g) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

private:
    juce::Colour getBackgroundColour() const;
    juce::Colour getTextColour() const;
    juce::Colour getBorderColour() const;

    juce::String text_;
    juce::Image icon_;
    BadgeColor color_ = BadgeColor::Default;
    BadgeVariant variant_ = BadgeVariant::Filled;
    bool compact_ = false;

    ColorTheme theme_;

    static constexpr int ICON_SIZE = 12;
    static constexpr int PADDING_H = 8;
    static constexpr int PADDING_V = 4;
    static constexpr int COMPACT_PADDING_H = 6;
    static constexpr int COMPACT_PADDING_V = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilBadge)
};

} // namespace oscil
