/*
    Oscil - Particle System Implementation
    GPU-accelerated particle rendering with instancing and soft particles
*/

#include "rendering/particles/ParticleSystem.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// ============================================================================
// Shaders
// ============================================================================

// Common Vertex Shader
static const char* particleVertexShader = R"(
    #version 330 core
    in vec2 aPosition; // Quad vertex (-0.5 to 0.5)
    in vec2 aInstancePos;
    in vec4 aInstanceColor;
    in float aInstanceSize;
    in float aInstanceRotation;
    in float aInstanceAge;

    uniform mat4 uProjection;
    uniform vec2 uFrameInfo; // x=rows, y=cols

    out vec4 vColor;
    out vec2 vTexCoord;
    out float vAge;

    void main()
    {
        // Rotate the quad vertex
        float c = cos(aInstanceRotation);
        float s = sin(aInstanceRotation);
        vec2 rotated = vec2(
            aPosition.x * c - aPosition.y * s,
            aPosition.x * s + aPosition.y * c
        );

        // Scale and translate
        vec2 worldPos = aInstancePos + rotated * aInstanceSize;

        gl_Position = uProjection * vec4(worldPos, 0.0, 1.0);

        vColor = aInstanceColor;
        vAge = aInstanceAge;

        // Texture Coordinates (Standard 0-1)
        vec2 baseUV = aPosition + 0.5;
        
        // Sprite Sheet Calculation
        float rows = uFrameInfo.x;
        float cols = uFrameInfo.y;
        
        if (rows > 1.0 || cols > 1.0)
        {
            // Animate based on age (0 to 1)
            float totalFrames = rows * cols;
            float currentFrame = floor(aInstanceAge * totalFrames);
            currentFrame = clamp(currentFrame, 0.0, totalFrames - 1.0);
            
            // Calculate column and row (top-left origin for texture UVs)
            // GL UVs are bottom-left 0,0. 
            // If texture is loaded standard, UV (0,1) is top-left.
            // Let's assume standard mapping: col increases X, row increases Y (or decreases Y).
            // Standard grid: row 0 is top.
            
            float col = mod(currentFrame, cols);
            float row = floor(currentFrame / cols);
            
            // Scale UVs
            vec2 size = vec2(1.0 / cols, 1.0 / rows);
            
            // Map UVs
            // Invert row because OpenGL UV (0,0) is bottom-left
            // So row 0 (top) should be high Y.
            vTexCoord = (baseUV * size) + vec2(col * size.x, 1.0 - (row + 1.0) * size.y);
        }
        else
        {
            vTexCoord = baseUV;
        }
    }
)";

// Basic Fragment Shader (Circle)
static const char* basicFragmentShader = R"(
    #version 330 core
    in vec4 vColor;
    in vec2 vTexCoord;
    in float vAge;

    out vec4 fragColor;

    void main()
    {
        // Soft circular particle
        // Re-center UVs to -0.5 to 0.5 for distance calculation
        // Note: vTexCoord here is the sprite sheet UV, which is tricky for circles if using atlas.
        // However, basic shader assumes rows=1, cols=1, so vTexCoord is 0-1.
        
        vec2 center = vTexCoord - vec2(0.5);
        float dist = length(center) * 2.0;

        // Smooth falloff
        float alpha = 1.0 - smoothstep(0.0, 1.0, dist);

        fragColor = vec4(vColor.rgb, vColor.a * alpha);
    }
)";

// Textured Fragment Shader
static const char* texturedFragmentShader = R"(
    #version 330 core
    in vec4 vColor;
    in vec2 vTexCoord;
    in float vAge;

    uniform sampler2D uTexture;

    out vec4 fragColor;

    void main()
    {
        vec4 texColor = texture(uTexture, vTexCoord);
        fragColor = vColor * texColor;
    }
)";

// Soft Particle Fragment Shader (Textured + Depth)
// Note: Requires depth texture bound to uDepthMap
static const char* softFragmentShader = R"(
    #version 330 core
    in vec4 vColor;
    in vec2 vTexCoord;
    in float vAge;

    uniform sampler2D uTexture;
    uniform sampler2D uDepthMap;
    uniform vec2 uViewportSize;
    uniform float uSoftDepthParam; // Controls softness range

    out vec4 fragColor;

    float linearizeDepth(float depth) {
        float zNear = 0.1;
        float zFar = 100.0;
        return (2.0 * zNear) / (zFar + zNear - depth * (zFar - zNear));
    }

    void main()
    {
        vec4 texColor = texture(uTexture, vTexCoord);
        
        // Screen coordinates for depth lookup
        vec2 screenUV = gl_FragCoord.xy / uViewportSize;
        
        // Sample scene depth
        float sceneDepth = texture(uDepthMap, screenUV).r;
        
        // Particle depth (gl_FragCoord.z is non-linear)
        float partDepth = gl_FragCoord.z;
        
        // Calculate fade factor
        // Use raw depth difference for simplicity in ortho/perspective mix
        // Higher difference = more opaque. As diff approaches 0 (intersection), alpha fades.
        float depthDiff = sceneDepth - partDepth;
        
        // Scale softness
        float alphaFade = clamp(depthDiff * uSoftDepthParam * 100.0, 0.0, 1.0);
        
        fragColor = vColor * texColor;
        fragColor.a *= alphaFade;
    }
)";

// ============================================================================
// Noise3D Implementation
// ============================================================================

// Permutation table
static const int p[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,
    125,136,171,168, 68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,
    105,92,41,55,46,245,40,244,102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,
    196,135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,5,202,38,147,118,126,
    255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152, 2,44,154,163, 70,
    221,153,101,155,167, 43,172,9,129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,
    97,228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,49,192,214, 31,
    181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,138,236,205,93,222,114,67,29,24,72,
    243,141,128,195,78,66,215,61,156,180,
    // Duplicate for overflow
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,
    125,136,171,168, 68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,
    105,92,41,55,46,245,40,244,102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,
    196,135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,5,202,38,147,118,126,
    255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152, 2,44,154,163, 70,
    221,153,101,155,167, 43,172,9,129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,
    97,228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,49,192,214, 31,
    181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,138,236,205,93,222,114,67,29,24,72,
    243,141,128,195,78,66,215,61,156,180
};

float Noise3D::fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
float Noise3D::lerp(float t, float a, float b) { return a + t * (b - a); }
float Noise3D::grad(int hash, float x, float y, float z)
{
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float Noise3D::perlin(float x, float y, float z)
{
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    int Z = (int)floor(z) & 255;
    x -= floor(x);
    y -= floor(y);
    z -= floor(z);
    float u = fade(x);
    float v = fade(y);
    float w = fade(z);
    int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
    int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

    return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z),
                                   grad(p[BA], x - 1, y, z)),
                           lerp(u, grad(p[AB], x, y - 1, z),
                                   grad(p[BB], x - 1, y - 1, z))),
                   lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1),
                                   grad(p[BA + 1], x - 1, y, z - 1)),
                           lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
                                   grad(p[BB + 1], x - 1, y - 1, z - 1))));
}

// ============================================================================
// ParticleSystem Implementation
// ============================================================================

ParticleSystem::ParticleSystem(int maxParticles)
    : pool_(maxParticles)
{
    instanceData_.reserve(static_cast<size_t>(maxParticles));
}

ParticleSystem::~ParticleSystem()
{
}

void ParticleSystem::release(juce::OpenGLContext& context)
{
    if (!initialized_)
        return;

    auto& ext = context.extensions;

    if (vao_ != 0)
    {
        ext.glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    if (quadVBO_ != 0)
    {
        ext.glDeleteBuffers(1, &quadVBO_);
        quadVBO_ = 0;
    }

    if (instanceVBO_ != 0)
    {
        ext.glDeleteBuffers(1, &instanceVBO_);
        instanceVBO_ = 0;
    }

    basicShader_.program.reset();
    texturedShader_.program.reset();
    softShader_.program.reset();
    
    initialized_ = false;
}

bool ParticleSystem::initialize(juce::OpenGLContext& context)
{
    if (initialized_)
        return true;

    auto& ext = context.extensions;

    // Compile shaders
    if (!compileShaders(context))
    {
        DBG("ParticleSystem: Failed to compile shaders");
        return false;
    }

    // Create VAO
    ext.glGenVertexArrays(1, &vao_);
    ext.glBindVertexArray(vao_);

    // Create quad VBO (unit quad centered at 0,0)
    float quadVertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f
    };

    ext.glGenBuffers(1, &quadVBO_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    ext.glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Fixed attribute locations (must match binding in compileShaders)
    GLuint posAttrib = 0;
    GLuint instancePosAttrib = 1;
    GLuint instanceColorAttrib = 2;
    GLuint instanceSizeAttrib = 3;
    GLuint instanceRotationAttrib = 4;
    GLuint instanceAgeAttrib = 5;

    // Setup quad vertex attribute
    ext.glEnableVertexAttribArray(posAttrib);
    ext.glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Create instance VBO
    ext.glGenBuffers(1, &instanceVBO_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_);

    // Pre-allocate buffer for max particles
    size_t instanceBufferSize = static_cast<size_t>(pool_.getCapacity()) * sizeof(ParticleInstanceData);
    ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instanceBufferSize), nullptr, GL_DYNAMIC_DRAW);

    // Setup instance attributes
    size_t stride = sizeof(ParticleInstanceData);

    ext.glEnableVertexAttribArray(instancePosAttrib);
    ext.glVertexAttribPointer(instancePosAttrib, 2, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(stride), reinterpret_cast<void*>(offsetof(ParticleInstanceData, x)));
    glVertexAttribDivisor(instancePosAttrib, 1);

    ext.glEnableVertexAttribArray(instanceColorAttrib);
    ext.glVertexAttribPointer(instanceColorAttrib, 4, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(stride), reinterpret_cast<void*>(offsetof(ParticleInstanceData, r)));
    glVertexAttribDivisor(instanceColorAttrib, 1);

    ext.glEnableVertexAttribArray(instanceSizeAttrib);
    ext.glVertexAttribPointer(instanceSizeAttrib, 1, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(stride), reinterpret_cast<void*>(offsetof(ParticleInstanceData, size)));
    glVertexAttribDivisor(instanceSizeAttrib, 1);

    ext.glEnableVertexAttribArray(instanceRotationAttrib);
    ext.glVertexAttribPointer(instanceRotationAttrib, 1, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(stride), reinterpret_cast<void*>(offsetof(ParticleInstanceData, rotation)));
    glVertexAttribDivisor(instanceRotationAttrib, 1);

    ext.glEnableVertexAttribArray(instanceAgeAttrib);
    ext.glVertexAttribPointer(instanceAgeAttrib, 1, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(stride), reinterpret_cast<void*>(offsetof(ParticleInstanceData, age)));
    glVertexAttribDivisor(instanceAgeAttrib, 1);

    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);

    initialized_ = true;
    DBG("ParticleSystem: Initialized with capacity " << pool_.getCapacity());
    return true;
}

bool ParticleSystem::compileShaders(juce::OpenGLContext& context)
{
    auto compile = [&](ShaderProgram& shader, const char* fragSource) -> bool {
        shader.program = std::make_unique<juce::OpenGLShaderProgram>(context);
        shader.program->addVertexShader(particleVertexShader);
        shader.program->addFragmentShader(fragSource);
        
        // Explicitly bind attribute locations before linking
        // This ensures consistency across all shaders and matches VAO setup
        context.extensions.glBindAttribLocation(shader.program->getProgramID(), 0, "aPosition");
        context.extensions.glBindAttribLocation(shader.program->getProgramID(), 1, "aInstancePos");
        context.extensions.glBindAttribLocation(shader.program->getProgramID(), 2, "aInstanceColor");
        context.extensions.glBindAttribLocation(shader.program->getProgramID(), 3, "aInstanceSize");
        context.extensions.glBindAttribLocation(shader.program->getProgramID(), 4, "aInstanceRotation");
        context.extensions.glBindAttribLocation(shader.program->getProgramID(), 5, "aInstanceAge");

        if (!shader.program->link())
        {
            DBG("ParticleSystem: Shader compile error: " << shader.program->getLastError());
            return false;
        }
        
        shader.projectionLoc = shader.program->getUniformIDFromName("uProjection");
        shader.textureLoc = shader.program->getUniformIDFromName("uTexture");
        shader.depthLoc = shader.program->getUniformIDFromName("uDepthMap");
        shader.viewportSizeLoc = shader.program->getUniformIDFromName("uViewportSize");
        shader.softDepthParamLoc = shader.program->getUniformIDFromName("uSoftDepthParam");
        shader.frameInfoLoc = shader.program->getUniformIDFromName("uFrameInfo");
        return true;
    };

    bool ok = compile(basicShader_, basicFragmentShader);
    ok &= compile(texturedShader_, texturedFragmentShader);
    ok &= compile(softShader_, softFragmentShader);

    return ok;
}

ParticleEmitterId ParticleSystem::createEmitter(const ParticleEmitterConfig& config)
{
    ParticleEmitterId id = nextEmitterId_++;
    auto emitter = std::make_unique<ParticleEmitter>(id);
    emitter->setConfig(config);
    emitters_[id] = std::move(emitter);
    return id;
}

ParticleEmitter* ParticleSystem::getEmitter(ParticleEmitterId id)
{
    auto it = emitters_.find(id);
    return it != emitters_.end() ? it->second.get() : nullptr;
}

void ParticleSystem::destroyEmitter(ParticleEmitterId id)
{
    emitters_.erase(id);
}

void ParticleSystem::destroyAllEmitters()
{
    emitters_.clear();
}

void ParticleSystem::update(float deltaTime)
{
    time_ += deltaTime;
    
    // Use turbulence if we can access settings?
    // Currently settings are passed in render().
    // We need stored settings or assume a global turbulence for now.
    // Let's implement a basic turbulence field that is always active but scales with a "strength" we can set later.
    // Ideally, `ParticleSystem` should store current config or have `setTurbulence(strength, scale, speed)`.
    
    // For now, hardcode a small turbulence or use 0 if not set.
    // Let's add member variables for turbulence to ParticleSystem class in header first?
    // No, let's just implement the mechanism. 
    // We will use a default strength of 0 unless set. 
    // Wait, I haven't added setters for turbulence yet.
    // Let's assume 0 for now, but write the code so it works when I add setters.
    
    float strength = turbulenceStrength_; 
    float scale = turbulenceScale_;
    float speed = turbulenceSpeed_;

    if (strength > 0.001f)
    {
        pool_.updateAllWithCallback(deltaTime, gravity_, drag_, [&](Particle& particle, float dt) {
            // Curl noise approximation or simple vector field
            // 2D curl noise from 3D perlin slice
            float x = particle.x * 0.01f * scale;
            float y = particle.y * 0.01f * scale;
            float z = time_ * speed;

            // Sample noise for angle
            float angle = Noise3D::perlin(x, y, z) * 6.28318f * 2.0f; // 2 loops
            
            // Force vector
            float fx = std::cos(angle) * strength;
            float fy = std::sin(angle) * strength;
            
            // Apply force to velocity
            particle.vx += fx * dt * 60.0f; // Scale to be noticeable
            particle.vy += fy * dt * 60.0f;
        });
    }
    else
    {
        pool_.updateAll(deltaTime, gravity_, drag_);
    }
}

void ParticleSystem::updateEmitter(ParticleEmitterId emitterId,
                                   const std::vector<float>& samples,
                                   const juce::Rectangle<float>& bounds,
                                   float deltaTime,
                                   float verticalScale)
{
    auto* emitter = getEmitter(emitterId);
    if (emitter)
    {
        emitter->update(pool_, samples, bounds, deltaTime, verticalScale);
    }
}

void ParticleSystem::render(juce::OpenGLContext& context,
                            int viewportWidth,
                            int viewportHeight,
                            ParticleBlendMode blendMode)
{
    // Legacy render all
    ParticleSettings s;
    s.blendMode = blendMode;
    render(context, viewportWidth, viewportHeight, {}, s); 
}

void ParticleSystem::render(juce::OpenGLContext& context,
                            int viewportWidth,
                            int viewportHeight,
                            const std::vector<ParticleEmitterId>& emitterIds,
                            const ParticleSettings& settings)
{
    if (!initialized_ || pool_.getAliveCount() == 0)
        return;

    // Upload particle data filtered by emitters (or all if empty)
    uploadParticleData(context, emitterIds);
    
    if (instanceData_.empty())
        return;

    auto& ext = context.extensions;

    // Select shader
    ShaderProgram* shader = &basicShader_;
    bool useTexture = settings.textureId.isNotEmpty();
    
    if (settings.softParticles && useTexture)
    {
        shader = &softShader_;
    }
    else if (useTexture)
    {
        shader = &texturedShader_;
    }

    shader->program->use();

    // Setup blend mode
    glEnable(GL_BLEND);
    switch (settings.blendMode)
    {
        case ParticleBlendMode::Additive:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case ParticleBlendMode::Alpha:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case ParticleBlendMode::Multiply:
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            break;
        case ParticleBlendMode::Screen:
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
            break;
    }

    // Depth testing
    // Soft particles require depth test OFF but depth sampling ON
    // Usually particles are transparent so we disable depth writes
    glDepthMask(GL_FALSE);
    if (settings.softParticles)
        glDisable(GL_DEPTH_TEST); // We manually check depth in shader
    else
        glDisable(GL_DEPTH_TEST); // 2D overlay usually ignores depth

    // Set projection
    float projection[16] = {
        2.0f / static_cast<float>(viewportWidth), 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / static_cast<float>(viewportHeight), 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    ext.glUniformMatrix4fv(shader->projectionLoc, 1, GL_FALSE, projection);

    // Set uniforms
    if (useTexture && textureManager_)
    {
        textureManager_->bindTexture(settings.textureId, 0);
        ext.glUniform1i(shader->textureLoc, 0);
        
        // Frame info
        ext.glUniform2f(shader->frameInfoLoc, (float)settings.textureRows, (float)settings.textureCols);
    }
    else
    {
        ext.glUniform2f(shader->frameInfoLoc, 1.0f, 1.0f);
    }

    if (settings.softParticles)
    {
        // Bind depth texture from RenderEngine's scene FBO (needs plumbing)
        // Actually, RenderEngine needs to expose the depth texture.
        // Since we don't have access to RenderEngine instance here (only textureManager),
        // we have a dependency loop.
        // Solution: Assume TextureManager or caller handles binding unit 1?
        // Or pass depth texture ID in `render`?
        // For now, assume unit 1 and caller binds it.
        ext.glUniform1i(shader->depthLoc, 1);
        ext.glUniform2f(shader->viewportSizeLoc, (float)viewportWidth, (float)viewportHeight);
        ext.glUniform1f(shader->softDepthParamLoc, settings.softDepthSensitivity);
    }

    // Bind VAO and draw
    ext.glBindVertexArray(vao_);
    
    // VBO binding and vertex attrib pointer setup is already done in initialize() and stored in VAO
    // Just need to ensure we're drawing instances if any
    
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, static_cast<GLsizei>(instanceData_.size()));

    ext.glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

void ParticleSystem::uploadParticleData(juce::OpenGLContext& context, const std::vector<ParticleEmitterId>& emitterIds)
{
    instanceData_.clear();
    bool filter = !emitterIds.empty();

    // Helper to check if ID is in list (linear search is fast for small N)
    auto shouldInclude = [&](int id) {
        if (!filter) return true;
        for (auto eid : emitterIds) if (eid == id) return true;
        return false;
    };

    pool_.forEachAlive([this, &shouldInclude](const Particle& particle, int) {
        if (!shouldInclude(particle.emitterId))
            return;

        ParticleInstanceData data;
        data.x = particle.x;
        data.y = particle.y;

        // Age based fade (handled in shader via vAge or precomputed here)
        // Let's precompute alpha fade but pass age for animation
        float age = particle.getAge();
        
        data.r = particle.r;
        data.g = particle.g;
        data.b = particle.b;
        data.a = particle.a * (1.0f - age * age); // Quadratic fade out

        data.size = particle.size;
        data.rotation = particle.rotation;
        data.age = age;

        instanceData_.push_back(data);
    });

    if (instanceData_.empty())
        return;

    auto& ext = context.extensions;
    ext.glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_);
    
    // Orphan buffer if needed, or just subdata
    // glBufferData with NULL orphans it
    ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instanceData_.size() * sizeof(ParticleInstanceData)), instanceData_.data(), GL_DYNAMIC_DRAW);
    
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleSystem::clearAllParticles()
{
    pool_.freeAll();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL