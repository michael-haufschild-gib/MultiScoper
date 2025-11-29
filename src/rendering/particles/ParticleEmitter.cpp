/*
    Oscil - Particle Emitter Implementation
*/

#include "rendering/particles/ParticleEmitter.h"
#include <cmath>

namespace oscil
{

ParticleEmitter::ParticleEmitter(ParticleEmitterId id)
    : id_(id)
{
    // Seed RNG with ID for reproducible but unique behavior per emitter
    rng_.seed(static_cast<unsigned int>(id + 12345));
}

void ParticleEmitter::update(ParticlePool& pool,
                             const std::vector<float>& samples,
                             const juce::Rectangle<float>& bounds,
                             float deltaTime)
{
    if (!enabled_ || samples.empty() || bounds.isEmpty())
        return;

    // Accumulate emission time
    emissionAccumulator_ += config_.emissionRate * deltaTime;

    // Calculate how many particles to spawn this frame
    int particlesToSpawn = static_cast<int>(emissionAccumulator_);
    emissionAccumulator_ -= static_cast<float>(particlesToSpawn);

    if (particlesToSpawn <= 0)
        return;

    switch (config_.mode)
    {
        case ParticleEmissionMode::AlongWaveform:
        {
            // Emit uniformly along the waveform
            for (int i = 0; i < particlesToSpawn && !pool.isFull(); ++i)
            {
                size_t idx = static_cast<size_t>(dist01_(rng_) * static_cast<float>(samples.size() - 1));
                auto pos = getWaveformPosition(samples, bounds, idx);
                spawnParticle(pool, pos.x, pos.y);
            }
            break;
        }

        case ParticleEmissionMode::AtPeaks:
        {
            auto peaks = findPeaks(samples);
            if (!peaks.empty())
            {
                for (int i = 0; i < particlesToSpawn && !pool.isFull(); ++i)
                {
                    size_t peakIdx = peaks[static_cast<size_t>(dist01_(rng_) * static_cast<float>(peaks.size() - 1))];
                    auto pos = getWaveformPosition(samples, bounds, peakIdx);
                    spawnParticle(pool, pos.x, pos.y);
                }
            }
            break;
        }

        case ParticleEmissionMode::AtZeroCrossings:
        {
            auto crossings = findZeroCrossings(samples);
            if (!crossings.empty())
            {
                for (int i = 0; i < particlesToSpawn && !pool.isFull(); ++i)
                {
                    size_t crossIdx = crossings[static_cast<size_t>(dist01_(rng_) * static_cast<float>(crossings.size() - 1))];
                    auto pos = getWaveformPosition(samples, bounds, crossIdx);
                    spawnParticle(pool, pos.x, pos.y);
                }
            }
            break;
        }

        case ParticleEmissionMode::Continuous:
        {
            // Emit from center of bounds
            float cx = bounds.getCentreX();
            float cy = bounds.getCentreY();

            for (int i = 0; i < particlesToSpawn && !pool.isFull(); ++i)
            {
                spawnParticle(pool, cx, cy);
            }
            break;
        }

        case ParticleEmissionMode::Burst:
            // Burst mode only spawns on triggerBurst() call
            break;
    }
}

void ParticleEmitter::triggerBurst(ParticlePool& pool,
                                   const std::vector<float>& samples,
                                   const juce::Rectangle<float>& bounds)
{
    if (!enabled_ || samples.empty() || bounds.isEmpty())
        return;

    int count = config_.burstCount;

    for (int i = 0; i < count && !pool.isFull(); ++i)
    {
        size_t idx = static_cast<size_t>(dist01_(rng_) * static_cast<float>(samples.size() - 1));
        auto pos = getWaveformPosition(samples, bounds, idx);
        spawnParticle(pool, pos.x, pos.y);
    }
}

void ParticleEmitter::reset()
{
    emissionAccumulator_ = 0.0f;
}

void ParticleEmitter::spawnParticle(ParticlePool& pool, float x, float y)
{
    Particle* p = pool.allocate();
    if (!p)
        return;

    // Random lifetime
    float life = config_.particleLifeMin +
                 dist01_(rng_) * (config_.particleLifeMax - config_.particleLifeMin);

    // Random size
    float size = config_.particleSizeMin +
                 dist01_(rng_) * (config_.particleSizeMax - config_.particleSizeMin);

    // Random velocity
    float speed = config_.velocityMin +
                  dist01_(rng_) * (config_.velocityMax - config_.velocityMin);

    // Random angle within spread
    float angleSpreadRad = config_.velocityAngleSpread * 3.14159265f / 180.0f;
    float angle = (dist01_(rng_) - 0.5f) * angleSpreadRad;

    // Apply direction bias
    float baseAngle = std::atan2(config_.directionBiasY, config_.directionBiasX);
    angle += baseAngle;

    float vx = std::cos(angle) * speed;
    float vy = std::sin(angle) * speed;

    // Select color
    juce::Colour color = config_.useWaveformColor ? waveformColor_ : config_.colorStart;

    // Spawn the particle
    p->spawn(x, y, vx, vy, size, life, color);

    // Set rotation
    p->rotationSpeed = config_.rotationSpeedMin +
                       dist01_(rng_) * (config_.rotationSpeedMax - config_.rotationSpeedMin);
    if (dist01_(rng_) > 0.5f)
        p->rotationSpeed = -p->rotationSpeed;
}

juce::Point<float> ParticleEmitter::getWaveformPosition(
    const std::vector<float>& samples,
    const juce::Rectangle<float>& bounds,
    size_t index) const
{
    if (samples.empty())
        return bounds.getCentre();

    index = std::min(index, samples.size() - 1);

    float xNorm = static_cast<float>(index) / static_cast<float>(samples.size() - 1);
    float x = bounds.getX() + xNorm * bounds.getWidth();

    // Sample is -1 to 1, map to bounds (inverted Y since screen coords)
    float sample = samples[index];
    float yNorm = (1.0f - sample) * 0.5f;  // Map -1..1 to 1..0
    float y = bounds.getY() + yNorm * bounds.getHeight();

    return {x, y};
}

std::vector<size_t> ParticleEmitter::findPeaks(const std::vector<float>& samples) const
{
    std::vector<size_t> peaks;

    if (samples.size() < 3)
        return peaks;

    peaks.reserve(samples.size() / 10);  // Rough estimate

    for (size_t i = 1; i < samples.size() - 1; ++i)
    {
        float prev = samples[i - 1];
        float curr = samples[i];
        float next = samples[i + 1];

        // Local maximum (peak) or local minimum (trough)
        if ((curr > prev && curr > next) || (curr < prev && curr < next))
        {
            // Only consider significant peaks (amplitude > 0.1)
            if (std::abs(curr) > 0.1f)
            {
                peaks.push_back(i);
            }
        }
    }

    return peaks;
}

std::vector<size_t> ParticleEmitter::findZeroCrossings(const std::vector<float>& samples) const
{
    std::vector<size_t> crossings;

    if (samples.size() < 2)
        return crossings;

    crossings.reserve(samples.size() / 20);  // Rough estimate

    for (size_t i = 1; i < samples.size(); ++i)
    {
        float prev = samples[i - 1];
        float curr = samples[i];

        // Sign change indicates zero crossing
        if ((prev < 0.0f && curr >= 0.0f) || (prev >= 0.0f && curr < 0.0f))
        {
            crossings.push_back(i);
        }
    }

    return crossings;
}

} // namespace oscil
