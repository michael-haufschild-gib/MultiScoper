#include <gtest/gtest.h>
#include "rendering/particles/ParticlePool.h"
#include "rendering/particles/ParticleEmitter.h"
#include "rendering/VisualConfiguration.h"
#include <vector>

using namespace oscil;

// --- ParticlePool Tests ---

TEST(ParticlePoolTest, Initialization) {
    ParticlePool pool(100);
    EXPECT_EQ(pool.getCapacity(), 100);
    EXPECT_EQ(pool.getAliveCount(), 0);
    EXPECT_FALSE(pool.isFull());
}

TEST(ParticlePoolTest, AllocationDeallocation) {
    ParticlePool pool(10);
    
    // Allocate one
    Particle* p1 = pool.allocate();
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(pool.getAliveCount(), 1);
    EXPECT_TRUE(p1->isAlive());
    
    // Allocate until full
    for(int i=0; i<9; ++i) {
        EXPECT_NE(pool.allocate(), nullptr);
    }
    EXPECT_EQ(pool.getAliveCount(), 10);
    EXPECT_TRUE(pool.isFull());
    
    // Allocate when full
    EXPECT_EQ(pool.allocate(), nullptr);
    
    // Free one
    pool.free(p1);
    EXPECT_EQ(pool.getAliveCount(), 9);
    EXPECT_FALSE(pool.isFull());
    
    // Reallocate
    Particle* pNew = pool.allocate();
    EXPECT_NE(pNew, nullptr);
    EXPECT_EQ(pool.getAliveCount(), 10);
}

TEST(ParticlePoolTest, UpdateAndDeath) {
    ParticlePool pool(10);
    Particle* p = pool.allocate();
    
    p->life = 1.0f;
    p->maxLife = 1.0f;
    // getAge() is 1.0 - (life/maxLife). Initially 0.
    
    // Update with small dt
    pool.updateAll(0.1f);
    EXPECT_NEAR(p->getAge(), 0.1f, 0.001f);
    EXPECT_TRUE(p->isAlive());
    EXPECT_EQ(pool.getAliveCount(), 1);
    
    // Update to exceed life (use > 1.0 to ensure death despite float precision)
    pool.updateAll(1.1f); 
    
    EXPECT_EQ(pool.getAliveCount(), 0);
}

// --- ParticleEmitter Tests ---

TEST(ParticleEmitterTest, Configuration) {
    ParticleEmitter emitter(1);
    ParticleEmitterConfig config;
    config.emissionRate = 500.0f;
    config.mode = ParticleEmissionMode::AtPeaks;
    
    emitter.setConfig(config);
    
    EXPECT_EQ(emitter.getId(), 1);
    EXPECT_EQ(emitter.getConfig().emissionRate, 500.0f);
    EXPECT_EQ(emitter.getConfig().mode, ParticleEmissionMode::AtPeaks);
}

TEST(ParticleEmitterTest, EmissionRate) {
    ParticlePool pool(1000);
    ParticleEmitter emitter(1);
    
    ParticleEmitterConfig config;
    config.emissionRate = 10.0f; // 10 particles per second
    config.mode = ParticleEmissionMode::AlongWaveform;
    emitter.setConfig(config);
    
    std::vector<float> samples(100, 0.0f); // Flat line
    juce::Rectangle<float> bounds(0, 0, 100, 100);
    
    // Update with 0.1s -> 1 particle expected
    emitter.update(pool, samples, bounds, 0.1f);
    EXPECT_EQ(pool.getAliveCount(), 1);
    
    // Update with 0.1s -> 1 more
    emitter.update(pool, samples, bounds, 0.1f);
    EXPECT_EQ(pool.getAliveCount(), 2);
}

TEST(ParticleEmitterTest, BurstEmission) {
    ParticlePool pool(1000);
    ParticleEmitter emitter(1);
    
    ParticleEmitterConfig config;
    config.burstCount = 50;
    config.mode = ParticleEmissionMode::Burst;
    emitter.setConfig(config);
    
    std::vector<float> samples(100, 0.0f);
    juce::Rectangle<float> bounds(0, 0, 100, 100);
    
    emitter.triggerBurst(pool, samples, bounds);
    
    EXPECT_EQ(pool.getAliveCount(), 50);
}

TEST(ParticleEmitterTest, PhysicsInit) {
    ParticlePool pool(100);
    ParticleEmitter emitter(1);
    
    ParticleEmitterConfig config;
    config.gravity = 100.0f;
    config.velocityMin = 10.0f;
    config.velocityMax = 10.0f;
    config.directionBiasX = 0.0f;
    config.directionBiasY = 1.0f; // Down (Y+)
    config.velocityAngleSpread = 0.0f; // Exact direction
    emitter.setConfig(config);
    
    std::vector<float> samples(100, 0.0f);
    juce::Rectangle<float> bounds(0, 0, 100, 100);
    
    // Force emission
    emitter.triggerBurst(pool, samples, bounds);
    
    ASSERT_GT(pool.getAliveCount(), 0);
    
    pool.forEachAlive([&](const Particle& p, int) {
        // Check velocity
        // With bias Y=1 (down) and spread 0, velocity should be positive Y
        // Adding context to failure
        EXPECT_GT(p.vy, 0.0f) << "Particle vy should be positive (downwards). Actual: " << p.vy << ", vx: " << p.vx;
    });
}
