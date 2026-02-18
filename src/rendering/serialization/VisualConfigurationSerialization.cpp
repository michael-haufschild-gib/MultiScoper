/*
    Oscil - Visual Configuration Serialization Implementation
*/

#include "rendering/VisualConfiguration.h"
#include "rendering/serialization/Settings3DSerialization.h"
#include <cmath>

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

// Forward declarations for JSON helper functions
namespace
{

juce::String blendModeToStringLocal(BlendMode mode)
{
    switch (mode)
    {
        case BlendMode::Alpha:    return "alpha";
        case BlendMode::Additive: return "additive";
        case BlendMode::Multiply: return "multiply";
        case BlendMode::Screen:   return "screen";
        default:                  return "alpha";
    }
}

BlendMode stringToBlendModeLocal(const juce::String& str)
{
    if (str == "additive") return BlendMode::Additive;
    if (str == "multiply") return BlendMode::Multiply;
    if (str == "screen")   return BlendMode::Screen;
    return BlendMode::Alpha;
}

juce::String colorToHexLocal(juce::Colour colour)
{
    return juce::String::toHexString(static_cast<int>(colour.getARGB()));
}

juce::Colour hexToColorLocal(const juce::String& hex)
{
    return juce::Colour(static_cast<juce::uint32>(hex.getHexValue64()));
}

// ============================================================================
// Validation Helper Functions
// ============================================================================

/**
 * Clamp and validate a float value, logging if out of range
 */
float clampAndValidateFloat(float value, float minVal, float maxVal, float defaultVal, const char* fieldName)
{
    if (std::isnan(value) || std::isinf(value))
    {
        juce::Logger::writeToLog(juce::String("VisualConfiguration: Invalid ") + fieldName +
                                 " (nan/inf), using default " + juce::String(defaultVal));
        return defaultVal;
    }
    if (value < minVal || value > maxVal)
    {
        juce::Logger::writeToLog(juce::String("VisualConfiguration: ") + fieldName + " " +
                                 juce::String(value) + " out of range [" +
                                 juce::String(minVal) + "," + juce::String(maxVal) + "], clamping");
        return std::clamp(value, minVal, maxVal);
    }
    return value;
}

/**
 * Clamp and validate an int value, logging if out of range
 */
int clampAndValidateInt(int value, int minVal, int maxVal, int defaultVal, const char* fieldName)
{
    if (value < minVal || value > maxVal)
    {
        juce::Logger::writeToLog(juce::String("VisualConfiguration: ") + fieldName + " " +
                                 juce::String(value) + " out of range [" +
                                 juce::String(minVal) + "," + juce::String(maxVal) + "], clamping");
        return std::clamp(value, minVal, maxVal);
    }
    return value;
}

} // anonymous namespace

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

    // 3D Settings
    tree.addChild(Settings3DSerialization::toValueTree(settings3D), -1, nullptr);

    // Lighting
    tree.addChild(Settings3DSerialization::toValueTree(lighting), -1, nullptr);

    // Material
    tree.addChild(Settings3DSerialization::toValueTree(material), -1, nullptr);

    return tree;
}

VisualConfiguration VisualConfiguration::fromValueTree(const juce::ValueTree& tree)
{
    VisualConfiguration config;

    if (!tree.hasType(VISUAL_CONFIG_TYPE))
    {
        juce::Logger::writeToLog("VisualConfiguration::fromValueTree: Invalid tree type, expected VisualConfiguration");
        return config;
    }

    // Shader type
    config.shaderType = idToShaderType(tree.getProperty("shaderType", "basic"));

    // Compositing - validate blend mode enum
    int blendModeVal = static_cast<int>(tree.getProperty("compositeBlendMode", 0));
    if (blendModeVal < 0 || blendModeVal > static_cast<int>(BlendMode::Screen))
    {
        juce::Logger::writeToLog("VisualConfiguration: Invalid compositeBlendMode " +
                                 juce::String(blendModeVal) + ", using Alpha");
        blendModeVal = static_cast<int>(BlendMode::Alpha);
    }
    config.compositeBlendMode = static_cast<BlendMode>(blendModeVal);

    // Validate compositeOpacity (0.0 to 1.0)
    float opacity = tree.getProperty("compositeOpacity", 1.0f);
    config.compositeOpacity = clampAndValidateFloat(opacity, 0.0f, 1.0f, 1.0f, "compositeOpacity");

    // Bloom
    auto bloomTree = tree.getChildWithName(BLOOM_TYPE);
    if (bloomTree.isValid())
    {
        config.bloom.enabled = bloomTree.getProperty("enabled", false);
        config.bloom.intensity = clampAndValidateFloat(
            bloomTree.getProperty("intensity", 1.0f), 0.0f, 10.0f, 1.0f, "bloom.intensity");
        config.bloom.threshold = clampAndValidateFloat(
            bloomTree.getProperty("threshold", 0.8f), 0.0f, 2.0f, 0.8f, "bloom.threshold");
        config.bloom.iterations = clampAndValidateInt(
            bloomTree.getProperty("iterations", 4), 1, 16, 4, "bloom.iterations");
        config.bloom.downsampleSteps = clampAndValidateInt(
            bloomTree.getProperty("downsampleSteps", 6), 1, 10, 6, "bloom.downsampleSteps");
        config.bloom.spread = clampAndValidateFloat(
            bloomTree.getProperty("spread", 1.0f), 0.1f, 5.0f, 1.0f, "bloom.spread");
        config.bloom.softKnee = clampAndValidateFloat(
            bloomTree.getProperty("softKnee", 0.5f), 0.0f, 1.0f, 0.5f, "bloom.softKnee");
    }

    // Radial Blur
    auto radialBlurTree = tree.getChildWithName(RADIALBLUR_TYPE);
    if (radialBlurTree.isValid())
    {
        config.radialBlur.enabled = radialBlurTree.getProperty("enabled", false);
        config.radialBlur.amount = clampAndValidateFloat(
            radialBlurTree.getProperty("amount", 0.1f), 0.0f, 1.0f, 0.1f, "radialBlur.amount");
        config.radialBlur.glow = clampAndValidateFloat(
            radialBlurTree.getProperty("glow", 1.0f), 0.0f, 5.0f, 1.0f, "radialBlur.glow");
        config.radialBlur.samples = clampAndValidateInt(
            radialBlurTree.getProperty("samples", 4), 1, 32, 4, "radialBlur.samples");
    }

    // Trails
    auto trailsTree = tree.getChildWithName(TRAILS_TYPE);
    if (trailsTree.isValid())
    {
        config.trails.enabled = trailsTree.getProperty("enabled", false);
        config.trails.decay = clampAndValidateFloat(
            trailsTree.getProperty("decay", 0.1f), 0.0f, 1.0f, 0.1f, "trails.decay");
        config.trails.opacity = clampAndValidateFloat(
            trailsTree.getProperty("opacity", 0.8f), 0.0f, 1.0f, 0.8f, "trails.opacity");
    }

    // Color Grade
    auto colorGradeTree = tree.getChildWithName(COLORGRADE_TYPE);
    if (colorGradeTree.isValid())
    {
        config.colorGrade.enabled = colorGradeTree.getProperty("enabled", false);
        config.colorGrade.brightness = clampAndValidateFloat(
            colorGradeTree.getProperty("brightness", 0.0f), -1.0f, 1.0f, 0.0f, "colorGrade.brightness");
        config.colorGrade.contrast = clampAndValidateFloat(
            colorGradeTree.getProperty("contrast", 1.0f), 0.0f, 3.0f, 1.0f, "colorGrade.contrast");
        config.colorGrade.saturation = clampAndValidateFloat(
            colorGradeTree.getProperty("saturation", 1.0f), 0.0f, 3.0f, 1.0f, "colorGrade.saturation");
        config.colorGrade.temperature = clampAndValidateFloat(
            colorGradeTree.getProperty("temperature", 0.0f), -1.0f, 1.0f, 0.0f, "colorGrade.temperature");
        config.colorGrade.tint = clampAndValidateFloat(
            colorGradeTree.getProperty("tint", 0.0f), -1.0f, 1.0f, 0.0f, "colorGrade.tint");
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
        config.vignette.intensity = clampAndValidateFloat(
            vignetteTree.getProperty("intensity", 0.5f), 0.0f, 2.0f, 0.5f, "vignette.intensity");
        config.vignette.softness = clampAndValidateFloat(
            vignetteTree.getProperty("softness", 0.5f), 0.0f, 1.0f, 0.5f, "vignette.softness");
        config.vignette.colour = juce::Colour(static_cast<juce::uint32>(
            static_cast<juce::int64>(vignetteTree.getProperty("colour", static_cast<juce::int64>(0xFF000000)))));
    }

    // Film Grain
    auto grainTree = tree.getChildWithName(FILMGRAIN_TYPE);
    if (grainTree.isValid())
    {
        config.filmGrain.enabled = grainTree.getProperty("enabled", false);
        config.filmGrain.intensity = clampAndValidateFloat(
            grainTree.getProperty("intensity", 0.1f), 0.0f, 1.0f, 0.1f, "filmGrain.intensity");
        config.filmGrain.speed = clampAndValidateFloat(
            grainTree.getProperty("speed", 24.0f), 1.0f, 120.0f, 24.0f, "filmGrain.speed");
    }

    // Chromatic Aberration
    auto chromaticTree = tree.getChildWithName(CHROMATIC_TYPE);
    if (chromaticTree.isValid())
    {
        config.chromaticAberration.enabled = chromaticTree.getProperty("enabled", false);
        config.chromaticAberration.intensity = clampAndValidateFloat(
            chromaticTree.getProperty("intensity", 0.005f), 0.0f, 0.1f, 0.005f, "chromaticAberration.intensity");
    }

    // Scanlines
    auto scanlineTree = tree.getChildWithName(SCANLINES_TYPE);
    if (scanlineTree.isValid())
    {
        config.scanlines.enabled = scanlineTree.getProperty("enabled", false);
        config.scanlines.intensity = clampAndValidateFloat(
            scanlineTree.getProperty("intensity", 0.3f), 0.0f, 1.0f, 0.3f, "scanlines.intensity");
        config.scanlines.density = clampAndValidateFloat(
            scanlineTree.getProperty("density", 2.0f), 0.5f, 10.0f, 2.0f, "scanlines.density");
        config.scanlines.phosphorGlow = scanlineTree.getProperty("phosphorGlow", true);
    }

    // Distortion
    auto distortionTree = tree.getChildWithName(DISTORTION_TYPE);
    if (distortionTree.isValid())
    {
        config.distortion.enabled = distortionTree.getProperty("enabled", false);
        config.distortion.intensity = clampAndValidateFloat(
            distortionTree.getProperty("intensity", 0.0f), 0.0f, 1.0f, 0.0f, "distortion.intensity");
        config.distortion.frequency = clampAndValidateFloat(
            distortionTree.getProperty("frequency", 4.0f), 0.1f, 20.0f, 4.0f, "distortion.frequency");
        config.distortion.speed = clampAndValidateFloat(
            distortionTree.getProperty("speed", 1.0f), 0.0f, 10.0f, 1.0f, "distortion.speed");
    }

    // Tilt Shift
    auto tiltTree = tree.getChildWithName(TILTSHIFT_TYPE);
    if (tiltTree.isValid())
    {
        config.tiltShift.enabled = tiltTree.getProperty("enabled", false);
        config.tiltShift.position = clampAndValidateFloat(
            tiltTree.getProperty("position", 0.5f), 0.0f, 1.0f, 0.5f, "tiltShift.position");
        config.tiltShift.range = clampAndValidateFloat(
            tiltTree.getProperty("range", 0.3f), 0.0f, 1.0f, 0.3f, "tiltShift.range");
        config.tiltShift.blurRadius = clampAndValidateFloat(
            tiltTree.getProperty("blurRadius", 2.0f), 0.0f, 20.0f, 2.0f, "tiltShift.blurRadius");
    }

    // 3D Settings
    config.settings3D = Settings3DSerialization::fromValueTree(tree);

    // Lighting
    config.lighting = Settings3DSerialization::lightingFromValueTree(tree);

    // Material
    config.material = Settings3DSerialization::materialFromValueTree(tree);

    return config;
}

juce::var VisualConfiguration::toJson() const
{
    // Use DynamicObject::Ptr for exception-safe memory management
    juce::DynamicObject::Ptr obj(new juce::DynamicObject());
    obj->setProperty("shader_type", shaderTypeToId(shaderType));
    obj->setProperty("composite_blend_mode", blendModeToStringLocal(compositeBlendMode));
    obj->setProperty("composite_opacity", compositeOpacity);

    // Bloom
    auto bloomObj = new juce::DynamicObject();
    bloomObj->setProperty("enabled", bloom.enabled);
    bloomObj->setProperty("intensity", bloom.intensity);
    bloomObj->setProperty("threshold", bloom.threshold);
    bloomObj->setProperty("iterations", bloom.iterations);
    bloomObj->setProperty("downsample_steps", bloom.downsampleSteps);
    bloomObj->setProperty("spread", bloom.spread);
    bloomObj->setProperty("soft_knee", bloom.softKnee);
    obj->setProperty("bloom", juce::var(bloomObj));

    // Radial Blur
    auto radialBlurObj = new juce::DynamicObject();
    radialBlurObj->setProperty("enabled", radialBlur.enabled);
    radialBlurObj->setProperty("amount", radialBlur.amount);
    radialBlurObj->setProperty("glow", radialBlur.glow);
    radialBlurObj->setProperty("samples", radialBlur.samples);
    obj->setProperty("radial_blur", juce::var(radialBlurObj));

    // Trails
    auto trailsObj = new juce::DynamicObject();
    trailsObj->setProperty("enabled", trails.enabled);
    trailsObj->setProperty("decay", trails.decay);
    trailsObj->setProperty("opacity", trails.opacity);
    obj->setProperty("trails", juce::var(trailsObj));

    // Color Grade
    auto colorGradeObj = new juce::DynamicObject();
    colorGradeObj->setProperty("enabled", colorGrade.enabled);
    colorGradeObj->setProperty("brightness", colorGrade.brightness);
    colorGradeObj->setProperty("contrast", colorGrade.contrast);
    colorGradeObj->setProperty("saturation", colorGrade.saturation);
    colorGradeObj->setProperty("temperature", colorGrade.temperature);
    colorGradeObj->setProperty("tint", colorGrade.tint);
    colorGradeObj->setProperty("shadows", colorToHexLocal(colorGrade.shadows));
    colorGradeObj->setProperty("highlights", colorToHexLocal(colorGrade.highlights));
    obj->setProperty("color_grade", juce::var(colorGradeObj));

    // Vignette
    auto vignetteObj = new juce::DynamicObject();
    vignetteObj->setProperty("enabled", vignette.enabled);
    vignetteObj->setProperty("intensity", vignette.intensity);
    vignetteObj->setProperty("softness", vignette.softness);
    vignetteObj->setProperty("colour", colorToHexLocal(vignette.colour));
    obj->setProperty("vignette", juce::var(vignetteObj));

    // Film Grain
    auto filmGrainObj = new juce::DynamicObject();
    filmGrainObj->setProperty("enabled", filmGrain.enabled);
    filmGrainObj->setProperty("intensity", filmGrain.intensity);
    filmGrainObj->setProperty("speed", filmGrain.speed);
    obj->setProperty("film_grain", juce::var(filmGrainObj));

    // Chromatic Aberration
    auto chromaticAberrationObj = new juce::DynamicObject();
    chromaticAberrationObj->setProperty("enabled", chromaticAberration.enabled);
    chromaticAberrationObj->setProperty("intensity", chromaticAberration.intensity);
    obj->setProperty("chromatic_aberration", juce::var(chromaticAberrationObj));

    // Scanlines
    auto scanlinesObj = new juce::DynamicObject();
    scanlinesObj->setProperty("enabled", scanlines.enabled);
    scanlinesObj->setProperty("intensity", scanlines.intensity);
    scanlinesObj->setProperty("density", scanlines.density);
    scanlinesObj->setProperty("phosphor_glow", scanlines.phosphorGlow);
    obj->setProperty("scanlines", juce::var(scanlinesObj));

    // Distortion
    auto distortionObj = new juce::DynamicObject();
    distortionObj->setProperty("enabled", distortion.enabled);
    distortionObj->setProperty("intensity", distortion.intensity);
    distortionObj->setProperty("frequency", distortion.frequency);
    distortionObj->setProperty("speed", distortion.speed);
    obj->setProperty("distortion", juce::var(distortionObj));

    // Tilt Shift
    auto tiltShiftObj = new juce::DynamicObject();
    tiltShiftObj->setProperty("enabled", tiltShift.enabled);
    tiltShiftObj->setProperty("position", tiltShift.position);
    tiltShiftObj->setProperty("range", tiltShift.range);
    tiltShiftObj->setProperty("blur_radius", tiltShift.blurRadius);
    tiltShiftObj->setProperty("iterations", tiltShift.iterations);
    obj->setProperty("tilt_shift", juce::var(tiltShiftObj));

    // 3D Settings
    obj->setProperty("settings_3d", Settings3DSerialization::toJson(settings3D));

    // Material
    obj->setProperty("material", Settings3DSerialization::toJson(material));

    // Lighting
    obj->setProperty("lighting", Settings3DSerialization::toJson(lighting));

    return juce::var(obj.get());
}

VisualConfiguration VisualConfiguration::fromJson(const juce::var& json)
{
    VisualConfiguration config;
    auto* obj = json.getDynamicObject();
    if (!obj)
    {
        juce::Logger::writeToLog("VisualConfiguration::fromJson: Invalid JSON - not a dynamic object");
        return config;
    }

    config.shaderType = idToShaderType(obj->getProperty("shader_type").toString());
    config.compositeBlendMode = stringToBlendModeLocal(obj->getProperty("composite_blend_mode").toString());
    config.compositeOpacity = clampAndValidateFloat(
        static_cast<float>(static_cast<double>(obj->getProperty("composite_opacity"))),
        0.0f, 1.0f, 1.0f, "compositeOpacity");

    // Bloom
    if (auto* bloomObj = obj->getProperty("bloom").getDynamicObject())
    {
        config.bloom.enabled = bloomObj->getProperty("enabled");
        config.bloom.intensity = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(bloomObj->getProperty("intensity"))),
            0.0f, 10.0f, 1.0f, "bloom.intensity");
        config.bloom.threshold = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(bloomObj->getProperty("threshold"))),
            0.0f, 2.0f, 0.8f, "bloom.threshold");
        config.bloom.iterations = clampAndValidateInt(
            static_cast<int>(bloomObj->getProperty("iterations")), 1, 16, 4, "bloom.iterations");
        config.bloom.downsampleSteps = clampAndValidateInt(
            static_cast<int>(bloomObj->getProperty("downsample_steps")), 1, 10, 6, "bloom.downsampleSteps");
        config.bloom.spread = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(bloomObj->getProperty("spread"))),
            0.1f, 5.0f, 1.0f, "bloom.spread");
        config.bloom.softKnee = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(bloomObj->getProperty("soft_knee"))),
            0.0f, 1.0f, 0.5f, "bloom.softKnee");
    }

    // Radial Blur
    if (auto* radialBlurObj = obj->getProperty("radial_blur").getDynamicObject())
    {
        config.radialBlur.enabled = radialBlurObj->getProperty("enabled");
        config.radialBlur.amount = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(radialBlurObj->getProperty("amount"))),
            0.0f, 1.0f, 0.1f, "radialBlur.amount");
        config.radialBlur.glow = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(radialBlurObj->getProperty("glow"))),
            0.0f, 5.0f, 1.0f, "radialBlur.glow");
        config.radialBlur.samples = clampAndValidateInt(
            static_cast<int>(radialBlurObj->getProperty("samples")), 1, 32, 4, "radialBlur.samples");
    }

    // Trails
    if (auto* trailsObj = obj->getProperty("trails").getDynamicObject())
    {
        config.trails.enabled = trailsObj->getProperty("enabled");
        config.trails.decay = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(trailsObj->getProperty("decay"))),
            0.0f, 1.0f, 0.1f, "trails.decay");
        config.trails.opacity = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(trailsObj->getProperty("opacity"))),
            0.0f, 1.0f, 0.8f, "trails.opacity");
    }

    // Color Grade
    if (auto* colorGradeObj = obj->getProperty("color_grade").getDynamicObject())
    {
        config.colorGrade.enabled = colorGradeObj->getProperty("enabled");
        config.colorGrade.brightness = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(colorGradeObj->getProperty("brightness"))),
            -1.0f, 1.0f, 0.0f, "colorGrade.brightness");
        config.colorGrade.contrast = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(colorGradeObj->getProperty("contrast"))),
            0.0f, 3.0f, 1.0f, "colorGrade.contrast");
        config.colorGrade.saturation = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(colorGradeObj->getProperty("saturation"))),
            0.0f, 3.0f, 1.0f, "colorGrade.saturation");
        config.colorGrade.temperature = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(colorGradeObj->getProperty("temperature"))),
            -1.0f, 1.0f, 0.0f, "colorGrade.temperature");
        config.colorGrade.tint = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(colorGradeObj->getProperty("tint"))),
            -1.0f, 1.0f, 0.0f, "colorGrade.tint");
        config.colorGrade.shadows = hexToColorLocal(colorGradeObj->getProperty("shadows").toString());
        config.colorGrade.highlights = hexToColorLocal(colorGradeObj->getProperty("highlights").toString());
    }

    // Vignette
    if (auto* vignetteObj = obj->getProperty("vignette").getDynamicObject())
    {
        config.vignette.enabled = vignetteObj->getProperty("enabled");
        config.vignette.intensity = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(vignetteObj->getProperty("intensity"))),
            0.0f, 2.0f, 0.5f, "vignette.intensity");
        config.vignette.softness = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(vignetteObj->getProperty("softness"))),
            0.0f, 1.0f, 0.5f, "vignette.softness");
        config.vignette.colour = hexToColorLocal(vignetteObj->getProperty("colour").toString());
    }

    // Film Grain
    if (auto* filmGrainObj = obj->getProperty("film_grain").getDynamicObject())
    {
        config.filmGrain.enabled = filmGrainObj->getProperty("enabled");
        config.filmGrain.intensity = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(filmGrainObj->getProperty("intensity"))),
            0.0f, 1.0f, 0.1f, "filmGrain.intensity");
        config.filmGrain.speed = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(filmGrainObj->getProperty("speed"))),
            1.0f, 120.0f, 24.0f, "filmGrain.speed");
    }

    // Chromatic Aberration
    if (auto* chromaticAberrationObj = obj->getProperty("chromatic_aberration").getDynamicObject())
    {
        config.chromaticAberration.enabled = chromaticAberrationObj->getProperty("enabled");
        config.chromaticAberration.intensity = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(chromaticAberrationObj->getProperty("intensity"))),
            0.0f, 0.1f, 0.005f, "chromaticAberration.intensity");
    }

    // Scanlines
    if (auto* scanlinesObj = obj->getProperty("scanlines").getDynamicObject())
    {
        config.scanlines.enabled = scanlinesObj->getProperty("enabled");
        config.scanlines.intensity = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(scanlinesObj->getProperty("intensity"))),
            0.0f, 1.0f, 0.3f, "scanlines.intensity");
        config.scanlines.density = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(scanlinesObj->getProperty("density"))),
            0.5f, 10.0f, 2.0f, "scanlines.density");
        config.scanlines.phosphorGlow = scanlinesObj->getProperty("phosphor_glow");
    }

    // Distortion
    if (auto* distortionObj = obj->getProperty("distortion").getDynamicObject())
    {
        config.distortion.enabled = distortionObj->getProperty("enabled");
        config.distortion.intensity = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(distortionObj->getProperty("intensity"))),
            0.0f, 1.0f, 0.0f, "distortion.intensity");
        config.distortion.frequency = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(distortionObj->getProperty("frequency"))),
            0.1f, 20.0f, 4.0f, "distortion.frequency");
        config.distortion.speed = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(distortionObj->getProperty("speed"))),
            0.0f, 10.0f, 1.0f, "distortion.speed");
    }

    // Tilt Shift
    if (auto* tiltShiftObj = obj->getProperty("tilt_shift").getDynamicObject())
    {
        config.tiltShift.enabled = tiltShiftObj->getProperty("enabled");
        config.tiltShift.position = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(tiltShiftObj->getProperty("position"))),
            0.0f, 1.0f, 0.5f, "tiltShift.position");
        config.tiltShift.range = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(tiltShiftObj->getProperty("range"))),
            0.0f, 1.0f, 0.3f, "tiltShift.range");
        config.tiltShift.blurRadius = clampAndValidateFloat(
            static_cast<float>(static_cast<double>(tiltShiftObj->getProperty("blur_radius"))),
            0.0f, 20.0f, 2.0f, "tiltShift.blurRadius");
        config.tiltShift.iterations = clampAndValidateInt(
            static_cast<int>(tiltShiftObj->getProperty("iterations")), 1, 16, 4, "tiltShift.iterations");
    }

    // 3D Settings
    config.settings3D = Settings3DSerialization::fromJson(json);

    // Material
    config.material = Settings3DSerialization::materialFromJson(json);

    // Lighting
    config.lighting = Settings3DSerialization::lightingFromJson(json);

    return config;
}

} // namespace oscil
