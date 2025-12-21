/*
    Oscil - Tabs Painter
    Handles rendering logic for OscilTabs
*/

#pragma once

#include <juce_graphics/juce_graphics.h>

namespace oscil
{

class OscilTabs;

class TabsPainter
{
public:
    static void paint(juce::Graphics& g, OscilTabs& tabs);

private:
    static void paintTab(juce::Graphics& g, OscilTabs& tabs, int index, juce::Rectangle<int> bounds);
    static void paintIndicator(juce::Graphics& g, OscilTabs& tabs);
    static void paintBadge(juce::Graphics& g, OscilTabs& tabs, juce::Rectangle<int> bounds, int count);
};

} // namespace oscil
