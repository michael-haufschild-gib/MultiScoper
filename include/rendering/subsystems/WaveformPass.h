/*
    Oscil - Waveform Pass
    Handles rendering of individual waveforms (geometry and grid).
*/

#pragma once

#include "rendering/Camera3D.h"
#include "rendering/GridRenderer.h"
#include "rendering/ShaderRegistry.h"
#include "rendering/WaveformGLRenderer.h"
#include "rendering/WaveformRenderState.h"
#include "rendering/WaveformShader.h"
#include "rendering/materials/EnvironmentMapManager.h"
#include "rendering/materials/TextureManager.h"
#include "rendering/subsystems/EffectPipeline.h"

#include <juce_opengl/juce_opengl.h>

#include <memory>
#include <unordered_map>

namespace oscil
{

// Forward declaration for VBO pooling
class VertexBufferPool;

class WaveformPass
{
public:
    WaveformPass();
    ~WaveformPass();

    bool initialize(juce::OpenGLContext& context, int width, int height);
    void shutdown(juce::OpenGLContext& context);

    Framebuffer* renderWaveform(const WaveformRenderData& data, WaveformRenderState& state,
                                EffectPipeline& effectPipeline, float deltaTime, float accumulatedTime,
                                juce::OpenGLShaderProgram* compositeShader, GLint compositeLoc);

    // Subsystem accessors
    Camera3D* getCamera() { return camera_.get(); }
    EnvironmentMapManager* getEnvironmentMapManager() { return envMapManager_.get(); }
    TextureManager* getTextureManager() { return textureManager_.get(); }

    // Split Rendering API
    void prepareRender(const WaveformRenderData& data, WaveformRenderState& state, float deltaTime);
    void renderGrid(const WaveformRenderData& data);
    void renderWaveformGeometry(const WaveformRenderData& data, const VisualConfiguration& config, float accumulatedTime);

    // ========================================================================
    // Shader State Caching (reduces redundant shader binds)
    // ========================================================================

    /**
     * Reset shader cache at the start of a new batch/frame.
     * Call this before beginning a batched render pass.
     */
    void resetShaderCache();

    /**
     * Bind a shader only if it's different from the currently bound one.
     * @param shaderType The shader type to bind
     * @return Pointer to the bound shader, or nullptr if binding failed
     */
    WaveformShader* bindShaderIfNeeded(ShaderType shaderType);

    /**
     * Get the currently bound shader type.
     */
    [[nodiscard]] ShaderType getCurrentShaderType() const { return currentBoundShaderType_; }

    /**
     * Set the vertex buffer pool for batched rendering.
     * Pass nullptr to disable pooled rendering.
     * @param pool Pointer to the pool, or nullptr
     */
    void setVertexBufferPool(VertexBufferPool* pool) { currentVertexBufferPool_ = pool; }

private:
    void setupCamera2D();
    void setupCamera3D(const Settings3D& settings, float deltaTime);
    void createDefaultEnvironmentMaps();

    // Subsystems
    std::unique_ptr<GridRenderer> gridRenderer_;
    std::unique_ptr<Camera3D> camera_;
    std::unique_ptr<EnvironmentMapManager> envMapManager_;
    std::unique_ptr<TextureManager> textureManager_;
    std::unique_ptr<ShaderRegistry> registry_;

    // Compiled per-instance shaders
    std::unordered_map<std::string, std::unique_ptr<WaveformShader>> compiledShaders_;

    juce::OpenGLContext* context_ = nullptr;
    int currentWidth_ = 0;
    int currentHeight_ = 0;
    bool initialized_ = false;

    // ========================================================================
    // Shader State Cache (avoids redundant shader binds)
    // ========================================================================
    
    WaveformShader* currentBoundShader_ = nullptr;
    ShaderType currentBoundShaderType_ = ShaderType::Basic2D;
    bool shaderCacheValid_ = false;

    // Vertex buffer pool for batched geometry (set during flushBatch)
    VertexBufferPool* currentVertexBufferPool_ = nullptr;
};

} // namespace oscil
