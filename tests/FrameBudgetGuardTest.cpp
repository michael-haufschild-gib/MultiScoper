/*
    Oscil - Frame Budget Guard Unit Tests
*/

#include <gtest/gtest.h>
#include "rendering/FrameBudgetGuard.h"
#include <thread>
#include <chrono>

namespace oscil
{

class FrameBudgetGuardTest : public ::testing::Test
{
protected:
    FrameBudgetGuard guard_;
    FrameBudgetGuard::Config defaultConfig_;
    
    void SetUp() override
    {
        defaultConfig_.targetFrameMs = 16.67;  // 60 FPS
        defaultConfig_.warningThreshold = 0.80;
        defaultConfig_.criticalThreshold = 1.00;
        defaultConfig_.emergencyThreshold = 1.20;
    }
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(FrameBudgetGuardTest, BeginFrameInitializesCorrectly)
{
    guard_.beginFrame(defaultConfig_);
    
    // Should be within budget immediately after starting
    EXPECT_TRUE(guard_.isWithinBudget());
    EXPECT_FALSE(guard_.isWarning());
    EXPECT_FALSE(guard_.isCritical());
    EXPECT_FALSE(guard_.isEmergency());
}

TEST_F(FrameBudgetGuardTest, ElapsedTimeIncreases)
{
    guard_.beginFrame(defaultConfig_);
    
    double elapsed1 = guard_.getElapsedMs();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    double elapsed2 = guard_.getElapsedMs();
    
    EXPECT_GT(elapsed2, elapsed1);
}

TEST_F(FrameBudgetGuardTest, RemainingTimeDecreases)
{
    guard_.beginFrame(defaultConfig_);
    
    double remaining1 = guard_.getRemainingMs();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    double remaining2 = guard_.getRemainingMs();
    
    EXPECT_LT(remaining2, remaining1);
}

TEST_F(FrameBudgetGuardTest, BudgetRatioCalculatesCorrectly)
{
    FrameBudgetGuard::Config shortConfig;
    shortConfig.targetFrameMs = 10.0;  // 10ms budget
    shortConfig.warningThreshold = 0.80;
    shortConfig.criticalThreshold = 1.00;
    shortConfig.emergencyThreshold = 1.20;
    
    guard_.beginFrame(shortConfig);
    
    // Initial ratio should be small
    EXPECT_LT(guard_.getBudgetRatio(), 0.5);
    
    // After waiting, ratio should increase
    std::this_thread::sleep_for(std::chrono::milliseconds(8));
    EXPECT_GT(guard_.getBudgetRatio(), 0.5);
}

// ============================================================================
// Threshold Tests
// ============================================================================

TEST_F(FrameBudgetGuardTest, DetectsWarningThreshold)
{
    // Use larger values to avoid timing flakiness on slow systems
    FrameBudgetGuard::Config shortConfig;
    shortConfig.targetFrameMs = 20.0;  // 20ms budget
    shortConfig.warningThreshold = 0.50;  // 10ms
    shortConfig.criticalThreshold = 1.00;  // 20ms
    shortConfig.emergencyThreshold = 1.50;  // 30ms

    guard_.beginFrame(shortConfig);
    EXPECT_FALSE(guard_.isWarning());

    // Wait past warning threshold (10ms for 20ms budget at 50%)
    // Use 15ms to ensure we're well past threshold even with timing variance
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    EXPECT_TRUE(guard_.isWarning() || guard_.isCritical());  // Might have passed critical too
}

TEST_F(FrameBudgetGuardTest, DetectsCriticalThreshold)
{
    // Use a wider range between critical and emergency to make testing reliable
    FrameBudgetGuard::Config testConfig;
    testConfig.targetFrameMs = 20.0;  // 20ms budget
    testConfig.warningThreshold = 0.50;  // 10ms
    testConfig.criticalThreshold = 0.75;  // 15ms
    testConfig.emergencyThreshold = 1.50;  // 30ms (wide gap for testing)
    
    guard_.beginFrame(testConfig);
    EXPECT_FALSE(guard_.isCritical());
    
    // Wait to enter critical zone (15-30ms range for 20ms budget at 75%-150%)
    std::this_thread::sleep_for(std::chrono::milliseconds(18));
    
    // Should be in critical zone (ratio ~0.9, between 0.75 and 1.50)
    double ratio = guard_.getBudgetRatio();
    EXPECT_GE(ratio, testConfig.warningThreshold);  // Above warning
    // Either in critical or past it - test flexibility for timer variations
    EXPECT_TRUE(guard_.isCritical() || guard_.isEmergency());
}

TEST_F(FrameBudgetGuardTest, DetectsEmergencyThreshold)
{
    FrameBudgetGuard::Config shortConfig;
    shortConfig.targetFrameMs = 10.0;  // 10ms budget
    shortConfig.warningThreshold = 0.80;
    shortConfig.criticalThreshold = 1.00;
    shortConfig.emergencyThreshold = 1.20;  // 12ms
    
    guard_.beginFrame(shortConfig);
    EXPECT_FALSE(guard_.isEmergency());
    
    // Wait past emergency threshold (12ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(13));
    EXPECT_TRUE(guard_.isEmergency());
}

// ============================================================================
// Recording Tests
// ============================================================================

TEST_F(FrameBudgetGuardTest, RecordsSkippedWaveforms)
{
    guard_.beginFrame(defaultConfig_);
    
    auto statsBefore = guard_.getFrameStats();
    EXPECT_EQ(statsBefore.skippedWaveforms, 0);
    
    guard_.recordSkippedWaveform();
    guard_.recordSkippedWaveform();
    guard_.recordSkippedWaveform();
    
    auto statsAfter = guard_.getFrameStats();
    EXPECT_EQ(statsAfter.skippedWaveforms, 3);
}

TEST_F(FrameBudgetGuardTest, RecordsSkippedEffects)
{
    guard_.beginFrame(defaultConfig_);
    
    auto statsBefore = guard_.getFrameStats();
    EXPECT_EQ(statsBefore.skippedEffects, 0);
    
    guard_.recordSkippedEffect();
    guard_.recordSkippedEffect();
    
    auto statsAfter = guard_.getFrameStats();
    EXPECT_EQ(statsAfter.skippedEffects, 2);
}

TEST_F(FrameBudgetGuardTest, RecordsSkippedShaderGroups)
{
    guard_.beginFrame(defaultConfig_);
    
    auto statsBefore = guard_.getFrameStats();
    EXPECT_EQ(statsBefore.skippedShaderGroups, 0);
    
    guard_.recordSkippedShaderGroup();
    
    auto statsAfter = guard_.getFrameStats();
    EXPECT_EQ(statsAfter.skippedShaderGroups, 1);
}

// ============================================================================
// EndFrame Tests
// ============================================================================

TEST_F(FrameBudgetGuardTest, EndFrameCalculatesFinalStats)
{
    guard_.beginFrame(defaultConfig_);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    guard_.recordSkippedWaveform();
    guard_.endFrame();
    
    auto stats = guard_.getFrameStats();
    EXPECT_GT(stats.elapsedMs, 4.0);
    EXPECT_LT(stats.elapsedMs, 20.0);
    EXPECT_GT(stats.budgetRatio, 0.0);
    EXPECT_EQ(stats.skippedWaveforms, 1);
}

TEST_F(FrameBudgetGuardTest, EndFrameDetectsBudgetExceeded)
{
    FrameBudgetGuard::Config shortConfig;
    shortConfig.targetFrameMs = 5.0;  // 5ms budget
    shortConfig.warningThreshold = 0.80;
    shortConfig.criticalThreshold = 1.00;
    shortConfig.emergencyThreshold = 1.20;
    
    guard_.beginFrame(shortConfig);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // 10ms > 5ms budget
    guard_.endFrame();
    
    auto stats = guard_.getFrameStats();
    EXPECT_TRUE(stats.budgetExceeded);
    EXPECT_GT(stats.budgetRatio, 1.0);
}

TEST_F(FrameBudgetGuardTest, EndFrameDetectsWithinBudget)
{
    guard_.beginFrame(defaultConfig_);  // 16.67ms budget
    // Don't wait - should be well within budget
    guard_.endFrame();
    
    auto stats = guard_.getFrameStats();
    EXPECT_FALSE(stats.budgetExceeded);
    EXPECT_LT(stats.budgetRatio, 1.0);
}

// ============================================================================
// Consecutive Counter Tests
// ============================================================================

TEST_F(FrameBudgetGuardTest, TracksConsecutiveOverBudget)
{
    FrameBudgetGuard::Config shortConfig;
    shortConfig.targetFrameMs = 5.0;  // 5ms budget
    shortConfig.warningThreshold = 0.80;
    shortConfig.criticalThreshold = 1.00;
    shortConfig.emergencyThreshold = 1.20;
    
    EXPECT_EQ(guard_.getConsecutiveOverBudget(), 0);
    
    // Simulate several over-budget frames
    for (int i = 0; i < 3; i++)
    {
        guard_.beginFrame(shortConfig);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Over budget
        guard_.endFrame();
    }
    
    EXPECT_EQ(guard_.getConsecutiveOverBudget(), 3);
}

TEST_F(FrameBudgetGuardTest, ResetsConsecutiveCounterAfterGoodFrames)
{
    FrameBudgetGuard::Config shortConfig;
    shortConfig.targetFrameMs = 5.0;
    shortConfig.warningThreshold = 0.80;
    shortConfig.criticalThreshold = 1.00;
    shortConfig.emergencyThreshold = 1.20;
    
    // Over budget frame
    guard_.beginFrame(shortConfig);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    guard_.endFrame();
    EXPECT_EQ(guard_.getConsecutiveOverBudget(), 1);
    
    // Many good frames (need 10 to reset)
    for (int i = 0; i < 12; i++)
    {
        guard_.beginFrame(defaultConfig_);  // 16.67ms budget - will be quick
        guard_.endFrame();
    }
    
    EXPECT_EQ(guard_.getConsecutiveOverBudget(), 0);
}

// ============================================================================
// Stats Reset Between Frames
// ============================================================================

TEST_F(FrameBudgetGuardTest, StatsResetBetweenFrames)
{
    guard_.beginFrame(defaultConfig_);
    guard_.recordSkippedWaveform();
    guard_.recordSkippedWaveform();
    guard_.endFrame();
    
    auto stats1 = guard_.getFrameStats();
    EXPECT_EQ(stats1.skippedWaveforms, 2);
    
    // Start new frame - stats should reset
    guard_.beginFrame(defaultConfig_);
    
    auto stats2 = guard_.getFrameStats();
    EXPECT_EQ(stats2.skippedWaveforms, 0);
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(FrameBudgetGuardTest, ReturnsConfig)
{
    guard_.beginFrame(defaultConfig_);
    
    auto config = guard_.getConfig();
    EXPECT_DOUBLE_EQ(config.targetFrameMs, 16.67);
    EXPECT_DOUBLE_EQ(config.warningThreshold, 0.80);
    EXPECT_DOUBLE_EQ(config.criticalThreshold, 1.00);
    EXPECT_DOUBLE_EQ(config.emergencyThreshold, 1.20);
}

TEST_F(FrameBudgetGuardTest, DifferentConfigPerFrame)
{
    FrameBudgetGuard::Config config60fps;
    config60fps.targetFrameMs = 16.67;
    
    FrameBudgetGuard::Config config30fps;
    config30fps.targetFrameMs = 33.33;
    
    guard_.beginFrame(config60fps);
    EXPECT_DOUBLE_EQ(guard_.getConfig().targetFrameMs, 16.67);
    guard_.endFrame();
    
    guard_.beginFrame(config30fps);
    EXPECT_DOUBLE_EQ(guard_.getConfig().targetFrameMs, 33.33);
    guard_.endFrame();
}

} // namespace oscil

