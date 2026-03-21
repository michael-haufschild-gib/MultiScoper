/*
    Oscil - Visual Configuration Serialization
    ValueTree persistence for VisualConfiguration
*/

#include "rendering/VisualConfiguration.h"

namespace oscil
{

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
static const juce::Identifier LIGHTING_TYPE("Lighting");
static const juce::Identifier MATERIAL_TYPE("Material");

// ============================================================================
// toValueTree helpers — one per subsection
// ============================================================================

static void serializeBloom(juce::ValueTree& parent, const BloomSettings& bloom)
{
    juce::ValueTree t(BLOOM_TYPE);
    t.setProperty("enabled", bloom.enabled, nullptr);
    t.setProperty("intensity", bloom.intensity, nullptr);
    t.setProperty("threshold", bloom.threshold, nullptr);
    t.setProperty("iterations", bloom.iterations, nullptr);
    t.setProperty("downsampleSteps", bloom.downsampleSteps, nullptr);
    t.setProperty("spread", bloom.spread, nullptr);
    t.setProperty("softKnee", bloom.softKnee, nullptr);
    parent.addChild(t, -1, nullptr);
}

static void serializeRadialBlur(juce::ValueTree& parent, const RadialBlurSettings& rb)
{
    juce::ValueTree t(RADIALBLUR_TYPE);
    t.setProperty("enabled", rb.enabled, nullptr);
    t.setProperty("amount", rb.amount, nullptr);
    t.setProperty("glow", rb.glow, nullptr);
    t.setProperty("samples", rb.samples, nullptr);
    parent.addChild(t, -1, nullptr);
}

static void serializeTrails(juce::ValueTree& parent, const TrailSettings& trails)
{
    juce::ValueTree t(TRAILS_TYPE);
    t.setProperty("enabled", trails.enabled, nullptr);
    t.setProperty("decay", trails.decay, nullptr);
    t.setProperty("opacity", trails.opacity, nullptr);
    parent.addChild(t, -1, nullptr);
}

static void serializeColorGrade(juce::ValueTree& parent, const ColorGradeSettings& cg)
{
    juce::ValueTree t(COLORGRADE_TYPE);
    t.setProperty("enabled", cg.enabled, nullptr);
    t.setProperty("brightness", cg.brightness, nullptr);
    t.setProperty("contrast", cg.contrast, nullptr);
    t.setProperty("saturation", cg.saturation, nullptr);
    t.setProperty("temperature", cg.temperature, nullptr);
    t.setProperty("tint", cg.tint, nullptr);
    t.setProperty("shadows", static_cast<juce::int64>(cg.shadows.getARGB()), nullptr);
    t.setProperty("highlights", static_cast<juce::int64>(cg.highlights.getARGB()), nullptr);
    parent.addChild(t, -1, nullptr);
}

static void serializeVignette(juce::ValueTree& parent, const VignetteSettings& v)
{
    juce::ValueTree t(VIGNETTE_TYPE);
    t.setProperty("enabled", v.enabled, nullptr);
    t.setProperty("intensity", v.intensity, nullptr);
    t.setProperty("softness", v.softness, nullptr);
    t.setProperty("colour", static_cast<juce::int64>(v.colour.getARGB()), nullptr);
    parent.addChild(t, -1, nullptr);
}

static void serializeFilmGrain(juce::ValueTree& parent, const FilmGrainSettings& fg)
{
    juce::ValueTree t(FILMGRAIN_TYPE);
    t.setProperty("enabled", fg.enabled, nullptr);
    t.setProperty("intensity", fg.intensity, nullptr);
    t.setProperty("speed", fg.speed, nullptr);
    parent.addChild(t, -1, nullptr);
}

static void serializeChromaticAberration(juce::ValueTree& parent, const ChromaticAberrationSettings& ca)
{
    juce::ValueTree t(CHROMATIC_TYPE);
    t.setProperty("enabled", ca.enabled, nullptr);
    t.setProperty("intensity", ca.intensity, nullptr);
    parent.addChild(t, -1, nullptr);
}

static void serializeScanlines(juce::ValueTree& parent, const ScanlineSettings& s)
{
    juce::ValueTree t(SCANLINES_TYPE);
    t.setProperty("enabled", s.enabled, nullptr);
    t.setProperty("intensity", s.intensity, nullptr);
    t.setProperty("density", s.density, nullptr);
    t.setProperty("phosphorGlow", s.phosphorGlow, nullptr);
    parent.addChild(t, -1, nullptr);
}

static void serializeDistortion(juce::ValueTree& parent, const DistortionSettings& d)
{
    juce::ValueTree t(DISTORTION_TYPE);
    t.setProperty("enabled", d.enabled, nullptr);
    t.setProperty("intensity", d.intensity, nullptr);
    t.setProperty("frequency", d.frequency, nullptr);
    t.setProperty("speed", d.speed, nullptr);
    parent.addChild(t, -1, nullptr);
}

static void serializeTiltShift(juce::ValueTree& parent, const TiltShiftSettings& ts)
{
    juce::ValueTree t(TILTSHIFT_TYPE);
    t.setProperty("enabled", ts.enabled, nullptr);
    t.setProperty("position", ts.position, nullptr);
    t.setProperty("range", ts.range, nullptr);
    t.setProperty("blurRadius", ts.blurRadius, nullptr);
    parent.addChild(t, -1, nullptr);
}

static void serializeParticles(juce::ValueTree& parent, const ParticleSettings& p)
{
    juce::ValueTree t(PARTICLES_TYPE);
    t.setProperty("enabled", p.enabled, nullptr);
    t.setProperty("emissionMode", static_cast<int>(p.emissionMode), nullptr);
    t.setProperty("emissionRate", p.emissionRate, nullptr);
    t.setProperty("particleLife", p.particleLife, nullptr);
    t.setProperty("particleSize", p.particleSize, nullptr);
    t.setProperty("particleColor", static_cast<juce::int64>(p.particleColor.getARGB()), nullptr);
    t.setProperty("blendMode", static_cast<int>(p.blendMode), nullptr);
    t.setProperty("gravity", p.gravity, nullptr);
    t.setProperty("drag", p.drag, nullptr);
    t.setProperty("randomness", p.randomness, nullptr);
    t.setProperty("velocityScale", p.velocityScale, nullptr);
    t.setProperty("audioReactive", p.audioReactive, nullptr);
    t.setProperty("audioEmissionBoost", p.audioEmissionBoost, nullptr);
    t.setProperty("textureId", p.textureId, nullptr);
    t.setProperty("textureRows", p.textureRows, nullptr);
    t.setProperty("textureCols", p.textureCols, nullptr);
    t.setProperty("softParticles", p.softParticles, nullptr);
    t.setProperty("softDepthSensitivity", p.softDepthSensitivity, nullptr);
    t.setProperty("useTurbulence", p.useTurbulence, nullptr);
    t.setProperty("turbulenceStrength", p.turbulenceStrength, nullptr);
    t.setProperty("turbulenceScale", p.turbulenceScale, nullptr);
    t.setProperty("turbulenceSpeed", p.turbulenceSpeed, nullptr);
    parent.addChild(t, -1, nullptr);
}

static void serializeSettings3D(juce::ValueTree& parent, const Settings3D& s3d)
{
    juce::ValueTree t(SETTINGS3D_TYPE);
    t.setProperty("enabled", s3d.enabled, nullptr);
    t.setProperty("cameraDistance", s3d.cameraDistance, nullptr);
    t.setProperty("cameraAngleX", s3d.cameraAngleX, nullptr);
    t.setProperty("cameraAngleY", s3d.cameraAngleY, nullptr);
    t.setProperty("autoRotate", s3d.autoRotate, nullptr);
    t.setProperty("rotateSpeed", s3d.rotateSpeed, nullptr);
    t.setProperty("meshResolutionX", s3d.meshResolutionX, nullptr);
    t.setProperty("meshResolutionZ", s3d.meshResolutionZ, nullptr);
    t.setProperty("meshScale", s3d.meshScale, nullptr);
    parent.addChild(t, -1, nullptr);
}

static void serializeLighting(juce::ValueTree& parent, const LightingConfig& l)
{
    juce::ValueTree t(LIGHTING_TYPE);
    t.setProperty("lightDirX", l.lightDirX, nullptr);
    t.setProperty("lightDirY", l.lightDirY, nullptr);
    t.setProperty("lightDirZ", l.lightDirZ, nullptr);
    t.setProperty("ambientR", l.ambientR, nullptr);
    t.setProperty("ambientG", l.ambientG, nullptr);
    t.setProperty("ambientB", l.ambientB, nullptr);
    t.setProperty("diffuseR", l.diffuseR, nullptr);
    t.setProperty("diffuseG", l.diffuseG, nullptr);
    t.setProperty("diffuseB", l.diffuseB, nullptr);
    t.setProperty("specularR", l.specularR, nullptr);
    t.setProperty("specularG", l.specularG, nullptr);
    t.setProperty("specularB", l.specularB, nullptr);
    t.setProperty("specularPower", l.specularPower, nullptr);
    t.setProperty("specularIntensity", l.specularIntensity, nullptr);
    parent.addChild(t, -1, nullptr);
}

static void serializeMaterial(juce::ValueTree& parent, const MaterialSettings& m)
{
    juce::ValueTree t(MATERIAL_TYPE);
    t.setProperty("enabled", m.enabled, nullptr);
    t.setProperty("reflectivity", m.reflectivity, nullptr);
    t.setProperty("refractiveIndex", m.refractiveIndex, nullptr);
    t.setProperty("fresnelPower", m.fresnelPower, nullptr);
    t.setProperty("tintColor", static_cast<juce::int64>(m.tintColor.getARGB()), nullptr);
    t.setProperty("roughness", m.roughness, nullptr);
    t.setProperty("useEnvironmentMap", m.useEnvironmentMap, nullptr);
    t.setProperty("environmentMapId", m.environmentMapId, nullptr);
    parent.addChild(t, -1, nullptr);
}

// ============================================================================
// fromValueTree helpers — one per subsection
// ============================================================================

static void deserializeBloom(const juce::ValueTree& tree, BloomSettings& bloom)
{
    auto t = tree.getChildWithName(BLOOM_TYPE);
    if (!t.isValid()) return;
    bloom.enabled = t.getProperty("enabled", false);
    bloom.intensity = t.getProperty("intensity", 1.0f);
    bloom.threshold = t.getProperty("threshold", 0.8f);
    bloom.iterations = t.getProperty("iterations", 4);
    bloom.downsampleSteps = t.getProperty("downsampleSteps", 6);
    bloom.spread = t.getProperty("spread", 1.0f);
    bloom.softKnee = t.getProperty("softKnee", 0.5f);
}

static void deserializeRadialBlur(const juce::ValueTree& tree, RadialBlurSettings& rb)
{
    auto t = tree.getChildWithName(RADIALBLUR_TYPE);
    if (!t.isValid()) return;
    rb.enabled = t.getProperty("enabled", false);
    rb.amount = t.getProperty("amount", 0.1f);
    rb.glow = t.getProperty("glow", 1.0f);
    rb.samples = t.getProperty("samples", 4);
}

static void deserializeTrails(const juce::ValueTree& tree, TrailSettings& trails)
{
    auto t = tree.getChildWithName(TRAILS_TYPE);
    if (!t.isValid()) return;
    trails.enabled = t.getProperty("enabled", false);
    trails.decay = t.getProperty("decay", 0.1f);
    trails.opacity = t.getProperty("opacity", 0.8f);
}

static void deserializeColorGrade(const juce::ValueTree& tree, ColorGradeSettings& cg)
{
    auto t = tree.getChildWithName(COLORGRADE_TYPE);
    if (!t.isValid()) return;
    cg.enabled = t.getProperty("enabled", false);
    cg.brightness = t.getProperty("brightness", 0.0f);
    cg.contrast = t.getProperty("contrast", 1.0f);
    cg.saturation = t.getProperty("saturation", 1.0f);
    cg.temperature = t.getProperty("temperature", 0.0f);
    cg.tint = t.getProperty("tint", 0.0f);
    cg.shadows = juce::Colour(static_cast<juce::uint32>(
        static_cast<juce::int64>(t.getProperty("shadows", static_cast<juce::int64>(0xFF000000)))));
    cg.highlights = juce::Colour(static_cast<juce::uint32>(
        static_cast<juce::int64>(t.getProperty("highlights", static_cast<juce::int64>(0xFFFFFFFF)))));
}

static void deserializeVignette(const juce::ValueTree& tree, VignetteSettings& v)
{
    auto t = tree.getChildWithName(VIGNETTE_TYPE);
    if (!t.isValid()) return;
    v.enabled = t.getProperty("enabled", false);
    v.intensity = t.getProperty("intensity", 0.5f);
    v.softness = t.getProperty("softness", 0.5f);
    v.colour = juce::Colour(static_cast<juce::uint32>(
        static_cast<juce::int64>(t.getProperty("colour", static_cast<juce::int64>(0xFF000000)))));
}

static void deserializeFilmGrain(const juce::ValueTree& tree, FilmGrainSettings& fg)
{
    auto t = tree.getChildWithName(FILMGRAIN_TYPE);
    if (!t.isValid()) return;
    fg.enabled = t.getProperty("enabled", false);
    fg.intensity = t.getProperty("intensity", 0.1f);
    fg.speed = t.getProperty("speed", 24.0f);
}

static void deserializeChromaticAberration(const juce::ValueTree& tree, ChromaticAberrationSettings& ca)
{
    auto t = tree.getChildWithName(CHROMATIC_TYPE);
    if (!t.isValid()) return;
    ca.enabled = t.getProperty("enabled", false);
    ca.intensity = t.getProperty("intensity", 0.005f);
}

static void deserializeScanlines(const juce::ValueTree& tree, ScanlineSettings& s)
{
    auto t = tree.getChildWithName(SCANLINES_TYPE);
    if (!t.isValid()) return;
    s.enabled = t.getProperty("enabled", false);
    s.intensity = t.getProperty("intensity", 0.3f);
    s.density = t.getProperty("density", 2.0f);
    s.phosphorGlow = t.getProperty("phosphorGlow", true);
}

static void deserializeDistortion(const juce::ValueTree& tree, DistortionSettings& d)
{
    auto t = tree.getChildWithName(DISTORTION_TYPE);
    if (!t.isValid()) return;
    d.enabled = t.getProperty("enabled", false);
    d.intensity = t.getProperty("intensity", 0.0f);
    d.frequency = t.getProperty("frequency", 4.0f);
    d.speed = t.getProperty("speed", 1.0f);
}

static void deserializeTiltShift(const juce::ValueTree& tree, TiltShiftSettings& ts)
{
    auto t = tree.getChildWithName(TILTSHIFT_TYPE);
    if (!t.isValid()) return;
    ts.enabled = t.getProperty("enabled", false);
    ts.position = t.getProperty("position", 0.5f);
    ts.range = t.getProperty("range", 0.3f);
    ts.blurRadius = t.getProperty("blurRadius", 2.0f);
}

static void deserializeParticles(const juce::ValueTree& tree, ParticleSettings& p)
{
    auto t = tree.getChildWithName(PARTICLES_TYPE);
    if (!t.isValid()) return;
    p.enabled = t.getProperty("enabled", false);
    p.emissionMode = static_cast<ParticleEmissionMode>(
        static_cast<int>(t.getProperty("emissionMode", 0)));
    p.emissionRate = t.getProperty("emissionRate", 100.0f);
    p.particleLife = t.getProperty("particleLife", 2.0f);
    p.particleSize = t.getProperty("particleSize", 4.0f);
    p.particleColor = juce::Colour(static_cast<juce::uint32>(
        static_cast<juce::int64>(t.getProperty("particleColor", static_cast<juce::int64>(0xFFFFAA00)))));
    p.blendMode = static_cast<ParticleBlendMode>(
        static_cast<int>(t.getProperty("blendMode", 0)));
    p.gravity = t.getProperty("gravity", 0.0f);
    p.drag = t.getProperty("drag", 0.1f);
    p.randomness = t.getProperty("randomness", 0.5f);
    p.velocityScale = t.getProperty("velocityScale", 1.0f);
    p.audioReactive = t.getProperty("audioReactive", true);
    p.audioEmissionBoost = t.getProperty("audioEmissionBoost", 2.0f);
    p.textureId = t.getProperty("textureId", "").toString();
    p.textureRows = t.getProperty("textureRows", 1);
    p.textureCols = t.getProperty("textureCols", 1);
    p.softParticles = t.getProperty("softParticles", false);
    p.softDepthSensitivity = t.getProperty("softDepthSensitivity", 1.0f);
    p.useTurbulence = t.getProperty("useTurbulence", false);
    p.turbulenceStrength = t.getProperty("turbulenceStrength", 0.0f);
    p.turbulenceScale = t.getProperty("turbulenceScale", 0.5f);
    p.turbulenceSpeed = t.getProperty("turbulenceSpeed", 0.5f);
}

static void deserializeSettings3D(const juce::ValueTree& tree, Settings3D& s3d)
{
    auto t = tree.getChildWithName(SETTINGS3D_TYPE);
    if (!t.isValid()) return;
    s3d.enabled = t.getProperty("enabled", false);
    s3d.cameraDistance = t.getProperty("cameraDistance", 5.0f);
    s3d.cameraAngleX = t.getProperty("cameraAngleX", 15.0f);
    s3d.cameraAngleY = t.getProperty("cameraAngleY", 0.0f);
    s3d.autoRotate = t.getProperty("autoRotate", false);
    s3d.rotateSpeed = t.getProperty("rotateSpeed", 10.0f);
    s3d.meshResolutionX = t.getProperty("meshResolutionX", 128);
    s3d.meshResolutionZ = t.getProperty("meshResolutionZ", 32);
    s3d.meshScale = t.getProperty("meshScale", 1.0f);
}

static void deserializeLighting(const juce::ValueTree& tree, LightingConfig& l)
{
    auto t = tree.getChildWithName(LIGHTING_TYPE);
    if (!t.isValid()) return;
    l.lightDirX = t.getProperty("lightDirX", 0.5f);
    l.lightDirY = t.getProperty("lightDirY", 1.0f);
    l.lightDirZ = t.getProperty("lightDirZ", 0.3f);
    l.ambientR = t.getProperty("ambientR", 0.1f);
    l.ambientG = t.getProperty("ambientG", 0.1f);
    l.ambientB = t.getProperty("ambientB", 0.15f);
    l.diffuseR = t.getProperty("diffuseR", 1.0f);
    l.diffuseG = t.getProperty("diffuseG", 1.0f);
    l.diffuseB = t.getProperty("diffuseB", 1.0f);
    l.specularR = t.getProperty("specularR", 1.0f);
    l.specularG = t.getProperty("specularG", 1.0f);
    l.specularB = t.getProperty("specularB", 1.0f);
    l.specularPower = t.getProperty("specularPower", 32.0f);
    l.specularIntensity = t.getProperty("specularIntensity", 0.5f);
}

static void deserializeMaterial(const juce::ValueTree& tree, MaterialSettings& m)
{
    auto t = tree.getChildWithName(MATERIAL_TYPE);
    if (!t.isValid()) return;
    m.enabled = t.getProperty("enabled", false);
    m.reflectivity = t.getProperty("reflectivity", 0.5f);
    m.refractiveIndex = t.getProperty("refractiveIndex", 1.5f);
    m.fresnelPower = t.getProperty("fresnelPower", 2.0f);
    m.tintColor = juce::Colour(static_cast<juce::uint32>(
        static_cast<juce::int64>(t.getProperty("tintColor", static_cast<juce::int64>(0xFFFFFFFF)))));
    m.roughness = t.getProperty("roughness", 0.1f);
    m.useEnvironmentMap = t.getProperty("useEnvironmentMap", true);
    m.environmentMapId = t.getProperty("environmentMapId", "default_studio").toString();
}

// ============================================================================
// Public methods
// ============================================================================

juce::ValueTree VisualConfiguration::toValueTree() const
{
    juce::ValueTree tree(VISUAL_CONFIG_TYPE);

    tree.setProperty("shaderType", shaderTypeToId(shaderType), nullptr);
    tree.setProperty("compositeBlendMode", static_cast<int>(compositeBlendMode), nullptr);
    tree.setProperty("compositeOpacity", compositeOpacity, nullptr);

    serializeBloom(tree, bloom);
    serializeRadialBlur(tree, radialBlur);
    serializeTrails(tree, trails);
    serializeColorGrade(tree, colorGrade);
    serializeVignette(tree, vignette);
    serializeFilmGrain(tree, filmGrain);
    serializeChromaticAberration(tree, chromaticAberration);
    serializeScanlines(tree, scanlines);
    serializeDistortion(tree, distortion);
    serializeTiltShift(tree, tiltShift);
    serializeParticles(tree, particles);
    serializeSettings3D(tree, settings3D);
    serializeLighting(tree, lighting);
    serializeMaterial(tree, material);

    return tree;
}

VisualConfiguration VisualConfiguration::fromValueTree(const juce::ValueTree& tree)
{
    VisualConfiguration config;

    if (!tree.hasType(VISUAL_CONFIG_TYPE))
        return config;

    config.shaderType = idToShaderType(tree.getProperty("shaderType", "basic"));
    config.compositeBlendMode = static_cast<BlendMode>(
        static_cast<int>(tree.getProperty("compositeBlendMode", 0)));
    config.compositeOpacity = tree.getProperty("compositeOpacity", 1.0f);

    deserializeBloom(tree, config.bloom);
    deserializeRadialBlur(tree, config.radialBlur);
    deserializeTrails(tree, config.trails);
    deserializeColorGrade(tree, config.colorGrade);
    deserializeVignette(tree, config.vignette);
    deserializeFilmGrain(tree, config.filmGrain);
    deserializeChromaticAberration(tree, config.chromaticAberration);
    deserializeScanlines(tree, config.scanlines);
    deserializeDistortion(tree, config.distortion);
    deserializeTiltShift(tree, config.tiltShift);
    deserializeParticles(tree, config.particles);
    deserializeSettings3D(tree, config.settings3D);
    deserializeLighting(tree, config.lighting);
    deserializeMaterial(tree, config.material);

    return config;
}

} // namespace oscil
