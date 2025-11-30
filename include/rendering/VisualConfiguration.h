/*
    Oscil - Visual Configuration
    Complete per-waveform visual settings for the render engine
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <vector>
#include <utility>

namespace oscil
{

// ============================================================================
// Shader Type Enumeration
// ============================================================================

/**
 * Available shader types for waveform rendering.
 */
enum class ShaderType
{
    // 2D Shaders
    Basic2D,
    NeonGlow,
    GradientFill,
    DualOutline,
    PlasmaSine,
    DigitalGlitch,

    // 3D Shaders
    VolumetricRibbon,
    WireframeMesh,
    VectorFlow,
    StringTheory,
    ElectricFlower,

    // Material Shaders
    GlassRefraction,
    LiquidChrome,
    Crystalline
};

/**
 * Blend modes for compositing.
 */
enum class BlendMode
{
    Alpha,      // Standard alpha blending
    Additive,   // Add colors (glow effect)
    Multiply,   // Darken
    Screen      // Lighten
};

// ============================================================================
// Post-Processing Settings Structures
// ============================================================================

/**
 * Bloom effect settings.
 */
struct BloomSettings
{
    bool enabled = false;
    float intensity = 1.0f;      // 0.0 - 2.0
    float threshold = 0.8f;      // Brightness threshold for bloom
    int iterations = 4;          // Blur iterations (2-8)
    float spread = 1.0f;         // Blur spread multiplier
    float softKnee = 0.5f;       // Soft threshold transition (0.0 = hard, 1.0 = very soft)
};

/**
 * Radial blur/zoom glow effect settings.
 */
struct RadialBlurSettings
{
    bool enabled = false;
    float amount = 0.1f;         // Zoom amount (0.0 - 0.5)
    float glow = 1.0f;           // Glow intensity multiplier
    int samples = 4;             // Number of zoom samples (2-8)
};

/**
 * Trail/persistence effect settings.
 */
struct TrailSettings
{
    bool enabled = false;
    float decay = 0.1f;          // How quickly trails fade (0.01-0.5)
    float opacity = 0.8f;        // Trail opacity
};

/**
 * Color grading settings.
 */
struct ColorGradeSettings
{
    bool enabled = false;
    float brightness = 0.0f;     // -1.0 to 1.0
    float contrast = 1.0f;       // 0.5 to 2.0
    float saturation = 1.0f;     // 0.0 to 2.0
    float temperature = 0.0f;    // -1.0 (cool) to 1.0 (warm)
    float tint = 0.0f;           // -1.0 (green) to 1.0 (magenta)
    juce::Colour shadows{0xFF000000};
    juce::Colour highlights{0xFFFFFFFF};
};

/**
 * Vignette effect settings.
 */
struct VignetteSettings
{
    bool enabled = false;
    float intensity = 0.5f;      // 0.0 to 1.0
    float softness = 0.5f;       // Edge softness
    juce::Colour colour{0xFF000000}; // Usually black
};

/**
 * Film grain effect settings.
 */
struct FilmGrainSettings
{
    bool enabled = false;
    float intensity = 0.1f;      // 0.0 to 0.5
    float speed = 24.0f;         // Animation speed (FPS)
};

/**
 * Chromatic aberration settings.
 */
struct ChromaticAberrationSettings
{
    bool enabled = false;
    float intensity = 0.005f;    // RGB channel offset (0.0 to 0.02)
};

/**
 * CRT scanline effect settings.
 */
struct ScanlineSettings
{
    bool enabled = false;
    float intensity = 0.3f;      // Line darkness
    float density = 2.0f;        // Lines per pixel
    bool phosphorGlow = true;    // Add subtle glow between lines
};

/**
 * Distortion effect settings.
 */
struct DistortionSettings
{
    bool enabled = false;
    float intensity = 0.0f;      // Wave distortion amount
    float frequency = 4.0f;      // Wave frequency
    float speed = 1.0f;          // Animation speed
};

// ============================================================================
// Particle System Settings
// ============================================================================

/**
 * Particle emission modes.
 */
enum class ParticleEmissionMode
{
    AlongWaveform,       // Emit uniformly along waveform path
    AtPeaks,             // Emit at amplitude peaks
    AtZeroCrossings,     // Emit at zero crossings
    Continuous,          // Emit from center regardless of waveform
    Burst                // Emit all at once on trigger
};

/**
 * Particle blend modes.
 */
enum class ParticleBlendMode
{
    Additive,            // Glow effect
    Alpha,               // Standard transparency
    Multiply,            // Darken
    Screen               // Lighten
};

/**
 * Particle system settings for a waveform.
 */
struct ParticleSettings
{
    bool enabled = false;
    ParticleEmissionMode emissionMode = ParticleEmissionMode::AlongWaveform;
    float emissionRate = 100.0f;           // Particles per second
    float particleLife = 2.0f;             // Seconds
    float particleSize = 4.0f;             // Pixels
    juce::Colour particleColor{0xFFFFAA00};
    ParticleBlendMode blendMode = ParticleBlendMode::Additive;

    // Physics
    float gravity = 0.0f;
    float drag = 0.1f;
    float randomness = 0.5f;
    float velocityScale = 1.0f;

    // Audio reactivity
    bool audioReactive = true;
    float audioEmissionBoost = 2.0f;       // Multiplier on transients
};

// ============================================================================
// 3D Rendering Settings
// ============================================================================

/**
 * 3D visualization settings.
 */
struct Settings3D
{
    bool enabled = false;
    float cameraDistance = 5.0f;
    float cameraAngleX = 15.0f;            // Degrees (pitch)
    float cameraAngleY = 0.0f;             // Degrees (yaw)
    bool autoRotate = false;
    float rotateSpeed = 10.0f;             // Degrees per second

    // For mesh-based shaders
    int meshResolutionX = 128;
    int meshResolutionZ = 32;              // History depth
    float meshScale = 1.0f;
};

// ============================================================================
// Material Settings
// ============================================================================

/**
 * Material settings for advanced shaders.
 */
struct MaterialSettings
{
    bool enabled = false;
    float reflectivity = 0.5f;
    float refractiveIndex = 1.5f;          // Glass = 1.5, Water = 1.33
    float fresnelPower = 2.0f;
    juce::Colour tintColor{0xFFFFFFFF};
    float roughness = 0.1f;

    // Environment
    bool useEnvironmentMap = true;
    juce::String environmentMapId = "default_studio";
};

// ============================================================================
// Complete Visual Configuration
// ============================================================================

/**
 * Complete visual configuration for a single waveform.
 * Contains all shader, post-processing, particle, 3D, and material settings.
 */
struct VisualConfiguration
{
    // Shader selection
    ShaderType shaderType = ShaderType::Basic2D;

    // Post-processing
    BloomSettings bloom;
    RadialBlurSettings radialBlur;
    TrailSettings trails;
    ColorGradeSettings colorGrade;
    VignetteSettings vignette;
    FilmGrainSettings filmGrain;
    ChromaticAberrationSettings chromaticAberration;
    ScanlineSettings scanlines;
    DistortionSettings distortion;

    // Particle system
    ParticleSettings particles;

    // 3D rendering
    Settings3D settings3D;

    // Material properties
    MaterialSettings material;

    // Compositing
    BlendMode compositeBlendMode = BlendMode::Alpha;
    float compositeOpacity = 1.0f;

    // Preset identifier (for tracking which preset was applied)
    juce::String presetId = "default";

    /**
     * Serialize to ValueTree for persistence.
     */
    juce::ValueTree toValueTree() const;

    /**
     * Deserialize from ValueTree.
     */
    static VisualConfiguration fromValueTree(const juce::ValueTree& tree);

    /**
     * Check if this configuration requires 3D rendering.
     */
    [[nodiscard]] bool requires3D() const;

    /**
     * Check if this configuration has any enabled post-processing effects.
     */
    [[nodiscard]] bool hasPostProcessing() const;

    /**
     * Check if this configuration requires the particle system.
     */
    [[nodiscard]] bool hasParticles() const;

    /**
     * Get a default configuration preset.
     */
    static VisualConfiguration getDefault();

    /**
     * Get a preset by name (e.g., "neon", "retro", "minimal").
     */
    static VisualConfiguration getPreset(const juce::String& presetName);

    /**
     * Get list of available preset IDs and display names.
     * Returns pairs of (presetId, displayName).
     */
    static std::vector<std::pair<juce::String, juce::String>> getAvailablePresets();
};

// ============================================================================
// Shader Type Utilities
// ============================================================================

/**
 * Check if a shader type is a 3D shader.
 */
inline bool is3DShader(ShaderType type)
{
    return type == ShaderType::VolumetricRibbon ||
           type == ShaderType::WireframeMesh ||
           type == ShaderType::VectorFlow ||
           type == ShaderType::StringTheory ||
           type == ShaderType::ElectricFlower ||
           // Material shaders are also 3D shaders
           type == ShaderType::GlassRefraction ||
           type == ShaderType::LiquidChrome ||
           type == ShaderType::Crystalline;
}

/**
 * Check if a shader type is a material shader.
 */
inline bool isMaterialShader(ShaderType type)
{
    return type == ShaderType::GlassRefraction ||
           type == ShaderType::LiquidChrome ||
           type == ShaderType::Crystalline;
}

/**
 * Convert ShaderType to string ID for registry lookup.
 */
juce::String shaderTypeToId(ShaderType type);

/**
 * Convert string ID to ShaderType.
 */
ShaderType idToShaderType(const juce::String& id);

} // namespace oscil
