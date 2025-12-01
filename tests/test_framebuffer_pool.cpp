#include <gtest/gtest.h>
#include "rendering/FramebufferPool.h"
#include "rendering/Framebuffer.h"

using namespace oscil;

class MockFramebuffer : public Framebuffer {
public:
    bool createCalled = false;
    bool destroyCalled = false;
    bool resizeCalled = false;
    int createW = 0, createH = 0;

    bool create(juce::OpenGLContext&, int w, int h, int, GLenum, bool, bool) override {
        createCalled = true;
        createW = w;
        createH = h;
        fbo = 123; // Fake ID
        return true;
    }
    void destroy(juce::OpenGLContext&) override { destroyCalled = true; }
    void resize(juce::OpenGLContext&, int w, int h) override { 
        resizeCalled = true;
        createW = w; 
        createH = h; 
    }
    bool isValid() const override { return fbo != 0; }
};

class TestableFramebufferPool : public FramebufferPool {
public:
    std::vector<MockFramebuffer*> createdFBOs;

    std::unique_ptr<Framebuffer> createFramebuffer() override {
        auto ptr = std::make_unique<MockFramebuffer>();
        createdFBOs.push_back(ptr.get());
        return ptr;
    }
    
    bool createFullscreenQuad(juce::OpenGLContext&) override { return true; }
    void destroyFullscreenQuad(juce::OpenGLContext&) override {}
};

TEST(FramebufferPoolTest, InitializeCreatesFBOs) {
    TestableFramebufferPool pool;
    juce::OpenGLContext context; // Dummy

    bool result = pool.initialize(context, 800, 600);
    EXPECT_TRUE(result);
    
    // Waveform, Ping, Pong
    ASSERT_EQ(pool.createdFBOs.size(), 3);
    EXPECT_TRUE(pool.createdFBOs[0]->createCalled); // Waveform
    EXPECT_EQ(pool.createdFBOs[0]->createW, 800);
    
    EXPECT_NE(pool.getWaveformFBO(), nullptr);
    EXPECT_NE(pool.getPingFBO(), nullptr);
    EXPECT_NE(pool.getPongFBO(), nullptr);
}

TEST(FramebufferPoolTest, ResizeUpdatesFBOs) {
    TestableFramebufferPool pool;
    juce::OpenGLContext context;
    pool.initialize(context, 800, 600);
    
    pool.resize(context, 1024, 768);
    
    EXPECT_EQ(pool.getWidth(), 1024);
    ASSERT_GE(pool.createdFBOs.size(), 3);
    EXPECT_TRUE(pool.createdFBOs[0]->resizeCalled);
    EXPECT_EQ(pool.createdFBOs[0]->createW, 1024);
}

TEST(FramebufferPoolTest, ShutdownDestroysFBOs) {
    TestableFramebufferPool pool;
    juce::OpenGLContext context;
    pool.initialize(context, 800, 600);
    
    pool.shutdown(context);
    
    ASSERT_GE(pool.createdFBOs.size(), 3);
    EXPECT_TRUE(pool.createdFBOs[0]->destroyCalled);
    EXPECT_FALSE(pool.isInitialized());
}
