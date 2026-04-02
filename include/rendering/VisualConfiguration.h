/*
    Oscil - Visual Configuration
    Complete per-waveform visual settings for the render engine
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_graphics/juce_graphics.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace oscil
{

// ============================================================================
// Shader Type Enumeration
// ============================================================================

/**
 * Available shader types for waveform rendering.
 */
enum class ShaderType : std::uint8_t
{
    Basic2D,
    NeonGlow,
    GradientFill,
    DualOutline
};

/**
 * Blend modes for compositing.
 */
enum class BlendMode : std::uint8_t
{
    Alpha,    // Standard alpha blending
    Additive, // Add colors (glow effect)
    Multiply, // Darken
    Screen    // Lighten
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
    float intensity = 1.0f;  // 0.0 - 2.0
    float threshold = 0.8f;  // Brightness threshold for bloom
    int iterations = 4;      // Blur iterations (2-8)
    int downsampleSteps = 6; // Number of Mip levels (2-8)
    float spread = 1.0f;     // Blur spread multiplier
    float softKnee = 0.5f;   // Soft threshold transition (0.0 = hard, 1.0 = very soft)
};

/**
 * Radial blur/zoom glow effect settings.
 */
struct RadialBlurSettings
{
    bool enabled = false;
    float amount = 0.1f; // Zoom amount (0.0 - 0.5)
    float glow = 1.0f;   // Glow intensity multiplier
    int samples = 4;     // Number of zoom samples (2-8)
};

/**
 * Trail/persistence effect settings.
 */
struct TrailSettings
{
    bool enabled = false;
    float decay = 0.1f;   // How quickly trails fade (0.01-0.5)
    float opacity = 0.8f; // Trail opacity
};

/**
 * Color grading settings.
 */
struct ColorGradeSettings
{
    bool enabled = false;
    float brightness = 0.0f;  // -1.0 to 1.0
    float contrast = 1.0f;    // 0.5 to 2.0
    float saturation = 1.0f;  // 0.0 to 2.0
    float temperature = 0.0f; // -1.0 (cool) to 1.0 (warm)
    float tint = 0.0f;        // -1.0 (green) to 1.0 (magenta)
    juce::Colour shadows{0xFF000000};
    juce::Colour highlights{0xFFFFFFFF};
};

/**
 * Vignette effect settings.
 */
struct VignetteSettings
{
    bool enabled = false;
    float intensity = 0.5f;          // 0.0 to 1.0
    float softness = 0.5f;           // Edge softness
    juce::Colour colour{0xFF000000}; // Usually black
};

/**
 * Film grain effect settings.
 */
struct FilmGrainSettings
{
    bool enabled = false;
    float intensity = 0.1f; // 0.0 to 0.5
    float speed = 24.0f;    // Animation speed (FPS)
};

/**
 * Chromatic aberration settings.
 */
struct ChromaticAberrationSettings
{
    bool enabled = false;
    float intensity = 0.005f; // RGB channel offset (0.0 to 0.02)
};

/**
 * CRT scanline effect settings.
 */
struct ScanlineSettings
{
    bool enabled = false;
    float intensity = 0.3f;   // Line darkness
    float density = 2.0f;     // Lines per pixel
    bool phosphorGlow = true; // Add subtle glow between lines
};

/**
 * Tilt Shift (Fake Depth of Field) settings.
 */
struct TiltShiftSettings
{
    bool enabled = false;
    float position = 0.5f;   // Center position (0.0 - 1.0)
    float range = 0.3f;      // In-focus range width (0.0 - 1.0)
    float blurRadius = 2.0f; // Blur amount
    int iterations = 3;      // Blur quality
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
    TiltShiftSettings tiltShift;

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
     * Check if this configuration has any enabled post-processing effects.
     */
    [[nodiscard]] bool hasPostProcessing() const;

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

private:
    static void setupVectorScopePreset(VisualConfiguration& c);

public:
    /**
     * Clamp all settings to their documented valid ranges.
     * Call after deserialization or user input to enforce invariants.
     */
    void validate();
};

// ============================================================================
// Shader Type Utilities
// ============================================================================

/**
 * Convert ShaderType to string ID for registry lookup.
 */
juce::String shaderTypeToId(ShaderType type);

/**
 * Convert string ID to ShaderType.
 */
ShaderType idToShaderType(const juce::String& id);

} // namespace oscil
