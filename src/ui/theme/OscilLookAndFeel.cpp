/*
    Oscil - Application-wide JUCE LookAndFeel Implementation
*/

#include "ui/theme/OscilLookAndFeel.h"

namespace oscil
{

OscilLookAndFeel::OscilLookAndFeel()
{
    // Initial apply with a default dark theme so the palette is sane before
    // the first theme-change callback arrives.
    applyTheme(ColorTheme{});
}

void OscilLookAndFeel::applyTheme(const ColorTheme& theme)
{
    // Derived accent colour matching SurfaceStyle::computeFrom().
    auto const accent =
        juce::Colour::fromHSV(theme.accentHue / 360.0f, theme.accentSaturation, theme.accentLightness, 1.0f);

    // ---- Label ----
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId, theme.textPrimary);
    setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::textWhenEditingColourId, theme.textPrimary);

    // ---- ListBox ----
    setColour(juce::ListBox::backgroundColourId, theme.backgroundPane);
    setColour(juce::ListBox::outlineColourId, theme.controlBorder);
    setColour(juce::ListBox::textColourId, theme.textPrimary);

    // ---- TextEditor ----
    setColour(juce::TextEditor::backgroundColourId, theme.controlBackground);
    setColour(juce::TextEditor::textColourId, theme.textPrimary);
    setColour(juce::TextEditor::highlightColourId, accent.withAlpha(0.30f));
    setColour(juce::TextEditor::highlightedTextColourId, theme.textHighlight);
    setColour(juce::TextEditor::outlineColourId, theme.controlBorder);
    setColour(juce::TextEditor::focusedOutlineColourId, accent);
    setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
    setColour(juce::CaretComponent::caretColourId, accent);

    // ---- ComboBox ----
    setColour(juce::ComboBox::backgroundColourId, theme.controlBackground);
    setColour(juce::ComboBox::textColourId, theme.textPrimary);
    setColour(juce::ComboBox::outlineColourId, theme.controlBorder);
    setColour(juce::ComboBox::buttonColourId, theme.controlHighlight);
    setColour(juce::ComboBox::arrowColourId, theme.textSecondary);
    setColour(juce::ComboBox::focusedOutlineColourId, accent);

    // ---- PopupMenu ----
    setColour(juce::PopupMenu::backgroundColourId, theme.backgroundPane);
    setColour(juce::PopupMenu::textColourId, theme.textPrimary);
    setColour(juce::PopupMenu::headerTextColourId, theme.textHighlight);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha(0.20f));
    setColour(juce::PopupMenu::highlightedTextColourId, theme.textHighlight);

    // ---- ScrollBar ----
    setColour(juce::ScrollBar::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ScrollBar::thumbColourId, theme.controlBorder);
    setColour(juce::ScrollBar::trackColourId, juce::Colours::transparentBlack);

    // ---- AlertWindow ----
    setColour(juce::AlertWindow::backgroundColourId, theme.backgroundPane);
    setColour(juce::AlertWindow::textColourId, theme.textPrimary);
    setColour(juce::AlertWindow::outlineColourId, theme.controlBorder);

    // ---- TextButton (raw juce::TextButton inside AlertWindow & elsewhere) ----
    // textColourOnId paints on top of buttonOnColourId (the saturated accent
    // fill). textHighlight is BLACK in light themes, so it would put black
    // text on a dark accent — pick a contrast-safe colour instead.
    setColour(juce::TextButton::buttonColourId, theme.controlBackground);
    setColour(juce::TextButton::buttonOnColourId, accent);
    setColour(juce::TextButton::textColourOnId, ColorTheme::pickContrastingText(accent));
    setColour(juce::TextButton::textColourOffId, theme.textPrimary);

    // ---- DocumentWindow / ResizableWindow (plugin host frames, rare) ----
    setColour(juce::ResizableWindow::backgroundColourId, theme.backgroundPrimary);
    setColour(juce::DocumentWindow::textColourId, theme.textPrimary);

    // ---- ToggleButton ----
    setColour(juce::ToggleButton::textColourId, theme.textPrimary);
    setColour(juce::ToggleButton::tickColourId, accent);
    setColour(juce::ToggleButton::tickDisabledColourId, theme.textSecondary);

    // ---- TooltipWindow ----
    setColour(juce::TooltipWindow::backgroundColourId, theme.backgroundSecondary);
    setColour(juce::TooltipWindow::textColourId, theme.textPrimary);
    setColour(juce::TooltipWindow::outlineColourId, theme.controlBorder);
}

} // namespace oscil
