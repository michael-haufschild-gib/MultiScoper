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
    // Note: triggerBurst also calculates tangents now.
    // But PhysicsInit test used triggerBurst.
    // With the new logic, triggerBurst uses tangent/normal logic if samples are provided.
    // For a flat line (samples=0), tangent is horizontal (0 deg), normal is +/- 90 deg (vertical).
    // So particles should move vertically.
    // The original test config had directionBiasY=1 (down), which might be overridden by the logic.
    // Let's check if the old test (PhysicsInit) still makes sense or if I broke it.
    // The old test expected vy > 0 based on directionBiasY = 1.
    // My change to `triggerBurst` IGNORES `directionBias` if `baseAngle` is passed!
    // In `update` and `triggerBurst`, I now pass a calculated angle.
    // So the config.directionBias is IGNORED for AlongWaveform/Burst!
    // This means the old test assertion "expecting down because I set bias down" might fail 
    // if the normal logic decides to go UP (-90 deg).
    // However, normal logic is random 50/50 up/down.
    // So the old test is now flaky or wrong.
    // I should update this test to reflect the NEW behavior.
    
    // Actually, let's rewrite PhysicsInit to test the NORMAL emission behavior.
    
    emitter.triggerBurst(pool, samples, bounds);
    
    ASSERT_GT(pool.getAliveCount(), 0);
    
    pool.forEachAlive([&](const Particle& p, int) {
        // For flat line, particles should be vertical (vx ~ 0)
        EXPECT_NEAR(p.vx, 0.0f, 1.0f); // Allow some slop due to float math/spread
        EXPECT_GT(std::abs(p.vy), 0.0f); // Should be moving vertically
    });
}

TEST(ParticleEmitterTest, DirectionalEmissionAlongFlatWaveform) {
    ParticlePool pool(100);
    ParticleEmitter emitter(1);
    
    ParticleEmitterConfig config;
    config.emissionRate = 100.0f;
    config.mode = ParticleEmissionMode::AlongWaveform;
    config.velocityAngleSpread = 0.0f; 
    config.velocityMin = 100.0f;
    config.velocityMax = 100.0f;
    emitter.setConfig(config);
    
    std::vector<float> samples(100, 0.0f); // Flat
    juce::Rectangle<float> bounds(0, 0, 100, 100);
    
    // Spawn some
    emitter.update(pool, samples, bounds, 0.5f); // ~50 particles
    
    ASSERT_GT(pool.getAliveCount(), 0);
    
    pool.forEachAlive([&](const Particle& p, int) {
        // Expect vertical movement (Normal to horizontal line)
        // Tangent is 0. Normal is +90 or -90.
        // vx = cos(90) * 100 = 0
        // vy = sin(90) * 100 = 100 OR -100
        EXPECT_NEAR(p.vx, 0.0f, 0.1f);
        EXPECT_NEAR(std::abs(p.vy), 100.0f, 0.1f);
    });
}

TEST(ParticleEmitterTest, DirectionalEmissionAlongSlope) {
    ParticlePool pool(100);
    ParticleEmitter emitter(1);
    
    ParticleEmitterConfig config;
    config.emissionRate = 100.0f;
    config.mode = ParticleEmissionMode::AlongWaveform;
    config.velocityAngleSpread = 0.0f; 
    config.velocityMin = 100.0f;
    config.velocityMax = 100.0f;
    emitter.setConfig(config);
    
    // Diagonal line from top-left to bottom-right (in JUCE coords) or similar
    // Let's make a ramp: sample -1 to 1
    // bounds 100x100.
    // y = centerY - sample * amp
    // sample -1 -> y = 50 - (-1)*45 = 95
    // sample 1 -> y = 50 - (1)*45 = 5
    // So Y goes 95 -> 5 (upwards visual, but decreasing Y value).
    // X goes 0 -> 100.
    // Slope dy/dx = (5 - 95) / 100 = -0.9.
    // Angle should be atan2(-0.9, 1) ~= -42 deg.
    // Normal should be -42 + 90 = 48 deg OR -42 - 90 = -132 deg.
    
    std::vector<float> samples(101);
    for(int i=0; i<=100; ++i) {
        samples[i] = -1.0f + 2.0f * (float)i/100.0f;
    }
    
    juce::Rectangle<float> bounds(0, 0, 100, 100);
    
    emitter.update(pool, samples, bounds, 0.5f);
    
    ASSERT_GT(pool.getAliveCount(), 0);
    
    pool.forEachAlive([&](const Particle& p, int) {
        // Check orthogonality roughly
        // Tangent vector roughly (1, -0.9)
        // Particle velocity vector (vx, vy)
        // Dot product should be ~0
        float dot = p.vx * 1.0f + p.vy * (-0.9f);
        // Normalize vectors for better dot check?
        // Tangent norm: sqrt(1 + 0.81) = 1.34
        // Vel norm: 100
        // dot / (1.34 * 100) should be close to 0
        EXPECT_NEAR(dot / 134.0f, 0.0f, 0.1f);
    });
}
