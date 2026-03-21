/*
    Oscil - Particle Emitter Implementation
*/

#include "rendering/particles/ParticleEmitter.h"
#include <cmath>
#include <algorithm>
#include <numbers>

namespace oscil
{

namespace
{
    // Helper to safely calculate a random index within bounds
    // This prevents out-of-bounds access from floating-point edge cases
    inline size_t safeRandomIndex(float random01, size_t size)
    {
        if (size == 0) return 0;
        if (size == 1) return 0;
        // Clamp the result to valid range to handle floating-point edge cases
        size_t maxIdx = size - 1;
        size_t idx = static_cast<size_t>(random01 * static_cast<float>(maxIdx));
        return std::min(idx, maxIdx);
    }
} // anonymous namespace

ParticleEmitter::ParticleEmitter(ParticleEmitterId id)
    : id_(id)
{
    // Seed RNG with ID for reproducible but unique behavior per emitter
    rng_.seed(static_cast<unsigned int>(id + 12345));
}

float ParticleEmitter::computeNormalAngle(const std::vector<float>& samples,
                                          const juce::Rectangle<float>& bounds,
                                          size_t idx, float verticalScale)
{
    size_t nextIdx = (idx + 1 < samples.size()) ? idx + 1 : idx;
    size_t prevIdx = (idx > 0) ? idx - 1 : idx;
    if (nextIdx == prevIdx)
    {
        if (idx < samples.size() - 1) nextIdx++;
        else if (idx > 0) prevIdx--;
    }

    auto pNext = getWaveformPosition(samples, bounds, nextIdx, verticalScale);
    auto pPrev = getWaveformPosition(samples, bounds, prevIdx, verticalScale);
    float tangentAngle = std::atan2(pNext.y - pPrev.y, pNext.x - pPrev.x);
    float side = (dist01_(rng_) > 0.5f) ? 1.57079633f : -1.57079633f;
    return tangentAngle + side;
}

void ParticleEmitter::emitAtIndices(ParticlePool& pool, const std::vector<size_t>& indices,
                                     const std::vector<float>& samples,
                                     const juce::Rectangle<float>& bounds,
                                     int count, float verticalScale)
{
    if (indices.empty()) return;
    for (int i = 0; i < count && !pool.isFull(); ++i)
    {
        size_t idx = indices[safeRandomIndex(dist01_(rng_), indices.size())];
        auto pos = getWaveformPosition(samples, bounds, idx, verticalScale);
        float angle = computeNormalAngle(samples, bounds, idx, verticalScale);
        spawnParticle(pool, pos.x, pos.y, angle);
    }
}

void ParticleEmitter::update(ParticlePool& pool,
                             const std::vector<float>& samples,
                             const juce::Rectangle<float>& bounds,
                             float deltaTime,
                             float verticalScale)
{
    if (!enabled_ || samples.empty() || bounds.isEmpty())
        return;

    emissionAccumulator_ += config_.emissionRate * deltaTime;
    int particlesToSpawn = static_cast<int>(emissionAccumulator_);
    emissionAccumulator_ -= static_cast<float>(particlesToSpawn);
    if (particlesToSpawn <= 0) return;

    switch (config_.mode)
    {
        case ParticleEmissionMode::AlongWaveform:
        {
            // Build index list of random positions along the waveform
            std::vector<size_t> indices(static_cast<size_t>(particlesToSpawn));
            for (auto& idx : indices)
                idx = safeRandomIndex(dist01_(rng_), samples.size());
            emitAtIndices(pool, indices, samples, bounds, particlesToSpawn, verticalScale);
            break;
        }
        case ParticleEmissionMode::AtPeaks:
            emitAtIndices(pool, findPeaks(samples), samples, bounds, particlesToSpawn, verticalScale);
            break;
        case ParticleEmissionMode::AtZeroCrossings:
            emitAtIndices(pool, findZeroCrossings(samples), samples, bounds, particlesToSpawn, verticalScale);
            break;
        case ParticleEmissionMode::Continuous:
            for (int i = 0; i < particlesToSpawn && !pool.isFull(); ++i)
                spawnParticle(pool, bounds.getCentreX(), bounds.getCentreY());
            break;
        case ParticleEmissionMode::Burst:
            break;
    }
}

void ParticleEmitter::triggerBurst(ParticlePool& pool,
                                   const std::vector<float>& samples,
                                   const juce::Rectangle<float>& bounds,
                                   float verticalScale)
{
    if (!enabled_ || samples.empty() || bounds.isEmpty())
        return;

    int count = config_.burstCount;

    for (int i = 0; i < count && !pool.isFull(); ++i)
    {
        size_t idx = safeRandomIndex(dist01_(rng_), samples.size());
        auto pos = getWaveformPosition(samples, bounds, idx, verticalScale);
        
        // Calculate tangent for burst too
        size_t nextIdx = (idx + 1 < samples.size()) ? idx + 1 : idx;
        size_t prevIdx = (idx > 0) ? idx - 1 : idx;
        
        auto pNext = getWaveformPosition(samples, bounds, nextIdx, verticalScale);
        auto pPrev = getWaveformPosition(samples, bounds, prevIdx, verticalScale);
        
        float tangentAngle = std::atan2(pNext.y - pPrev.y, pNext.x - pPrev.x);
        float side = (dist01_(rng_) > 0.5f) ? 1.57079633f : -1.57079633f;
        
        spawnParticle(pool, pos.x, pos.y, tangentAngle + side);
    }
}

void ParticleEmitter::reset()
{
    emissionAccumulator_ = 0.0f;
}

void ParticleEmitter::spawnParticle(ParticlePool& pool, float x, float y, std::optional<float> baseAngleOpt)
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
    static constexpr float DEG_TO_RAD = std::numbers::pi_v<float> / 180.0f;
    float angleSpreadRad = config_.velocityAngleSpread * DEG_TO_RAD;
    float angle = (dist01_(rng_) - 0.5f) * angleSpreadRad;

    // Apply direction bias
    float baseAngle;
    if (baseAngleOpt.has_value())
    {
        baseAngle = baseAngleOpt.value();
    }
    else
    {
        baseAngle = std::atan2(config_.directionBiasY, config_.directionBiasX);
    }
    
    angle += baseAngle;

    float vx = std::cos(angle) * speed;
    float vy = std::sin(angle) * speed;

    // Select color
    juce::Colour color = config_.useWaveformColor ? waveformColor_ : config_.colorStart;

    // Spawn the particle
    p->spawn(x, y, vx, vy, size, life, color, id_);

    // Randomize initial rotation (0 to 2pi) to prevent axis-aligned "square" look
    p->rotation = dist01_(rng_) * 6.2831853f;

    // Set rotation
    p->rotationSpeed = config_.rotationSpeedMin +
                       dist01_(rng_) * (config_.rotationSpeedMax - config_.rotationSpeedMin);
    if (dist01_(rng_) > 0.5f)
        p->rotationSpeed = -p->rotationSpeed;
}

juce::Point<float> ParticleEmitter::getWaveformPosition(
    const std::vector<float>& samples,
    const juce::Rectangle<float>& bounds,
    size_t index,
    float verticalScale) const
{
    if (samples.size() <= 1)
        return bounds.getCentre();

    index = std::min(index, samples.size() - 1);

    float xNorm = static_cast<float>(index) / static_cast<float>(samples.size() - 1);
    float x = bounds.getX() + xNorm * bounds.getWidth();

    // Sample is -1 to 1. 
    // Visual waveform logic: centerY - sample * amplitude
    // Use 0.45f factor to match WaveformComponent's software rendering logic
    float sample = samples[index];
    float centerY = bounds.getCentreY();
    float amplitude = bounds.getHeight() * 0.45f * verticalScale;
    
    float y = centerY - sample * amplitude;

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
