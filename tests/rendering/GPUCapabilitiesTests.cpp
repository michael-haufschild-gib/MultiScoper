/*
    Oscil - GPU Capabilities Tests
    Tests for GPU feature detection and tier determination
*/

#include <gtest/gtest.h>
#include "rendering/GLErrorGuard.h"

namespace oscil::tests
{

class GLErrorStatsTest : public ::testing::Test
{
protected:
    GLErrorStats stats_;
    
    void SetUp() override
    {
        stats_.reset();
    }
};

// =============================================================================
// GLErrorStats Tests
// =============================================================================

TEST_F(GLErrorStatsTest, InitialStateIsClean)
{
    EXPECT_EQ(stats_.totalErrors.load(), 0u);
    EXPECT_EQ(stats_.errorsThisFrame.load(), 0u);
    EXPECT_EQ(stats_.consecutiveErrorFrames.load(), 0u);
    EXPECT_FALSE(stats_.fallbackTriggered.load());
    EXPECT_EQ(stats_.lastError.load(), GL_NO_ERROR);
}

TEST_F(GLErrorStatsTest, RecordErrorIncrementsCounters)
{
    stats_.recordError(GL_INVALID_ENUM, "test_operation");
    
    EXPECT_EQ(stats_.totalErrors.load(), 1u);
    EXPECT_EQ(stats_.errorsThisFrame.load(), 1u);
    EXPECT_EQ(stats_.lastError.load(), GL_INVALID_ENUM);
}

TEST_F(GLErrorStatsTest, MultipleErrorsAccumulate)
{
    stats_.recordError(GL_INVALID_ENUM, "op1");
    stats_.recordError(GL_INVALID_VALUE, "op2");
    stats_.recordError(GL_INVALID_OPERATION, "op3");
    
    EXPECT_EQ(stats_.totalErrors.load(), 3u);
    EXPECT_EQ(stats_.errorsThisFrame.load(), 3u);
    EXPECT_EQ(stats_.lastError.load(), GL_INVALID_OPERATION);  // Last error
}

TEST_F(GLErrorStatsTest, BeginFrameResetsFrameErrors)
{
    stats_.recordError(GL_INVALID_ENUM, "op1");
    stats_.recordError(GL_INVALID_VALUE, "op2");
    
    EXPECT_EQ(stats_.errorsThisFrame.load(), 2u);
    
    stats_.beginFrame();
    
    EXPECT_EQ(stats_.errorsThisFrame.load(), 0u);
    EXPECT_EQ(stats_.totalErrors.load(), 2u);  // Total not reset
}

TEST_F(GLErrorStatsTest, ConsecutiveErrorFramesTracked)
{
    // Frame 1 with errors
    stats_.recordError(GL_INVALID_ENUM, "op1");
    stats_.beginFrame();
    EXPECT_EQ(stats_.consecutiveErrorFrames.load(), 1u);
    
    // Frame 2 with errors
    stats_.recordError(GL_INVALID_VALUE, "op2");
    stats_.beginFrame();
    EXPECT_EQ(stats_.consecutiveErrorFrames.load(), 2u);
    
    // Frame 3 without errors
    stats_.beginFrame();
    EXPECT_EQ(stats_.consecutiveErrorFrames.load(), 0u);  // Reset on clean frame
}

TEST_F(GLErrorStatsTest, ShouldTriggerFallbackAfterConsecutiveErrors)
{
    // Less than 3 consecutive frames - no fallback
    stats_.recordError(GL_INVALID_ENUM, "op1");
    stats_.beginFrame();
    stats_.recordError(GL_INVALID_ENUM, "op2");
    stats_.beginFrame();
    
    EXPECT_FALSE(stats_.shouldTriggerFallback());
    
    // 3rd consecutive frame with errors
    stats_.recordError(GL_INVALID_ENUM, "op3");
    stats_.beginFrame();
    
    EXPECT_TRUE(stats_.shouldTriggerFallback());
}

TEST_F(GLErrorStatsTest, OutOfMemoryTriggersImmediateFallback)
{
    stats_.recordError(GL_OUT_OF_MEMORY, "allocation");
    
    EXPECT_TRUE(stats_.shouldTriggerFallback());
}

TEST_F(GLErrorStatsTest, ResetClearsAllCounters)
{
    stats_.recordError(GL_INVALID_ENUM, "op1");
    stats_.recordError(GL_INVALID_VALUE, "op2");
    stats_.beginFrame();
    stats_.recordError(GL_OUT_OF_MEMORY, "op3");
    stats_.fallbackTriggered.store(true);
    
    stats_.reset();
    
    EXPECT_EQ(stats_.totalErrors.load(), 0u);
    EXPECT_EQ(stats_.errorsThisFrame.load(), 0u);
    EXPECT_EQ(stats_.consecutiveErrorFrames.load(), 0u);
    EXPECT_FALSE(stats_.fallbackTriggered.load());
    EXPECT_EQ(stats_.lastError.load(), GL_NO_ERROR);
}

// =============================================================================
// RenderTier Tests
// =============================================================================

TEST(RenderTierTest, TierOrderingIsCorrect)
{
    EXPECT_LT(static_cast<int>(RenderTier::Baseline), static_cast<int>(RenderTier::Instanced));
    EXPECT_LT(static_cast<int>(RenderTier::Instanced), static_cast<int>(RenderTier::SSBO));
    EXPECT_LT(static_cast<int>(RenderTier::SSBO), static_cast<int>(RenderTier::Compute));
}

// =============================================================================
// GL Error String Tests
// =============================================================================

TEST(GLErrorStringTest, ErrorsHaveReadableStrings)
{
    EXPECT_STREQ(glErrorToString(GL_NO_ERROR), "GL_NO_ERROR");
    EXPECT_STREQ(glErrorToString(GL_INVALID_ENUM), "GL_INVALID_ENUM");
    EXPECT_STREQ(glErrorToString(GL_INVALID_VALUE), "GL_INVALID_VALUE");
    EXPECT_STREQ(glErrorToString(GL_INVALID_OPERATION), "GL_INVALID_OPERATION");
    EXPECT_STREQ(glErrorToString(GL_OUT_OF_MEMORY), "GL_OUT_OF_MEMORY");
    EXPECT_STREQ(glErrorToString(GL_INVALID_FRAMEBUFFER_OPERATION), "GL_INVALID_FRAMEBUFFER_OPERATION");
    EXPECT_STREQ(glErrorToString(0x9999), "UNKNOWN_GL_ERROR");  // Unknown code
}

} // namespace oscil::tests













