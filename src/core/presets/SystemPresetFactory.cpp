#include "core/presets/SystemPresetFactory.h"
#include <juce_core/juce_core.h>

namespace oscil
{

// ============================================================================
// System Preset Validation
// ============================================================================

/**
 * Validates that a preset definition has valid required fields.
 */
static bool validatePresetDefinition(const SystemPresetFactory::PresetDefinition& def)
{
    if (def.id == nullptr || def.id[0] == '\0')
    {
        juce::Logger::writeToLog("SystemPresetFactory: Preset definition has empty ID");
        return false;
    }
    if (def.name == nullptr || def.name[0] == '\0')
    {
        juce::Logger::writeToLog(juce::String("SystemPresetFactory: Preset '") + def.id + "' has empty name");
        return false;
    }
    if (!def.setup)
    {
        juce::Logger::writeToLog(juce::String("SystemPresetFactory: Preset '") + def.id + "' has null setup function");
        return false;
    }
    return true;
}

/**
 * Tests that a preset's setup function executes without throwing.
 */
static bool testPresetSetup(const SystemPresetFactory::PresetDefinition& def)
{
    try
    {
        VisualConfiguration testConfig;
        def.setup(testConfig);
        return true;
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog(juce::String("SystemPresetFactory: Preset '") + def.id +
                                 "' setup function threw exception: " + e.what());
        return false;
    }
    catch (...)
    {
        juce::Logger::writeToLog(juce::String("SystemPresetFactory: Preset '") + def.id +
                                 "' setup function threw unknown exception");
        return false;
    }
}

std::vector<SystemPresetFactory::PresetDefinition> SystemPresetFactory::getSystemPresets()
{
    std::vector<PresetDefinition> allPresets = {
        {
            "default",
            "Default",
            "Clean waveform visualization with no effects",
            [](VisualConfiguration& c) {
                c.shaderType = ShaderType::Basic2D;
            }
        },
        {
            "vector_scope",
            "Vector Scope",
            "Classic oscilloscope look with scanlines and phosphor glow",
            [](VisualConfiguration& c) {
                c.shaderType = ShaderType::Basic2D;
                c.scanlines.enabled = true;
                c.scanlines.intensity = 0.4f;
                c.scanlines.density = 2.0f;
                c.scanlines.phosphorGlow = true;
                c.bloom.enabled = true;
                c.bloom.intensity = 1.2f;
                c.bloom.threshold = 0.1f;
                c.bloom.spread = 1.5f;
                c.filmGrain.enabled = true;
                c.filmGrain.intensity = 0.15f;
                c.colorGrade.enabled = true;
                c.colorGrade.tint = 0.1f;
                c.trails.enabled = true;
                c.trails.decay = 0.2f;
                c.trails.opacity = 0.6f;
            }
        },
        {
            "neon_glow",
            "Neon Glow",
            "Intense neon glow effect with additive blending",
            [](VisualConfiguration& c) {
                c.shaderType = ShaderType::NeonGlow;
                c.compositeBlendMode = BlendMode::Additive;
                c.bloom.enabled = true;
                c.bloom.intensity = 2.0f;
                c.bloom.threshold = 0.2f;
                c.bloom.spread = 1.5f;
            }
        },
        {
            "minimal",
            "Minimal",
            "Clean and simple visualization with reduced opacity",
            [](VisualConfiguration& c) {
                c.shaderType = ShaderType::Basic2D;
                c.compositeOpacity = 0.7f;
            }
        }
    };

    // Validate all presets and filter out invalid ones
    std::vector<PresetDefinition> validPresets;
    validPresets.reserve(allPresets.size());

    for (const auto& def : allPresets)
    {
        if (!validatePresetDefinition(def))
        {
            juce::Logger::writeToLog("SystemPresetFactory: Skipping invalid preset definition");
            continue;
        }

        if (!testPresetSetup(def))
        {
            juce::Logger::writeToLog(juce::String("SystemPresetFactory: Skipping preset '") + def.id +
                                     "' due to setup function failure");
            continue;
        }

        validPresets.push_back(def);
    }

    if (validPresets.size() != allPresets.size())
    {
        juce::Logger::writeToLog("SystemPresetFactory: " +
                                 juce::String(allPresets.size() - validPresets.size()) +
                                 " preset(s) failed validation");
    }

    return validPresets;
}

}
