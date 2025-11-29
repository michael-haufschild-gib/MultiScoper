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
    tree.addChild(bloomTree, -1, nullptr);

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

    // Set the preset identifier
    config.presetId = presetName;

    // ==========================================================================
    // Basic Presets
    // ==========================================================================

    if (presetName == "default" || presetName == "minimal")
    {
        config.shaderType = ShaderType::Basic2D;
        // All effects disabled by default - clean baseline
    }
    else if (presetName == "neon")
    {
        config.shaderType = ShaderType::NeonGlow;
        config.bloom.enabled = true;
        config.bloom.intensity = 1.5f;
        config.bloom.threshold = 0.6f;
    }
    else if (presetName == "retro")
    {
        config.shaderType = ShaderType::Basic2D;
        config.scanlines.enabled = true;
        config.scanlines.intensity = 0.4f;
        config.colorGrade.enabled = true;
        config.colorGrade.saturation = 0.8f;
        config.filmGrain.enabled = true;
        config.filmGrain.intensity = 0.15f;
        config.vignette.enabled = true;
    }
    else if (presetName == "gradient")
    {
        config.shaderType = ShaderType::GradientFill;
    }
    else if (presetName == "outline")
    {
        config.shaderType = ShaderType::DualOutline;
        config.bloom.enabled = true;
        config.bloom.intensity = 0.8f;
    }
    else if (presetName == "plasma")
    {
        config.shaderType = ShaderType::PlasmaSine;
    }
    else if (presetName == "glitch")
    {
        config.shaderType = ShaderType::DigitalGlitch;
        config.chromaticAberration.enabled = true;
        config.chromaticAberration.intensity = 0.01f;
        config.distortion.enabled = true;
        config.distortion.intensity = 0.02f;
    }
    else if (presetName == "vector_flow")
    {
        config.shaderType = ShaderType::VectorFlow;
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 5.0f;
        config.settings3D.autoRotate = true;
    }
    else if (presetName == "string_theory")
    {
        config.shaderType = ShaderType::StringTheory;
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 6.0f;
        config.settings3D.cameraAngleX = 25.0f;
        config.settings3D.autoRotate = true;
        config.bloom.enabled = true;
        config.bloom.intensity = 0.6f;
    }
    else if (presetName == "crystal")
    {
        config.shaderType = ShaderType::Crystalline;
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 4.0f;
        config.settings3D.autoRotate = true;
        config.material.enabled = true;
        config.material.roughness = 0.05f;
        config.material.reflectivity = 0.8f;
        config.material.useEnvironmentMap = true;
    }

    // ==========================================================================
    // Post-Processing Test Presets
    // ==========================================================================

    else if (presetName == "test_bloom")
    {
        // Tests: Bloom post-processing effect
        // Look for: Bright glow/halo around waveform, soft edges
        config.shaderType = ShaderType::Basic2D;
        config.bloom.enabled = true;
        config.bloom.intensity = 2.0f;      // High intensity for visibility
        config.bloom.threshold = 0.5f;      // Lower threshold to catch more
        config.bloom.iterations = 6;        // More blur iterations
        config.bloom.spread = 1.5f;         // Wider spread
    }
    else if (presetName == "test_trails")
    {
        // Tests: Trail/persistence effect
        // Look for: Ghostly trail following waveform, fading afterimages
        config.shaderType = ShaderType::Basic2D;
        config.trails.enabled = true;
        config.trails.decay = 0.05f;        // Slow decay = longer trails
        config.trails.opacity = 0.9f;       // High trail opacity
    }
    else if (presetName == "test_chromatic")
    {
        // Tests: Chromatic aberration effect
        // Look for: RGB color fringing at edges, rainbow separation
        config.shaderType = ShaderType::Basic2D;
        config.chromaticAberration.enabled = true;
        config.chromaticAberration.intensity = 0.015f;  // High intensity
    }
    else if (presetName == "test_scanlines")
    {
        // Tests: CRT scanline effect
        // Look for: Horizontal dark lines, retro CRT monitor look
        config.shaderType = ShaderType::Basic2D;
        config.scanlines.enabled = true;
        config.scanlines.intensity = 0.5f;  // Strong lines
        config.scanlines.density = 2.0f;    // Visible spacing
        config.scanlines.phosphorGlow = true;
    }
    else if (presetName == "test_vignette")
    {
        // Tests: Vignette effect
        // Look for: Dark corners, spotlight/tunnel vision effect
        config.shaderType = ShaderType::Basic2D;
        config.vignette.enabled = true;
        config.vignette.intensity = 0.8f;   // Strong darkening
        config.vignette.softness = 0.4f;    // Moderate edge softness
    }
    else if (presetName == "test_grain")
    {
        // Tests: Film grain effect
        // Look for: Noisy animated texture, film-like grain
        config.shaderType = ShaderType::Basic2D;
        config.filmGrain.enabled = true;
        config.filmGrain.intensity = 0.3f;  // Very visible grain
        config.filmGrain.speed = 24.0f;     // Film-like animation speed
    }
    else if (presetName == "test_distortion")
    {
        // Tests: Wave distortion effect
        // Look for: Wavy/warped display, ripple-like deformation
        config.shaderType = ShaderType::Basic2D;
        config.distortion.enabled = true;
        config.distortion.intensity = 0.03f;   // Visible distortion
        config.distortion.frequency = 6.0f;    // Multiple waves
        config.distortion.speed = 2.0f;        // Animated
    }
    else if (presetName == "test_color_grade")
    {
        // Tests: Color grading effect
        // Look for: Shifted colors, contrast/saturation changes
        config.shaderType = ShaderType::Basic2D;
        config.colorGrade.enabled = true;
        config.colorGrade.brightness = 0.1f;
        config.colorGrade.contrast = 1.3f;
        config.colorGrade.saturation = 1.5f;
        config.colorGrade.temperature = 0.3f;  // Warm tint
    }
    else if (presetName == "test_all_post")
    {
        // Tests: Multiple post-processing effects combined
        // Look for: Cinematic look with bloom, vignette, subtle grain
        config.shaderType = ShaderType::Basic2D;
        config.bloom.enabled = true;
        config.bloom.intensity = 1.2f;
        config.bloom.threshold = 0.7f;
        config.trails.enabled = true;
        config.trails.decay = 0.15f;
        config.vignette.enabled = true;
        config.vignette.intensity = 0.5f;
        config.filmGrain.enabled = true;
        config.filmGrain.intensity = 0.08f;
        config.colorGrade.enabled = true;
        config.colorGrade.saturation = 1.2f;
    }

    // ==========================================================================
    // Particle System Test Presets
    // ==========================================================================

    else if (presetName == "test_particles_along")
    {
        // Tests: Particles emitted along waveform path
        // Look for: Stream of particles following waveform shape
        config.shaderType = ShaderType::Basic2D;
        config.particles.enabled = true;
        config.particles.emissionMode = ParticleEmissionMode::AlongWaveform;
        config.particles.emissionRate = 200.0f;
        config.particles.particleLife = 2.0f;
        config.particles.particleSize = 6.0f;
        config.particles.particleColor = juce::Colour(0xFFFFAA00);  // Orange
        config.particles.blendMode = ParticleBlendMode::Additive;
        config.particles.gravity = 0.0f;
        config.particles.randomness = 0.5f;
        config.bloom.enabled = true;
        config.bloom.intensity = 0.8f;
    }
    else if (presetName == "test_particles_peaks")
    {
        // Tests: Particles emitted at amplitude peaks
        // Look for: Burst of particles at waveform peaks/transients
        config.shaderType = ShaderType::Basic2D;
        config.particles.enabled = true;
        config.particles.emissionMode = ParticleEmissionMode::AtPeaks;
        config.particles.emissionRate = 150.0f;
        config.particles.particleLife = 1.5f;
        config.particles.particleSize = 8.0f;
        config.particles.particleColor = juce::Colour(0xFF00FFFF);  // Cyan
        config.particles.blendMode = ParticleBlendMode::Additive;
        config.particles.gravity = -50.0f;  // Float upward
        config.particles.audioReactive = true;
        config.particles.audioEmissionBoost = 3.0f;
        config.bloom.enabled = true;
        config.bloom.intensity = 1.0f;
    }
    else if (presetName == "test_particles_zero")
    {
        // Tests: Particles emitted at zero crossings
        // Look for: Particles appearing where waveform crosses center
        config.shaderType = ShaderType::Basic2D;
        config.particles.enabled = true;
        config.particles.emissionMode = ParticleEmissionMode::AtZeroCrossings;
        config.particles.emissionRate = 100.0f;
        config.particles.particleLife = 2.5f;
        config.particles.particleSize = 5.0f;
        config.particles.particleColor = juce::Colour(0xFFFF00FF);  // Magenta
        config.particles.blendMode = ParticleBlendMode::Additive;
        config.particles.randomness = 0.8f;
    }

    // ==========================================================================
    // 3D Shader Test Presets
    // ==========================================================================

    else if (presetName == "test_3d_ribbon")
    {
        // Tests: 3D volumetric ribbon shader
        // Look for: Tube-like 3D waveform with lighting
        config.shaderType = ShaderType::VolumetricRibbon;
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 4.0f;
        config.settings3D.cameraAngleX = 20.0f;
        config.settings3D.autoRotate = true;
        config.settings3D.rotateSpeed = 15.0f;
        config.bloom.enabled = true;
        config.bloom.intensity = 0.6f;
    }
    else if (presetName == "test_3d_wireframe")
    {
        // Tests: 3D wireframe mesh shader
        // Look for: Retro grid/wireframe 3D mesh, neon lines
        config.shaderType = ShaderType::WireframeMesh;
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 5.0f;
        config.settings3D.cameraAngleX = 30.0f;
        config.settings3D.autoRotate = true;
        config.settings3D.rotateSpeed = 10.0f;
        config.settings3D.meshResolutionX = 64;
        config.settings3D.meshResolutionZ = 24;
        config.bloom.enabled = true;
        config.bloom.intensity = 1.2f;
        config.scanlines.enabled = true;
        config.scanlines.intensity = 0.2f;
    }
    else if (presetName == "test_3d_orbit")
    {
        // Tests: 3D camera orbit controls
        // Look for: Smooth auto-rotation around 3D waveform
        config.shaderType = ShaderType::VolumetricRibbon;
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 3.5f;
        config.settings3D.cameraAngleX = 45.0f;
        config.settings3D.cameraAngleY = 30.0f;
        config.settings3D.autoRotate = true;
        config.settings3D.rotateSpeed = 25.0f;
    }

    // ==========================================================================
    // Material Shader Test Presets
    // ==========================================================================

    else if (presetName == "test_glass")
    {
        // Tests: Glass refraction material shader
        // Look for: Transparent glass tube with refraction, chromatic dispersion
        config.shaderType = ShaderType::GlassRefraction;
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 4.0f;
        config.settings3D.autoRotate = true;
        config.settings3D.rotateSpeed = 8.0f;
        config.material.enabled = true;
        config.material.reflectivity = 0.3f;
        config.material.refractiveIndex = 1.5f;     // Glass IOR
        config.material.fresnelPower = 3.0f;
        config.material.useEnvironmentMap = true;
        config.material.environmentMapId = "neon";
    }
    else if (presetName == "test_chrome")
    {
        // Tests: Liquid chrome material shader
        // Look for: Reflective metallic surface with animated ripples
        config.shaderType = ShaderType::LiquidChrome;
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 4.0f;
        config.settings3D.cameraAngleX = 25.0f;
        config.settings3D.autoRotate = true;
        config.settings3D.rotateSpeed = 12.0f;
        config.material.enabled = true;
        config.material.reflectivity = 1.0f;
        config.material.roughness = 0.05f;
        config.material.useEnvironmentMap = true;
        config.material.environmentMapId = "studio";
    }
    else if (presetName == "test_chrome_cyber")
    {
        // Tests: Chrome with cyber environment
        // Look for: Neon reflections on liquid metal surface
        config.shaderType = ShaderType::LiquidChrome;
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 3.5f;
        config.settings3D.autoRotate = true;
        config.settings3D.rotateSpeed = 10.0f;
        config.material.enabled = true;
        config.material.reflectivity = 1.0f;
        config.material.roughness = 0.1f;
        config.material.useEnvironmentMap = true;
        config.material.environmentMapId = "cyber_space";
        config.bloom.enabled = true;
        config.bloom.intensity = 0.8f;
    }

    // ==========================================================================
    // Combined Feature Test Presets
    // ==========================================================================

    else if (presetName == "test_full_demo")
    {
        // Tests: Multiple systems working together
        // Look for: 3D ribbon with particles, bloom, and color effects
        config.shaderType = ShaderType::VolumetricRibbon;
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 4.5f;
        config.settings3D.autoRotate = true;
        config.settings3D.rotateSpeed = 8.0f;
        config.particles.enabled = true;
        config.particles.emissionMode = ParticleEmissionMode::AtPeaks;
        config.particles.emissionRate = 100.0f;
        config.particles.particleColor = juce::Colour(0xFFFFCC00);
        config.bloom.enabled = true;
        config.bloom.intensity = 1.0f;
        config.vignette.enabled = true;
        config.vignette.intensity = 0.4f;
        config.colorGrade.enabled = true;
        config.colorGrade.saturation = 1.3f;
    }
    else if (presetName == "test_retro_3d")
    {
        // Tests: Retro aesthetic with 3D wireframe
        // Look for: Neon wireframe mesh with CRT effects
        config.shaderType = ShaderType::WireframeMesh;
        config.settings3D.enabled = true;
        config.settings3D.cameraDistance = 5.0f;
        config.settings3D.cameraAngleX = 35.0f;
        config.settings3D.autoRotate = true;
        config.bloom.enabled = true;
        config.bloom.intensity = 1.5f;
        config.scanlines.enabled = true;
        config.scanlines.intensity = 0.4f;
        config.chromaticAberration.enabled = true;
        config.chromaticAberration.intensity = 0.008f;
        config.vignette.enabled = true;
        config.vignette.intensity = 0.6f;
        config.filmGrain.enabled = true;
        config.filmGrain.intensity = 0.1f;
    }

    return config;
}

std::vector<std::pair<juce::String, juce::String>> VisualConfiguration::getAvailablePresets()
{
    // Returns pairs of (id, displayName)
    return {
        // Basic presets
        {"default", "Default (Clean)"},
        {"neon", "Neon Glow"},
        {"retro", "Retro CRT"},
        {"gradient", "Gradient Fill"},
        {"outline", "Dual Outline"},
        {"plasma", "Plasma Sine"},
        {"glitch", "Digital Glitch"},
        {"vector_flow", "Vector Flow"},
        {"string_theory", "String Theory"},
        {"crystal", "Crystalline"},

        // Post-processing tests
        {"test_bloom", "Test: Bloom Effect"},
        {"test_trails", "Test: Trails/Persistence"},
        {"test_chromatic", "Test: Chromatic Aberration"},
        {"test_scanlines", "Test: Scanlines"},
        {"test_vignette", "Test: Vignette"},
        {"test_grain", "Test: Film Grain"},
        {"test_distortion", "Test: Wave Distortion"},
        {"test_color_grade", "Test: Color Grading"},
        {"test_all_post", "Test: All Post-Processing"},

        // Particle tests
        {"test_particles_along", "Test: Particles Along"},
        {"test_particles_peaks", "Test: Particles at Peaks"},
        {"test_particles_zero", "Test: Particles Zero-Cross"},

        // 3D shader tests
        {"test_3d_ribbon", "Test: 3D Ribbon"},
        {"test_3d_wireframe", "Test: 3D Wireframe"},
        {"test_3d_orbit", "Test: 3D Camera Orbit"},

        // Material tests
        {"test_glass", "Test: Glass Material"},
        {"test_chrome", "Test: Chrome Material"},
        {"test_chrome_cyber", "Test: Chrome Cyber"},

        // Combined tests
        {"test_full_demo", "Test: Full Demo"},
        {"test_retro_3d", "Test: Retro 3D"}
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
    if (id == "glass_refraction")  return ShaderType::GlassRefraction;
    if (id == "liquid_chrome")     return ShaderType::LiquidChrome;
    if (id == "crystalline")       return ShaderType::Crystalline;
    return ShaderType::Basic2D;
}

} // namespace oscil
