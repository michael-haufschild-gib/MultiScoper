/*
    Oscil - Visual Configuration Implementation
*/

#include "rendering/VisualConfiguration.h"

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
static const juce::Identifier RADIALBLUR_TYPE("RadialBlur");
static const juce::Identifier PARTICLES_TYPE("Particles");
static const juce::Identifier SETTINGS3D_TYPE("Settings3D");
static const juce::Identifier MATERIAL_TYPE("Material");

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
    VisualConfiguration config;
    config.presetId = presetName;

    // ==========================================================================
    // Clean Baseline
    // ==========================================================================
    if (presetName == "default")
    {
        config.shaderType = ShaderType::Basic2D;
        // All effects disabled by default for a clean, technical look
    }

    // ==========================================================================
    // High-Impact Visual Presets
    // ==========================================================================

    else if (presetName == "neon")
    {
        // Pure 80s Neon aesthetic
        config.shaderType = ShaderType::NeonGlow;

        // Bloom enhances the shader's internal glow
        config.bloom.enabled = true;
        config.bloom.intensity = 1.5f;
        config.bloom.threshold = 0.1f; // Low threshold to pick up the colored halo
        config.bloom.spread = 1.5f;
        config.bloom.iterations = 4;

        // Subtle vignette for atmosphere
        config.vignette.enabled = true;
        config.vignette.intensity = 0.3f;
        config.vignette.softness = 0.6f;

        // Punchy colors
        config.colorGrade.enabled = true;
        config.colorGrade.contrast = 1.1f;
        config.colorGrade.saturation = 1.2f;

        // Slight trails for persistence
        config.trails.enabled = true;
        config.trails.decay = 0.1f;
        config.trails.opacity = 0.6f;
    }
    else if (presetName == "cyberpunk")
    {
        // Aggressive neon aesthetic with high contrast and digital artifacts
        config.shaderType = ShaderType::NeonGlow;
        
        // Post-Processing
        config.bloom.enabled = true;
        config.bloom.intensity = 1.8f;
        config.bloom.threshold = 0.4f;
        config.bloom.spread = 1.2f;
        
        config.chromaticAberration.enabled = true;
        config.chromaticAberration.intensity = 0.015f;
        
        config.scanlines.enabled = true;
        config.scanlines.intensity = 0.5f;
        config.scanlines.density = 1.5f;
        config.scanlines.phosphorGlow = true;
        
        config.filmGrain.enabled = true;
        config.filmGrain.intensity = 0.15f;

        // Particles: Digital sparks
        config.particles.enabled = true;
        config.particles.emissionMode = ParticleEmissionMode::AlongWaveform;
        config.particles.emissionRate = 120.0f;
        config.particles.particleLife = 1.0f;
        config.particles.particleSize = 5.0f;
        config.particles.particleColor = juce::Colour(0xFF00FFFF); // Cyan
        config.particles.blendMode = ParticleBlendMode::Additive;
        config.particles.velocityScale = 1.5f;
        config.particles.audioReactive = true;
    }
    else if (presetName == "liquid_gold")
    {
        // Luxurious, physically-based liquid metal
        config.shaderType = ShaderType::LiquidChrome;
        
        // 3D Setup
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 3.8f;
        config.settings3D.cameraAngleX = 25.0f;
        config.settings3D.autoRotate = true;
        config.settings3D.rotateSpeed = 5.0f; // Slow, heavy rotation
        
        // Material: Gold
        config.material.enabled = true;
        config.material.reflectivity = 1.0f;
        config.material.roughness = 0.15f;
        config.material.refractiveIndex = 0.2f; // Metallic
        config.material.tintColor = juce::Colour(0xFFFFD700); // Gold
        config.material.useEnvironmentMap = true;
        config.material.environmentMapId = "sunset"; // Warm lighting
        
        // Post
        config.bloom.enabled = true;
        config.bloom.intensity = 1.2f; // Golden glow
        config.bloom.threshold = 0.7f;
        
        config.colorGrade.enabled = true;
        config.colorGrade.temperature = 0.2f; // Enhance warmth
        config.colorGrade.contrast = 1.1f;
    }
    else if (presetName == "deep_ocean")
    {
        // Bioluminescent underwater organic look
        config.shaderType = ShaderType::VolumetricRibbon;
        
        // 3D Setup
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 5.0f;
        config.settings3D.autoRotate = true;
        config.settings3D.rotateSpeed = 8.0f;
        
        // Post
        config.distortion.enabled = true; // Water wobble
        config.distortion.intensity = 0.015f;
        config.distortion.speed = 0.5f;
        config.distortion.frequency = 3.0f;
        
        config.bloom.enabled = true;
        config.bloom.intensity = 1.5f; // Bio-glow
        config.bloom.threshold = 0.3f;
        config.bloom.spread = 2.0f; // Wide, soft glow
        
        config.colorGrade.enabled = true;
        config.colorGrade.tint = -0.1f; // Shift towards green/teal
        config.colorGrade.shadows = juce::Colour(0xFF001133); // Deep blue shadows
        
        // Particles: Rising bubbles
        config.particles.enabled = false;
        config.particles.emissionMode = ParticleEmissionMode::AlongWaveform;
        config.particles.emissionRate = 80.0f;
        config.particles.particleLife = 3.0f;
        config.particles.particleSize = 3.0f;
        config.particles.particleColor = juce::Colour(0xFF00FFFF); // Cyan/White
        config.particles.gravity = -10.0f; // Float up
        config.particles.drag = 0.5f; // Water resistance
    }
    else if (presetName == "synthwave")
    {
        // Retro-futuristic 80s grid aesthetic with heavy neon glow
        config.shaderType = ShaderType::WireframeMesh;

        // 3D Setup
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 6.0f;
        config.settings3D.cameraAngleX = 30.0f;
        config.settings3D.autoRotate = true;
        config.settings3D.rotateSpeed = 12.0f;
        config.settings3D.meshResolutionX = 64;
        config.settings3D.meshResolutionZ = 24;

        // Post-processing: Heavy 80s synthwave glow
        config.bloom.enabled = true;
        config.bloom.intensity = 3.5f;      // Strong neon glow
        config.bloom.threshold = 0.2f;      // Low threshold for more glow
        config.bloom.spread = 2.5f;         // Wide, soft glow spread

        config.chromaticAberration.enabled = true;
        config.chromaticAberration.intensity = 0.008f; // Subtle RGB separation

        config.scanlines.enabled = true;
        config.scanlines.intensity = 0.25f;

        config.vignette.enabled = true;
        config.vignette.intensity = 0.7f;   // Darker corners for focus

        config.colorGrade.enabled = true;
        config.colorGrade.tint = 0.25f;     // Magenta/pink shift
        config.colorGrade.saturation = 1.3f; // Vibrant colors
        config.colorGrade.contrast = 1.15f;  // Punchy contrast

        config.trails.enabled = true;
        config.trails.decay = 0.15f;        // Short motion trails
        config.trails.opacity = 0.4f;
    }
    else if (presetName == "ethereal")
    {
        // Dreamy, prismatic glass rendering
        config.shaderType = ShaderType::GlassRefraction;
        
        // 3D Setup
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 4.0f;
        config.settings3D.autoRotate = true;
        
        // Material: Prismatic Glass
        config.material.enabled = true;
        config.material.refractiveIndex = 1.8f; // High refraction (Crystal/Diamond)
        config.material.reflectivity = 0.4f;
        config.material.fresnelPower = 2.0f;
        config.material.roughness = 0.0f;
        config.material.useEnvironmentMap = true;
        config.material.environmentMapId = "abstract"; // Colorful environment for refraction
        
        // Post
        config.bloom.enabled = true;
        config.bloom.intensity = 1.0f;
        config.bloom.spread = 2.0f; // Very soft
        
        config.trails.enabled = true;
        config.trails.decay = 0.1f; // Long, smooth trails
        config.trails.opacity = 0.7f;
        
        config.colorGrade.enabled = true;
        config.colorGrade.saturation = 0.8f; // Slightly desaturated/pastel
        config.colorGrade.brightness = 0.1f;
    }
    else if (presetName == "matrix")
    {
        // Digital rain code aesthetic
        config.shaderType = ShaderType::DigitalGlitch;
        
        // Post
        config.scanlines.enabled = true;
        config.scanlines.intensity = 0.4f;
        config.scanlines.density = 3.0f;
        
        config.bloom.enabled = true;
        config.bloom.intensity = 1.0f;
        
        config.colorGrade.enabled = true;
        config.colorGrade.tint = -0.5f; // Strong Green shift
        config.colorGrade.contrast = 1.5f; // High contrast
        config.colorGrade.shadows = juce::Colour(0xFF001100); // Dark green shadows
        
        // Particles: Falling code
        config.particles.enabled = true;
        config.particles.emissionMode = ParticleEmissionMode::AlongWaveform;
        config.particles.emissionRate = 300.0f;
        config.particles.particleLife = 1.5f;
        config.particles.particleSize = 4.0f;
        config.particles.particleColor = juce::Colour(0xFF00FF00); // Matrix Green
        config.particles.blendMode = ParticleBlendMode::Additive;
        config.particles.gravity = 80.0f; // Fall down fast
        config.particles.velocityScale = 0.2f; // Little horizontal movement
    }
    else if (presetName == "plasma_sine")
    {
        // Electric plasma beam aesthetic - bright blue glow on dark background
        config.shaderType = ShaderType::PlasmaSine;

        // Bloom - selective, moderate glow (only brightest parts)
        config.bloom.enabled = true;
        config.bloom.intensity = 1.0f;      // Moderate glow (was 1.5)
        config.bloom.threshold = 0.5f;      // Higher threshold - selective (was 0.2)
        config.bloom.spread = 1.2f;         // Slightly reduced spread (was 1.5)
        config.bloom.iterations = 4;

        // Trails - DISABLED (causes brightness accumulation issues)
        config.trails.enabled = false;

        // Vignette - dark corners for focus on plasma
        config.vignette.enabled = true;
        config.vignette.intensity = 0.4f;
        config.vignette.softness = 0.5f;
        config.vignette.colour = juce::Colour(0xFF000510);  // Very dark blue-black

        // Color Grade - subtle enhancement for electric look
        config.colorGrade.enabled = true;
        config.colorGrade.contrast = 1.15f;
        config.colorGrade.saturation = 1.1f;    // Slightly vivid colors
        config.colorGrade.shadows = juce::Colour(0xFF000818);  // Deep blue shadows
    }
    else if (presetName == "glitch_art")
    {
        // Chaotic, distorted data-moshing
        config.shaderType = ShaderType::DigitalGlitch;

        // Post
        config.distortion.enabled = true;
        config.distortion.intensity = 0.05f; // Heavy distortion
        config.distortion.frequency = 8.0f;
        config.distortion.speed = 4.0f;

        config.chromaticAberration.enabled = true;
        config.chromaticAberration.intensity = 0.03f; // Extreme separation

        config.filmGrain.enabled = true;
        config.filmGrain.intensity = 0.3f;

        config.bloom.enabled = true;
        config.bloom.intensity = 0.8f;

        config.trails.enabled = true;
        config.trails.decay = 0.4f; // Short trails/smear
    }
    else if (presetName == "electric_flower")
    {
        // Electric flower visualization inspired by webglsamples.org/electricflower
        // Uses cascaded 3D rotations with irrational multipliers for organic motion
        // Colors derived from oscillator base color
        config.shaderType = ShaderType::ElectricFlower;

        // 3D Setup - orbital view for flower pattern
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 4.0f;
        config.settings3D.cameraAngleX = 20.0f;
        config.settings3D.autoRotate = true;
        config.settings3D.rotateSpeed = 8.0f;

        // Bloom - soft knee for smooth glow transition
        config.bloom.enabled = true;
        config.bloom.intensity = 1.8f;
        config.bloom.threshold = 0.3f;
        config.bloom.spread = 2.0f;
        config.bloom.softKnee = 0.8f;  // Soft threshold like reference

        // Radial blur - zoom-glow effect from center (like reference)
        config.radialBlur.enabled = true;
        config.radialBlur.amount = 0.08f;   // Subtle zoom
        config.radialBlur.glow = 1.2f;      // Slight brightening
        config.radialBlur.samples = 4;      // Match reference

        // Trails for smooth motion persistence
        config.trails.enabled = true;
        config.trails.decay = 0.08f;  // Slow decay for dreamy look
        config.trails.opacity = 0.7f;

        // Color grading - enhance vibrancy
        config.colorGrade.enabled = true;
        config.colorGrade.saturation = 1.2f;
        config.colorGrade.contrast = 1.1f;

        // Vignette for focus
        config.vignette.enabled = true;
        config.vignette.intensity = 0.4f;
        config.vignette.softness = 0.6f;
    }

    return config;
}

std::vector<std::pair<juce::String, juce::String>> VisualConfiguration::getAvailablePresets()
{
    // Returns pairs of (id, displayName)
    return {
        {"default", "Basic (Default)"},
        {"neon", "Neon"},
        {"plasma_sine", "Plasma Sine"},
        {"cyberpunk", "Cyberpunk Edge"},
        {"liquid_gold", "Liquid Gold"},
        {"deep_ocean", "Deep Ocean"},
        {"synthwave", "Synthwave Grid"},
        {"ethereal", "Ethereal Dream"},
        {"matrix", "Matrix Rain"},
        {"glitch_art", "Glitch Art"},
        {"electric_flower", "Electric Flower"}
    };
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
    if (id == "glass_refraction")  return ShaderType::GlassRefraction;
    if (id == "liquid_chrome")     return ShaderType::LiquidChrome;
    if (id == "crystalline")       return ShaderType::Crystalline;
    return ShaderType::Basic2D;
}

} // namespace oscil
