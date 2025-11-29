/*
    Oscil - Particle System Implementation
    GPU-accelerated particle rendering with instancing
*/

#include "rendering/particles/ParticleSystem.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// Vertex shader for instanced particle rendering
static const char* particleVertexShader = R"(
    attribute vec2 aPosition;
    attribute vec2 aInstancePos;
    attribute vec4 aInstanceColor;
    attribute float aInstanceSize;
    attribute float aInstanceRotation;

    uniform mat4 uProjection;

    varying vec4 vColor;
    varying vec2 vTexCoord;

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
        vTexCoord = aPosition * 0.5 + 0.5;
    }
)";

// Fragment shader for particle rendering
static const char* particleFragmentShader = R"(
    varying vec4 vColor;
    varying vec2 vTexCoord;

    void main()
    {
        // Soft circular particle
        vec2 center = vTexCoord - vec2(0.5);
        float dist = length(center) * 2.0;

        // Smooth falloff
        float alpha = 1.0 - smoothstep(0.0, 1.0, dist);

        gl_FragColor = vec4(vColor.rgb, vColor.a * alpha);
    }
)";

ParticleSystem::ParticleSystem(int maxParticles)
    : pool_(maxParticles)
{
    instanceData_.reserve(static_cast<size_t>(maxParticles));
}

ParticleSystem::~ParticleSystem()
{
    // OpenGL resources should be released via release() before destruction
}

bool ParticleSystem::initialize(juce::OpenGLContext& context)
{
    if (initialized_)
        return true;

    auto& ext = context.extensions;

    // Compile shader
    if (!compileShader(context))
    {
        DBG("ParticleSystem: Failed to compile shader");
        return false;
    }

    // Create VAO
    ext.glGenVertexArrays(1, &vao_);
    ext.glBindVertexArray(vao_);

    // Create quad VBO (unit quad, will be scaled per-instance)
    float quadVertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f
    };

    ext.glGenBuffers(1, &quadVBO_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    ext.glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Get attribute locations
    posAttrib_ = ext.glGetAttribLocation(shader_->getProgramID(), "aPosition");
    instancePosAttrib_ = ext.glGetAttribLocation(shader_->getProgramID(), "aInstancePos");
    instanceColorAttrib_ = ext.glGetAttribLocation(shader_->getProgramID(), "aInstanceColor");
    instanceSizeAttrib_ = ext.glGetAttribLocation(shader_->getProgramID(), "aInstanceSize");
    instanceRotationAttrib_ = ext.glGetAttribLocation(shader_->getProgramID(), "aInstanceRotation");

    // Setup quad vertex attribute
    if (posAttrib_ >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(posAttrib_));
        ext.glVertexAttribPointer(static_cast<GLuint>(posAttrib_), 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    // Create instance VBO
    ext.glGenBuffers(1, &instanceVBO_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_);

    // Pre-allocate buffer for max particles
    size_t instanceBufferSize = static_cast<size_t>(pool_.getCapacity()) * sizeof(ParticleInstanceData);
    ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instanceBufferSize), nullptr, GL_DYNAMIC_DRAW);

    // Setup instance attributes
    size_t stride = sizeof(ParticleInstanceData);

    if (instancePosAttrib_ >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(instancePosAttrib_));
        ext.glVertexAttribPointer(static_cast<GLuint>(instancePosAttrib_), 2, GL_FLOAT, GL_FALSE,
                                  static_cast<GLsizei>(stride), reinterpret_cast<void*>(offsetof(ParticleInstanceData, x)));
        ext.glVertexAttribDivisor(static_cast<GLuint>(instancePosAttrib_), 1);
    }

    if (instanceColorAttrib_ >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(instanceColorAttrib_));
        ext.glVertexAttribPointer(static_cast<GLuint>(instanceColorAttrib_), 4, GL_FLOAT, GL_FALSE,
                                  static_cast<GLsizei>(stride), reinterpret_cast<void*>(offsetof(ParticleInstanceData, r)));
        ext.glVertexAttribDivisor(static_cast<GLuint>(instanceColorAttrib_), 1);
    }

    if (instanceSizeAttrib_ >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(instanceSizeAttrib_));
        ext.glVertexAttribPointer(static_cast<GLuint>(instanceSizeAttrib_), 1, GL_FLOAT, GL_FALSE,
                                  static_cast<GLsizei>(stride), reinterpret_cast<void*>(offsetof(ParticleInstanceData, size)));
        ext.glVertexAttribDivisor(static_cast<GLuint>(instanceSizeAttrib_), 1);
    }

    if (instanceRotationAttrib_ >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(instanceRotationAttrib_));
        ext.glVertexAttribPointer(static_cast<GLuint>(instanceRotationAttrib_), 1, GL_FLOAT, GL_FALSE,
                                  static_cast<GLsizei>(stride), reinterpret_cast<void*>(offsetof(ParticleInstanceData, rotation)));
        ext.glVertexAttribDivisor(static_cast<GLuint>(instanceRotationAttrib_), 1);
    }

    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);

    initialized_ = true;
    DBG("ParticleSystem: Initialized with capacity " << pool_.getCapacity());
    return true;
}

void ParticleSystem::release(juce::OpenGLContext& context)
{
    if (!initialized_)
        return;

    auto& ext = context.extensions;

    if (instanceVBO_ != 0)
    {
        ext.glDeleteBuffers(1, &instanceVBO_);
        instanceVBO_ = 0;
    }

    if (quadVBO_ != 0)
    {
        ext.glDeleteBuffers(1, &quadVBO_);
        quadVBO_ = 0;
    }

    if (vao_ != 0)
    {
        ext.glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    shader_.reset();
    initialized_ = false;
}

bool ParticleSystem::compileShader(juce::OpenGLContext& context)
{
    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!shader_->addVertexShader(particleVertexShader))
    {
        DBG("ParticleSystem: Vertex shader error: " << shader_->getLastError());
        return false;
    }

    if (!shader_->addFragmentShader(particleFragmentShader))
    {
        DBG("ParticleSystem: Fragment shader error: " << shader_->getLastError());
        return false;
    }

    if (!shader_->link())
    {
        DBG("ParticleSystem: Link error: " << shader_->getLastError());
        return false;
    }

    projectionLoc_ = shader_->getUniformIDFromName("uProjection");

    return true;
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
    // Update all particles in the pool
    pool_.updateAll(deltaTime, gravity_, drag_);
}

void ParticleSystem::updateEmitter(ParticleEmitterId emitterId,
                                   const std::vector<float>& samples,
                                   const juce::Rectangle<float>& bounds,
                                   float deltaTime)
{
    auto* emitter = getEmitter(emitterId);
    if (emitter)
    {
        emitter->update(pool_, samples, bounds, deltaTime);
    }
}

void ParticleSystem::render(juce::OpenGLContext& context,
                            int viewportWidth,
                            int viewportHeight,
                            ParticleBlendMode blendMode)
{
    if (!initialized_ || pool_.getAliveCount() == 0)
        return;

    // Upload particle data to GPU
    uploadParticleData(context);

    auto& ext = context.extensions;

    // Setup blend mode
    glEnable(GL_BLEND);
    switch (blendMode)
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

    glDisable(GL_DEPTH_TEST);

    // Use shader
    shader_->use();

    // Set orthographic projection (screen space)
    float projection[16] = {
        2.0f / static_cast<float>(viewportWidth), 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / static_cast<float>(viewportHeight), 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    ext.glUniformMatrix4fv(projectionLoc_, 1, GL_FALSE, projection);

    // Bind VAO and draw instanced
    ext.glBindVertexArray(vao_);

    // Re-bind quad VBO for position attribute
    ext.glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    if (posAttrib_ >= 0)
    {
        ext.glVertexAttribPointer(static_cast<GLuint>(posAttrib_), 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    // Draw instanced quads (4 vertices per quad, triangle fan)
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, static_cast<GLsizei>(instanceData_.size()));

    ext.glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void ParticleSystem::uploadParticleData(juce::OpenGLContext& context)
{
    instanceData_.clear();

    pool_.forEachAlive([this](const Particle& p, int) {
        ParticleInstanceData data;
        data.x = p.x;
        data.y = p.y;

        // Apply age-based alpha fade
        float age = p.getAge();
        data.r = p.r;
        data.g = p.g;
        data.b = p.b;
        data.a = p.a * (1.0f - age);  // Fade out over lifetime

        // Apply age-based size change (grow slightly, then shrink)
        float sizeMod = age < 0.2f ? (1.0f + age * 0.5f) : (1.1f - (age - 0.2f) * 0.3f);
        data.size = p.size * sizeMod;

        data.rotation = p.rotation;

        instanceData_.push_back(data);
    });

    if (instanceData_.empty())
        return;

    auto& ext = context.extensions;
    ext.glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_);
    ext.glBufferSubData(GL_ARRAY_BUFFER, 0,
                        static_cast<GLsizeiptr>(instanceData_.size() * sizeof(ParticleInstanceData)),
                        instanceData_.data());
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleSystem::clearAllParticles()
{
    pool_.freeAll();
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
