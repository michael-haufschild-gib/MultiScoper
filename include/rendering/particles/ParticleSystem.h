/*
    Oscil - Particle System
    GPU-accelerated particle rendering with instancing
*/

#pragma once

#include "Particle.h"
#include "ParticlePool.h"
#include "ParticleEmitter.h"
#include <juce_opengl/juce_opengl.h>
#include <unordered_map>
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * GPU-accelerated particle system with instanced rendering.
 * Manages multiple emitters and renders all particles efficiently.
 */
class ParticleSystem
{
public:
    /**
     * Create particle system.
     * @param maxParticles Maximum total particles across all emitters
     */
    explicit ParticleSystem(int maxParticles = 10000);

    ~ParticleSystem();

    // Non-copyable
    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    /**
     * Initialize OpenGL resources.
     * Must be called on the OpenGL thread.
     */
    bool initialize(juce::OpenGLContext& context);

    /**
     * Release OpenGL resources.
     */
    void release(juce::OpenGLContext& context);

    /**
     * Check if system is initialized.
     */
    [[nodiscard]] bool isInitialized() const { return initialized_; }

    /**
     * Create a new emitter.
     * @param config Emitter configuration
     * @return Emitter ID for future reference
     */
    ParticleEmitterId createEmitter(const ParticleEmitterConfig& config = {});

    /**
     * Get an emitter by ID.
     * @return Pointer to emitter, or nullptr if not found
     */
    ParticleEmitter* getEmitter(ParticleEmitterId id);

    /**
     * Destroy an emitter.
     */
    void destroyEmitter(ParticleEmitterId id);

    /**
     * Destroy all emitters.
     */
    void destroyAllEmitters();

    /**
     * Update all emitters and particles.
     * @param deltaTime Time step in seconds
     */
    void update(float deltaTime);

    /**
     * Update a specific emitter with waveform data.
     * @param emitterId Emitter to update
     * @param samples Waveform sample data
     * @param bounds Screen bounds for the waveform
     * @param deltaTime Time step
     * @param verticalScale Waveform vertical scaling factor
     */
    void updateEmitter(ParticleEmitterId emitterId,
                       const std::vector<float>& samples,
                       const juce::Rectangle<float>& bounds,
                       float deltaTime,
                       float verticalScale = 1.0f);

    /**
     * Render all particles.
     * @param context OpenGL context
     * @param viewportWidth Viewport width in pixels
     * @param viewportHeight Viewport height in pixels
     * @param blendMode Particle blend mode
     */
    void render(juce::OpenGLContext& context,
                int viewportWidth,
                int viewportHeight,
                ParticleBlendMode blendMode = ParticleBlendMode::Additive);

    /**
     * Get the particle pool.
     */
    [[nodiscard]] ParticlePool& getPool() { return pool_; }
    [[nodiscard]] const ParticlePool& getPool() const { return pool_; }

    /**
     * Get alive particle count.
     */
    [[nodiscard]] int getAliveCount() const { return pool_.getAliveCount(); }

    /**
     * Set global physics parameters.
     */
    void setGravity(float gravity) { gravity_ = gravity; }
    void setDrag(float drag) { drag_ = drag; }

    /**
     * Clear all particles immediately.
     */
    void clearAllParticles();

private:
    /**
     * Upload particle data to GPU.
     */
    void uploadParticleData(juce::OpenGLContext& context);

    /**
     * Compile particle shader.
     */
    bool compileShader(juce::OpenGLContext& context);

    ParticlePool pool_;
    std::unordered_map<ParticleEmitterId, std::unique_ptr<ParticleEmitter>> emitters_;
    ParticleEmitterId nextEmitterId_ = 1;

    // Physics
    float gravity_ = 0.0f;
    float drag_ = 0.0f;

    // OpenGL resources
    bool initialized_ = false;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;
    GLuint vao_ = 0;
    GLuint quadVBO_ = 0;      // Quad vertices (shared)
    GLuint instanceVBO_ = 0;  // Per-particle instance data

    // Shader uniforms
    GLint projectionLoc_ = -1;

    // Attribute locations
    GLint posAttrib_ = -1;
    GLint instancePosAttrib_ = -1;
    GLint instanceColorAttrib_ = -1;
    GLint instanceSizeAttrib_ = -1;
    GLint instanceRotationAttrib_ = -1;

    // Instance data buffer for CPU staging
    struct ParticleInstanceData
    {
        float x, y;       // Position
        float r, g, b, a; // Color
        float size;       // Size
        float rotation;   // Rotation angle
    };
    std::vector<ParticleInstanceData> instanceData_;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
