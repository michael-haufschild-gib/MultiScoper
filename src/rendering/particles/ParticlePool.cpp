/*
    Oscil - Particle Pool Implementation
    Memory pool for efficient particle allocation
*/

#include "rendering/particles/ParticlePool.h"
#include <algorithm>

namespace oscil
{

ParticlePool::ParticlePool(int maxParticles)
{
    particles_.resize(static_cast<size_t>(maxParticles));
    freeIndices_.reserve(static_cast<size_t>(maxParticles));

    // Initialize free list with all indices (in reverse for efficient pop_back)
    for (int i = maxParticles - 1; i >= 0; --i)
    {
        particles_[static_cast<size_t>(i)].life = 0.0f; // Ensure dead initially
        freeIndices_.push_back(i);
    }

    aliveCount_ = 0;
}

Particle* ParticlePool::allocate()
{
    if (freeIndices_.empty())
        return nullptr;

    int index = freeIndices_.back();
    freeIndices_.pop_back();

    Particle& p = particles_[static_cast<size_t>(index)];
    p.life = 1.0f; // Mark as alive
    ++aliveCount_;

    return &p;
}

void ParticlePool::free(Particle* particle)
{
    if (!particle)
        return;

    // Calculate index from pointer
    ptrdiff_t index = particle - particles_.data();
    if (index < 0 || index >= static_cast<ptrdiff_t>(particles_.size()))
        return;

    // Only free if actually alive
    if (particle->life > 0.0f)
    {
        particle->life = 0.0f;
        freeIndices_.push_back(static_cast<int>(index));
        --aliveCount_;
    }
}

void ParticlePool::freeAll()
{
    freeIndices_.clear();
    freeIndices_.reserve(particles_.size());

    for (int i = static_cast<int>(particles_.size()) - 1; i >= 0; --i)
    {
        particles_[static_cast<size_t>(i)].life = 0.0f;
        freeIndices_.push_back(i);
    }

    aliveCount_ = 0;
}

void ParticlePool::updateAll(float deltaTime, float gravity, float drag)
{
    int newAliveCount = 0;

    for (size_t i = 0; i < particles_.size(); ++i)
    {
        Particle& p = particles_[i];

        if (p.isAlive())
        {
            p.update(deltaTime, gravity, drag);

            if (p.isAlive())
            {
                ++newAliveCount;
            }
            else
            {
                // Particle just died, return to free list
                freeIndices_.push_back(static_cast<int>(i));
            }
        }
    }

    aliveCount_ = newAliveCount;
}

} // namespace oscil
