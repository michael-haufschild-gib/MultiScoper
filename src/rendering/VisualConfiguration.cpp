/*
    Oscil - Visual Configuration Implementation
*/

#include "rendering/VisualConfiguration.h"
#include <map>

namespace oscil
{

// toValueTree() and fromValueTree() are in VisualConfigurationSerialization.cpp


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

bool VisualConfiguration::hasParticles() const
{
    return particles.enabled;
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

        // 3. String Theory (3D/Volumetric)
        addPreset("string_theory", [](VisualConfiguration& c) {
            c.shaderType = ShaderType::StringTheory;
            c.settings3D.enabled = true;
            c.settings3D.cameraDistance = 4.5f;
            c.settings3D.cameraAngleX = 15.0f;
            c.settings3D.autoRotate = true;
            c.settings3D.rotateSpeed = 5.0f;
            c.bloom.enabled = true;
            c.bloom.intensity = 1.5f;
            c.particles.enabled = true;
            c.particles.textureId = "sparkle";
            c.particles.emissionMode = ParticleEmissionMode::AlongWaveform;
            c.particles.emissionRate = 50.0f;
            c.particles.particleSize = 2.0f;
            c.particles.particleLife = 1.0f;
            c.particles.velocityScale = 0.5f;
            c.particles.blendMode = ParticleBlendMode::Additive;
        });

        // 4. Crystalline (Material)
        addPreset("crystalline", [](VisualConfiguration& c) {
            c.shaderType = ShaderType::Crystalline;
            c.settings3D.enabled = true;
            c.settings3D.cameraDistance = 4.0f;
            c.settings3D.autoRotate = true;
            c.settings3D.rotateSpeed = 4.0f;
            c.material.enabled = true;
            c.material.refractiveIndex = 2.4f; // Diamond
            c.material.reflectivity = 0.8f;
            c.material.roughness = 0.0f;
            c.material.useEnvironmentMap = true;
            c.material.environmentMapId = "sunset";
            c.bloom.enabled = true;
            c.bloom.intensity = 1.8f; // Sparkle
            c.particles.enabled = true;
            c.particles.textureId = "sparkle";
            c.particles.emissionMode = ParticleEmissionMode::AtPeaks;
            c.particles.emissionRate = 20.0f;
            c.particles.particleSize = 5.0f;
            c.particles.particleLife = 0.5f;
            c.particles.blendMode = ParticleBlendMode::Additive;
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
        {"vector_scope", "Vector Scope"},
        {"string_theory", "String Theory"},
        {"crystalline", "Crystalline"}
    };
}

void VisualConfiguration::applyOverrides(VisualConfiguration& config, const juce::ValueTree& overrides)
{
    if (overrides.hasProperty("particlesEnabled"))
        config.particles.enabled = overrides.getProperty("particlesEnabled");
    
    if (overrides.hasProperty("particleTextureId"))
        config.particles.textureId = overrides.getProperty("particleTextureId").toString();
        
    if (overrides.hasProperty("particleTurbulenceStrength"))
        config.particles.turbulenceStrength = overrides.getProperty("particleTurbulenceStrength");
    
    if (overrides.hasProperty("particleTurbulenceScale"))
        config.particles.turbulenceScale = overrides.getProperty("particleTurbulenceScale");
        
    if (overrides.hasProperty("particleTurbulenceSpeed"))
        config.particles.turbulenceSpeed = overrides.getProperty("particleTurbulenceSpeed");
        
    if (overrides.hasProperty("particleUseTurbulence"))
        config.particles.useTurbulence = overrides.getProperty("particleUseTurbulence");
        
    if (overrides.hasProperty("particleSoftness"))
    {
        config.particles.softParticles = true;
        config.particles.softDepthSensitivity = overrides.getProperty("particleSoftness");
    }
}

juce::String shaderTypeToId(ShaderType type)
{
    switch (type)
    {
        case ShaderType::Basic2D:         return "basic";
        case ShaderType::NeonGlow:        return "neon_glow";
        case ShaderType::GradientFill:    return "gradient_fill";
        case ShaderType::DualOutline:     return "dual_outline";
        case ShaderType::PlasmaSine:      return "plasma_sine";
        case ShaderType::DigitalGlitch:   return "digital_glitch";
        case ShaderType::VolumetricRibbon: return "volumetric_ribbon";
        case ShaderType::WireframeMesh:   return "wireframe_mesh";
        case ShaderType::VectorFlow:      return "vector_flow";
        case ShaderType::StringTheory:    return "string_theory";
        case ShaderType::ElectricFlower:  return "electric_flower";
        case ShaderType::ElectricFiligree:return "electric_filigree";
        case ShaderType::GlassRefraction: return "glass_refraction";
        case ShaderType::LiquidChrome:    return "liquid_chrome";
        case ShaderType::Crystalline:     return "crystalline";
        default:                          return "basic";
    }
}

ShaderType idToShaderType(const juce::String& id)
{
    if (id == "basic")             return ShaderType::Basic2D;
    if (id == "neon_glow")         return ShaderType::NeonGlow;
    if (id == "gradient_fill")     return ShaderType::GradientFill;
    if (id == "dual_outline")      return ShaderType::DualOutline;
    if (id == "plasma_sine")       return ShaderType::PlasmaSine;
    if (id == "digital_glitch")    return ShaderType::DigitalGlitch;
    if (id == "volumetric_ribbon") return ShaderType::VolumetricRibbon;
    if (id == "wireframe_mesh")    return ShaderType::WireframeMesh;
    if (id == "vector_flow")       return ShaderType::VectorFlow;
    if (id == "string_theory")     return ShaderType::StringTheory;
    if (id == "electric_flower")   return ShaderType::ElectricFlower;
    if (id == "electric_filigree") return ShaderType::ElectricFiligree;
    if (id == "glass_refraction")  return ShaderType::GlassRefraction;
    if (id == "liquid_chrome")     return ShaderType::LiquidChrome;
    if (id == "crystalline")       return ShaderType::Crystalline;
    return ShaderType::Basic2D;
}

} // namespace oscil
