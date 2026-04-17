/*
    Oscil - Typography Tokens
    Single source for UI font construction. Sizes are pixels at 1.0x DPI;
    JUCE handles HiDPI scaling automatically.
*/

#pragma once

#include <juce_graphics/juce_graphics.h>

namespace oscil
{

struct Typography
{
    // Canonical scale. Numbers align with legacy FONT_SIZE_* to avoid layout shifts.
    static constexpr float sizeCaption = 11.0f;
    static constexpr float sizeSmall = 12.0f;
    static constexpr float sizeBody = 13.0f;
    static constexpr float sizeHeading = 14.0f;
    static constexpr float sizeHeader = 16.0f;
    static constexpr float sizeDisplay = 20.0f;
    static constexpr float sizeReadout = 15.0f;

    // Tracked small-caps for section header chips; keeps small-size copy legible.
    static constexpr float kerningCaps = 0.08f;

    static juce::Font caption() { return juce::Font{juce::FontOptions(sizeCaption)}; }

    static juce::Font captionBold() { return juce::Font{juce::FontOptions(sizeCaption).withStyle("Bold")}; }

    static juce::Font captionCaps()
    {
        return juce::Font{juce::FontOptions(sizeCaption).withStyle("Bold")}.withExtraKerningFactor(kerningCaps);
    }

    static juce::Font small() { return juce::Font{juce::FontOptions(sizeSmall)}; }

    static juce::Font smallBold() { return juce::Font{juce::FontOptions(sizeSmall).withStyle("Bold")}; }

    static juce::Font body() { return juce::Font{juce::FontOptions(sizeBody)}; }

    static juce::Font bodyBold() { return juce::Font{juce::FontOptions(sizeBody).withStyle("Bold")}; }

    static juce::Font heading() { return juce::Font{juce::FontOptions(sizeHeading).withStyle("Bold")}; }

    // Legacy FONT_SIZE_DEFAULT was 14pt regular; keep a regular variant for delegation sites.
    static juce::Font headingRegular() { return juce::Font{juce::FontOptions(sizeHeading)}; }

    static juce::Font header() { return juce::Font{juce::FontOptions(sizeHeader).withStyle("Bold")}; }

    // Legacy FONT_SIZE_HEADER was 16pt regular; used for icon glyphs where bold is unwanted.
    static juce::Font headerRegular() { return juce::Font{juce::FontOptions(sizeHeader)}; }

    static juce::Font display() { return juce::Font{juce::FontOptions(sizeDisplay).withStyle("Bold")}; }

    static juce::Font readout()
    {
        return juce::Font{
            juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), sizeReadout, juce::Font::plain)};
    }

    static juce::Font readoutBold()
    {
        return juce::Font{juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), sizeReadout, juce::Font::bold)};
    }
};

} // namespace oscil
