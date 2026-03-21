/*
    Oscil - Particle Emitter
    Controls particle spawning behavior and patterns
*/

#pragma once

#include "Particle.h"
#include "ParticlePool.h"
#include <juce_graphics/juce_graphics.h>
#include <vector>
#include <random>
#include <optional>

namespace oscil
{

/**
 * Unique identifier for emitters.
 */
using ParticleEmitterId = int;

/**
 * Configuration for a particle emitter.
 */
struct ParticleEmitterConfig
{
    // Emission mode
    ParticleEmissionMode mode = ParticleEmissionMode::AlongWaveform;

    // Emission rate (particles per second)
    float emissionRate = 100.0f;

    // Particle properties
    float particleLifeMin = 1.0f;
    float particleLifeMax = 2.0f;
    float particleSizeMin = 2.0f;
    float particleSizeMax = 6.0f;

    // Velocity settings
    float velocityMin = 10.0f;
    float velocityMax = 50.0f;
    float velocityAngleSpread = 360.0f;  // Degrees, 360 = omnidirectional

    // Initial direction bias (normalized, added to random direction)
    float directionBiasX = 0.0f;
    float directionBiasY = -1.0f;  // Default: upward

    // Physics
    float gravity = 50.0f;   // Pixels per second squared
    float drag = 0.5f;       // Velocity damping

    // Color
    juce::Colour colorStart = juce::Colours::white;
    juce::Colour colorEnd = juce::Colours::white.withAlpha(0.0f);
    bool useWaveformColor = true;  // Override with waveform's color

    // Rotation
    float rotationSpeedMin = 0.0f;
    float rotationSpeedMax = 0.0f;

    // Burst mode settings
    int burstCount = 50;
};

/**
 * Emits particles based on waveform data and configuration.
 */
class ParticleEmitter
{
public:
    /**
     * Create an emitter with given ID.
     * @param id Unique identifier
     */
    explicit ParticleEmitter(ParticleEmitterId id);

    /**
     * Get emitter ID.
     */
    [[nodiscard]] ParticleEmitterId getId() const { return id_; }

    /**
     * Set emitter configuration.
     */
    void setConfig(const ParticleEmitterConfig& config) { config_ = config; }

    /**
     * Get current configuration.
     */
    [[nodiscard]] const ParticleEmitterConfig& getConfig() const { return config_; }

    /**
     * Set the waveform color (used when useWaveformColor is true).
     */
    void setWaveformColor(juce::Colour color) { waveformColor_ = color; }

    /**
     * Update emitter and spawn particles.
     * @param pool Particle pool to allocate from
     * @param samples Current waveform sample data (normalized -1 to 1)
     * @param bounds Screen bounds for the waveform
     * @param deltaTime Time step in seconds
     * @param verticalScale Waveform vertical scaling factor
     */
    void update(ParticlePool& pool,
                const std::vector<float>& samples,
                const juce::Rectangle<float>& bounds,
                float deltaTime,
                float verticalScale = 1.0f);

    /**
     * Trigger a burst emission.
     * @param pool Particle pool
     * @param samples Waveform data
     * @param bounds Screen bounds
     * @param verticalScale Waveform vertical scaling factor
     */
    void triggerBurst(ParticlePool& pool,
                      const std::vector<float>& samples,
                      const juce::Rectangle<float>& bounds,
                      float verticalScale = 1.0f);

    /**
     * Enable/disable the emitter.
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }
    [[nodiscard]] bool isEnabled() const { return enabled_; }

    /**
     * Reset emitter state.
     */
    void reset();

private:
    /**
     * Spawn a single particle at given position.
     */
    void spawnParticle(ParticlePool& pool, float x, float y, std::optional<float> baseAngle = std::nullopt);

    /**
     * Get waveform position at given index.
     */
    juce::Point<float> getWaveformPosition(
        const std::vector<float>& samples,
        const juce::Rectangle<float>& bounds,
        size_t index,
        float verticalScale) const;

    /**
     * Find peak indices in samples.
     */
    std::vector<size_t> findPeaks(const std::vector<float>& samples) const;

    /**
     * Find zero crossing indices in samples.
     */
    std::vector<size_t> findZeroCrossings(const std::vector<float>& samples) const;

    float computeNormalAngle(const std::vector<float>& samples,
                             const juce::Rectangle<float>& bounds,
                             size_t idx, float verticalScale);

    void emitAtIndices(ParticlePool& pool, const std::vector<size_t>& indices,
                       const std::vector<float>& samples,
                       const juce::Rectangle<float>& bounds,
                       int count, float verticalScale);

    ParticleEmitterId id_;
    ParticleEmitterConfig config_;
    juce::Colour waveformColor_ = juce::Colours::cyan;

    bool enabled_ = true;
    float emissionAccumulator_ = 0.0f;

    // Random number generation
    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist01_{0.0f, 1.0f};
};

} // namespace oscil
