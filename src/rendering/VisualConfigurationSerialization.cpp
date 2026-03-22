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
static const juce::Identifier TILTSHIFT_TYPE("TiltShift");
static const juce::Identifier RADIALBLUR_TYPE("RadialBlur");

// ============================================================================
// toValueTree helpers
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

static void serializeTiltShift(juce::ValueTree& parent, const TiltShiftSettings& ts)
{
    juce::ValueTree t(TILTSHIFT_TYPE);
    t.setProperty("enabled", ts.enabled, nullptr);
    t.setProperty("position", ts.position, nullptr);
    t.setProperty("range", ts.range, nullptr);
    t.setProperty("blurRadius", ts.blurRadius, nullptr);
    t.setProperty("iterations", ts.iterations, nullptr);
    parent.addChild(t, -1, nullptr);
}

// ============================================================================
// fromValueTree helpers
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

static void deserializeTiltShift(const juce::ValueTree& tree, TiltShiftSettings& ts)
{
    auto t = tree.getChildWithName(TILTSHIFT_TYPE);
    if (!t.isValid()) return;
    ts.enabled = t.getProperty("enabled", false);
    ts.position = t.getProperty("position", 0.5f);
    ts.range = t.getProperty("range", 0.3f);
    ts.blurRadius = t.getProperty("blurRadius", 2.0f);
    ts.iterations = t.getProperty("iterations", 3);
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
    serializeTiltShift(tree, tiltShift);

    if (presetId.isNotEmpty())
        tree.setProperty("presetId", presetId, nullptr);

    return tree;
}

VisualConfiguration VisualConfiguration::fromValueTree(const juce::ValueTree& tree)
{
    VisualConfiguration config;

    if (!tree.hasType(VISUAL_CONFIG_TYPE))
        return config;

    config.shaderType = idToShaderType(tree.getProperty("shaderType", "basic"));
    int blendModeInt = static_cast<int>(tree.getProperty("compositeBlendMode", 0));
    config.compositeBlendMode = (blendModeInt >= 0 && blendModeInt <= static_cast<int>(BlendMode::Screen))
                                    ? static_cast<BlendMode>(blendModeInt)
                                    : BlendMode::Alpha;
    config.compositeOpacity = tree.getProperty("compositeOpacity", 1.0f);

    deserializeBloom(tree, config.bloom);
    deserializeRadialBlur(tree, config.radialBlur);
    deserializeTrails(tree, config.trails);
    deserializeColorGrade(tree, config.colorGrade);
    deserializeVignette(tree, config.vignette);
    deserializeFilmGrain(tree, config.filmGrain);
    deserializeChromaticAberration(tree, config.chromaticAberration);
    deserializeScanlines(tree, config.scanlines);
    deserializeTiltShift(tree, config.tiltShift);

    config.presetId = tree.getProperty("presetId", "default").toString();

    config.validate();

    return config;
}

} // namespace oscil
