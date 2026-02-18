/*
    Oscil - Visual Configuration Implementation
*/

#include "rendering/VisualConfiguration.h"
#include <map>

namespace oscil
{

// Note: toValueTree(), fromValueTree(), toJson(), and fromJson() are defined in
// serialization/VisualConfigurationSerialization.cpp to avoid ODR violations

bool VisualConfiguration::requires3D() const
{
    return settings3D.enabled || is3DShader(shaderType);
}

bool VisualConfiguration::hasPostProcessing() const
{
    return bloom.enabled ||
           radialBlur.enabled ||
           tiltShift.enabled ||
           trails.enabled ||
           colorGrade.enabled ||
           vignette.enabled ||
           filmGrain.enabled ||
           chromaticAberration.enabled ||
           scanlines.enabled ||
           distortion.enabled;
}

VisualConfiguration VisualConfiguration::getDefault()
{
    return VisualConfiguration();
}

VisualConfiguration VisualConfiguration::getPreset(const juce::String& presetName)
{
    static const std::map<juce::String, VisualConfiguration> presets = []() {
        std::map<juce::String, VisualConfiguration> m;

        auto addPreset = [&](const juce::String& id, std::function<void(VisualConfiguration&)> setup) {
            VisualConfiguration c;
            c.presetId = id;
            setup(c);
            m[id] = c;
        };

        // 1. Default (Utility)
        addPreset("default", [](VisualConfiguration& c) {
            c.shaderType = ShaderType::Basic2D;
        });

        // 2. Vector Scope (Utility/Retro)
        addPreset("vector_scope", [](VisualConfiguration& c) {
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
            c.colorGrade.tint = 0.1f; // Slight green tint usually handled by color picker, but config helps
            c.trails.enabled = true;
            c.trails.decay = 0.2f;
            c.trails.opacity = 0.6f;
        });

        return m;
    }();

    auto it = presets.find(presetName);
    if (it != presets.end())
        return it->second;

    // Fallback for unknown presets
    VisualConfiguration config;
    config.presetId = presetName;
    return config;
}

std::vector<std::pair<juce::String, juce::String>> VisualConfiguration::getAvailablePresets()
{
    // Returns pairs of (id, displayName)
    return {
        {"default", "Default"},
        {"vector_scope", "Vector Scope"}
    };
}

void VisualConfiguration::applyOverrides([[maybe_unused]] VisualConfiguration& config, [[maybe_unused]] const juce::ValueTree& overrides)
{
    // This method is kept for future extensibility
}

juce::String shaderTypeToId(ShaderType type)
{
    switch (type)
    {
        case ShaderType::Basic2D:         return "basic";
        case ShaderType::NeonGlow:        return "neon_glow";
        case ShaderType::GradientFill:    return "gradient_fill";
        default:                          return "basic";
    }
}

ShaderType idToShaderType(const juce::String& id)
{
    if (id == "basic")             return ShaderType::Basic2D;
    if (id == "neon_glow")         return ShaderType::NeonGlow;
    if (id == "gradient_fill")     return ShaderType::GradientFill;
    return ShaderType::Basic2D;
}

} // namespace oscil
