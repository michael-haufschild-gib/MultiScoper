/*
    Oscil - Instanced Waveform Shader
    Renders multiple waveforms with a single draw call using instancing (Tier 2)
*/

#pragma once

#include "rendering/VisualConfiguration.h"
#include "rendering/WaveformShader.h"

#if OSCIL_ENABLE_OPENGL
#include <juce_opengl/juce_opengl.h>

#include <array>
#include <vector>

namespace oscil
{

// Forward declarations
struct WaveformRenderData;
struct WaveformRenderState;

/**
 * Maximum number of waveforms that can be rendered in a single instanced draw call.
 * Limited by UBO size (typically 16KB on most GPUs).
 * 64 instances * (4 vec4s * 16 bytes) = 4KB, well within limits.
 */
static constexpr size_t kMaxInstancedWaveforms = 64;

/**
 * Per-instance data for instanced waveform rendering.
 * This structure is packed into a Uniform Buffer Object (UBO).
 * 
 * IMPORTANT: Must match layout(std140) in waveform_instanced.vert
 */
struct alignas(16) InstanceDataUBO
{
    // Transform: x-offset, y-offset, x-scale, y-scale
    std::array<std::array<float, 4>, kMaxInstancedWaveforms> transforms;
    
    // Color: r, g, b, a
    std::array<std::array<float, 4>, kMaxInstancedWaveforms> colors;
    
    // Params: opacity, lineWidth, glowIntensity, reserved
    std::array<std::array<float, 4>, kMaxInstancedWaveforms> params;
    
    // Viewport: x, y, width, height (normalized 0-1)
    std::array<std::array<float, 4>, kMaxInstancedWaveforms> viewports;
};

/**
 * Instanced waveform shader for Tier 2 rendering.
 * Uses Uniform Buffer Objects (UBOs) for per-instance data.
 * Requires OpenGL 3.3+.
 */
class InstancedShader : public WaveformShader
{
public:
    InstancedShader();
    ~InstancedShader() override;

    // WaveformShader interface
    bool compile(juce::OpenGLContext& context) override;
    void release(juce::OpenGLContext& context) override;
    [[nodiscard]] bool isCompiled() const override { return compiled_; }
    
    // Shader identification (WaveformShader interface)
    [[nodiscard]] juce::String getId() const override { return "instanced"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Instanced"; }
    [[nodiscard]] juce::String getDescription() const override { return "GPU instanced rendering for multiple waveforms"; }
    
    // Get the shader type for batching
    [[nodiscard]] ShaderType getShaderType() const { return ShaderType::Basic2D; }

    // WaveformShader render interface (not used for instanced rendering - use updateInstanceData instead)
    void render(juce::OpenGLContext& context,
                const std::vector<float>& channel1,
                const std::vector<float>* channel2,
                const ShaderRenderParams& params) override;

    /**
     * Bind the shader for instanced rendering.
     * @param context The OpenGL context
     */
    void bind(juce::OpenGLContext& context);

    /**
     * Unbind the shader after rendering.
     * @param context The OpenGL context
     */
    void unbind(juce::OpenGLContext& context);

    /**
     * Update the instance data UBO with new waveform configurations.
     * Call this before rendering.
     * 
     * @param context The OpenGL context
     * @param entries Vector of waveforms to render
     * @param viewportWidth Total viewport width in pixels
     * @param viewportHeight Total viewport height in pixels
     */
    void updateInstanceData(juce::OpenGLContext& context,
                           const std::vector<std::pair<const WaveformRenderData*, const WaveformRenderState*>>& entries,
                           float viewportWidth, float viewportHeight);

    /**
     * Set the projection matrix uniform.
     */
    void setProjection(const float* projectionMatrix);

    /**
     * Get the number of instances currently configured.
     */
    [[nodiscard]] size_t getInstanceCount() const { return instanceCount_; }

    // Not copyable
    InstancedShader(const InstancedShader&) = delete;
    InstancedShader& operator=(const InstancedShader&) = delete;

private:
    struct GLResources;
    std::unique_ptr<GLResources> gl_;
    
    InstanceDataUBO instanceData_;
    size_t instanceCount_ = 0;
    bool compiled_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL

