/*
    Oscil - Visual Configuration Implementation
*/

#include "rendering/VisualConfiguration.h"

#include <cmath>
#include <map>

namespace oscil
{

// toValueTree() and fromValueTree() are in VisualConfigurationSerialization.cpp

namespace
{

/// Replace NaN/Inf with a default, then clamp to [lo, hi].
/// juce::jlimit alone cannot catch NaN because NaN comparisons always return false.
float sanitize(float value, float lo, float hi, float fallback)
{
    return std::isfinite(value) ? juce::jlimit(lo, hi, value) : fallback;
}

void validateCoreEffects(VisualConfiguration& c)
{
    c.bloom.intensity = sanitize(c.bloom.intensity, 0.0f, 2.0f, 1.0f);
    c.bloom.threshold = sanitize(c.bloom.threshold, 0.0f, 1.0f, 0.8f);
    c.bloom.iterations = juce::jlimit(2, 8, c.bloom.iterations);
    c.bloom.downsampleSteps = juce::jlimit(2, 8, c.bloom.downsampleSteps);
    c.bloom.spread = sanitize(c.bloom.spread, 0.0f, 4.0f, 1.0f);
    c.bloom.softKnee = sanitize(c.bloom.softKnee, 0.0f, 1.0f, 0.5f);

    c.radialBlur.amount = sanitize(c.radialBlur.amount, 0.0f, 0.5f, 0.1f);
    c.radialBlur.glow = sanitize(c.radialBlur.glow, 0.0f, 4.0f, 1.0f);
    c.radialBlur.samples = juce::jlimit(2, 8, c.radialBlur.samples);

    c.trails.decay = sanitize(c.trails.decay, 0.01f, 0.5f, 0.1f);
    c.trails.opacity = sanitize(c.trails.opacity, 0.0f, 1.0f, 0.8f);

    c.colorGrade.brightness = sanitize(c.colorGrade.brightness, -1.0f, 1.0f, 0.0f);
    c.colorGrade.contrast = sanitize(c.colorGrade.contrast, 0.5f, 2.0f, 1.0f);
    c.colorGrade.saturation = sanitize(c.colorGrade.saturation, 0.0f, 2.0f, 1.0f);
    c.colorGrade.temperature = sanitize(c.colorGrade.temperature, -1.0f, 1.0f, 0.0f);
    c.colorGrade.tint = sanitize(c.colorGrade.tint, -1.0f, 1.0f, 0.0f);
}

void validateStyleEffects(VisualConfiguration& c)
{
    c.vignette.intensity = sanitize(c.vignette.intensity, 0.0f, 1.0f, 0.5f);
    c.vignette.softness = sanitize(c.vignette.softness, 0.0f, 1.0f, 0.5f);

    c.filmGrain.intensity = sanitize(c.filmGrain.intensity, 0.0f, 0.5f, 0.1f);
    c.filmGrain.speed = sanitize(c.filmGrain.speed, 1.0f, 60.0f, 24.0f);

    c.chromaticAberration.intensity = sanitize(c.chromaticAberration.intensity, 0.0f, 0.02f, 0.005f);

    c.scanlines.intensity = sanitize(c.scanlines.intensity, 0.0f, 1.0f, 0.3f);
    c.scanlines.density = sanitize(c.scanlines.density, 0.5f, 8.0f, 2.0f);

    c.tiltShift.position = sanitize(c.tiltShift.position, 0.0f, 1.0f, 0.5f);
    c.tiltShift.range = sanitize(c.tiltShift.range, 0.0f, 1.0f, 0.3f);
    c.tiltShift.blurRadius = sanitize(c.tiltShift.blurRadius, 0.0f, 10.0f, 2.0f);
    c.tiltShift.iterations = juce::jlimit(1, 8, c.tiltShift.iterations);
}

} // namespace

void VisualConfiguration::validate()
{
    validateCoreEffects(*this);
    validateStyleEffects(*this);
    compositeOpacity = sanitize(compositeOpacity, 0.0f, 1.0f, 1.0f);
}

bool VisualConfiguration::hasPostProcessing() const
{
    return bloom.enabled || radialBlur.enabled || tiltShift.enabled || trails.enabled || colorGrade.enabled ||
           vignette.enabled || filmGrain.enabled || chromaticAberration.enabled || scanlines.enabled;
}

VisualConfiguration VisualConfiguration::getDefault() { return VisualConfiguration(); }

void VisualConfiguration::setupVectorScopePreset(VisualConfiguration& c)
{
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

VisualConfiguration VisualConfiguration::getPreset(const juce::String& presetName)
{
    static const std::map<juce::String, VisualConfiguration> presets = []() {
        std::map<juce::String, VisualConfiguration> m;

        VisualConfiguration defaultConfig;
        defaultConfig.presetId = "default";
        defaultConfig.shaderType = ShaderType::Basic2D;
        m["default"] = defaultConfig;

        VisualConfiguration vectorScope;
        vectorScope.presetId = "vector_scope";
        setupVectorScopePreset(vectorScope);
        m["vector_scope"] = vectorScope;

        return m;
    }();

    auto it = presets.find(presetName);
    if (it != presets.end())
        return it->second;

    VisualConfiguration config;
    config.presetId = presetName;
    return config;
}

std::vector<std::pair<juce::String, juce::String>> VisualConfiguration::getAvailablePresets()
{
    return {{"default", "Default"}, {"vector_scope", "Vector Scope"}};
}

juce::String shaderTypeToId(ShaderType type)
{
    switch (type)
    {
        case ShaderType::Basic2D:
            return "basic";
        case ShaderType::NeonGlow:
            return "neon_glow";
        case ShaderType::GradientFill:
            return "gradient_fill";
        case ShaderType::DualOutline:
            return "dual_outline";
    }
    jassertfalse; // Unhandled ShaderType enum value
    return "basic";
}

ShaderType idToShaderType(const juce::String& id)
{
    if (id == "basic")
        return ShaderType::Basic2D;
    if (id == "neon_glow")
        return ShaderType::NeonGlow;
    if (id == "gradient_fill")
        return ShaderType::GradientFill;
    if (id == "dual_outline")
        return ShaderType::DualOutline;

    DBG("VisualConfiguration: unknown shader ID \"" + id + "\", falling back to Basic2D");
    return ShaderType::Basic2D;
}

} // namespace oscil
