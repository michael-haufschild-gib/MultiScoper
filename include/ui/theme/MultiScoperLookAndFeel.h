/*
    MultiScoper - Application-wide JUCE LookAndFeel

    Maps JUCE's built-in widget colour IDs (Label, ListBox, TextEditor,
    ComboBox, PopupMenu, ScrollBar, AlertWindow, TextButton) to the active
    ColorTheme tokens so raw JUCE widgets — which would otherwise render with
    LookAndFeel_V4 defaults and leak light/native chrome into dark themes —
    adopt MultiScoper's theme without per-widget overrides.
*/

#pragma once

#include "ui/theme/ColorTheme.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace multiscoper
{

class MultiScoperLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MultiScoperLookAndFeel();
    ~MultiScoperLookAndFeel() override = default;

    /** Refresh every colour ID against the supplied theme. Safe to call from
        the message thread whenever the active theme changes. */
    void applyTheme(const ColorTheme& theme);

    /** Resolve every JUCE font request to the bundled PT Root UI typeface,
        picking Light / Regular / Medium / Bold by the font's style tag so the
        whole UI renders with a consistent branded sans-serif. */
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& font) override;

private:
    juce::Typeface::Ptr regularTypeface_;
    juce::Typeface::Ptr mediumTypeface_;
    juce::Typeface::Ptr boldTypeface_;
    juce::Typeface::Ptr lightTypeface_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiScoperLookAndFeel)
};

} // namespace multiscoper
