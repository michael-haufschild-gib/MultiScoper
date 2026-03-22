#include <gtest/gtest.h>
#include "rendering/EffectChain.h"
#include "rendering/IEffectProvider.h"
#include "rendering/FramebufferPool.h"
#include "rendering/VisualConfiguration.h"
#include "rendering/effects/PostProcessEffect.h"
#include <map>

using namespace oscil;

// Mock Effect
class MockEffect : public PostProcessEffect {
public:
    MockEffect() = default;
    bool applyCalled = false;
    Framebuffer* lastSrc = nullptr;
    Framebuffer* lastDest = nullptr;

    // Pure virtuals implementation
    juce::String getId() const override { return "mock"; }
    juce::String getDisplayName() const override { return "Mock Effect"; }
    void release(juce::OpenGLContext&) override {}
    bool isCompiled() const override { return true; }
    bool compile(juce::OpenGLContext&) override { return true; }

    void apply(juce::OpenGLContext&, Framebuffer* src, Framebuffer* dest, FramebufferPool&, float) override {
        applyCalled = true;
        lastSrc = src;
        lastDest = dest;
    }
    
    // Using base setEnabled/isEnabled
};

// Mock Effect Provider
class MockEffectProvider : public IEffectProvider {
public:
    std::map<juce::String, MockEffect*> effects;
    
    void addEffect(const juce::String& id, MockEffect* effect) {
        effects[id] = effect;
    }

    PostProcessEffect* getEffect(const juce::String& id) override {
        auto it = effects.find(id);
        if (it != effects.end()) return it->second;
        return nullptr;
    }
};

// Mock Framebuffer Pool
class MockFramebufferPool : public FramebufferPool {
public:
    Framebuffer ping, pong;
    MockFramebufferPool() {
        ping.fbo = 1; // Mark as valid
        pong.fbo = 2;
    }
    Framebuffer* getPingFBO() override { return &ping; }
    Framebuffer* getPongFBO() override { return &pong; }
};

// Testable Effect Chain (intercepts GL calls)
class TestableEffectChain : public EffectChain {
public:
    std::vector<Framebuffer*> boundFBOs;
    
    void bindFramebuffer(Framebuffer* fb) override {
        boundFBOs.push_back(fb);
    }
    void unbindFramebuffer(Framebuffer*) override {
        // No-op
    }
};

class EffectChainTest : public ::testing::Test {
protected:
    juce::OpenGLContext context; // Dummy context
    MockEffectProvider provider;
    MockFramebufferPool pool;
    TestableEffectChain chain;
    VisualConfiguration config;
    Framebuffer sourceFBO;

    MockEffect effect1;
    MockEffect effect2;
    MockEffect effect3;

    void SetUp() override {
        sourceFBO.fbo = 10; // Source is valid
        
        provider.addEffect("effect1", &effect1);
        provider.addEffect("effect2", &effect2);
        provider.addEffect("effect3", &effect3);

        // Configure chain steps
        chain.addStep({
            "effect1",
            [](const VisualConfiguration& c) { return c.bloom.enabled; }, // Use bloom flag for effect1
            nullptr
        });
        chain.addStep({
            "effect2",
            [](const VisualConfiguration& c) { return c.trails.enabled; }, // Use trails flag for effect2
            nullptr
        });
        chain.addStep({
            "effect3",
            [](const VisualConfiguration& c) { return c.vignette.enabled; }, // Use vignette flag for effect3
            nullptr
        });
    }
};

TEST_F(EffectChainTest, SingleEffectEnabled) {
    config.bloom.enabled = true;   // Enable effect1
    config.trails.enabled = false;
    config.vignette.enabled = false;

    chain.process(context, &sourceFBO, pool, 0.01f, config, provider);

    EXPECT_TRUE(effect1.applyCalled);
    EXPECT_FALSE(effect2.applyCalled);
    EXPECT_FALSE(effect3.applyCalled);

    // Verify buffers
    // Source -> Ping
    EXPECT_EQ(effect1.lastSrc, &sourceFBO);
    EXPECT_EQ(effect1.lastDest, pool.getPingFBO());
}

TEST_F(EffectChainTest, MultipleEffectsPingPong) {
    config.bloom.enabled = true;    // Enable effect1
    config.trails.enabled = true;   // Enable effect2
    config.vignette.enabled = false;

    chain.process(context, &sourceFBO, pool, 0.01f, config, provider);

    EXPECT_TRUE(effect1.applyCalled);
    EXPECT_TRUE(effect2.applyCalled);

    // 1. Source -> Ping
    EXPECT_EQ(effect1.lastSrc, &sourceFBO);
    EXPECT_EQ(effect1.lastDest, pool.getPingFBO());

    // 2. Ping -> Pong
    EXPECT_EQ(effect2.lastSrc, pool.getPingFBO());
    EXPECT_EQ(effect2.lastDest, pool.getPongFBO());
}

TEST_F(EffectChainTest, AllEffectsEnabled) {
    config.bloom.enabled = true;
    config.trails.enabled = true;
    config.vignette.enabled = true;

    chain.process(context, &sourceFBO, pool, 0.01f, config, provider);

    // 1. Source -> Ping
    EXPECT_EQ(effect1.lastSrc, &sourceFBO);
    EXPECT_EQ(effect1.lastDest, pool.getPingFBO());

    // 2. Ping -> Pong
    EXPECT_EQ(effect2.lastSrc, pool.getPingFBO());
    EXPECT_EQ(effect2.lastDest, pool.getPongFBO());

    // 3. Pong -> Ping
    EXPECT_EQ(effect3.lastSrc, pool.getPongFBO());
    EXPECT_EQ(effect3.lastDest, pool.getPingFBO());
}

TEST_F(EffectChainTest, EffectDisabledInternallySkipped) {
    config.bloom.enabled = true;
    effect1.setEnabled(false); // Internally disabled

    chain.process(context, &sourceFBO, pool, 0.01f, config, provider);

    EXPECT_FALSE(effect1.applyCalled);
}

TEST_F(EffectChainTest, MissingEffectSkipped) {
    // Add step for non-existent effect
    chain.addStep({
        "ghost_effect",
        [](const VisualConfiguration&) { return true; },
        nullptr
    });

    chain.process(context, &sourceFBO, pool, 0.01f, config, provider);

    // Config should still be in its original state after processing
    EXPECT_FALSE(config.bloom.enabled);
    EXPECT_FALSE(config.scanlines.enabled);
}

TEST_F(EffectChainTest, BindsDestinationFramebuffer) {
    config.bloom.enabled = true;
    
    chain.process(context, &sourceFBO, pool, 0.01f, config, provider);
    
    ASSERT_EQ(chain.boundFBOs.size(), 1);
    EXPECT_EQ(chain.boundFBOs[0], pool.getPingFBO());
}
