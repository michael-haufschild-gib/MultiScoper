/*
    Oscil - Environment Map Manager
    Manages procedural cubemap generation for reflections
*/

#pragma once

#include <juce_opengl/juce_opengl.h>
#include <string>
#include <unordered_map>
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Preset environment map styles.
 */
enum class EnvironmentPreset
{
    Dark,           // Dark space with subtle gradients
    Neon,           // Vibrant neon colors
    Studio,         // Soft studio lighting
    Sunset,         // Warm sunset gradient
    CyberSpace,     // Digital/cyber aesthetic
    Abstract        // Abstract color patterns
};

/**
 * Configuration for procedural environment map generation.
 */
struct EnvironmentConfig
{
    // Colors
    float skyColorTopR = 0.1f, skyColorTopG = 0.1f, skyColorTopB = 0.2f;
    float skyColorBottomR = 0.02f, skyColorBottomG = 0.02f, skyColorBottomB = 0.05f;
    float horizonColorR = 0.15f, horizonColorG = 0.1f, horizonColorB = 0.2f;

    // Gradient controls
    float horizonSharpness = 2.0f;
    float skyGradientPower = 1.0f;

    // Optional accent lights/colors
    float accentColorR = 0.0f, accentColorG = 0.8f, accentColorB = 1.0f;
    float accentIntensity = 0.3f;
    float accentAngle = 45.0f;  // Degrees from horizon

    // Resolution
    int resolution = 256;  // Cubemap face resolution

    /**
     * Create config from preset.
     */
    static EnvironmentConfig fromPreset(EnvironmentPreset preset);
};

/**
 * Manages environment cubemaps for material reflections.
 * Generates procedural cubemaps or can load external HDR images.
 */
class EnvironmentMapManager
{
public:
    EnvironmentMapManager();
    ~EnvironmentMapManager();

    // Non-copyable
    EnvironmentMapManager(const EnvironmentMapManager&) = delete;
    EnvironmentMapManager& operator=(const EnvironmentMapManager&) = delete;

    /**
     * Initialize OpenGL resources.
     */
    bool initialize(juce::OpenGLContext& context);

    /**
     * Release OpenGL resources.
     */
    void release(juce::OpenGLContext& context);

    /**
     * Check if initialized.
     */
    [[nodiscard]] bool isInitialized() const { return initialized_; }

    /**
     * Create a procedural environment map.
     * @param name Unique name for the map
     * @param config Configuration for generation
     * @return OpenGL texture ID, or 0 on failure
     */
    GLuint createProceduralMap(const juce::String& name,
                                         const EnvironmentConfig& config);

    /**
     * Create environment map from preset.
     */
    GLuint createFromPreset(const juce::String& name,
                                      EnvironmentPreset preset);

    /**
     * Get environment map by name.
     * @return Texture ID, or 0 if not found
     */
    [[nodiscard]] GLuint getMap(const juce::String& name) const;

    /**
     * Check if map exists.
     */
    [[nodiscard]] bool hasMap(const juce::String& name) const;

    /**
     * Destroy a specific map.
     */
    void destroyMap(const juce::String& name);

    /**
     * Destroy all maps.
     */
    void destroyAllMaps();

    /**
     * Get the default environment map (creates if needed).
     */
    GLuint getDefaultMap();

private:
    /**
     * Generate cubemap face data.
     */
    void generateFaceData(std::vector<float>& data,
                          int face,
                          int resolution,
                          const EnvironmentConfig& config);

    /**
     * Get direction vector for cubemap face and UV.
     */
    static void getCubeDirection(int face, float u, float v,
                                  float& x, float& y, float& z);

    bool initialized_ = false;
    juce::OpenGLContext* context_ = nullptr;

    std::unordered_map<juce::String, GLuint> envMaps_;

    GLuint defaultMap_ = 0;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
