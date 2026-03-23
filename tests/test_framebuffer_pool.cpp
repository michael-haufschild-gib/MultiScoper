/*
    Oscil - Framebuffer Pool Tests
    Verifies FBO lifecycle, resize behavior, shutdown cleanup, and edge cases.
    Uses mock framebuffers since OpenGL context is not available in headless tests.
*/

#include "rendering/Framebuffer.h"
#include "rendering/FramebufferPool.h"

#include <gtest/gtest.h>

using namespace oscil;

class MockFramebuffer : public Framebuffer
{
public:
    bool createCalled = false;
    bool destroyCalled = false;
    bool resizeCalled = false;
    int createW = 0, createH = 0;
    int resizeW = 0, resizeH = 0;

    bool create(juce::OpenGLContext&, int w, int h, int, GLenum, bool, bool) override
    {
        createCalled = true;
        createW = w;
        createH = h;
        fbo = 123;
        return true;
    }
    void destroy(juce::OpenGLContext&) override
    {
        destroyCalled = true;
        fbo = 0;
    }
    void resize(juce::OpenGLContext&, int w, int h) override
    {
        resizeCalled = true;
        resizeW = w;
        resizeH = h;
    }
    bool isValid() const override { return fbo != 0; }
};

class TestableFramebufferPool : public FramebufferPool
{
public:
    std::vector<MockFramebuffer*> createdFBOs;

    std::unique_ptr<Framebuffer> createFramebuffer() override
    {
        auto ptr = std::make_unique<MockFramebuffer>();
        createdFBOs.push_back(ptr.get());
        return ptr;
    }

    bool createFullscreenQuad(juce::OpenGLContext&) override { return true; }
    void destroyFullscreenQuad(juce::OpenGLContext&) override {}
};

// =============================================================================
// Initialization
// =============================================================================

// Bug caught: initialize doesn't create all three FBOs, leaving one null.
// First use of getPingFBO() during effect rendering causes null dereference.
TEST(FramebufferPoolTest, InitializeCreatesExactlyThreeFBOs)
{
    TestableFramebufferPool pool;
    juce::OpenGLContext context;

    bool result = pool.initialize(context, 800, 600);
    EXPECT_TRUE(result);

    ASSERT_EQ(pool.createdFBOs.size(), 3u);
    EXPECT_NE(pool.getWaveformFBO(), nullptr);
    EXPECT_NE(pool.getPingFBO(), nullptr);
    EXPECT_NE(pool.getPongFBO(), nullptr);
}

// Bug caught: FBOs created with wrong dimensions, causing rendering at
// wrong resolution and producing black or stretched output.
TEST(FramebufferPoolTest, InitializePassesCorrectDimensionsToFBOs)
{
    TestableFramebufferPool pool;
    juce::OpenGLContext context;

    pool.initialize(context, 1920, 1080);

    for (auto* fbo : pool.createdFBOs)
    {
        EXPECT_TRUE(fbo->createCalled);
        EXPECT_EQ(fbo->createW, 1920);
        EXPECT_EQ(fbo->createH, 1080);
    }

    EXPECT_EQ(pool.getWidth(), 1920);
    EXPECT_EQ(pool.getHeight(), 1080);
    EXPECT_TRUE(pool.isInitialized());
}

// =============================================================================
// Resize
// =============================================================================

// Bug caught: resize doesn't update stored dimensions, causing subsequent
// resize calls to skip because old == new comparison uses stale values.
TEST(FramebufferPoolTest, ResizeUpdatesDimensionsAndFBOs)
{
    TestableFramebufferPool pool;
    juce::OpenGLContext context;
    pool.initialize(context, 800, 600);

    pool.resize(context, 1024, 768);

    EXPECT_EQ(pool.getWidth(), 1024);
    EXPECT_EQ(pool.getHeight(), 768);

    for (auto* fbo : pool.createdFBOs)
    {
        EXPECT_TRUE(fbo->resizeCalled);
        EXPECT_EQ(fbo->resizeW, 1024);
        EXPECT_EQ(fbo->resizeH, 768);
    }
}

// Bug caught: resize to same dimensions still triggers FBO recreation,
// causing unnecessary GPU memory churn and frame drops.
TEST(FramebufferPoolTest, ResizeToSameDimensionsIsNoOp)
{
    TestableFramebufferPool pool;
    juce::OpenGLContext context;
    pool.initialize(context, 800, 600);

    // Clear the resize flags set during init
    for (auto* fbo : pool.createdFBOs)
        fbo->resizeCalled = false;

    pool.resize(context, 800, 600);

    for (auto* fbo : pool.createdFBOs)
    {
        EXPECT_FALSE(fbo->resizeCalled) << "Resize to same dimensions should not trigger FBO resize";
    }
}

// =============================================================================
// Shutdown
// =============================================================================

// Bug caught: shutdown doesn't destroy FBOs, causing GPU memory leak.
TEST(FramebufferPoolTest, ShutdownDestroysFBOs)
{
    TestableFramebufferPool pool;
    juce::OpenGLContext context;
    pool.initialize(context, 800, 600);

    pool.shutdown(context);

    for (auto* fbo : pool.createdFBOs)
        EXPECT_TRUE(fbo->destroyCalled);

    EXPECT_FALSE(pool.isInitialized());
}

// Bug caught: rendering pipeline calls FBO methods after shutdown without
// checking isInitialized(), causing GL errors. The pool marks itself as
// not-initialized but FBO objects persist in destroyed state — callers
// must check isInitialized() before accessing FBOs.
TEST(FramebufferPoolTest, IsInitializedFalseAfterShutdown)
{
    TestableFramebufferPool pool;
    juce::OpenGLContext context;
    pool.initialize(context, 800, 600);
    pool.shutdown(context);

    EXPECT_FALSE(pool.isInitialized()) << "Pool must report not-initialized after shutdown";

    // FBO objects still exist but are destroyed — verify destroy was called
    for (auto* fbo : pool.createdFBOs)
        EXPECT_TRUE(fbo->destroyCalled);
}

// =============================================================================
// Lifecycle edge cases
// =============================================================================

// Bug caught: calling initialize twice without shutdown causes FBO leak
// (old FBOs abandoned, not destroyed).
TEST(FramebufferPoolTest, DoubleInitializeIsIdempotent)
{
    TestableFramebufferPool pool;
    juce::OpenGLContext context;

    pool.initialize(context, 800, 600);
    EXPECT_EQ(pool.createdFBOs.size(), 3u);

    // Second init should not create additional FBOs
    pool.initialize(context, 800, 600);

    // Should still have valid FBOs
    EXPECT_NE(pool.getWaveformFBO(), nullptr);
    EXPECT_TRUE(pool.isInitialized());
}

// Bug caught: shutdown on uninitialized pool crashes because FBO pointers
// are null and destroy() is called unconditionally.
TEST(FramebufferPoolTest, ShutdownOnUninitializedPoolIsSafe)
{
    TestableFramebufferPool pool;
    juce::OpenGLContext context;

    // Should not crash
    pool.shutdown(context);

    EXPECT_FALSE(pool.isInitialized());
}

// Bug caught: initialize-shutdown-initialize cycle fails because shutdown
// doesn't reset internal state, causing second initialize to skip creation.
TEST(FramebufferPoolTest, InitShutdownInitCycleWorks)
{
    TestableFramebufferPool pool;
    juce::OpenGLContext context;

    pool.initialize(context, 800, 600);
    pool.shutdown(context);
    EXPECT_FALSE(pool.isInitialized());

    pool.initialize(context, 1024, 768);
    EXPECT_TRUE(pool.isInitialized());
    EXPECT_EQ(pool.getWidth(), 1024);
    EXPECT_EQ(pool.getHeight(), 768);
    EXPECT_NE(pool.getWaveformFBO(), nullptr);

    pool.shutdown(context);
}

// Bug caught: querying dimensions before initialization returns garbage values.
TEST(FramebufferPoolTest, DimensionsZeroBeforeInitialization)
{
    TestableFramebufferPool pool;

    EXPECT_EQ(pool.getWidth(), 0);
    EXPECT_EQ(pool.getHeight(), 0);
    EXPECT_FALSE(pool.isInitialized());
}
