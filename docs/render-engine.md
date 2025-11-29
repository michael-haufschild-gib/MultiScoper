# Render Engine Architecture for Oscil

This document defines the **render engine architecture** and **visual effects pipeline** for the Oscil multi-track oscilloscope plugin. It provides a complete roadmap from current state to AAA-quality visual rendering including post-processing, particle systems, 3D visualization, and advanced materials.

---

## 1. Current State Analysis

### What We Have Now

```
Current Architecture:
┌─────────────────────────────────────────────────────────┐
│ WaveformGLRenderer                                       │
│   - Single-pass rendering                               │
│   - Renders directly to screen (default framebuffer)    │
│   - No FBO management                                   │
│   - No post-processing                                  │
└─────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│ WaveformShader (base class)                             │
│   - BaseShader (current implementation)                 │
│   - Simply renders waveform geometry                    │
│   - No effect chain support                             │
└─────────────────────────────────────────────────────────┘
```

**Key Files:**
- `include/rendering/WaveformGLRenderer.h` - Current renderer
- `include/rendering/WaveformShader.h` - Shader base class
- `src/rendering/shaders/BasicShader.cpp` - Example shader

### What We're Building

A **per-waveform visual effects pipeline** with four major systems:

1. **Post-Processing Effects** - Bloom, trails, color grading, etc.
2. **Particle Systems** - Particle Stream, Granular Dust, Nebula effects
3. **3D Visualization** - Volumetric ribbons, wireframe meshes, vector flow
4. **Advanced Materials** - Glass refraction, liquid chrome, crystalline effects

---

## 2. Target Architecture

### Design Principle: Per-Waveform Effects

Each waveform is an independent visual entity with its own:
- Waveform shader (2D or 3D geometry rendering)
- Particle emitters (optional)
- Effect chain (post-processing)
- Material properties (for advanced shaders)

### Complete Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              RenderEngine                                    │
│         Orchestrates all rendering systems per-waveform                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐  ┌────────────┐ │
│  │FramebufferPool │  │ WaveformStates │  │ ParticleSystem │  │ Camera3D   │ │
│  │ - waveformFBO  │  │ - historyFBO   │  │ - emitters[]   │  │ - view     │ │
│  │ - pingFBO      │  │ - effectConfig │  │ - particlePool │  │ - proj     │ │
│  │ - pongFBO      │  │ - particles[]  │  │ - GPU buffers  │  │ - controls │ │
│  └────────────────┘  └────────────────┘  └────────────────┘  └────────────┘ │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                     Per-Waveform Render Pipeline                             │
│                                                                              │
│   1. Render waveform geometry ──► waveformFBO                               │
│      (2D shader OR 3D shader with camera)                                   │
│                                                                              │
│   2. Render particles ──► waveformFBO (additive blend)                      │
│      (GPU instanced quads, waveform-driven emission)                        │
│                                                                              │
│   3. Apply post-processing ──► ping/pong FBOs                               │
│      (Bloom → Trails → ColorGrade → etc.)                                   │
│                                                                              │
│   4. Composite ──► sceneFBO (with blend mode)                               │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                            Final Output                                      │
│   - Optional global effects (vignette, film grain)                          │
│   - Blit to screen                                                          │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Visual Effect Categories (from Reference Designs)

| Category | Effects | Implementation |
|----------|---------|----------------|
| **Post-Processing** | Neon Bloom, Phosphor CRT, Digital Glitch, Heat Signature, Dual Outline Glow | PostProcessEffect classes |
| **Particle Systems** | Particle Stream, Particle Pulse, Granular Dust, Nebula Additive | ParticleEmitter + ParticleSystem |
| **3D Geometry** | Volumetric Ribbon, Wireframe Mesh, Vector Flow, String Theory | WaveformShader3D subclasses |
| **Advanced Materials** | Glass Refraction, Liquid Chrome, Crystalline Wavetable, Plasma Sine | Material shaders with env mapping |

---

## 3. Core Components

### 3.1 RenderEngine

The central orchestrator managing all rendering subsystems.

```cpp
// include/rendering/RenderEngine.h

namespace oscil
{

class RenderEngine
{
public:
    RenderEngine();
    ~RenderEngine();

    // Lifecycle
    void initialize(juce::OpenGLContext& context);
    void shutdown();
    void resize(int width, int height);

    // Per-frame rendering
    void beginFrame();
    void renderWaveform(const WaveformRenderData& data);
    void endFrame();

    // Waveform state management
    void registerWaveform(int waveformId);
    void unregisterWaveform(int waveformId);

    // Subsystem access
    ParticleSystem& getParticleSystem() { return *particleSystem_; }
    Camera3D& getCamera() { return camera_; }

    // Global settings
    void setQualityLevel(QualityLevel level);
    void setBackgroundColour(juce::Colour colour);

private:
    void renderWaveformGeometry(const WaveformRenderData& data, Framebuffer* target);
    void renderWaveformParticles(const WaveformRenderData& data, Framebuffer* target);
    void applyPostProcessing(WaveformRenderState& state, Framebuffer* source);
    void compositeToScene(Framebuffer* source, const WaveformRenderData& data);

    std::unique_ptr<FramebufferPool> fbPool_;
    std::unique_ptr<Framebuffer> sceneFBO_;
    std::unique_ptr<ParticleSystem> particleSystem_;
    std::unordered_map<int, WaveformRenderState> waveformStates_;

    Camera3D camera_;
    QualityLevel qualityLevel_ = QualityLevel::Normal;
    juce::Colour backgroundColour_{0xFF1A1A1A};

    juce::OpenGLContext* context_ = nullptr;
    bool initialized_ = false;
};

} // namespace oscil
```

### 3.2 FramebufferPool

Manages shared FBOs for efficient memory usage.

```cpp
// include/rendering/FramebufferPool.h

namespace oscil
{

struct Framebuffer
{
    GLuint fbo = 0;
    GLuint colorTexture = 0;
    GLuint depthBuffer = 0;  // For 3D rendering
    int width = 0;
    int height = 0;
    GLenum format = GL_RGBA8;

    void create(juce::OpenGLContext& context, int w, int h, GLenum fmt, bool withDepth = false);
    void destroy(juce::OpenGLContext& context);
    void resize(juce::OpenGLContext& context, int w, int h);
    void bind();
    void unbind();
};

class FramebufferPool
{
public:
    void initialize(juce::OpenGLContext& context, int width, int height);
    void shutdown(juce::OpenGLContext& context);
    void resize(int width, int height);

    Framebuffer* acquireWaveformFBO();  // With depth buffer for 3D
    Framebuffer* acquirePingFBO();
    Framebuffer* acquirePongFBO();
    void releaseAll();

    void renderFullscreenQuad();

private:
    juce::OpenGLContext* context_ = nullptr;
    std::unique_ptr<Framebuffer> waveformFBO_;
    std::unique_ptr<Framebuffer> pingFBO_;
    std::unique_ptr<Framebuffer> pongFBO_;

    GLuint quadVAO_ = 0;
    GLuint quadVBO_ = 0;
    int width_ = 0;
    int height_ = 0;
};

} // namespace oscil
```

### 3.3 WaveformRenderState

Persistent per-waveform state including particles and history.

```cpp
// include/rendering/WaveformRenderState.h

namespace oscil
{

struct WaveformRenderState
{
    int waveformId = 0;

    // Trails (stateful post-processing)
    bool trailsEnabled = false;
    std::unique_ptr<Framebuffer> historyFBO;

    // Particle emitters for this waveform
    std::vector<ParticleEmitterId> emitterIds;

    // Full visual configuration
    VisualConfiguration visualConfig;

    void enableTrails(juce::OpenGLContext& context, int width, int height);
    void disableTrails(juce::OpenGLContext& context);
    void addParticleEmitter(ParticleEmitterId id);
    void removeParticleEmitter(ParticleEmitterId id);
};

} // namespace oscil
```

### 3.4 VisualConfiguration

Complete per-waveform visual settings (replaces EffectConfiguration).

```cpp
// include/rendering/VisualConfiguration.h

namespace oscil
{

// Shader selection
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

    // Material Shaders
    GlassRefraction,
    LiquidChrome,
    Crystalline
};

// Post-processing settings (unchanged from before)
struct BloomSettings { /* ... */ };
struct TrailSettings { /* ... */ };
struct ColorGradeSettings { /* ... */ };
struct VignetteSettings { /* ... */ };
struct FilmGrainSettings { /* ... */ };
struct ChromaticAberrationSettings { /* ... */ };
struct ScanlineSettings { /* ... */ };
struct DistortionSettings { /* ... */ };

// NEW: Particle settings
struct ParticleSettings
{
    bool enabled = false;
    ParticleEmissionMode emissionMode = ParticleEmissionMode::AlongWaveform;
    float emissionRate = 100.0f;       // Particles per second
    float particleLife = 2.0f;         // Seconds
    float particleSize = 4.0f;         // Pixels
    juce::Colour particleColor{0xFFFFAA00};
    ParticleBlendMode blendMode = ParticleBlendMode::Additive;

    // Physics
    float gravity = 0.0f;
    float drag = 0.1f;
    float randomness = 0.5f;
    float velocityScale = 1.0f;

    // Audio reactivity
    bool audioReactive = true;
    float audioEmissionBoost = 2.0f;   // Multiplier on transients
};

// NEW: 3D settings
struct Settings3D
{
    bool enabled = false;
    float cameraDistance = 5.0f;
    float cameraAngleX = 15.0f;        // Degrees
    float cameraAngleY = 0.0f;
    bool autoRotate = false;
    float rotateSpeed = 10.0f;         // Degrees per second

    // For mesh-based shaders
    int meshResolutionX = 128;
    int meshResolutionZ = 32;          // History depth
    float meshScale = 1.0f;
};

// NEW: Material settings
struct MaterialSettings
{
    bool enabled = false;
    float reflectivity = 0.5f;
    float refractiveIndex = 1.5f;      // Glass = 1.5, Water = 1.33
    float fresnelPower = 2.0f;
    juce::Colour tintColor{0xFFFFFFFF};
    float roughness = 0.1f;

    // Environment
    bool useEnvironmentMap = true;
    juce::String environmentMapId = "default_studio";
};

// Complete visual configuration
struct VisualConfiguration
{
    // Shader selection
    ShaderType shaderType = ShaderType::NeonGlow;

    // Post-processing
    BloomSettings bloom;
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

    // Serialization
    juce::ValueTree toValueTree() const;
    static VisualConfiguration fromValueTree(const juce::ValueTree& tree);
};

} // namespace oscil
```

---

## 4. Render Pipeline Flow

### 4.1 Complete Per-Frame Rendering

```cpp
void RenderEngine::renderFrame(const std::vector<WaveformRenderData>& waveforms)
{
    // 1. Update particle system (physics simulation)
    float deltaTime = getFrameDeltaTime();
    particleSystem_->update(deltaTime);

    // 2. Begin frame - clear scene FBO
    sceneFBO_->bind();
    glClearColor(bg.r, bg.g, bg.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 3. Render each waveform with full pipeline
    for (const auto& waveform : waveforms)
    {
        if (!waveform.visible)
            continue;

        auto& state = waveformStates_[waveform.id];
        renderWaveformComplete(waveform, state);
    }

    // 4. Apply global effects (optional)
    applyGlobalEffects();

    // 5. Blit scene to screen
    sceneFBO_->unbind();
    blitToScreen(sceneFBO_.get());
}

void RenderEngine::renderWaveformComplete(
    const WaveformRenderData& data,
    WaveformRenderState& state)
{
    const auto& config = state.visualConfig;
    auto* waveformFBO = fbPool_->acquireWaveformFBO();

    // Step 1: Clear and set up waveform FBO
    waveformFBO->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Step 2: Set up camera for 3D shaders
    if (is3DShader(config.shaderType))
    {
        setupCamera3D(config.settings3D);
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        setupCamera2D();
        glDisable(GL_DEPTH_TEST);
    }

    // Step 3: Render waveform geometry
    renderWaveformGeometry(data, config);

    // Step 4: Render particles (additive blend onto waveform)
    if (config.particles.enabled)
    {
        renderWaveformParticles(data, state);
    }

    // Step 5: Apply post-processing effects
    Framebuffer* current = waveformFBO;
    current = applyPostProcessing(current, state);

    // Step 6: Composite onto scene
    sceneFBO_->bind();
    compositeToScene(current, config);

    // Step 7: Release pool resources
    fbPool_->releaseAll();
}
```

### 4.2 Geometry Rendering (2D and 3D)

```cpp
void RenderEngine::renderWaveformGeometry(
    const WaveformRenderData& data,
    const VisualConfiguration& config)
{
    auto* shader = ShaderRegistry::getInstance().getShader(config.shaderType);

    if (is3DShader(config.shaderType))
    {
        // 3D shaders receive view/projection matrices
        auto* shader3D = static_cast<WaveformShader3D*>(shader);
        shader3D->setViewMatrix(camera_.getViewMatrix());
        shader3D->setProjectionMatrix(camera_.getProjectionMatrix());
        shader3D->setMeshSettings(config.settings3D);
    }

    if (isMaterialShader(config.shaderType))
    {
        // Material shaders receive environment and material properties
        auto* materialShader = static_cast<MaterialShader*>(shader);
        materialShader->setMaterialSettings(config.material);
        materialShader->bindEnvironmentMap(config.material.environmentMapId);
    }

    // Render the waveform
    ShaderRenderParams params;
    params.colour = data.colour;
    params.opacity = data.opacity;
    params.lineWidth = data.lineWidth;
    params.bounds = data.bounds;
    params.isStereo = data.isStereo;
    params.verticalScale = data.verticalScale;

    shader->render(*context_, data.channel1, data.channel2, params);
}
```

---

## 5. Particle System Architecture

### 5.1 Overview

The particle system enables effects like "Particle Stream", "Granular Dust", and "Nebula Additive" from the reference designs.

```
Particle System Architecture:
┌─────────────────────────────────────────────────────────────────┐
│ ParticleSystem                                                   │
│   - Manages all particle emitters and rendering                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────┐  │
│  │ ParticlePool    │    │ EmitterRegistry │    │ GPU Buffers │  │
│  │ - particles[]   │    │ - emitters[]    │    │ - positionVBO│  │
│  │ - MAX_PARTICLES │    │ - per-waveform  │    │ - colorVBO  │  │
│  │ - freeList      │    │ - configurations│    │ - instanceVAO│  │
│  └─────────────────┘    └─────────────────┘    └─────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 Core Classes

```cpp
// include/rendering/particles/Particle.h

namespace oscil
{

struct Particle
{
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec4 color;
    float size;
    float life;          // 0.0 = dead, 1.0 = just born
    float maxLife;
    float rotation;
    float rotationSpeed;
};

enum class ParticleEmissionMode
{
    AlongWaveform,       // Emit uniformly along waveform path
    AtPeaks,             // Emit at amplitude peaks
    AtZeroCrossings,     // Emit at zero crossings
    Continuous,          // Emit from center regardless of waveform
    Burst                // Emit all at once on trigger
};

enum class ParticleBlendMode
{
    Additive,            // Glow effect (GL_ONE, GL_ONE)
    Alpha,               // Standard transparency
    Multiply,            // Darken
    Screen               // Lighten
};

} // namespace oscil
```

```cpp
// include/rendering/particles/ParticleEmitter.h

namespace oscil
{

using ParticleEmitterId = int;

struct ParticleEmitterConfig
{
    ParticleEmissionMode emissionMode = ParticleEmissionMode::AlongWaveform;

    // Emission
    float emissionRate = 100.0f;       // Particles/second
    int maxParticles = 1000;           // Per emitter limit

    // Particle properties
    float particleLife = 2.0f;
    float particleLifeVariance = 0.5f;
    float particleSize = 4.0f;
    float particleSizeVariance = 2.0f;
    float particleSizeOverLife = -0.5f; // Shrink over lifetime

    // Initial velocity
    float initialSpeed = 50.0f;
    float initialSpeedVariance = 20.0f;
    float spreadAngle = 45.0f;         // Degrees from waveform normal

    // Physics
    glm::vec2 gravity{0.0f, 50.0f};    // Pixels/sec^2
    float drag = 0.1f;
    float turbulence = 0.0f;

    // Color
    juce::Colour colorStart{0xFFFFAA00};
    juce::Colour colorEnd{0x00FF4400};  // Alpha fades out

    // Rendering
    ParticleBlendMode blendMode = ParticleBlendMode::Additive;
    juce::String spriteTextureId = "";  // Empty = circle

    // Audio reactivity
    bool audioReactive = true;
    float audioEmissionMultiplier = 2.0f;
    float audioSizeMultiplier = 1.5f;
};

class ParticleEmitter
{
public:
    ParticleEmitter(ParticleEmitterId id, const ParticleEmitterConfig& config);

    void update(float deltaTime, ParticlePool& pool);
    void emitAlongWaveform(const std::vector<float>& samples,
                           const juce::Rectangle<float>& bounds,
                           ParticlePool& pool);
    void emitAtPeaks(const std::vector<float>& samples,
                     const juce::Rectangle<float>& bounds,
                     float threshold,
                     ParticlePool& pool);

    void setAudioMetrics(const AudioMetrics& metrics);
    void burst(int count, glm::vec2 position, ParticlePool& pool);

    ParticleEmitterId getId() const { return id_; }
    const ParticleEmitterConfig& getConfig() const { return config_; }
    void setConfig(const ParticleEmitterConfig& config) { config_ = config; }

private:
    ParticleEmitterId id_;
    ParticleEmitterConfig config_;
    float emissionAccumulator_ = 0.0f;
    AudioMetrics lastAudioMetrics_;

    Particle createParticle(glm::vec2 position, glm::vec2 direction);
};

} // namespace oscil
```

```cpp
// include/rendering/particles/ParticleSystem.h

namespace oscil
{

class ParticleSystem
{
public:
    static constexpr int MAX_PARTICLES = 50000;

    ParticleSystem();
    ~ParticleSystem();

    void initialize(juce::OpenGLContext& context);
    void shutdown();

    // Emitter management
    ParticleEmitterId createEmitter(const ParticleEmitterConfig& config);
    void destroyEmitter(ParticleEmitterId id);
    ParticleEmitter* getEmitter(ParticleEmitterId id);

    // Per-frame operations
    void update(float deltaTime);
    void render(juce::OpenGLContext& context,
                const glm::mat4& projection,
                ParticleEmitterId emitterId = -1);  // -1 = render all

    // Waveform integration
    void feedWaveformData(ParticleEmitterId emitterId,
                          const std::vector<float>& samples,
                          const juce::Rectangle<float>& bounds);

    // Audio reactivity
    void setGlobalAudioMetrics(const AudioMetrics& metrics);

    // Stats
    int getActiveParticleCount() const;
    int getActiveEmitterCount() const;

private:
    void updateGPUBuffers();
    void compileShaders();

    ParticlePool pool_;
    std::unordered_map<ParticleEmitterId, std::unique_ptr<ParticleEmitter>> emitters_;
    ParticleEmitterId nextEmitterId_ = 1;

    // GPU resources
    GLuint vao_ = 0;
    GLuint positionVBO_ = 0;
    GLuint colorVBO_ = 0;
    GLuint sizeVBO_ = 0;
    std::unique_ptr<juce::OpenGLShaderProgram> particleShader_;

    juce::OpenGLContext* context_ = nullptr;
    bool initialized_ = false;
};

} // namespace oscil
```

### 5.3 Particle Pool (Memory Management)

```cpp
// include/rendering/particles/ParticlePool.h

namespace oscil
{

class ParticlePool
{
public:
    explicit ParticlePool(int maxParticles);

    Particle* allocate();
    void free(Particle* particle);
    void freeAll();

    void updateAll(float deltaTime);

    // Iteration for rendering
    template<typename Func>
    void forEachAlive(Func&& func);

    int getAliveCount() const { return aliveCount_; }
    int getCapacity() const { return static_cast<int>(particles_.size()); }

private:
    std::vector<Particle> particles_;
    std::vector<int> freeIndices_;
    int aliveCount_ = 0;
};

} // namespace oscil
```

### 5.4 GPU Instanced Rendering

```glsl
// Particle vertex shader
attribute vec2 vertexPosition;    // Quad corner (-1 to 1)
attribute vec2 particlePosition;  // Instance: world position
attribute vec4 particleColor;     // Instance: RGBA
attribute float particleSize;     // Instance: size in pixels

uniform mat4 projection;
uniform float time;

varying vec4 vColor;
varying vec2 vTexCoord;

void main() {
    vColor = particleColor;
    vTexCoord = vertexPosition * 0.5 + 0.5;

    vec2 worldPos = particlePosition + vertexPosition * particleSize;
    gl_Position = projection * vec4(worldPos, 0.0, 1.0);
}
```

```glsl
// Particle fragment shader
uniform sampler2D spriteTexture;
uniform bool useSprite;

varying vec4 vColor;
varying vec2 vTexCoord;

void main() {
    vec4 texColor = useSprite
        ? texture2D(spriteTexture, vTexCoord)
        : vec4(1.0);  // Default: use color only

    // Soft circle falloff for non-sprite particles
    if (!useSprite) {
        float dist = length(vTexCoord - vec2(0.5));
        float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
        texColor = vec4(1.0, 1.0, 1.0, alpha);
    }

    gl_FragColor = vColor * texColor;
}
```

### 5.5 Particle Effect Presets

| Preset | Emission Mode | Blend | Physics | Use Case |
|--------|--------------|-------|---------|----------|
| **Particle Stream** | AlongWaveform | Additive | Low gravity, high drag | Flowing energy |
| **Particle Pulse** | AtPeaks | Additive | Burst outward | Reactive to transients |
| **Granular Dust** | AlongWaveform | Alpha | Gravity down, turbulence | Dusty/sandy look |
| **Nebula Additive** | Continuous | Additive | Very low drag, no gravity | Ethereal clouds |
| **Spark Burst** | Burst | Additive | High gravity, low drag | Firework-like |

---

## 6. 3D Shader Support

### 6.1 Overview

3D shaders enable effects like "Volumetric Ribbon", "Wireframe Mesh", and "Vector Flow".

### 6.2 WaveformShader3D Base Class

```cpp
// include/rendering/shaders/WaveformShader3D.h

namespace oscil
{

class WaveformShader3D : public WaveformShader
{
public:
    // Dimension identification
    ShaderDimension getDimension() const override { return ShaderDimension::ThreeD; }

    // 3D-specific setup
    virtual void setViewMatrix(const glm::mat4& view) = 0;
    virtual void setProjectionMatrix(const glm::mat4& proj) = 0;
    virtual void setMeshSettings(const Settings3D& settings) = 0;

    // History buffer for temporal effects (waterfall, etc.)
    virtual void setHistorySamples(const std::vector<std::vector<float>>& history) {}
    virtual int getRequiredHistoryDepth() const { return 0; }

protected:
    glm::mat4 viewMatrix_;
    glm::mat4 projectionMatrix_;
};

} // namespace oscil
```

### 6.3 Camera3D

```cpp
// include/rendering/Camera3D.h

namespace oscil
{

class Camera3D
{
public:
    Camera3D();

    // Configuration
    void setDistance(float distance);
    void setAngles(float angleX, float angleY);  // Degrees
    void setTarget(const glm::vec3& target);
    void setFOV(float fovDegrees);
    void setAspectRatio(float aspect);

    // Animation
    void setAutoRotate(bool enabled, float degreesPerSecond = 10.0f);
    void update(float deltaTime);

    // Matrices
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;

    // Interaction (for future mouse control)
    void orbit(float deltaX, float deltaY);
    void zoom(float delta);

private:
    glm::vec3 target_{0.0f, 0.0f, 0.0f};
    float distance_ = 5.0f;
    float angleX_ = 15.0f;  // Pitch
    float angleY_ = 0.0f;   // Yaw
    float fov_ = 45.0f;
    float aspectRatio_ = 16.0f / 9.0f;
    float nearPlane_ = 0.1f;
    float farPlane_ = 100.0f;

    bool autoRotate_ = false;
    float rotateSpeed_ = 10.0f;
};

} // namespace oscil
```

### 6.4 3D Shader: Volumetric Ribbon

Creates a 3D tube/ribbon extruded along the waveform path.

```cpp
// include/rendering/shaders/VolumetricRibbonShader.h

namespace oscil
{

class VolumetricRibbonShader : public WaveformShader3D
{
public:
    juce::String getId() const override { return "volumetric_ribbon"; }
    juce::String getDisplayName() const override { return "Volumetric Ribbon"; }

    void render(juce::OpenGLContext& context,
                const std::vector<float>& channel1,
                const std::vector<float>* channel2,
                const ShaderRenderParams& params) override;

private:
    void generateTubeGeometry(const std::vector<float>& samples,
                               std::vector<float>& vertices,
                               std::vector<float>& normals,
                               std::vector<unsigned int>& indices);

    int tubeSegments_ = 8;  // Circular cross-section resolution
    float tubeRadius_ = 0.05f;
};

} // namespace oscil
```

### 6.5 3D Shader: Wireframe Mesh

Creates a 3D terrain/waterfall visualization.

```cpp
// include/rendering/shaders/WireframeMeshShader.h

namespace oscil
{

class WireframeMeshShader : public WaveformShader3D
{
public:
    juce::String getId() const override { return "wireframe_mesh"; }
    juce::String getDisplayName() const override { return "Wireframe Mesh"; }

    int getRequiredHistoryDepth() const override { return meshDepth_; }

    void setHistorySamples(const std::vector<std::vector<float>>& history) override;

    void render(juce::OpenGLContext& context,
                const std::vector<float>& channel1,
                const std::vector<float>* channel2,
                const ShaderRenderParams& params) override;

private:
    void generateMeshGeometry(std::vector<float>& vertices,
                               std::vector<unsigned int>& indices);

    std::vector<std::vector<float>> sampleHistory_;
    int meshWidth_ = 128;   // X resolution
    int meshDepth_ = 32;    // Z resolution (history)
    float meshScaleX_ = 2.0f;
    float meshScaleY_ = 1.0f;
    float meshScaleZ_ = 2.0f;
};

} // namespace oscil
```

### 6.6 History Buffer Management

For 3D effects that show waveform over time (waterfall displays):

```cpp
// In WaveformRenderState
struct WaveformRenderState
{
    // ... existing members ...

    // Sample history for 3D temporal effects
    std::deque<std::vector<float>> sampleHistory;
    int maxHistoryDepth = 64;

    void pushSamples(const std::vector<float>& samples)
    {
        sampleHistory.push_front(samples);
        while (sampleHistory.size() > maxHistoryDepth)
            sampleHistory.pop_back();
    }
};
```

---

## 7. Advanced Material Shaders

### 7.1 Overview

Material shaders create effects like "Glass Refraction", "Liquid Chrome", and "Crystalline Wavetable".

### 7.2 MaterialShader Base Class

```cpp
// include/rendering/shaders/MaterialShader.h

namespace oscil
{

class MaterialShader : public WaveformShader
{
public:
    virtual void setMaterialSettings(const MaterialSettings& settings) = 0;
    virtual void bindEnvironmentMap(const juce::String& mapId) = 0;

protected:
    MaterialSettings material_;

    // Environment mapping
    GLuint environmentCubemap_ = 0;
    bool hasEnvironmentMap_ = false;
};

} // namespace oscil
```

### 7.3 Environment Map Manager

```cpp
// include/rendering/EnvironmentMapManager.h

namespace oscil
{

class EnvironmentMapManager
{
public:
    static EnvironmentMapManager& getInstance();

    void initialize(juce::OpenGLContext& context);
    void shutdown();

    // Load cubemap from 6 faces or equirectangular HDR
    void loadCubemap(const juce::String& id, const juce::File& file);
    void loadBuiltInMaps();

    GLuint getCubemap(const juce::String& id) const;
    std::vector<juce::String> getAvailableMapIds() const;

private:
    std::unordered_map<juce::String, GLuint> cubemaps_;
    juce::OpenGLContext* context_ = nullptr;
};

} // namespace oscil
```

### 7.4 Material Shader: Glass Refraction

```glsl
// Glass refraction fragment shader
uniform sampler2D sceneTexture;      // Background to refract
uniform samplerCube environmentMap;
uniform vec3 cameraPosition;
uniform float refractiveIndex;       // Glass = 1.5
uniform float fresnelPower;
uniform vec4 tintColor;

varying vec3 vWorldPosition;
varying vec3 vWorldNormal;
varying vec2 vTexCoord;

void main() {
    vec3 viewDir = normalize(cameraPosition - vWorldPosition);
    vec3 normal = normalize(vWorldNormal);

    // Fresnel effect
    float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), fresnelPower);

    // Refraction
    vec3 refractDir = refract(-viewDir, normal, 1.0 / refractiveIndex);
    vec2 refractOffset = refractDir.xy * 0.1;
    vec4 refractColor = texture2D(sceneTexture, vTexCoord + refractOffset);

    // Reflection from environment
    vec3 reflectDir = reflect(-viewDir, normal);
    vec4 reflectColor = textureCube(environmentMap, reflectDir);

    // Combine based on fresnel
    vec4 finalColor = mix(refractColor, reflectColor, fresnel);
    finalColor *= tintColor;

    gl_FragColor = finalColor;
}
```

### 7.5 Material Shader: Liquid Chrome

```glsl
// Liquid chrome fragment shader
uniform samplerCube environmentMap;
uniform vec3 cameraPosition;
uniform float reflectivity;
uniform float roughness;

varying vec3 vWorldPosition;
varying vec3 vWorldNormal;

void main() {
    vec3 viewDir = normalize(cameraPosition - vWorldPosition);
    vec3 normal = normalize(vWorldNormal);

    // Reflection
    vec3 reflectDir = reflect(-viewDir, normal);

    // Roughness: blur by sampling multiple mip levels
    float mipLevel = roughness * 8.0;
    vec4 reflectColor = textureCubeLod(environmentMap, reflectDir, mipLevel);

    // Chrome tint (slight blue/purple shift in shadows)
    vec3 chrome = reflectColor.rgb;
    float ndotv = max(dot(normal, viewDir), 0.0);
    chrome = mix(chrome * vec3(0.8, 0.8, 1.0), chrome, ndotv);

    gl_FragColor = vec4(chrome * reflectivity, 1.0);
}
```

### 7.6 Built-in Environment Maps

| Map ID | Description | Use Case |
|--------|-------------|----------|
| `default_studio` | Soft studio lighting | General purpose |
| `neon_city` | Colorful neon reflections | Cyberpunk themes |
| `gradient_warm` | Warm orange/yellow gradient | Analog feel |
| `gradient_cool` | Cool blue/purple gradient | Digital/clean |
| `abstract_noise` | Procedural noise pattern | Experimental |

---

## 8. Post-Processing Effects Specifications

*(Unchanged from previous version - see Sections 5.1-5.10)*

### 8.1 Effect Ordering

```
Complete Render Order (per-waveform):
1. Waveform geometry (2D or 3D shader)
2. Particles (additive blend)
3. Bloom
4. Trails
5. Color Grade
6. Chromatic Aberration
7. Vignette
8. Film Grain
9. Composite to scene
```

---

## 9. Quality Presets

### 9.1 Eco (Performance Priority)

**Enabled:**
- Basic 2D shaders only
- Vignette
- Film Grain (low)

**Disabled:**
- Particles
- 3D shaders
- Material shaders
- Bloom, Trails, Chromatic Aberration

**Limits:**
- Max waveforms: 8
- Max particles: 0

---

### 9.2 Normal (Balanced)

**Enabled:**
- All 2D shaders
- Particles (limited)
- Bloom (2 iterations)
- Trails
- Color Grade
- Vignette
- Film Grain

**Disabled:**
- 3D shaders
- Material shaders

**Limits:**
- Max waveforms with effects: 6
- Max particles per waveform: 500
- Max total particles: 5000

---

### 9.3 Ultra (Visual Priority)

**Enabled:**
- All shader types
- Full particle system
- All post-processing effects
- Environment mapping

**Limits:**
- Max waveforms with full effects: 4
- Max particles per waveform: 2000
- Max total particles: 20000
- 3D mesh resolution: 128x64

---

## 10. Implementation Phases

### Phase 0: Foundation (Prerequisites)

**Goal:** Build core infrastructure.

**Tasks:**
1. Create `Framebuffer` with depth buffer support
2. Create `FramebufferPool`
3. Create `RenderEngine` shell
4. Implement fullscreen quad rendering
5. Create `PostProcessEffect` base class

**Files:**
- `include/rendering/Framebuffer.h`
- `include/rendering/FramebufferPool.h`
- `include/rendering/RenderEngine.h`
- `src/rendering/*.cpp`

---

### Phase 1: Basic Post-Processing

**Goal:** Validate FBO pipeline with simple effects.

**Tasks:**
1. Implement `VignetteEffect`
2. Implement `FilmGrainEffect`
3. Add ping-pong buffer management
4. Per-waveform effect application

---

### Phase 2: Core Post-Processing

**Goal:** Implement main visual effects.

**Tasks:**
1. Bloom (multi-pass)
2. Color Grading
3. Trails (with history FBO)
4. Chromatic Aberration

---

### Phase 3: Visual Configuration System

**Goal:** Per-waveform settings infrastructure.

**Tasks:**
1. Create `VisualConfiguration` struct
2. ValueTree serialization
3. Extend `Oscillator` class
4. Basic UI for effect toggles

---

### Phase 4: Additional Post-Processing

**Goal:** Complete effect library.

**Tasks:**
1. Scanlines/CRT
2. Distortion
3. Sharpen
4. Digital Glitch effect

---

### Phase 5: Particle System Foundation

**Goal:** Core particle infrastructure.

**Tasks:**
1. Create `Particle` and `ParticlePool`
2. Create `ParticleEmitter` class
3. Create `ParticleSystem` manager
4. GPU instanced rendering

**Files:**
- `include/rendering/particles/Particle.h`
- `include/rendering/particles/ParticlePool.h`
- `include/rendering/particles/ParticleEmitter.h`
- `include/rendering/particles/ParticleSystem.h`
- `src/rendering/particles/*.cpp`

---

### Phase 6: Particle Effects

**Goal:** Waveform-integrated particle emission.

**Tasks:**
1. AlongWaveform emission mode
2. AtPeaks emission mode
3. Audio-reactive emission
4. Particle effect presets

---

### Phase 7: 3D Shader Foundation

**Goal:** 3D rendering infrastructure.

**Tasks:**
1. Create `Camera3D` class
2. Create `WaveformShader3D` base class
3. Depth buffer management
4. View/projection matrix pipeline

**Files:**
- `include/rendering/Camera3D.h`
- `include/rendering/shaders/WaveformShader3D.h`
- `src/rendering/Camera3D.cpp`

---

### Phase 8: 3D Shader Effects

**Goal:** Implement 3D visualizations.

**Tasks:**
1. Volumetric Ribbon shader
2. Wireframe Mesh shader
3. History buffer for temporal 3D
4. Vector Flow shader

---

### Phase 9: Material System

**Goal:** Advanced material rendering.

**Tasks:**
1. Create `MaterialShader` base class
2. Create `EnvironmentMapManager`
3. Load built-in cubemaps
4. Glass Refraction shader
5. Liquid Chrome shader
6. Crystalline shader

**Files:**
- `include/rendering/shaders/MaterialShader.h`
- `include/rendering/EnvironmentMapManager.h`
- `src/rendering/shaders/GlassRefractionShader.cpp`
- `src/rendering/shaders/LiquidChromeShader.cpp`

---

### Phase 10: Polish & Optimization

**Goal:** Production-ready quality.

**Tasks:**
1. Quality tier enforcement
2. Performance profiling
3. Visual presets (complete themes)
4. Full UI integration
5. Audio reactivity system
6. Documentation and examples

---

## 11. Technical Requirements

### 11.1 OpenGL Version

- **Minimum:** OpenGL 3.2 Core Profile
- **Recommended:** OpenGL 4.1 for instancing and cubemap LOD

### 11.2 FBO Texture Formats

| FBO | Format | Depth | Usage |
|-----|--------|-------|-------|
| Scene | GL_RGBA8 | No | Final composite |
| Waveform | GL_RGBA16F | Yes | HDR + 3D rendering |
| Ping/Pong | GL_RGBA16F | No | Effect chain |
| History | GL_RGBA8 | No | Trails |

### 11.3 Performance Budgets

| Quality | FPS | Post-FX Passes | Particles | 3D Enabled |
|---------|-----|----------------|-----------|------------|
| Eco | 60 | 2 | 0 | No |
| Normal | 60 | 8 | 5000 | No |
| Ultra | 30-60 | 16+ | 20000 | Yes |

### 11.4 Memory Budgets

| Resource | Eco | Normal | Ultra |
|----------|-----|--------|-------|
| FBOs | 4 | 6 | 10 |
| History FBOs | 2 | 4 | 8 |
| Particle buffer | 0 | 5K particles | 50K particles |
| Environment maps | 0 | 0 | 3 cubemaps |

---

## 12. Effect Catalog (Reference Design Mapping)

### From waveforms1.jpeg

| Design | Implementation |
|--------|----------------|
| Neon Bloom | BasicShader + BloomEffect (high intensity) |
| Phosphor CRT | BasicShader + ScanlineEffect + ColorGrade (green tint) |
| Gradient Fill | GradientFillShader (new 2D shader) |
| Glass Refraction | GlassRefractionShader (material) |
| Volumetric Ribbon | VolumetricRibbonShader (3D) |
| Particle Stream | ParticleEmitter (AlongWaveform, Additive) |
| Heat Signature | BasicShader + ColorGrade (thermal LUT) |
| Wireframe Mesh | WireframeMeshShader (3D) |
| Liquid Chrome | LiquidChromeShader (material) |
| Aurora Borealis | GradientShader + BloomEffect + ColorGrade |
| Digital Glitch | BasicShader + GlitchEffect (new post-process) |
| Dual Outline Glow | DualOutlineShader (new 2D shader) |

### From waveforms2.jpeg

| Design | Implementation |
|--------|----------------|
| Plasma Sine | PlasmaShader (new 2D shader, procedural) |
| Neon Square | BasicShader + BloomEffect |
| Glitch Sawtooth | BasicShader + GlitchEffect |
| Liquid Triangle | LiquidChromeShader (material) |
| Particle Pulse | ParticleEmitter (AtPeaks, Burst) |
| Spectral Noise | SpectralNoiseShader (new, uses FFT data) |
| Crystalline Wavetable | CrystallineShader (material) |
| Fractal FM | BasicShader (thin) + BloomEffect (high) |
| Nebula Additive | ParticleEmitter (Continuous, very slow) |
| Granular Dust | ParticleEmitter (AlongWaveform, Alpha, gravity) |
| String Theory Karplus | MultiLineShader (new 2D, multiple oscillating lines) |
| Vector Flow | VectorFlowShader (3D ribbons) |

---

## 13. Audio Reactivity System

*(Optional enhancement - implement after Phase 10)*

### 13.1 AudioMetrics

```cpp
struct AudioMetrics
{
    float rms = 0.0f;
    float peak = 0.0f;
    float lowEnergy = 0.0f;
    float midEnergy = 0.0f;
    float highEnergy = 0.0f;
    bool transientDetected = false;
    float spectralCentroid = 0.0f;
    float spectralFlux = 0.0f;
};
```

### 13.2 Integration Points

| System | Metric | Effect |
|--------|--------|--------|
| Particles | transientDetected | Burst emission |
| Particles | rms | Emission rate multiplier |
| Bloom | rms | Intensity modulation |
| Trails | transientDetected | Decay reduction (sharper) |
| Color Grade | lowEnergy | Temperature shift |
| 3D Camera | rms | Distance/zoom |

---

## 14. Testing Strategy

### 14.1 Unit Tests

- `test_framebuffer.cpp`
- `test_particle_pool.cpp`
- `test_particle_emitter.cpp`
- `test_camera3d.cpp`
- `test_visual_configuration.cpp`

### 14.2 Visual Tests

- Screenshot comparison per shader
- Particle system stress test
- 3D rendering validation
- Material reflection accuracy

### 14.3 Performance Tests

- FPS with particle counts: 1K, 5K, 10K, 20K, 50K
- 3D shader mesh complexity scaling
- Effect chain throughput
- Memory usage tracking

---

## 15. Appendix: Additional Shader Examples

### 15.1 Gradient Fill Shader

```glsl
// Fragment shader for gradient fill under waveform
uniform sampler2D waveformTexture;
uniform vec4 colorTop;
uniform vec4 colorBottom;
uniform float waveformY;  // Normalized 0-1

varying vec2 vTexCoord;

void main() {
    float waveY = texture2D(waveformTexture, vec2(vTexCoord.x, 0.5)).r;
    float fillMask = step(vTexCoord.y, waveY);

    vec4 gradient = mix(colorBottom, colorTop, vTexCoord.y);
    gl_FragColor = gradient * fillMask;
}
```

### 15.2 Plasma Procedural Shader

```glsl
uniform float time;
uniform vec4 color1;
uniform vec4 color2;
uniform float frequency;
uniform float amplitude;

varying vec2 vTexCoord;

float plasma(vec2 uv, float t) {
    float v = sin(uv.x * 10.0 + t);
    v += sin((uv.y * 10.0 + t) * 0.5);
    v += sin((uv.x * 10.0 + uv.y * 10.0 + t) * 0.5);
    v += sin(length(uv * 10.0) + t);
    return v * 0.25 + 0.5;
}

void main() {
    float p = plasma(vTexCoord * frequency, time);
    vec4 color = mix(color1, color2, p);

    // Apply to waveform amplitude
    float waveform = getWaveformAmplitude(vTexCoord.x);
    float mask = smoothstep(0.0, 0.1, abs(vTexCoord.y - 0.5 - waveform * amplitude));

    gl_FragColor = color * (1.0 - mask);
}
```

---

*Document Version: 3.0*
*Last Updated: 2025-11-29*
*Status: Complete Architecture Specification - Particles, 3D, Materials Included*
