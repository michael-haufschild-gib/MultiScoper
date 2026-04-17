/*
    Oscil - Application-wide JUCE LookAndFeel

    Maps JUCE's built-in widget colour IDs (Label, ListBox, TextEditor,
    ComboBox, PopupMenu, ScrollBar, AlertWindow, TextButton) to the active
    ColorTheme tokens so raw JUCE widgets — which would otherwise render with
    LookAndFeel_V4 defaults and leak light/native chrome into dark themes —
    adopt Oscil's theme without per-widget overrides.
*/

#pragma once

#include "ui/theme/ColorTheme.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace oscil
{

class OscilLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OscilLookAndFeel();
    ~OscilLookAndFeel() override = default;

    /** Refresh every colour ID against the supplied theme. Safe to call from
        the message thread whenever the active theme changes. */
    void applyTheme(const ColorTheme& theme);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilLookAndFeel)
};

} // namespace oscil
