#pragma once
#include <juce_core/juce_core.h>
#include "plugin/PluginEditor.h"
#include <nlohmann/json.hpp>

namespace oscil
{

class BaseTestRunner
{
public:
    explicit BaseTestRunner(OscilPluginEditor& editor) : editor_(editor) {}
    virtual ~BaseTestRunner() = default;

    virtual nlohmann::json runTests() = 0;

protected:
    OscilPluginEditor& editor_;
};

}
