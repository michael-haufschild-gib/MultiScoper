/*
    Oscil - Visual Configuration Implementation
*/

#include "rendering/VisualConfiguration.h"
#include <map>

namespace oscil
{

// ValueTree type identifiers
static const juce::Identifier VISUAL_CONFIG_TYPE("VisualConfiguration");
static const juce::Identifier BLOOM_TYPE("Bloom");
static const juce::Identifier TRAILS_TYPE("Trails");
static const juce::Identifier COLORGRADE_TYPE("ColorGrade");
static const juce::Identifier VIGNETTE_TYPE("Vignette");
static const juce::Identifier FILMGRAIN_TYPE("FilmGrain");
static const juce::Identifier CHROMATIC_TYPE("ChromaticAberration");
static const juce::Identifier SCANLINES_TYPE("Scanlines");
static const juce::Identifier DISTORTION_TYPE("Distortion");
static const juce::Identifier TILTSHIFT_TYPE("TiltShift");
static const juce::Identifier RADIALBLUR_TYPE("RadialBlur");
static const juce::Identifier PARTICLES_TYPE("Particles");
static const juce::Identifier SETTINGS3D_TYPE("Settings3D");
static const juce::Identifier MATERIAL_TYPE("Material");

// Force rebuild
juce::ValueTree VisualConfiguration::toValueTree() const
{
    juce::ValueTree tree(VISUAL_CONFIG_TYPE);

    // Shader type
    tree.setProperty("shaderType", shaderTypeToId(shaderType), nullptr);

    // Compositing
    tree.setProperty("compositeBlendMode", static_cast<int>(compositeBlendMode), nullptr);
    tree.setProperty("compositeOpacity", compositeOpacity, nullptr);

    // Bloom
    juce::ValueTree bloomTree(BLOOM_TYPE);
    bloomTree.setProperty("enabled", bloom.enabled, nullptr);
    bloomTree.setProperty("intensity", bloom.intensity, nullptr);
    bloomTree.setProperty("threshold", bloom.threshold, nullptr);
    bloomTree.setProperty("iterations", bloom.iterations, nullptr);
    bloomTree.setProperty("downsampleSteps", bloom.downsampleSteps, nullptr);
    bloomTree.setProperty("spread", bloom.spread, nullptr);
    bloomTree.setProperty("softKnee", bloom.softKnee, nullptr);
    tree.addChild(bloomTree, -1, nullptr);

    // Radial Blur
    juce::ValueTree radialBlurTree(RADIALBLUR_TYPE);
    radialBlurTree.setProperty("enabled", radialBlur.enabled, nullptr);
    radialBlurTree.setProperty("amount", radialBlur.amount, nullptr);
    radialBlurTree.setProperty("glow", radialBlur.glow, nullptr);
    radialBlurTree.setProperty("samples", radialBlur.samples, nullptr);
    tree.addChild(radialBlurTree, -1, nullptr);

    // Trails
    juce::ValueTree trailsTree(TRAILS_TYPE);
    trailsTree.setProperty("enabled", trails.enabled, nullptr);
    trailsTree.setProperty("decay", trails.decay, nullptr);
    trailsTree.setProperty("opacity", trails.opacity, nullptr);
    tree.addChild(trailsTree, -1, nullptr);

    // Color Grade
    juce::ValueTree colorGradeTree(COLORGRADE_TYPE);
    colorGradeTree.setProperty("enabled", colorGrade.enabled, nullptr);
    colorGradeTree.setProperty("brightness", colorGrade.brightness, nullptr);
    colorGradeTree.setProperty("contrast", colorGrade.contrast, nullptr);
    colorGradeTree.setProperty("saturation", colorGrade.saturation, nullptr);
    colorGradeTree.setProperty("temperature", colorGrade.temperature, nullptr);
    colorGradeTree.setProperty("tint", colorGrade.tint, nullptr);
    colorGradeTree.setProperty("shadows", static_cast<juce::int64>(colorGrade.shadows.getARGB()), nullptr);
    colorGradeTree.setProperty("highlights", static_cast<juce::int64>(colorGrade.highlights.getARGB()), nullptr);
    tree.addChild(colorGradeTree, -1, nullptr);

    // Vignette
    juce::ValueTree vignetteTree(VIGNETTE_TYPE);
    vignetteTree.setProperty("enabled", vignette.enabled, nullptr);
    vignetteTree.setProperty("intensity", vignette.intensity, nullptr);
    vignetteTree.setProperty("softness", vignette.softness, nullptr);
    vignetteTree.setProperty("colour", static_cast<juce::int64>(vignette.colour.getARGB()), nullptr);
    tree.addChild(vignetteTree, -1, nullptr);

    // Film Grain
    juce::ValueTree grainTree(FILMGRAIN_TYPE);
    grainTree.setProperty("enabled", filmGrain.enabled, nullptr);
    grainTree.setProperty("intensity", filmGrain.intensity, nullptr);
    grainTree.setProperty("speed", filmGrain.speed, nullptr);
    tree.addChild(grainTree, -1, nullptr);

    // Chromatic Aberration
    juce::ValueTree chromaticTree(CHROMATIC_TYPE);
    chromaticTree.setProperty("enabled", chromaticAberration.enabled, nullptr);
    chromaticTree.setProperty("intensity", chromaticAberration.intensity, nullptr);
    tree.addChild(chromaticTree, -1, nullptr);

    // Scanlines
    juce::ValueTree scanlineTree(SCANLINES_TYPE);
    scanlineTree.setProperty("enabled", scanlines.enabled, nullptr);
    scanlineTree.setProperty("intensity", scanlines.intensity, nullptr);
    scanlineTree.setProperty("density", scanlines.density, nullptr);
    scanlineTree.setProperty("phosphorGlow", scanlines.phosphorGlow, nullptr);
    tree.addChild(scanlineTree, -1, nullptr);

    // Distortion
    juce::ValueTree distortionTree(DISTORTION_TYPE);
    distortionTree.setProperty("enabled", distortion.enabled, nullptr);
    distortionTree.setProperty("intensity", distortion.intensity, nullptr);
    distortionTree.setProperty("frequency", distortion.frequency, nullptr);
    distortionTree.setProperty("speed", distortion.speed, nullptr);
    tree.addChild(distortionTree, -1, nullptr);

    // Tilt Shift
    juce::ValueTree tiltTree(TILTSHIFT_TYPE);
    tiltTree.setProperty("enabled", tiltShift.enabled, nullptr);
    tiltTree.setProperty("position", tiltShift.position, nullptr);
    tiltTree.setProperty("range", tiltShift.range, nullptr);
    tiltTree.setProperty("blurRadius", tiltShift.blurRadius, nullptr);
    tree.addChild(tiltTree, -1, nullptr);

    // Particles
    juce::ValueTree particlesTree(PARTICLES_TYPE);
    particlesTree.setProperty("enabled", particles.enabled, nullptr);
    particlesTree.setProperty("emissionMode", static_cast<int>(particles.emissionMode), nullptr);
    particlesTree.setProperty("emissionRate", particles.emissionRate, nullptr);
    particlesTree.setProperty("particleLife", particles.particleLife, nullptr);
    particlesTree.setProperty("particleSize", particles.particleSize, nullptr);
    particlesTree.setProperty("particleColor", static_cast<juce::int64>(particles.particleColor.getARGB()), nullptr);
    particlesTree.setProperty("blendMode", static_cast<int>(particles.blendMode), nullptr);
    particlesTree.setProperty("gravity", particles.gravity, nullptr);
    particlesTree.setProperty("drag", particles.drag, nullptr);
    particlesTree.setProperty("randomness", particles.randomness, nullptr);
    particlesTree.setProperty("velocityScale", particles.velocityScale, nullptr);
    particlesTree.setProperty("audioReactive", particles.audioReactive, nullptr);
    particlesTree.setProperty("audioEmissionBoost", particles.audioEmissionBoost, nullptr);
    particlesTree.setProperty("textureId", particles.textureId, nullptr);
    particlesTree.setProperty("textureRows", particles.textureRows, nullptr);
    particlesTree.setProperty("textureCols", particles.textureCols, nullptr);
    particlesTree.setProperty("softParticles", particles.softParticles, nullptr);
    particlesTree.setProperty("softDepthSensitivity", particles.softDepthSensitivity, nullptr);
    particlesTree.setProperty("useTurbulence", particles.useTurbulence, nullptr);
    particlesTree.setProperty("turbulenceStrength", particles.turbulenceStrength, nullptr);
    particlesTree.setProperty("turbulenceScale", particles.turbulenceScale, nullptr);
    particlesTree.setProperty("turbulenceSpeed", particles.turbulenceSpeed, nullptr);
    tree.addChild(particlesTree, -1, nullptr);

    // 3D Settings
    juce::ValueTree settings3DTree(SETTINGS3D_TYPE);
    settings3DTree.setProperty("enabled", settings3D.enabled, nullptr);
    settings3DTree.setProperty("cameraDistance", settings3D.cameraDistance, nullptr);
    settings3DTree.setProperty("cameraAngleX", settings3D.cameraAngleX, nullptr);
    settings3DTree.setProperty("cameraAngleY", settings3D.cameraAngleY, nullptr);
    settings3DTree.setProperty("autoRotate", settings3D.autoRotate, nullptr);
    settings3DTree.setProperty("rotateSpeed", settings3D.rotateSpeed, nullptr);
    settings3DTree.setProperty("meshResolutionX", settings3D.meshResolutionX, nullptr);
    settings3DTree.setProperty("meshResolutionZ", settings3D.meshResolutionZ, nullptr);
    settings3DTree.setProperty("meshScale", settings3D.meshScale, nullptr);
    tree.addChild(settings3DTree, -1, nullptr);

    // Material
    juce::ValueTree materialTree(MATERIAL_TYPE);
    materialTree.setProperty("enabled", material.enabled, nullptr);
    materialTree.setProperty("reflectivity", material.reflectivity, nullptr);
    materialTree.setProperty("refractiveIndex", material.refractiveIndex, nullptr);
    materialTree.setProperty("fresnelPower", material.fresnelPower, nullptr);
    materialTree.setProperty("tintColor", static_cast<juce::int64>(material.tintColor.getARGB()), nullptr);
    materialTree.setProperty("roughness", material.roughness, nullptr);
    materialTree.setProperty("useEnvironmentMap", material.useEnvironmentMap, nullptr);
    materialTree.setProperty("environmentMapId", material.environmentMapId, nullptr);
    tree.addChild(materialTree, -1, nullptr);

    return tree;
}

VisualConfiguration VisualConfiguration::fromValueTree(const juce::ValueTree& tree)
{
    VisualConfiguration config;

    if (!tree.hasType(VISUAL_CONFIG_TYPE))
        return config;

    // Shader type
    config.shaderType = idToShaderType(tree.getProperty("shaderType", "basic"));

    // Compositing
    config.compositeBlendMode = static_cast<BlendMode>(
        static_cast<int>(tree.getProperty("compositeBlendMode", 0)));
    config.compositeOpacity = tree.getProperty("compositeOpacity", 1.0f);

    // Bloom
    auto bloomTree = tree.getChildWithName(BLOOM_TYPE);
    if (bloomTree.isValid())
    {
        config.bloom.enabled = bloomTree.getProperty("enabled", false);
        config.bloom.intensity = bloomTree.getProperty("intensity", 1.0f);
        config.bloom.threshold = bloomTree.getProperty("threshold", 0.8f);
        config.bloom.iterations = bloomTree.getProperty("iterations", 4);
        config.bloom.downsampleSteps = bloomTree.getProperty("downsampleSteps", 6);
        config.bloom.spread = bloomTree.getProperty("spread", 1.0f);
        config.bloom.softKnee = bloomTree.getProperty("softKnee", 0.5f);
    }

    // Radial Blur
    auto radialBlurTree = tree.getChildWithName(RADIALBLUR_TYPE);
    if (radialBlurTree.isValid())
    {
        config.radialBlur.enabled = radialBlurTree.getProperty("enabled", false);
        config.radialBlur.amount = radialBlurTree.getProperty("amount", 0.1f);
        config.radialBlur.glow = radialBlurTree.getProperty("glow", 1.0f);
        config.radialBlur.samples = radialBlurTree.getProperty("samples", 4);
    }

    // Trails
    auto trailsTree = tree.getChildWithName(TRAILS_TYPE);
    if (trailsTree.isValid())
    {
        config.trails.enabled = trailsTree.getProperty("enabled", false);
        config.trails.decay = trailsTree.getProperty("decay", 0.1f);
        config.trails.opacity = trailsTree.getProperty("opacity", 0.8f);
    }

    // Color Grade
    auto colorGradeTree = tree.getChildWithName(COLORGRADE_TYPE);
    if (colorGradeTree.isValid())
    {
        config.colorGrade.enabled = colorGradeTree.getProperty("enabled", false);
        config.colorGrade.brightness = colorGradeTree.getProperty("brightness", 0.0f);
        config.colorGrade.contrast = colorGradeTree.getProperty("contrast", 1.0f);
        config.colorGrade.saturation = colorGradeTree.getProperty("saturation", 1.0f);
        config.colorGrade.temperature = colorGradeTree.getProperty("temperature", 0.0f);
        config.colorGrade.tint = colorGradeTree.getProperty("tint", 0.0f);
        config.colorGrade.shadows = juce::Colour(static_cast<juce::uint32>(
            static_cast<juce::int64>(colorGradeTree.getProperty("shadows", static_cast<juce::int64>(0xFF000000)))));
        config.colorGrade.highlights = juce::Colour(static_cast<juce::uint32>(
            static_cast<juce::int64>(colorGradeTree.getProperty("highlights", static_cast<juce::int64>(0xFFFFFFFF)))));
    }

    // Vignette
    auto vignetteTree = tree.getChildWithName(VIGNETTE_TYPE);
    if (vignetteTree.isValid())
    {
        config.vignette.enabled = vignetteTree.getProperty("enabled", false);
        config.vignette.intensity = vignetteTree.getProperty("intensity", 0.5f);
        config.vignette.softness = vignetteTree.getProperty("softness", 0.5f);
        config.vignette.colour = juce::Colour(static_cast<juce::uint32>(
            static_cast<juce::int64>(vignetteTree.getProperty("colour", static_cast<juce::int64>(0xFF000000)))));
    }

    // Film Grain
    auto grainTree = tree.getChildWithName(FILMGRAIN_TYPE);
    if (grainTree.isValid())
    {
        config.filmGrain.enabled = grainTree.getProperty("enabled", false);
        config.filmGrain.intensity = grainTree.getProperty("intensity", 0.1f);
        config.filmGrain.speed = grainTree.getProperty("speed", 24.0f);
    }

    // Chromatic Aberration
    auto chromaticTree = tree.getChildWithName(CHROMATIC_TYPE);
    if (chromaticTree.isValid())
    {
        config.chromaticAberration.enabled = chromaticTree.getProperty("enabled", false);
        config.chromaticAberration.intensity = chromaticTree.getProperty("intensity", 0.005f);
    }

    // Scanlines
    auto scanlineTree = tree.getChildWithName(SCANLINES_TYPE);
    if (scanlineTree.isValid())
    {
        config.scanlines.enabled = scanlineTree.getProperty("enabled", false);
        config.scanlines.intensity = scanlineTree.getProperty("intensity", 0.3f);
        config.scanlines.density = scanlineTree.getProperty("density", 2.0f);
        config.scanlines.phosphorGlow = scanlineTree.getProperty("phosphorGlow", true);
    }

    // Distortion
    auto distortionTree = tree.getChildWithName(DISTORTION_TYPE);
    if (distortionTree.isValid())
    {
        config.distortion.enabled = distortionTree.getProperty("enabled", false);
        config.distortion.intensity = distortionTree.getProperty("intensity", 0.0f);
        config.distortion.frequency = distortionTree.getProperty("frequency", 4.0f);
        config.distortion.speed = distortionTree.getProperty("speed", 1.0f);
    }

    // Tilt Shift
    auto tiltTree = tree.getChildWithName(TILTSHIFT_TYPE);
    if (tiltTree.isValid())
    {
        config.tiltShift.enabled = tiltTree.getProperty("enabled", false);
        config.tiltShift.position = tiltTree.getProperty("position", 0.5f);
        config.tiltShift.range = tiltTree.getProperty("range", 0.3f);
        config.tiltShift.blurRadius = tiltTree.getProperty("blurRadius", 2.0f);
    }

    // Particles
    auto particlesTree = tree.getChildWithName(PARTICLES_TYPE);
    if (particlesTree.isValid())
    {
        config.particles.enabled = particlesTree.getProperty("enabled", false);
        config.particles.emissionMode = static_cast<ParticleEmissionMode>(
            static_cast<int>(particlesTree.getProperty("emissionMode", 0)));
        config.particles.emissionRate = particlesTree.getProperty("emissionRate", 100.0f);
        config.particles.particleLife = particlesTree.getProperty("particleLife", 2.0f);
        config.particles.particleSize = particlesTree.getProperty("particleSize", 4.0f);
        config.particles.particleColor = juce::Colour(static_cast<juce::uint32>(
            static_cast<juce::int64>(particlesTree.getProperty("particleColor", static_cast<juce::int64>(0xFFFFAA00)))));
        config.particles.blendMode = static_cast<ParticleBlendMode>(
            static_cast<int>(particlesTree.getProperty("blendMode", 0)));
        config.particles.gravity = particlesTree.getProperty("gravity", 0.0f);
        config.particles.drag = particlesTree.getProperty("drag", 0.1f);
        config.particles.randomness = particlesTree.getProperty("randomness", 0.5f);
        config.particles.velocityScale = particlesTree.getProperty("velocityScale", 1.0f);
        config.particles.audioReactive = particlesTree.getProperty("audioReactive", true);
        config.particles.audioEmissionBoost = particlesTree.getProperty("audioEmissionBoost", 2.0f);
        config.particles.textureId = particlesTree.getProperty("textureId", "").toString();
        config.particles.textureRows = particlesTree.getProperty("textureRows", 1);
        config.particles.textureCols = particlesTree.getProperty("textureCols", 1);
        config.particles.softParticles = particlesTree.getProperty("softParticles", false);
        config.particles.softDepthSensitivity = particlesTree.getProperty("softDepthSensitivity", 1.0f);
        config.particles.useTurbulence = particlesTree.getProperty("useTurbulence", false);
        config.particles.turbulenceStrength = particlesTree.getProperty("turbulenceStrength", 0.0f);
        config.particles.turbulenceScale = particlesTree.getProperty("turbulenceScale", 0.5f);
        config.particles.turbulenceSpeed = particlesTree.getProperty("turbulenceSpeed", 0.5f);
    }

    // 3D Settings
    auto settings3DTree = tree.getChildWithName(SETTINGS3D_TYPE);
    if (settings3DTree.isValid())
    {
        config.settings3D.enabled = settings3DTree.getProperty("enabled", false);
        config.settings3D.cameraDistance = settings3DTree.getProperty("cameraDistance", 5.0f);
        config.settings3D.cameraAngleX = settings3DTree.getProperty("cameraAngleX", 15.0f);
        config.settings3D.cameraAngleY = settings3DTree.getProperty("cameraAngleY", 0.0f);
        config.settings3D.autoRotate = settings3DTree.getProperty("autoRotate", false);
        config.settings3D.rotateSpeed = settings3DTree.getProperty("rotateSpeed", 10.0f);
        config.settings3D.meshResolutionX = settings3DTree.getProperty("meshResolutionX", 128);
        config.settings3D.meshResolutionZ = settings3DTree.getProperty("meshResolutionZ", 32);
        config.settings3D.meshScale = settings3DTree.getProperty("meshScale", 1.0f);
    }

    // Material
    auto materialTree = tree.getChildWithName(MATERIAL_TYPE);
    if (materialTree.isValid())
    {
        config.material.enabled = materialTree.getProperty("enabled", false);
        config.material.reflectivity = materialTree.getProperty("reflectivity", 0.5f);
        config.material.refractiveIndex = materialTree.getProperty("refractiveIndex", 1.5f);
        config.material.fresnelPower = materialTree.getProperty("fresnelPower", 2.0f);
        config.material.tintColor = juce::Colour(static_cast<juce::uint32>(
            static_cast<juce::int64>(materialTree.getProperty("tintColor", static_cast<juce::int64>(0xFFFFFFFF)))));
        config.material.roughness = materialTree.getProperty("roughness", 0.1f);
        config.material.useEnvironmentMap = materialTree.getProperty("useEnvironmentMap", true);
        config.material.environmentMapId = materialTree.getProperty("environmentMapId", "default_studio").toString();
    }

    return config;
}

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
