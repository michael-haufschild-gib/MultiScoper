/*
    Oscil - Render Engine Implementation
*/

#include "rendering/RenderEngine.h"
#include "rendering/ShaderRegistry.h"
#include "rendering/effects/PostProcessEffect.h"
#include <cmath>

// Release-mode logging macro for render engine debugging
// Uses JUCE DBG macro which outputs to the debugger/console correctly on all platforms
#define RE_LOG(msg) DBG("[RenderEngine] " << msg)

// Rate-limited logging macro (logs every N frames)
#define RE_LOG_THROTTLED(interval_frames, msg) \
    do { \
        static int _log_counter = 0; \
        if (++_log_counter >= interval_frames) { \
            _log_counter = 0; \
            RE_LOG(msg); \
        } \
    } while (0)

#include "rendering/effects/VignetteEffect.h"
#include "rendering/effects/FilmGrainEffect.h"
#include "rendering/effects/BloomEffect.h"
#include "rendering/effects/TrailsEffect.h"
#include "rendering/effects/ColorGradeEffect.h"
#include "rendering/effects/TiltShiftEffect.h"
#include "rendering/effects/ChromaticAberrationEffect.h"
#include "rendering/effects/ScanlineEffect.h"
#include "rendering/effects/DistortionEffect.h"
#include "rendering/effects/GlitchEffect.h"
#include "rendering/effects/RadialBlurEffect.h"
#include "rendering/Camera3D.h"
#include "rendering/particles/ParticleSystem.h"
#include "rendering/materials/EnvironmentMapManager.h"
#include "rendering/materials/TextureManager.h"
#include "rendering/materials/MaterialShader.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// Blit shader for final output to screen
static const char* blitVertexShader = R"(
    #version 330 core
    in vec2 position;
    in vec2 texCoord;
    out vec2 vTexCoord;
    void main() {
        gl_Position = vec4(position, 0.0, 1.0);
        vTexCoord = texCoord;
    }
)";

static const char* blitFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    in vec2 vTexCoord;
    out vec4 fragColor;

    // ACES Filmic Tone Mapping Curve
    // Adapted for real-time rendering (Narkowicz 2015)
    vec3 aces(vec3 x) {
        const float a = 2.51;
        const float b = 0.03;
        const float c = 2.43;
        const float d = 0.59;
        const float e = 0.14;
        return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
    }

    // Simple hash for dithering
    float random(vec2 p) {
        return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
    }

    void main() {
        vec4 color = texture(sourceTexture, vTexCoord);
        vec3 hdrColor = color.rgb;

        // 1. Tone Mapping (HDR -> LDR)
        // ACES gives a filmic look with nice highlight rolloff
        vec3 mapped = aces(hdrColor);

        // 2. Gamma Correction (Linear -> sRGB)
        // Standard 2.2 gamma approximation
        mapped = pow(mapped, vec3(1.0 / 2.2));

        // 3. Dithering
        // Prevents banding in dark gradients on 8-bit displays
        mapped += (random(gl_FragCoord.xy) - 0.5) * (1.0 / 255.0);

        fragColor = vec4(mapped, color.a);
    }
)";

// Simple pass-through shader for compositing into the scene FBO while staying in linear HDR space
static const char* compositeFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    in vec2 vTexCoord;
    out vec4 fragColor;

    void main() {
        fragColor = texture(sourceTexture, vTexCoord);
    }
)";

RenderEngine::RenderEngine()
    : fbPool_(std::make_unique<FramebufferPool>())
    , sceneFBO_(std::make_unique<Framebuffer>())
    , particleSystem_(std::make_unique<ParticleSystem>())
    , registry_(std::make_unique<ShaderRegistry>())
    , gridRenderer_(std::make_unique<GridRenderer>())
    , particleRenderer_(std::make_unique<ParticleRenderer>(*particleSystem_))
{
}

RenderEngine::~RenderEngine()
{
    // Note: shutdown() should be called explicitly while context is active
}

bool RenderEngine::initialize(juce::OpenGLContext& context)
{
    if (initialized_)
        shutdown();

    context_ = &context;

    // Get initial dimensions from target component
    auto* targetComponent = context.getTargetComponent();
    if (!targetComponent)
    {
        DBG("RenderEngine: No target component");
        return false;
    }

    // Use physical (scaled) dimensions for internal state and FBOs
    double scale = context.getRenderingScale();
    currentWidth_ = static_cast<int>(targetComponent->getWidth() * scale);
    currentHeight_ = static_cast<int>(targetComponent->getHeight() * scale);

    if (currentWidth_ <= 0 || currentHeight_ <= 0)
    {
        DBG("RenderEngine: Invalid dimensions " << currentWidth_ << "x" << currentHeight_);
        return false;
    }

    // LAZY SHADER LOADING: Only compile the essential "basic" shader at startup.
    // All other shaders are compiled on-demand when first requested.
    // This reduces initialization time from ~1s to ~100ms.
    {
        auto basicShader = registry_->createShader("basic");
        if (basicShader && basicShader->compile(context))
        {
            compiledShaders_["basic"] = std::move(basicShader);
            DBG("RenderEngine: Compiled essential 'basic' shader");
        }
        else
        {
            DBG("RenderEngine: WARNING - Failed to compile basic shader!");
        }
    }
    DBG("RenderEngine: Lazy loading enabled - other shaders compiled on demand");

    // Initialize framebuffer pool
    if (!fbPool_->initialize(context, currentWidth_, currentHeight_))
    {
        DBG("RenderEngine: Failed to initialize framebuffer pool");
        return false;
    }

    // Create scene FBO (where all waveforms are composited)
    // Pass 0 for samples (no MSAA)
    if (!sceneFBO_->create(context, currentWidth_, currentHeight_, 0, GL_RGBA16F, false))
    {
        DBG("RenderEngine: Failed to create scene FBO");
        fbPool_->shutdown(context);
        return false;
    }

    // Compile blit shader (tone map + gamma) for final output
    blitShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!blitShader_->addVertexShader(blitVertexShader) ||
        !blitShader_->addFragmentShader(blitFragmentShader))
    {
        DBG("RenderEngine: Failed to compile blit shader: " << blitShader_->getLastError());
        sceneFBO_->destroy(context);
        fbPool_->shutdown(context);
        return false;
    }

    // Bind attribute locations BEFORE linking to ensure they match FramebufferPool's quad VAO
    // The fullscreen quad VAO uses location 0 for position and location 1 for texCoord
    GLuint programID = blitShader_->getProgramID();
    context.extensions.glBindAttribLocation(programID, 0, "position");
    context.extensions.glBindAttribLocation(programID, 1, "texCoord");

    if (!blitShader_->link())
    {
        DBG("RenderEngine: Failed to link blit shader: " << blitShader_->getLastError());
        sceneFBO_->destroy(context);
        fbPool_->shutdown(context);
        return false;
    }

    blitTextureLoc_ = blitShader_->getUniformIDFromName("sourceTexture");
    DBG("RenderEngine: Blit shader compiled, sourceTexture uniform=" << blitTextureLoc_);

    // Initialize GridRenderer
    if (gridRenderer_)
    {
        gridRenderer_->initialize(context);
    }

    // Compile composite shader (linear pass-through) for layering waveforms into scene FBO
    compositeShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compositeShader_->addVertexShader(blitVertexShader) ||
        !compositeShader_->addFragmentShader(compositeFragmentShader))
    {
        DBG("RenderEngine: Failed to compile composite shader: " << compositeShader_->getLastError());
        blitShader_.reset();
        sceneFBO_->destroy(context);
        fbPool_->shutdown(context);
        return false;
    }

    GLuint compositeProgramId = compositeShader_->getProgramID();
    context.extensions.glBindAttribLocation(compositeProgramId, 0, "position");
    context.extensions.glBindAttribLocation(compositeProgramId, 1, "texCoord");

    if (!compositeShader_->link())
    {
        DBG("RenderEngine: Failed to link composite shader: " << compositeShader_->getLastError());
        compositeShader_.reset();
        blitShader_.reset();
        sceneFBO_->destroy(context);
        fbPool_->shutdown(context);
        return false;
    }

    compositeTextureLoc_ = compositeShader_->getUniformIDFromName("sourceTexture");
    DBG("RenderEngine: Composite shader compiled, sourceTexture uniform=" << compositeTextureLoc_);

    // Initialize post-processing effects
    initializeEffects();

    // Initialize particle system
    if (!particleSystem_->initialize(context))
    {
        DBG("RenderEngine: Failed to initialize particle system");
        // Non-fatal, continue without particles
    }

    // Initialize texture manager
    textureManager_ = std::make_unique<TextureManager>();
    textureManager_->initialize(context);

    // Initialize environment map manager
    envMapManager_ = std::make_unique<EnvironmentMapManager>();
    if (!envMapManager_->initialize(context))
    {
        DBG("RenderEngine: Failed to initialize environment map manager");
        // Non-fatal, continue without environment maps
    }
    else
    {
        // Create default environment maps for material shaders
        createDefaultEnvironmentMaps();
    }

    // Initialize camera
    camera_ = std::make_unique<Camera3D>();
    camera_->setAspectRatio(static_cast<float>(currentWidth_) / static_cast<float>(currentHeight_));

    initialized_ = true;
    DBG("RenderEngine: Initialized " << currentWidth_ << "x" << currentHeight_);
    return true;
}

void RenderEngine::shutdown()
{
    if (!initialized_ || !context_)
        return;

    // Release compiled shaders
    for (auto& pair : compiledShaders_)
    {
        pair.second->release(*context_);
    }
    compiledShaders_.clear();

    // Release all waveform states
    for (auto& pair : waveformStates_)
    {
        pair.second.release(*context_);
    }
    waveformStates_.clear();

    // Release effects
    releaseEffects();

    // Release particle system
    if (particleSystem_)
    {
        particleSystem_->release(*context_);
    }

    // Release texture manager
    if (textureManager_)
    {
        textureManager_->release(*context_);
        textureManager_.reset();
    }

    // Release environment map manager
    if (envMapManager_)
    {
        envMapManager_->release(*context_);
        envMapManager_.reset();
    }

    // Release camera
    camera_.reset();

    // Release shaders
    compositeShader_.reset();
    blitShader_.reset();
    
    // Release GridRenderer
    if (gridRenderer_)
    {
        gridRenderer_->release(*context_);
    }

    // Release scene FBO
    sceneFBO_->destroy(*context_);

    // Shutdown framebuffer pool
    fbPool_->shutdown(*context_);

    context_ = nullptr;
    initialized_ = false;

    DBG("RenderEngine: Shutdown complete");
}

// ... (keep existing methods) ...

void RenderEngine::renderWaveformComplete(const WaveformRenderData& data, WaveformRenderState& state)
{
    const auto& config = state.visualConfig;

    RE_LOG_THROTTLED(60, "renderWaveformComplete: presetId=" << config.presetId.toStdString()
               << ", shaderType=" << static_cast<int>(config.shaderType)
               << ", bloom=" << (config.bloom.enabled ? "true" : "false")
               << ", trails=" << (config.trails.enabled ? "true" : "false")
               << ", vignette=" << (config.vignette.enabled ? "true" : "false"));

    // Update state timing
    state.updateTiming(deltaTime_);

    // Get working FBO from pool
    auto* waveformFBO = fbPool_->getWaveformFBO();
    if (!waveformFBO)
    {
        RE_LOG("ERROR: No waveform FBO available!");
        return;
    }

    // Step 1: Clear waveform FBO
    waveformFBO->bind();
    waveformFBO->clear(juce::Colours::transparentBlack, true);

    // Store previous viewport state
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // Step 2: Set up camera (2D or 3D)
    if (is3DShader(config.shaderType))
    {
        // For 3D shaders, we must restrict rendering to the specific pane bounds.
        // Calculate viewport using Logical dimensions from target component for reliability.
        float scale = static_cast<float>(context_->getRenderingScale());
        auto* target = context_->getTargetComponent();
        float logicalHeight = static_cast<float>(target->getHeight());

        // Logical bounds
        float lx = data.bounds.getX();
        float ly = data.bounds.getY();
        float lw = data.bounds.getWidth();
        float lh = data.bounds.getHeight();

        // Convert to scaled viewport coordinates (Physical)
        // OpenGL origin is bottom-left, JUCE is top-left.
        // Y = (LogicalTotalHeight - (TopY + Height)) * Scale
        int vx = static_cast<int>(lx * scale);
        int vy = static_cast<int>((logicalHeight - (ly + lh)) * scale);
        int vw = static_cast<int>(lw * scale);
        int vh = static_cast<int>(lh * scale);

        // Clamp to prevent invalid viewports
        vx = std::max(0, vx);
        vy = std::max(0, vy);
        vw = std::max(1, vw);
        vh = std::max(1, vh);

        glViewport(vx, vy, vw, vh);

        // Adjust camera aspect ratio for the pane
        if (camera_)
        {
            camera_->setAspectRatio(static_cast<float>(vw) / static_cast<float>(vh));
        }

        setupCamera3D(config.settings3D);
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        // 2D Mode: Use full FBO or pane viewport?
        // WaveformFBO is full screen sized (usually).
        // But WaveformShader usually renders using 'bounds' in Normalized Device Coords?
        // No, WaveformShader usually assumes full screen quad [-1, 1] and does bounds internally?
        // Let's check renderWaveformGeometry. It passes bounds to shader params.
        // So we keep full viewport but shader clips?
        // Actually, to be safe and efficient, we should probably set viewport to pane bounds too
        // but standard 2D shaders might rely on full viewport NDC mapping.
        // Let's assume standard 2D setup (full viewport) is fine for now.
        setupCamera2D();
        glDisable(GL_DEPTH_TEST);
    }
    
    // Render Grid (New)
    if (data.gridConfig.enabled && !is3DShader(config.shaderType)) // Only render grid in 2D mode for now
    {
        // Set viewport to pane bounds for grid rendering ease (0..1 mapping)
         float scale = static_cast<float>(context_->getRenderingScale());
        auto* target = context_->getTargetComponent();
        float logicalHeight = static_cast<float>(target->getHeight());

        // Logical bounds
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
        
        // Restore viewport if needed (though 3D sets it, 2D might not)
        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    }

    // Step 3: Render waveform geometry
    renderWaveformGeometry(data, config);

    // Restore full viewport for particles and post-processing
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    // Step 4: Render particles (if enabled)
    if (config.particles.enabled)
    {
        // TODO: Soft particles require a valid depth buffer.
        // Currently, we render into waveformFBO (which has depth), but we cannot bind its
        // own depth texture for reading while writing to it (feedback loop).
        // sceneFBO_ has no depth buffer.
        // Future improvement: Resolve waveformFBO depth to a texture or use a depth pre-pass.
        // For now, we pass nullptr, effectively disabling soft particles.
        renderWaveformParticles(data, state);
    }

    waveformFBO->unbind();

    // Step 5: Apply post-processing effects
    Framebuffer* current = waveformFBO;

    if (config.hasPostProcessing())
    {
        RE_LOG_THROTTLED(60, "Applying post-processing effects...");
        current = applyPostProcessing(current, state);
        if (!current)
        {
            RE_LOG("ERROR: applyPostProcessing returned null!");
            current = waveformFBO;  // Fall back to original
        }
    }

    // Step 6: Composite onto scene
    RE_LOG_THROTTLED(60, "Compositing to scene, current FBO valid=" << ((current && current->isValid()) ? "true" : "false"));
    compositeToScene(current, config);

    // Step 7: Update sample history for 3D temporal effects
    if (state.needsSampleHistory())
    {
        state.pushSamples(data.channel1);
    }
}

void RenderEngine::renderGrid(const WaveformRenderData& data)
{
    if (gridRenderer_ && context_)
    {
        gridRenderer_->render(*context_, data);
    }
}

void RenderEngine::resize(int width, int height)
{
    if (!initialized_ || !context_ || width <= 0 || height <= 0)
        return;

    if (width == currentWidth_ && height == currentHeight_)
        return;

    currentWidth_ = width;
    currentHeight_ = height;

    // Resize all FBOs
    fbPool_->resize(*context_, width, height);
    sceneFBO_->resize(*context_, width, height);

    // Resize history FBOs for waveforms with trails
    for (auto& [key, state] : waveformStates_)
    {
        if (state.trailsEnabled)
        {
            state.resizeHistoryFBO(*context_, width, height);
        }
    }

    DBG("RenderEngine: Resized to " << width << "x" << height);
}

void RenderEngine::beginFrame(float deltaTime)
{
    if (!initialized_)
        return;

    deltaTime_ = deltaTime;
    accumulatedTime_ += deltaTime;

    RE_LOG_THROTTLED(60, "beginFrame: dt=" << deltaTime << ", sceneFBO valid=" << (sceneFBO_->isValid() ? "true" : "false")
               << " (" << sceneFBO_->width << "x" << sceneFBO_->height << ")");

    // Update particle system
    if (particleSystem_)
        particleSystem_->update(deltaTime);

    // Update camera animation
    if (camera_)
        camera_->update(deltaTime);

    // Clear scene FBO
    sceneFBO_->bind();
    sceneFBO_->clear(backgroundColour_, false);
    sceneFBO_->unbind();
}

void RenderEngine::renderWaveform(const WaveformRenderData& data)
{
    RE_LOG_THROTTLED(60, "renderWaveform: id=" << data.id << ", visible=" << (data.visible ? "true" : "false")
               << ", ch1 size=" << data.channel1.size()
               << ", bounds=(" << data.bounds.getX() << "," << data.bounds.getY()
               << "," << data.bounds.getWidth() << "x" << data.bounds.getHeight() << ")"
               << ", initialized=" << (initialized_ ? "true" : "false"));

    if (!initialized_ || !data.visible || data.channel1.size() < 2)
    {
        // Only log early exit occasionally to avoid spam
        RE_LOG_THROTTLED(60, "  -> early exit: init=" << (initialized_ ? "true" : "false") << ", vis=" << (data.visible ? "true" : "false")
                   << ", samples=" << data.channel1.size());
        return;
    }

    // Get or create waveform state
    auto it = waveformStates_.find(data.id);
    if (it == waveformStates_.end())
    {
        registerWaveform(data.id);
        it = waveformStates_.find(data.id);
    }

    if (it != waveformStates_.end())
    {
        RE_LOG_THROTTLED(60, "  -> calling renderWaveformComplete");
        renderWaveformComplete(data, it->second);
    }
}

void RenderEngine::endFrame()
{
    if (!initialized_)
        return;

    // Apply global effects to scene FBO
    if (globalPostProcessEnabled_)
    {
        applyGlobalEffects();
    }

    // Blit scene to screen
    blitToScreen();
}

void RenderEngine::registerWaveform(int waveformId)
{
    if (waveformStates_.find(waveformId) == waveformStates_.end())
    {
        WaveformRenderState state;
        state.waveformId = waveformId;
        waveformStates_[waveformId] = std::move(state);
        DBG("RenderEngine: Registered waveform " << waveformId);
    }
}

void RenderEngine::unregisterWaveform(int waveformId)
{
    auto it = waveformStates_.find(waveformId);
    if (it != waveformStates_.end())
    {
        if (context_)
            it->second.release(*context_);
        waveformStates_.erase(it);
        DBG("RenderEngine: Unregistered waveform " << waveformId);
    }
}

VisualConfiguration* RenderEngine::getWaveformConfig(int waveformId)
{
    auto it = waveformStates_.find(waveformId);
    if (it != waveformStates_.end())
        return &it->second.visualConfig;
    return nullptr;
}

void RenderEngine::setWaveformConfig(int waveformId, const VisualConfiguration& config)
{
    auto it = waveformStates_.find(waveformId);
    if (it != waveformStates_.end())
    {
        it->second.visualConfig = config;

        // Update trails state based on config
        if (context_)
        {
            if (config.trails.enabled && !it->second.trailsEnabled)
            {
                it->second.enableTrails(*context_, currentWidth_, currentHeight_);
            }
            else if (!config.trails.enabled && it->second.trailsEnabled)
            {
                it->second.disableTrails(*context_);
            }
            
            // Update turbulence
            if (particleSystem_ && config.particles.enabled)
            {
                if (config.particles.useTurbulence)
                {
                    particleSystem_->setTurbulence(
                        config.particles.turbulenceStrength,
                        config.particles.turbulenceScale,
                        config.particles.turbulenceSpeed
                    );
                }
                else
                {
                    particleSystem_->setTurbulence(0.0f, 0.5f, 0.5f);
                }
            }
        }
    }
}

void RenderEngine::clearAllWaveforms()
{
    if (context_)
    {
        for (auto& pair : waveformStates_)
        {
            pair.second.release(*context_);
        }
    }
    waveformStates_.clear();
}

void RenderEngine::syncWaveforms(const std::unordered_set<int>& activeIds)
{
    if (!context_)
        return;

    // Find waveform states that are no longer active
    std::vector<int> toRemove;
    for (const auto& pair : waveformStates_)
    {
        if (activeIds.find(pair.first) == activeIds.end())
        {
            toRemove.push_back(pair.first);
        }
    }

    // Unregister stale waveforms (releases GPU resources)
    for (int id : toRemove)
    {
        unregisterWaveform(id);
        DBG("RenderEngine: Synced out stale waveform " << id);
    }
}

void RenderEngine::setQualityLevel(QualityLevel level)
{
    qualityLevel_ = level;

    // Apply quality-based limits
    switch (level)
    {
        case QualityLevel::Eco:
            // Performance priority - minimal effects
            // Disable expensive effects
            if (auto* bloom = getEffect("bloom"))
                bloom->setEnabled(false);
            if (auto* trails = getEffect("trails"))
                trails->setEnabled(false);
            if (auto* chromaticAberration = getEffect("chromatic_aberration"))
                chromaticAberration->setEnabled(false);
            if (auto* glitch = getEffect("glitch"))
                glitch->setEnabled(false);
            if (auto* distortion = getEffect("distortion"))
                distortion->setEnabled(false);

            // Limit particle count
            if (particleSystem_)
            {
                // In Eco mode, reduce particle capacity
                // (Particle system would need a method to adjust this)
            }
            break;

        case QualityLevel::Normal:
            // Balanced - enable most 2D effects
            if (auto* bloom = getEffect("bloom"))
                bloom->setEnabled(true);
            if (auto* trails = getEffect("trails"))
                trails->setEnabled(true);
            if (auto* chromaticAberration = getEffect("chromatic_aberration"))
                chromaticAberration->setEnabled(true);
            if (auto* glitch = getEffect("glitch"))
                glitch->setEnabled(true);
            if (auto* distortion = getEffect("distortion"))
                distortion->setEnabled(true);
            break;

        case QualityLevel::Ultra:
            // Visual priority - all effects including 3D
            // Enable everything at highest quality
            for (auto& [id, effect] : effects_)
            {
                effect->setEnabled(true);
            }
            break;
    }

    DBG("RenderEngine: Quality level set to " << static_cast<int>(level));
}

PostProcessEffect* RenderEngine::getEffect(const juce::String& effectId)
{
    auto it = effects_.find(effectId);
    if (it != effects_.end())
    {
        auto* effect = it->second.get();
        // LAZY EFFECT COMPILATION: Compile on first access
        if (effect && !effect->isCompiled() && context_)
        {
            if (effect->compile(*context_))
            {
                RE_LOG("Lazy-compiled effect: " << effectId.toStdString());
            }
            else
            {
                RE_LOG("Failed to lazy-compile effect: " << effectId.toStdString());
            }
        }
        return effect;
    }
    return nullptr;
}

// ============================================================================
// Private Implementation
// ============================================================================



// Helper to convert MaterialSettings to MaterialProperties
static MaterialProperties convertMaterialSettings(const MaterialSettings& settings, juce::Colour overrideColour)
{
    MaterialProperties props;

    // Map from config to shader properties
    props.reflectivity = settings.reflectivity;
    props.roughness = settings.roughness;
    props.ior = settings.refractiveIndex;
    props.fresnelPower = settings.fresnelPower;

    // Use oscillator color directly as base color
    // This ensures visual consistency with the track color
    props.baseColorR = overrideColour.getFloatRed();
    props.baseColorG = overrideColour.getFloatGreen();
    props.baseColorB = overrideColour.getFloatBlue();
    props.baseColorA = overrideColour.getFloatAlpha();

    /*
    // Previous logic (disabled):
    // Use tint color from preset multiplied by oscillator color
    float r = settings.tintColor.getFloatRed() * overrideColour.getFloatRed();
    float g = settings.tintColor.getFloatGreen() * overrideColour.getFloatGreen();
    float b = settings.tintColor.getFloatBlue() * overrideColour.getFloatBlue();
    float a = settings.tintColor.getFloatAlpha() * overrideColour.getFloatAlpha();

    props.baseColorR = r;
    props.baseColorG = g;
    props.baseColorB = b;
    props.baseColorA = a;
    */

    // Material type detection (glass vs chrome)
    // Glass: low metallic, high refraction
    // Chrome: high metallic, no refraction
    if (settings.refractiveIndex > 1.0f && settings.reflectivity < 0.5f)
    {
        props.metallic = 0.0f;  // Dielectric (glass)
        props.refractionStrength = 1.0f;
    }
    else
    {
        props.metallic = 1.0f;  // Metallic (chrome)
        props.refractionStrength = 0.0f;
    }

    props.envMapStrength = settings.useEnvironmentMap ? 1.0f : 0.0f;

    return props;
}

void RenderEngine::renderWaveformGeometry(const WaveformRenderData& data, const VisualConfiguration& config)
{
    // Get shader from local map using string ID (for compatibility)
    juce::String shaderId = shaderTypeToId(config.shaderType);
    WaveformShader* shader = nullptr;

    auto it = compiledShaders_.find(shaderId.toStdString());
    if (it != compiledShaders_.end())
    {
        shader = it->second.get();
    }

    // LAZY SHADER COMPILATION: If shader not found, compile it on-demand
    if (!shader && context_)
    {
        auto newShader = registry_->createShader(shaderId);
        if (newShader && newShader->compile(*context_))
        {
            RE_LOG("Lazy-compiled shader: " << shaderId.toStdString());
            compiledShaders_[shaderId.toStdString()] = std::move(newShader);
            shader = compiledShaders_[shaderId.toStdString()].get();
        }
        else
        {
            RE_LOG("Failed to lazy-compile shader: " << shaderId.toStdString() << " - using fallback");
        }
    }

    RE_LOG_THROTTLED(60, "renderWaveformGeometry: shaderId=" << shaderId.toStdString()
               << ", shader=" << (shader ? "found" : "null"));

    // Fallback to basic shader if requested shader not found or failed to compile
    if (!shader || !shader->isCompiled())
    {
        auto basicIt = compiledShaders_.find("basic");
        if (basicIt != compiledShaders_.end())
        {
            shader = basicIt->second.get();
            RE_LOG_THROTTLED(60, "  -> fallback to basic: found");
        }
    }

    if (!shader || !shader->isCompiled())
    {
        RE_LOG("ERROR: Shader not available or not compiled: " << shaderId.toStdString()
               << " (shader=" << (shader ? "exists" : "null")
               << ", compiled=" << (shader ? (shader->isCompiled() ? "true" : "false") : "false") << ")");
        return;
    }

    // Set up render parameters
    ShaderRenderParams params;
    params.colour = data.colour;
    params.opacity = data.opacity;
    params.lineWidth = data.lineWidth;
    params.bounds = data.bounds;
    params.isStereo = data.isStereo;
    params.verticalScale = data.verticalScale;
    params.time = accumulatedTime_;
    params.shaderIntensity = config.bloom.enabled ? config.bloom.intensity : 1.0f;

    // Configure 3D shader if applicable
    if (is3DShader(config.shaderType))
    {
        if (auto* shader3D = dynamic_cast<WaveformShader3D*>(shader))
        {
            if (camera_)
                shader3D->setCamera(*camera_);
            shader3D->setLighting(lightingConfig_);
        }

        // Handle material shaders specifically
        if (isMaterialShader(config.shaderType))
        {
            if (auto* materialShader = dynamic_cast<MaterialShader*>(shader))
            {
                // Convert and set material properties from config
                // Pass data.colour to ensure the oscillator's color is used
                MaterialProperties matProps = convertMaterialSettings(config.material, data.colour);
                materialShader->setMaterial(matProps);

                // Bind environment map if available
                if (envMapManager_ && config.material.useEnvironmentMap)
                {
                    GLuint envMap = envMapManager_->getMap(config.material.environmentMapId);
                    if (envMap == 0)
                    {
                        // Fallback to default map
                        envMap = envMapManager_->getDefaultMap();
                    }
                    materialShader->setEnvironmentMap(envMap);
                }
            }
        }
    }

    RE_LOG_THROTTLED(60, "  -> rendering with shader, bounds=(" << params.bounds.getX() << ","
               << params.bounds.getY() << "," << params.bounds.getWidth() << "x"
               << params.bounds.getHeight() << ")");

    // Render using the shader
    const std::vector<float>* channel2Ptr = data.isStereo ? &data.channel2 : nullptr;
    shader->render(*context_, data.channel1, channel2Ptr, params);
}

void RenderEngine::renderWaveformParticles(const WaveformRenderData& data, WaveformRenderState& state)
{
    if (particleRenderer_)
    {
        // Pass nullptr for depth buffer as soft particles are currently not supported
        // due to lack of depth copy/resolve mechanism.
        particleRenderer_->render(*context_, data, state, deltaTime_, nullptr);
    }
}

Framebuffer* RenderEngine::applyPostProcessing(Framebuffer* source, WaveformRenderState& state)
{
    const auto& config = state.visualConfig;
    Framebuffer* current = source;

    // Step 1: Apply trails effect BEFORE the generic effect chain
    // Trails requires per-waveform history FBO from WaveformRenderState
    if (config.trails.enabled && state.trailsEnabled &&
        state.historyFBO && state.historyFBO->isValid())
    {
        auto* trailsEffect = dynamic_cast<TrailsEffect*>(getEffect("trails"));
        if (trailsEffect && trailsEffect->isCompiled())
        {
            trailsEffect->setSettings(config.trails);

            // Get a destination FBO from the pool
            Framebuffer* trailsDest = fbPool_->getPingFBO();
            if (trailsDest && trailsDest->isValid())
            {
                // Apply trails blending current frame with history
                trailsEffect->applyWithHistory(
                    *context_,
                    current,               // Current frame
                    state.historyFBO.get(), // Previous frame history
                    trailsDest,            // Output
                    *fbPool_,
                    deltaTime_
                );

                // Copy the result back to historyFBO for the next frame
                copyFramebuffer(trailsDest, state.historyFBO.get());

                // Use trails output as input for the rest of the chain
                current = trailsDest;
            }
        }
    }

    // Step 2: Apply the generic effect chain (stateless effects)
    return effectChain_.process(*context_, current, *fbPool_, deltaTime_, config, *this);
}

void RenderEngine::compositeToScene(Framebuffer* source, const VisualConfiguration& config)
{
    if (!source || !source->isValid())
        return;

    sceneFBO_->bind();

    // Set blend mode based on configuration
    glEnable(GL_BLEND);
    switch (config.compositeBlendMode)
    {
        case BlendMode::Alpha:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BlendMode::Additive:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case BlendMode::Multiply:
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            break;
        case BlendMode::Screen:
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
            break;
    }

    // Use composite shader to keep linear HDR data in the scene FBO
    compositeShader_->use();
    source->bindTexture(0);
    context_->extensions.glUniform1i(compositeTextureLoc_, 0);

    // Render fullscreen quad
    fbPool_->renderFullscreenQuad();

    glDisable(GL_BLEND);
    sceneFBO_->unbind();
}

void RenderEngine::applyGlobalEffects()
{
    // Global effects are applied to the entire scene
    // These would be things like global vignette, film grain overlay, etc.
    // Currently a placeholder - can be expanded based on needs
}

void RenderEngine::blitToScreen()
{
    // Bind default framebuffer
    context_->extensions.glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Set viewport to screen size
    auto* targetComponent = context_->getTargetComponent();
    if (!targetComponent)
    {
        RE_LOG("ERROR: blitToScreen - no target component");
        return;
    }

    float desktopScale = static_cast<float>(context_->getRenderingScale());
    auto width = static_cast<GLsizei>(static_cast<float>(targetComponent->getWidth()) * desktopScale);
    auto height = static_cast<GLsizei>(static_cast<float>(targetComponent->getHeight()) * desktopScale);
    glViewport(0, 0, width, height);

    RE_LOG_THROTTLED(60, "blitToScreen: viewport=" << width << "x" << height
               << ", sceneFBO valid=" << (sceneFBO_->isValid() ? "true" : "false")
               << ", blitTextureLoc=" << blitTextureLoc_);

    // Enable blending for final blit to ensure transparency
    // Use PREMULTIPLIED alpha blending (GL_ONE, GL_ONE_MINUS_SRC_ALPHA)
    // This is essential for HDR/Bloom effects where the RGB values represent
    // accumulated light energy and should not be multiplied by alpha again.
    // Standard blending (GL_SRC_ALPHA) causes double-darkening of faint glows.
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // Use tone-mapping blit shader for final presentation
    blitShader_->use();
    sceneFBO_->bindTexture(0);
    context_->extensions.glUniform1i(blitTextureLoc_, 0);

    // Check for GL errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        RE_LOG_THROTTLED(60, "  GL error before quad draw: " << (int)err);

    // Render fullscreen quad to screen
    fbPool_->renderFullscreenQuad();

    err = glGetError();
    if (err != GL_NO_ERROR)
        RE_LOG("  GL error after quad draw: " << (int)err);
    else
        RE_LOG_THROTTLED(60, "  quad draw OK");
}

void RenderEngine::setupCamera2D()
{
    // 2D rendering uses the shader's internal orthographic projection
    // based on bounds passed in ShaderRenderParams
}

void RenderEngine::setupCamera3D(const Settings3D& settings)
{
    if (!camera_)
        return;

    // Set camera position using orbit parameters
    camera_->setOrbitDistance(settings.cameraDistance);
    camera_->setOrbitPitch(settings.cameraAngleX);
    camera_->setOrbitYaw(settings.cameraAngleY);

    // Note: Auto-rotate would require adding animation support to Camera3D
    // For now, we can animate manually if autoRotate is enabled
    if (settings.autoRotate)
    {
        // Increment yaw based on rotate speed and delta time
        float currentYaw = camera_->getOrbitYaw();
        camera_->setOrbitYaw(currentYaw + settings.rotateSpeed * deltaTime_);
    }
}

void RenderEngine::copyFramebuffer(Framebuffer* source, Framebuffer* destination)
{
    if (!source || !destination || !source->isValid() || !destination->isValid())
        return;

    // Use composite shader for simple copy (linear pass-through)
    destination->bind();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    compositeShader_->use();
    source->bindTexture(0);
    context_->extensions.glUniform1i(compositeTextureLoc_, 0);

    fbPool_->renderFullscreenQuad();

    destination->unbind();
}

void RenderEngine::initializeEffects()
{
    // LAZY EFFECT LOADING: Create effect objects but don't compile them yet.
    // Effects are compiled on-demand when first used in applyPostProcessing().
    // This significantly reduces initialization time.
    effects_["vignette"] = std::make_unique<VignetteEffect>();
    effects_["film_grain"] = std::make_unique<FilmGrainEffect>();
    effects_["bloom"] = std::make_unique<BloomEffect>();
    effects_["trails"] = std::make_unique<TrailsEffect>();
    effects_["tilt_shift"] = std::make_unique<TiltShiftEffect>();
    effects_["color_grade"] = std::make_unique<ColorGradeEffect>();
    effects_["chromatic_aberration"] = std::make_unique<ChromaticAberrationEffect>();
    effects_["scanlines"] = std::make_unique<ScanlineEffect>();
    effects_["distortion"] = std::make_unique<DistortionEffect>();
    effects_["glitch"] = std::make_unique<GlitchEffect>();
    effects_["radial_blur"] = std::make_unique<RadialBlurEffect>();

    DBG("RenderEngine: Created " << effects_.size() << " effects (lazy compilation enabled)");

    // Initialize Effect Chain
    effectChain_.clear();

    // 1. Bloom
    effectChain_.addStep({
        "bloom",
        [](const VisualConfiguration& c) { return c.bloom.enabled; },
        [](PostProcessEffect* e, const VisualConfiguration& c) {
            if (auto* bloom = dynamic_cast<BloomEffect*>(e)) bloom->setSettings(c.bloom);
        }
    });

    // 1.5 Radial Blur
    effectChain_.addStep({
        "radial_blur",
        [](const VisualConfiguration& c) { return c.radialBlur.enabled; },
        [](PostProcessEffect* e, const VisualConfiguration& c) {
            if (auto* rb = dynamic_cast<RadialBlurEffect*>(e)) rb->setSettings(c.radialBlur);
        }
    });

    // NOTE: Trails effect is handled specially in applyPostProcessing()
    // because it requires per-waveform history FBO from WaveformRenderState.
    // The generic EffectChain only supports stateless effects.

    // 2. Tilt Shift
    effectChain_.addStep({
        "tilt_shift",
        [](const VisualConfiguration& c) { return c.tiltShift.enabled; },
        [](PostProcessEffect* e, const VisualConfiguration& c) {
            if (auto* ts = dynamic_cast<TiltShiftEffect*>(e)) ts->setSettings(c.tiltShift);
        }
    });

    // 3. Color Grade
    effectChain_.addStep({
        "color_grade",
        [](const VisualConfiguration& c) { return c.colorGrade.enabled; },
        [](PostProcessEffect* e, const VisualConfiguration& c) {
            if (auto* cg = dynamic_cast<ColorGradeEffect*>(e)) cg->setSettings(c.colorGrade);
        }
    });

    // 4. Chromatic Aberration
    effectChain_.addStep({
        "chromatic_aberration",
        [](const VisualConfiguration& c) { return c.chromaticAberration.enabled; },
        [](PostProcessEffect* e, const VisualConfiguration& c) {
            if (auto* ca = dynamic_cast<ChromaticAberrationEffect*>(e)) ca->setSettings(c.chromaticAberration);
        }
    });

    // 5. Scanlines
    effectChain_.addStep({
        "scanlines",
        [](const VisualConfiguration& c) { return c.scanlines.enabled; },
        [](PostProcessEffect* e, const VisualConfiguration& c) {
            if (auto* sl = dynamic_cast<ScanlineEffect*>(e)) sl->setSettings(c.scanlines);
        }
    });

    // 6. Distortion
    effectChain_.addStep({
        "distortion",
        [](const VisualConfiguration& c) { return c.distortion.enabled; },
        [](PostProcessEffect* e, const VisualConfiguration& c) {
            if (auto* d = dynamic_cast<DistortionEffect*>(e)) d->setSettings(c.distortion);
        }
    });

    // 7. Vignette
    effectChain_.addStep({
        "vignette",
        [](const VisualConfiguration& c) { return c.vignette.enabled; },
        [](PostProcessEffect* e, const VisualConfiguration& c) {
            if (auto* v = dynamic_cast<VignetteEffect*>(e)) v->setSettings(c.vignette);
        }
    });

    // 8. Film Grain
    effectChain_.addStep({
        "film_grain",
        [](const VisualConfiguration& c) { return c.filmGrain.enabled; },
        [](PostProcessEffect* e, const VisualConfiguration& c) {
            if (auto* fg = dynamic_cast<FilmGrainEffect*>(e)) fg->setSettings(c.filmGrain);
        }
    });
}

void RenderEngine::releaseEffects()
{
    for (auto& pair : effects_)
    {
        pair.second->release(*context_);
    }
    effects_.clear();
}

void RenderEngine::createDefaultEnvironmentMaps()
{
    if (!envMapManager_)
        return;

    // Create environment maps matching preset IDs used in VisualConfiguration
    envMapManager_->createFromPreset("default_studio", EnvironmentPreset::Studio);
    envMapManager_->createFromPreset("studio", EnvironmentPreset::Studio);
    envMapManager_->createFromPreset("neon", EnvironmentPreset::Neon);
    envMapManager_->createFromPreset("cyber_space", EnvironmentPreset::CyberSpace);
    envMapManager_->createFromPreset("dark", EnvironmentPreset::Dark);
    envMapManager_->createFromPreset("sunset", EnvironmentPreset::Sunset);
    envMapManager_->createFromPreset("abstract", EnvironmentPreset::Abstract);

    DBG("RenderEngine: Created 7 environment maps");
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
