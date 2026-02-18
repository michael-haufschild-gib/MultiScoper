/*
    Oscil - Waveform Pass Implementation
*/

#include "rendering/subsystems/WaveformPass.h"

#include "rendering/subsystems/RenderStats.h"

#include <iostream>

namespace oscil
{

using namespace juce::gl;

WaveformPass::WaveformPass()
    : gridRenderer_(std::make_unique<GridRenderer>())
    , registry_(std::make_unique<ShaderRegistry>())
{
}

WaveformPass::~WaveformPass()
{
    // If we're being destroyed while still initialized, shaders will leak.
    // This happens when shutdown() wasn't called before destruction.
    // We can't call OpenGL functions here (no context), so we just warn.
    if (initialized_ && !compiledShaders_.empty())
    {
        std::cerr << "[WaveformPass] LEAK: Destructor called without shutdown(). "
                  << compiledShaders_.size() << " compiled shaders will leak GPU resources."
                  << std::endl;
    }
}

bool WaveformPass::initialize(juce::OpenGLContext& context, int width, int height)
{
    // Guard against invalid dimensions that would cause division by zero
    if (width <= 0 || height <= 0)
    {
        RE_LOG("WaveformPass: Invalid dimensions " << width << "x" << height);
        return false;
    }

    context_ = &context;
    currentWidth_ = width;
    currentHeight_ = height;

    // LAZY SHADER LOADING: Only compile the essential "basic" shader at startup.
    {
        auto basicShader = registry_->createShader("basic");
        if (basicShader && basicShader->compile(context))
        {
            compiledShaders_["basic"] = std::move(basicShader);
            RE_LOG("WaveformPass: Compiled essential 'basic' shader");
        }
        else
        {
            RE_LOG("WaveformPass: WARNING - Failed to compile basic shader!");
        }
    }

    if (gridRenderer_)
        gridRenderer_->initialize(context);

    textureManager_ = std::make_unique<TextureManager>();
    textureManager_->initialize(context);

    envMapManager_ = std::make_unique<EnvironmentMapManager>();
    if (!envMapManager_->initialize(context))
    {
        RE_LOG("WaveformPass: Failed to initialize environment map manager");
    }
    else
    {
        createDefaultEnvironmentMaps();
    }

    camera_ = std::make_unique<Camera3D>();
    camera_->setAspectRatio(static_cast<float>(width) / static_cast<float>(height));

    initialized_ = true;
    return true;
}

void WaveformPass::shutdown(juce::OpenGLContext& context)
{
    if (!initialized_)
        return;

    for (auto& pair : compiledShaders_)
        pair.second->release(context);
    compiledShaders_.clear();

    if (textureManager_)
        textureManager_->release(context);
    if (envMapManager_)
        envMapManager_->release(context);
    if (gridRenderer_)
        gridRenderer_->release(context);

    camera_.reset();
    context_ = nullptr;
    initialized_ = false;
}

void WaveformPass::prepareRender(const WaveformRenderData& data, WaveformRenderState& state, float deltaTime)
{
    const auto& config = state.visualConfig;
    state.updateTiming(deltaTime);

    if (is3DShader(config.shaderType))
    {
        float scale = static_cast<float>(context_->getRenderingScale());
        auto* target = context_->getTargetComponent();
        if (!target) return;
        float logicalHeight = static_cast<float>(target->getHeight());

        float lx = data.bounds.getX();
        float ly = data.bounds.getY();
        float lw = data.bounds.getWidth();
        float lh = data.bounds.getHeight();

        int vx = static_cast<int>(lx * scale);
        int vy = static_cast<int>((logicalHeight - (ly + lh)) * scale);
        int vw = static_cast<int>(lw * scale);
        int vh = static_cast<int>(lh * scale);

        vx = std::max(0, vx);
        vy = std::max(0, vy);
        vw = std::max(1, vw);
        vh = std::max(1, vh);

        glViewport(vx, vy, vw, vh);

        if (camera_)
            camera_->setAspectRatio(static_cast<float>(vw) / static_cast<float>(vh));

        setupCamera3D(config.settings3D, deltaTime);
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        setupCamera2D();
        glDisable(GL_DEPTH_TEST);
    }
}

Framebuffer* WaveformPass::renderWaveform(const WaveformRenderData& data, WaveformRenderState& state,
                                          EffectPipeline& effectPipeline, float deltaTime, float accumulatedTime,
                                          juce::OpenGLShaderProgram* compositeShader, GLint compositeLoc)
{
    const auto& config = state.visualConfig;

    // Update state timing
    state.updateTiming(deltaTime);

    // Get working FBO from pool
    auto* waveformFBO = effectPipeline.getFramebufferPool()->getWaveformFBO();
    if (!waveformFBO)
    {
        RE_LOG("ERROR: No waveform FBO available!");
        return nullptr;
    }

    // Step 1: Clear waveform FBO
    waveformFBO->bind();
    waveformFBO->clear(juce::Colours::transparentBlack, true);

    // Store previous viewport and depth test state
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);

    // Step 2: Set up camera (2D or 3D)
    if (is3DShader(config.shaderType))
    {
        float scale = static_cast<float>(context_->getRenderingScale());
        auto* target = context_->getTargetComponent();
        if (!target) return nullptr;
        float logicalHeight = static_cast<float>(target->getHeight());

        float lx = data.bounds.getX();
        float ly = data.bounds.getY();
        float lw = data.bounds.getWidth();
        float lh = data.bounds.getHeight();

        int vx = static_cast<int>(lx * scale);
        int vy = static_cast<int>((logicalHeight - (ly + lh)) * scale);
        int vw = static_cast<int>(lw * scale);
        int vh = static_cast<int>(lh * scale);

        vx = std::max(0, vx);
        vy = std::max(0, vy);
        vw = std::max(1, vw);
        vh = std::max(1, vh);

        glViewport(vx, vy, vw, vh);

        if (camera_)
            camera_->setAspectRatio(static_cast<float>(vw) / static_cast<float>(vh));

        setupCamera3D(config.settings3D, deltaTime);
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        setupCamera2D();
        glDisable(GL_DEPTH_TEST);
    }

    // Render Grid
    if (data.gridConfig.enabled && !is3DShader(config.shaderType))
    {
        float scale = static_cast<float>(context_->getRenderingScale());
        auto* target = context_->getTargetComponent();
        if (!target) return nullptr;
        float logicalHeight = static_cast<float>(target->getHeight());

        float lx = data.bounds.getX();
        float ly = data.bounds.getY();
        float lw = data.bounds.getWidth();
        float lh = data.bounds.getHeight();

        int vx = static_cast<int>(lx * scale);
        int vy = static_cast<int>((logicalHeight - (ly + lh)) * scale);
        int vw = static_cast<int>(lw * scale);
        int vh = static_cast<int>(lh * scale);

        glViewport(vx, vy, vw, vh);

        renderGrid(data);

        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    }

    // Step 3: Render waveform geometry
    renderWaveformGeometry(data, config, accumulatedTime);

    // Restore full viewport for post-processing
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    waveformFBO->unbind();

    // Step 5: Apply post-processing effects
    Framebuffer* current = waveformFBO;

    if (config.hasPostProcessing())
    {
        current = effectPipeline.applyPostProcessing(current, state, *context_, deltaTime, compositeShader, compositeLoc);
        if (!current)
        {
            current = waveformFBO;
        }
    }

    // Restore GL depth test state
    if (depthTestWasEnabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    // Step 6: Composite onto scene - handled by caller
    return current;
}

void WaveformPass::renderWaveformGeometry(const WaveformRenderData& data, const VisualConfiguration& config, float accumulatedTime)
{
    // Ensure viewport is set correctly for 3D shaders (which rely on it for projection)
    if (is3DShader(config.shaderType) && context_)
    {
        float scale = static_cast<float>(context_->getRenderingScale());
        auto* target = context_->getTargetComponent();
        if (!target) return;
        float logicalHeight = static_cast<float>(target->getHeight());

        float lx = data.bounds.getX();
        float ly = data.bounds.getY();
        float lw = data.bounds.getWidth();
        float lh = data.bounds.getHeight();

        int vx = static_cast<int>(lx * scale);
        int vy = static_cast<int>((logicalHeight - (ly + lh)) * scale);
        int vw = static_cast<int>(lw * scale);
        int vh = static_cast<int>(lh * scale);

        glViewport(vx, vy, vw, vh);
    }

    // Use shader cache to avoid redundant lookups and binds
    WaveformShader* shader = bindShaderIfNeeded(config.shaderType);
    if (!shader || !shader->isCompiled())
        return;

    // Set vertex buffer pool for batched geometry upload
    shader->setVertexBufferPool(currentVertexBufferPool_);

    ShaderRenderParams params;
    params.colour = data.colour;
    params.opacity = data.opacity;
    params.lineWidth = data.lineWidth;
    params.bounds = data.bounds;
    params.isStereo = data.isStereo;
    params.verticalScale = data.verticalScale;
    params.time = accumulatedTime;
    params.shaderIntensity = config.bloom.enabled ? config.bloom.intensity : 1.0f;

    const std::vector<float>* channel2Ptr = data.isStereo ? &data.channel2 : nullptr;
    shader->render(*context_, data.channel1, channel2Ptr, params);
}

void WaveformPass::renderGrid(const WaveformRenderData& data)
{
    if (!gridRenderer_ || !context_)
        return;

    // Store previous viewport
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    float scale = static_cast<float>(context_->getRenderingScale());
    auto* target = context_->getTargetComponent();
    if (!target) return;
    float logicalHeight = static_cast<float>(target->getHeight());

    float lx = data.bounds.getX();
    float ly = data.bounds.getY();
    float lw = data.bounds.getWidth();
    float lh = data.bounds.getHeight();

    int vx = static_cast<int>(lx * scale);
    int vy = static_cast<int>((logicalHeight - (ly + lh)) * scale);
    int vw = static_cast<int>(lw * scale);
    int vh = static_cast<int>(lh * scale);

    glViewport(vx, vy, vw, vh);

    gridRenderer_->render(*context_, data);

    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}

void WaveformPass::setupCamera2D()
{
}

void WaveformPass::setupCamera3D(const Settings3D& settings, float deltaTime)
{
    if (!camera_)
        return;

    camera_->setOrbitDistance(settings.cameraDistance);
    camera_->setOrbitPitch(settings.cameraAngleX);
    camera_->setOrbitYaw(settings.cameraAngleY);

    if (settings.autoRotate)
    {
        float currentYaw = camera_->getOrbitYaw();
        camera_->setOrbitYaw(currentYaw + settings.rotateSpeed * deltaTime);
    }
}

void WaveformPass::createDefaultEnvironmentMaps()
{
    if (!envMapManager_)
        return;
    envMapManager_->createFromPreset("default_studio", EnvironmentPreset::Studio);
    envMapManager_->createFromPreset("studio", EnvironmentPreset::Studio);
    envMapManager_->createFromPreset("neon", EnvironmentPreset::Neon);
    envMapManager_->createFromPreset("cyber_space", EnvironmentPreset::CyberSpace);
    envMapManager_->createFromPreset("dark", EnvironmentPreset::Dark);
    envMapManager_->createFromPreset("sunset", EnvironmentPreset::Sunset);
    envMapManager_->createFromPreset("abstract", EnvironmentPreset::Abstract);
}

// ============================================================================
// Shader State Caching Implementation
// ============================================================================

void WaveformPass::resetShaderCache()
{
    currentBoundShader_ = nullptr;
    currentBoundShaderType_ = ShaderType::Basic2D;
    shaderCacheValid_ = false;
}

WaveformShader* WaveformPass::bindShaderIfNeeded(ShaderType shaderType)
{
    // Check if we already have this shader bound
    if (shaderCacheValid_ && currentBoundShaderType_ == shaderType && currentBoundShader_ != nullptr)
    {
        // Shader already bound, skip redundant bind
        return currentBoundShader_;
    }

    // Need to bind a different shader
    juce::String shaderId = shaderTypeToId(shaderType);
    WaveformShader* shader = nullptr;

    // Look up in compiled shaders
    auto it = compiledShaders_.find(shaderId.toStdString());
    if (it != compiledShaders_.end())
    {
        shader = it->second.get();
    }

    // Lazy compile if needed
    if (!shader && context_)
    {
        auto newShader = registry_->createShader(shaderId);
        if (newShader && newShader->compile(*context_))
        {
            RE_LOG("Lazy-compiled shader (batched): " << shaderId.toStdString());
            compiledShaders_[shaderId.toStdString()] = std::move(newShader);
            shader = compiledShaders_[shaderId.toStdString()].get();
        }
    }

    // Fall back to basic shader if needed
    if (!shader || !shader->isCompiled())
    {
        auto basicIt = compiledShaders_.find("basic");
        if (basicIt != compiledShaders_.end())
        {
            shader = basicIt->second.get();
        }
    }

    if (shader && shader->isCompiled())
    {
        // Update cache state (the shader's render() method will call use() internally)
        currentBoundShader_ = shader;
        currentBoundShaderType_ = shaderType;
        shaderCacheValid_ = true;
        return shader;
    }

    return nullptr;
}

} // namespace oscil
