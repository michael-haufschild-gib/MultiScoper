#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <functional>
#include "rendering/VisualConfiguration.h"

namespace oscil
{

class SystemPresetFactory
{
public:
    struct PresetDefinition
    {
        const char* id;
        const char* name;
        const char* description;
        std::function<void(VisualConfiguration&)> setup;
    };

    static std::vector<PresetDefinition> getSystemPresets();
};

}
