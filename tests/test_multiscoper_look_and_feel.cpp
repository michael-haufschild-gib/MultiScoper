/*
    MultiScoper - MultiScoperLookAndFeel Tests

    Verifies that applyTheme() correctly propagates ColorTheme tokens onto
    every JUCE widget colour ID we override. The LookAndFeel is what keeps
    raw juce::Label / juce::TextEditor / juce::ComboBox / juce::AlertWindow
    in sync with MultiScoper themes — without these mappings, dark themes leak
    LookAndFeel_V4 defaults (light backgrounds, native chrome) into modal
    dialogs, tooltips and popups.

    Notes on design-by-contract:
      * accent is derived from theme HSV (hue / saturation / lightness).
      * Transparent overrides (bg transparent for Label, ScrollBar tracks)
        are deliberate and asserted as such.
      * focus outlines use the solid accent (alpha 1.0) — test verifies this
        rather than an alpha-applied variant.
*/

#include "ui/theme/ColorTheme.h"
#include "ui/theme/MultiScoperLookAndFeel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <gtest/gtest.h>

using namespace multiscoper;

namespace
{
ColorTheme makeCanonicalTheme()
{
    ColorTheme theme;
    theme.backgroundPrimary = juce::Colour(0xFF101010);
    theme.backgroundSecondary = juce::Colour(0xFF202020);
    theme.backgroundPane = juce::Colour(0xFF181818);
    theme.textPrimary = juce::Colour(0xFFE0E0E0);
    theme.textSecondary = juce::Colour(0xFF909090);
    theme.textHighlight = juce::Colour(0xFFFFFFFF);
    theme.controlBackground = juce::Colour(0xFF161B24);
    theme.controlBorder = juce::Colour(0xFF242A35);
    theme.controlHighlight = juce::Colour(0xFF1F2630);
    theme.accentHue = 200.0f;
    theme.accentSaturation = 0.8f;
    theme.accentLightness = 0.7f;
    return theme;
}

juce::Colour expectedAccent(const ColorTheme& theme)
{
    return juce::Colour::fromHSV(theme.accentHue / 360.0f, theme.accentSaturation, theme.accentLightness, 1.0f);
}
} // namespace

TEST(MultiScoperLookAndFeelTest, LabelTokensMapToThemeOnApply)
{
    MultiScoperLookAndFeel laf;
    auto theme = makeCanonicalTheme();
    laf.applyTheme(theme);

    EXPECT_EQ(laf.findColour(juce::Label::backgroundColourId), juce::Colours::transparentBlack);
    EXPECT_EQ(laf.findColour(juce::Label::textColourId), theme.textPrimary);
    EXPECT_EQ(laf.findColour(juce::Label::outlineColourId), juce::Colours::transparentBlack);
    EXPECT_EQ(laf.findColour(juce::Label::textWhenEditingColourId), theme.textPrimary);
}

TEST(MultiScoperLookAndFeelTest, ListBoxTokensMapToTheme)
{
    MultiScoperLookAndFeel laf;
    auto theme = makeCanonicalTheme();
    laf.applyTheme(theme);

    EXPECT_EQ(laf.findColour(juce::ListBox::backgroundColourId), theme.backgroundPane);
    EXPECT_EQ(laf.findColour(juce::ListBox::outlineColourId), theme.controlBorder);
    EXPECT_EQ(laf.findColour(juce::ListBox::textColourId), theme.textPrimary);
}

TEST(MultiScoperLookAndFeelTest, TextEditorFocusOutlineUsesAccent)
{
    MultiScoperLookAndFeel laf;
    auto theme = makeCanonicalTheme();
    laf.applyTheme(theme);

    const auto accent = expectedAccent(theme);
    EXPECT_EQ(laf.findColour(juce::TextEditor::focusedOutlineColourId), accent);
    EXPECT_EQ(laf.findColour(juce::CaretComponent::caretColourId), accent);
    EXPECT_EQ(laf.findColour(juce::TextEditor::backgroundColourId), theme.controlBackground);
    EXPECT_EQ(laf.findColour(juce::TextEditor::textColourId), theme.textPrimary);
    EXPECT_EQ(laf.findColour(juce::TextEditor::outlineColourId), theme.controlBorder);
}

TEST(MultiScoperLookAndFeelTest, TextEditorHighlightUsesAccentAt30PercentAlpha)
{
    MultiScoperLookAndFeel laf;
    auto theme = makeCanonicalTheme();
    laf.applyTheme(theme);

    const auto accent = expectedAccent(theme);
    const auto highlight = laf.findColour(juce::TextEditor::highlightColourId);
    EXPECT_EQ(highlight.withAlpha(1.0f), accent);
    // Colour alpha is stored as uint8; 0.30 * 255 = 76.5 → rounded to 76,
    // giving 76/255 ≈ 0.2980.  Allow one byte of rounding tolerance.
    EXPECT_NEAR(highlight.getFloatAlpha(), 0.30f, 1.0f / 255.0f);
}

TEST(MultiScoperLookAndFeelTest, ComboBoxTokensMapToTheme)
{
    MultiScoperLookAndFeel laf;
    auto theme = makeCanonicalTheme();
    laf.applyTheme(theme);

    EXPECT_EQ(laf.findColour(juce::ComboBox::backgroundColourId), theme.controlBackground);
    EXPECT_EQ(laf.findColour(juce::ComboBox::textColourId), theme.textPrimary);
    EXPECT_EQ(laf.findColour(juce::ComboBox::outlineColourId), theme.controlBorder);
    EXPECT_EQ(laf.findColour(juce::ComboBox::arrowColourId), theme.textSecondary);
    EXPECT_EQ(laf.findColour(juce::ComboBox::focusedOutlineColourId), expectedAccent(theme));
}

TEST(MultiScoperLookAndFeelTest, PopupMenuTokensMapToTheme)
{
    MultiScoperLookAndFeel laf;
    auto theme = makeCanonicalTheme();
    laf.applyTheme(theme);

    EXPECT_EQ(laf.findColour(juce::PopupMenu::backgroundColourId), theme.backgroundPane);
    EXPECT_EQ(laf.findColour(juce::PopupMenu::textColourId), theme.textPrimary);
    EXPECT_EQ(laf.findColour(juce::PopupMenu::headerTextColourId), theme.textHighlight);
    EXPECT_EQ(laf.findColour(juce::PopupMenu::highlightedTextColourId), theme.textHighlight);

    const auto highlightBg = laf.findColour(juce::PopupMenu::highlightedBackgroundColourId);
    EXPECT_EQ(highlightBg.withAlpha(1.0f), expectedAccent(theme));
    EXPECT_NEAR(highlightBg.getFloatAlpha(), 0.20f, 1.0f / 255.0f);
}

TEST(MultiScoperLookAndFeelTest, AlertWindowUsesPaneBackground)
{
    MultiScoperLookAndFeel laf;
    auto theme = makeCanonicalTheme();
    laf.applyTheme(theme);

    EXPECT_EQ(laf.findColour(juce::AlertWindow::backgroundColourId), theme.backgroundPane);
    EXPECT_EQ(laf.findColour(juce::AlertWindow::textColourId), theme.textPrimary);
    EXPECT_EQ(laf.findColour(juce::AlertWindow::outlineColourId), theme.controlBorder);
}

TEST(MultiScoperLookAndFeelTest, TextButtonUsesAccentWhenOn)
{
    MultiScoperLookAndFeel laf;
    auto theme = makeCanonicalTheme();
    laf.applyTheme(theme);

    EXPECT_EQ(laf.findColour(juce::TextButton::buttonColourId), theme.controlBackground);
    EXPECT_EQ(laf.findColour(juce::TextButton::buttonOnColourId), expectedAccent(theme));
    // textColourOnId must stay contrast-safe against the accent fill — not
    // mapped to theme.textHighlight, which is BLACK in light themes and would
    // land black-on-dark-accent. See MultiScoperLookAndFeel.cpp:95-100.
    EXPECT_EQ(laf.findColour(juce::TextButton::textColourOnId), ColorTheme::pickContrastingText(expectedAccent(theme)));
    EXPECT_EQ(laf.findColour(juce::TextButton::textColourOffId), theme.textPrimary);
}

TEST(MultiScoperLookAndFeelTest, ToggleButtonTickUsesAccent)
{
    MultiScoperLookAndFeel laf;
    auto theme = makeCanonicalTheme();
    laf.applyTheme(theme);

    EXPECT_EQ(laf.findColour(juce::ToggleButton::textColourId), theme.textPrimary);
    EXPECT_EQ(laf.findColour(juce::ToggleButton::tickColourId), expectedAccent(theme));
    EXPECT_EQ(laf.findColour(juce::ToggleButton::tickDisabledColourId), theme.textSecondary);
}

TEST(MultiScoperLookAndFeelTest, TooltipWindowUsesSecondaryBackground)
{
    MultiScoperLookAndFeel laf;
    auto theme = makeCanonicalTheme();
    laf.applyTheme(theme);

    EXPECT_EQ(laf.findColour(juce::TooltipWindow::backgroundColourId), theme.backgroundSecondary);
    EXPECT_EQ(laf.findColour(juce::TooltipWindow::textColourId), theme.textPrimary);
    EXPECT_EQ(laf.findColour(juce::TooltipWindow::outlineColourId), theme.controlBorder);
}

TEST(MultiScoperLookAndFeelTest, ResizableWindowUsesPrimaryBackground)
{
    MultiScoperLookAndFeel laf;
    auto theme = makeCanonicalTheme();
    laf.applyTheme(theme);

    EXPECT_EQ(laf.findColour(juce::ResizableWindow::backgroundColourId), theme.backgroundPrimary);
    EXPECT_EQ(laf.findColour(juce::DocumentWindow::textColourId), theme.textPrimary);
}

TEST(MultiScoperLookAndFeelTest, ScrollBarTokensMapToTheme)
{
    MultiScoperLookAndFeel laf;
    auto theme = makeCanonicalTheme();
    laf.applyTheme(theme);

    EXPECT_EQ(laf.findColour(juce::ScrollBar::backgroundColourId), juce::Colours::transparentBlack);
    EXPECT_EQ(laf.findColour(juce::ScrollBar::thumbColourId), theme.controlBorder);
    EXPECT_EQ(laf.findColour(juce::ScrollBar::trackColourId), juce::Colours::transparentBlack);
}

TEST(MultiScoperLookAndFeelTest, ApplyThemeIsIdempotent)
{
    MultiScoperLookAndFeel laf;
    auto theme = makeCanonicalTheme();

    laf.applyTheme(theme);
    const auto firstAccent = laf.findColour(juce::TextEditor::focusedOutlineColourId);
    const auto firstText = laf.findColour(juce::Label::textColourId);

    laf.applyTheme(theme);
    const auto secondAccent = laf.findColour(juce::TextEditor::focusedOutlineColourId);
    const auto secondText = laf.findColour(juce::Label::textColourId);

    EXPECT_EQ(firstAccent, secondAccent);
    EXPECT_EQ(firstText, secondText);
}

TEST(MultiScoperLookAndFeelTest, DifferentThemesProduceDifferentAccents)
{
    MultiScoperLookAndFeel laf;

    auto themeA = makeCanonicalTheme();
    themeA.accentHue = 200.0f;
    laf.applyTheme(themeA);
    const auto accentA = laf.findColour(juce::TextEditor::focusedOutlineColourId);

    auto themeB = makeCanonicalTheme();
    themeB.accentHue = 30.0f;
    laf.applyTheme(themeB);
    const auto accentB = laf.findColour(juce::TextEditor::focusedOutlineColourId);

    EXPECT_NE(accentA, accentB);
}
