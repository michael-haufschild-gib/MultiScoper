/*
    Oscil - Waveform Color Palette
    Centralized predefined colors for waveform oscillators
*/

#pragma once

#include <juce_graphics/juce_graphics.h>

#include <array>
#include <random>

namespace oscil
{

/**
 * Centralized color palette for waveform oscillators.
 * These are the predefined colors shown in the color picker and used for new oscillators.
 */
struct WaveformColorPalette
{
    static constexpr size_t NUM_COLORS = 12;

    // Predefined synthwave/retro color palette
    static constexpr std::array<juce::uint32, NUM_COLORS> colors = {{
        0xFFFF355E, // Radical Red
        0xFFFF6037, // Outrun Orange
        0xFFFFCC00, // VHS Gold
        0xFFCCFF00, // Mutant Green
        0xFF39FF14, // Arcade Green
        0xFF00FF99, // Turbo Teal
        0xFF40E0D0, // Poolside Aqua
        0xFF00FFFF, // Miami Cyan
        0xFF1E90FF, // Dodger Blue
        0xFF3333FF, // Video Blue
        0xFF9933FF, // Electric Violet
        0xFFFF00CC  // Hot Magenta
    }};

    /**
     * Get a color by index (wraps around if out of bounds)
     */
    static juce::Colour getColor(size_t index) { return juce::Colour(colors[index % NUM_COLORS]); }

    /**
     * Get all colors as a vector (useful for UI components)
     */
    static std::vector<juce::Colour> getAllColors()
    {
        std::vector<juce::Colour> result;
        result.reserve(NUM_COLORS);
        for (auto c : colors)
        {
            result.emplace_back(c);
        }
        return result;
    }

    /**
     * Get a random color from the palette.
     * Thread-safe: uses thread_local PRNG to avoid data races.
     */
    static juce::Colour getRandomColor()
    {
        thread_local std::mt19937 gen(std::random_device{}());
        thread_local std::uniform_int_distribution<size_t> dist(0, NUM_COLORS - 1);
        return getColor(dist(gen));
    }

    /**
     * Get the index of a color in the palette, or -1 if not found
     */
    static int getColorIndex(juce::Colour colour)
    {
        juce::uint32 argb = colour.getARGB();
        for (size_t i = 0; i < NUM_COLORS; ++i)
        {
            if (colors[i] == argb)
                return static_cast<int>(i);
        }
        return -1;
    }
};

} // namespace oscil
