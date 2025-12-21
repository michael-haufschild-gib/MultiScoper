#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace oscil
{

/**
 * Dropdown item with optional icon and description
 */
struct DropdownItem
{
    juce::String id;
    juce::String label;
    juce::String description;
    juce::Image icon;
    bool enabled = true;
    bool isSeparator = false;

    static DropdownItem separator()
    {
        DropdownItem item;
        item.isSeparator = true;
        return item;
    }
};

}
