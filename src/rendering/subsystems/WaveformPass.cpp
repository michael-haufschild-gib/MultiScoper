/*
    Oscil - Waveform Pass Implementation
*/

#include "rendering/subsystems/WaveformPass.h"

#include "rendering/materials/MaterialShader.h"
#include "rendering/shaders3d/WaveformShader3D.h"
#include "rendering/subsystems/RenderStats.h"


namespace oscil
{

using namespace juce::gl;

// Helper to convert MaterialSettings to MaterialProperties
static MaterialProperties convertMaterialSettings(const MaterialSettings& settings, juce::Colour overrideColour)
{
    MaterialProperties props;
    props.reflectivity = settings.reflectivity;
    props.roughness = settings.roughness;
    props.ior = settings.refractiveIndex;
    props.fresnelPower = settings.fresnelPower;
    props.baseColorR = overrideColour.getFloatRed();
    props.baseColorG = overrideColour.getFloatGreen();
    props.baseColorB = overrideColour.getFloatBlue();
    props.baseColorA = overrideColour.getFloatAlpha();

    if (settings.refractiveIndex > 1.0f && settings.reflectivity < 0.5f)
    {
        props.metallic = 0.0f; // Dielectric (glass)
        props.refractionStrength = 1.0f;
    }
    else
    {
        props.metallic = 1.0f; // Metallic (chrome)
        props.refractionStrength = 0.0f;
    }

    props.envMapStrength = settings.useEnvironmentMap ? 1.0f : 0.0f;
    return props;
}

WaveformPass::WaveformPass()
    : particleSystem_(std::make_unique<ParticleSystem>())
    , particleRenderer_(std::make_unique<ParticleRenderer>(*particleSystem_))
    , gridRenderer_(std::make_unique<GridRenderer>())
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
        DBG("[WaveformPass] LEAK: Destructor called without shutdown(). "
            << compiledShaders_.size() << " compiled shaders will leak GPU resources.");
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

    if (!particleSystem_->initialize(context))
        RE_LOG("WaveformPass: Failed to initialize particle system");

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

    if (particleSystem_)
        particleSystem_->release(context);
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

    // Restore full viewport for particles and post-processing
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    // Step 4: Render particles
    if (config.particles.enabled)
    {
        renderWaveformParticles(data, state, deltaTime);
    }

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

void WaveformPass::setWaveformViewport(const WaveformRenderData& data)
{
    if (!context_) return;
    float scale = static_cast<float>(context_->getRenderingScale());
    auto* target = context_->getTargetComponent();
    if (!target) return;
    float logicalHeight = static_cast<float>(target->getHeight());

    int vx = std::max(0, static_cast<int>(data.bounds.getX() * scale));
    int vy = std::max(0, static_cast<int>((logicalHeight - (data.bounds.getY() + data.bounds.getHeight())) * scale));
    int vw = std::max(1, static_cast<int>(data.bounds.getWidth() * scale));
    int vh = std::max(1, static_cast<int>(data.bounds.getHeight() * scale));

    glViewport(vx, vy, vw, vh);
}

WaveformShader* WaveformPass::resolveShader(const juce::String& shaderId)
{
    auto it = compiledShaders_.find(shaderId.toStdString());
    if (it != compiledShaders_.end())
        return it->second.get();

    if (context_)
    {
        auto newShader = registry_->createShader(shaderId);
        if (newShader && newShader->compile(*context_))
        {
            compiledShaders_[shaderId.toStdString()] = std::move(newShader);
            return compiledShaders_[shaderId.toStdString()].get();
        }
    }

    auto basicIt = compiledShaders_.find("basic");
    return (basicIt != compiledShaders_.end()) ? basicIt->second.get() : nullptr;
}

void WaveformPass::configure3DShader(WaveformShader* shader, const VisualConfiguration& config,
                                      const WaveformRenderData& data)
{
    if (auto* shader3D = dynamic_cast<WaveformShader3D*>(shader))
    {
        if (camera_) shader3D->setCamera(*camera_);
        shader3D->setLighting(config.lighting);
    }

    if (isMaterialShader(config.shaderType))
    {
        if (auto* materialShader = dynamic_cast<MaterialShader*>(shader))
        {
            materialShader->setMaterial(convertMaterialSettings(config.material, data.colour));
            if (envMapManager_ && config.material.useEnvironmentMap)
            {
                GLuint envMap = envMapManager_->getMap(config.material.environmentMapId);
                if (envMap == 0) envMap = envMapManager_->getDefaultMap();
                materialShader->setEnvironmentMap(envMap);
            }
        }
    }
}

void WaveformPass::renderWaveformGeometry(const WaveformRenderData& data, const VisualConfiguration& config, float accumulatedTime)
{
    if (is3DShader(config.shaderType) && context_)
        setWaveformViewport(data);

    WaveformShader* shader = resolveShader(shaderTypeToId(config.shaderType));
    if (!shader || !shader->isCompiled()) return;

    ShaderRenderParams params;
    params.colour = data.colour;
    params.opacity = data.opacity;
    params.lineWidth = data.lineWidth;
    params.bounds = data.bounds;
    params.isStereo = data.isStereo;
    params.verticalScale = data.verticalScale;
    params.time = accumulatedTime;
    params.shaderIntensity = config.bloom.enabled ? config.bloom.intensity : 1.0f;

    if (is3DShader(config.shaderType))
        configure3DShader(shader, config, data);

    const std::vector<float>* channel2Ptr = data.isStereo ? &data.channel2 : nullptr;
    shader->render(*context_, data.channel1, channel2Ptr, params);
}

void WaveformPass::renderWaveformParticles(const WaveformRenderData& data, WaveformRenderState& state, float deltaTime)
{
    // Ensure viewport is set correctly for particles
    if (context_)
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

    if (particleRenderer_)
    {
        particleRenderer_->render(*context_, data, state, deltaTime, nullptr);
    }
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

} // namespace oscil
