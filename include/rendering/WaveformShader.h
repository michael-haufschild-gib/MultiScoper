/*
    Oscil - Waveform Shader Base Class
    Abstract base for GPU-accelerated waveform rendering shaders
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#if OSCIL_ENABLE_OPENGL
    #include <juce_opengl/juce_opengl.h>
#endif
#include <vector>

namespace oscil
{

/**
 * Information about a shader for UI display
 */
struct ShaderInfo
{
    juce::String id;          // Unique identifier (e.g., "basic")
    juce::String displayName; // Human-readable name (e.g., "Basic")
    juce::String description; // Brief description for tooltips
};

/**
 * Parameters passed to shader for rendering
 */
struct ShaderRenderParams
{
    juce::Colour colour;
    float opacity = 1.0f;
    float lineWidth = 1.5f;
    juce::Rectangle<float> bounds;
    bool isStereo = false;
    float verticalScale = 1.0f;   // Vertical scale factor (includes auto-scale)
    float time = 0.0f;            // Time in seconds for animation
    float shaderIntensity = 1.0f; // Intensity multiplier for shader effects (e.g. glow)
};

/**
 * Abstract base class for waveform rendering shaders.
 * Derive from this class to create custom GPU-accelerated waveform visualizations.
 */
class WaveformShader
{
public:
    virtual ~WaveformShader() = default;

    // Shader identification
    [[nodiscard]] virtual juce::String getId() const = 0;
    [[nodiscard]] virtual juce::String getDisplayName() const = 0;
    [[nodiscard]] virtual juce::String getDescription() const = 0;

    /**
     * Get shader info for UI display
     */
    [[nodiscard]] ShaderInfo getInfo() const
    {
        return {.id = getId(), .displayName = getDisplayName(), .description = getDescription()};
    }

#if OSCIL_ENABLE_OPENGL
    /**
     * Compile the shader program
     * @param context The OpenGL context to compile for
     * @return true if compilation succeeded
     */
    virtual bool compile(juce::OpenGLContext& context) = 0;

    /**
     * Release OpenGL resources
     * @param context The OpenGL context (must be active when called)
     */
    virtual void release(juce::OpenGLContext& context) = 0;

    /**
     * Check if shader is compiled and ready to use
     */
    [[nodiscard]] virtual bool isCompiled() const = 0;

    /**
     * Render waveform samples using this shader
     * @param context The OpenGL context
     * @param channel1 First channel samples (or mono)
     * @param channel2 Second channel samples (nullptr for mono)
     * @param params Rendering parameters (colour, bounds, etc.)
     */
    virtual void render(juce::OpenGLContext& context, const std::vector<float>& channel1,
                        const std::vector<float>* channel2, const ShaderRenderParams& params) = 0;
#endif

    /**
     * Fallback software rendering when OpenGL is not available
     * Default implementation draws a simple line path
     */
    virtual void renderSoftware(juce::Graphics& g, const std::vector<float>& channel1,
                                const std::vector<float>* channel2, const ShaderRenderParams& params);

protected:
    WaveformShader() = default;

#if OSCIL_ENABLE_OPENGL
    /**
     * Common OpenGL resources shared by all waveform shaders.
     * Shader-specific subclasses extend this with extra uniform locations.
     */
    struct WaveformGLResources
    {
        std::unique_ptr<juce::OpenGLShaderProgram> program;
        GLuint vao = 0;
        GLuint vbo = 0;
        bool compiled = false;

        GLint projectionLoc = -1;
        GLint baseColorLoc = -1;
        GLint opacityLoc = -1;
    };

    /**
     * Compile a waveform shader from BinaryData resources.
     * Loads vertex/fragment source from BinaryData, compiles, links,
     * resolves common uniforms (projection, baseColor, opacity), and generates VAO/VBO.
     * @return true if successful; on failure, gl is left in uncompiled state.
     */
    static bool compileFromBinaryData(WaveformGLResources& gl, juce::OpenGLContext& context, const char* vertData,
                                      int vertSize, const char* fragData, int fragSize, const juce::String& shaderName);

    /**
     * Release common GL resources (VAO, VBO, program).
     * Safe to call even if not compiled.
     */
    static void releaseGLResources(WaveformGLResources& gl);

    /**
     * Helper to compile vertex and fragment shaders
     * @return true if successful
     */
    static bool compileShaderProgram(juce::OpenGLShaderProgram& program, const char* vertexSource,
                                     const char* fragmentSource);

    /**
     * Helper to build a triangle strip for line rendering
     * Creates anti-aliased line geometry from sample points
     * @param boundsX X offset for screen-space coordinates (bounds.getX())
     */
    static void buildLineGeometry(std::vector<float>& vertices, const std::vector<float>& samples, float centerY,
                                  float amplitude, float lineWidth, float boundsX, float boundsWidth);

    /**
     * Helper to build a triangle strip for filling the area under the waveform.
     * Creates geometry extending from the waveform line down to y=0 (relative to center).
     */
    static void buildFillGeometry(std::vector<float>& vertices, const std::vector<float>& samples, float centerY,
                                  float zeroY, // The Y coordinate to fill down/up to
                                  float amplitude, float boundsX, float boundsWidth);

    /**
     * Helper to check and log OpenGL errors
     * @param location Description of where the check is being made
     * @return true if no errors were found
     */
    static bool checkGLError(const char* location);

    /**
     * Helper to compute and set orthographic 2D projection matrix.
     * @return false if the component has zero dimensions (skip rendering)
     */
    static bool setup2DProjection(juce::OpenGLContext& context, juce::OpenGLExtensionFunctions& ext,
                                  GLint projectionLoc);

#endif

    /**
     * Calculate stereo/mono layout positions for channel rendering.
     */
    static void calculateStereoLayout(const ShaderRenderParams& params, const std::vector<float>* channel2,
                                      float height, float& centerY1, float& centerY2, float& amp1, float& amp2);
};

} // namespace oscil
