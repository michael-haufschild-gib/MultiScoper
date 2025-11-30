/*
    Oscil - Particle Pool
    Memory pool for efficient particle allocation
*/

#pragma once

#include "Particle.h"
#include <vector>
#include <functional>

namespace oscil
{

/**
 * Memory pool for particles.
 * Provides O(1) allocation and deallocation.
 */
class ParticlePool
{
public:
    /**
     * Create a pool with maximum capacity.
     * @param maxParticles Maximum number of particles
     */
    explicit ParticlePool(int maxParticles);

    /**
     * Allocate a particle from the pool.
     * @return Pointer to particle, or nullptr if pool is full
     */
    Particle* allocate();

    /**
     * Free a particle back to the pool.
     * @param particle Pointer to particle to free
     */
    void free(Particle* particle);

    /**
     * Free all particles.
     */
    void freeAll();

    /**
     * Update all alive particles.
     * @param deltaTime Time step in seconds
     * @param gravity Gravity in pixels/sec^2
     * @param drag Velocity damping
     */
    void updateAll(float deltaTime, float gravity = 0.0f, float drag = 0.0f);

    /**
     * Iterate over all alive particles.
     * @param func Function to call for each alive particle
     */
    template<typename Func>
    void forEachAlive(Func&& func)
    {
        for (size_t i = 0; i < particles_.size(); ++i)
        {
            if (particles_[i].isAlive())
            {
                func(particles_[i], static_cast<int>(i));
            }
        }
    }

    /**
     * Iterate over all alive particles (const version).
     */
    template<typename Func>
    void forEachAlive(Func&& func) const
    {
        for (size_t i = 0; i < particles_.size(); ++i)
        {
            if (particles_[i].isAlive())
            {
                func(particles_[i], static_cast<int>(i));
            }
        }
    }

    /**
     * Get the number of alive particles.
     */
    [[nodiscard]] int getAliveCount() const { return aliveCount_; }

    /**
     * Get the maximum capacity.
     */
    [[nodiscard]] int getCapacity() const { return static_cast<int>(particles_.size()); }

    /**
     * Check if pool is full.
     */
    [[nodiscard]] bool isFull() const { return freeIndices_.empty(); }

    /**
     * Direct access to particle data for GPU upload.
     */
    [[nodiscard]] const std::vector<Particle>& getParticles() const { return particles_; }

private:
    std::vector<Particle> particles_;
    std::vector<int> freeIndices_;
    int aliveCount_ = 0;
};

} // namespace oscil
