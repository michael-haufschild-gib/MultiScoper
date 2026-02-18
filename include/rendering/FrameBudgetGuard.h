/*
    Oscil - Frame Budget Guard
    Tracks elapsed time during render frames and enables graceful degradation
    when the frame budget is exceeded, preventing UI freezes.
    
    Thread Safety:
    - Used exclusively on the OpenGL render thread
    - No locks, no allocations during frame
    - All query methods are inline for minimal overhead
*/

#pragma once

#include <chrono>

namespace oscil
{

/**
 * Frame time budget guard for preventing UI freezes.
 * 
 * Tracks elapsed time during a render frame and provides budget status
 * queries to enable graceful degradation when overloaded.
 * 
 * Budget Thresholds:
 * - Normal (0-80%): Render everything
 * - Warning (80-100%): Start skipping post-processing
 * - Critical (100-120%): Skip waveforms with expensive effects
 * - Emergency (>120%): Skip shader groups and global effects
 * 
 * Usage:
 *   guard.beginFrame(config);
 *   // ... render loop ...
 *   if (guard.isCritical()) { skip expensive work; }
 *   // ... always blit to screen ...
 *   guard.endFrame();
 */
class FrameBudgetGuard
{
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    
    /**
     * Configuration for frame budget thresholds.
     */
    struct Config
    {
        double targetFrameMs = 16.67;       // 60 FPS target
        double warningThreshold = 0.80;     // 80% = start degrading
        double criticalThreshold = 1.00;    // 100% = skip non-essential
        double emergencyThreshold = 1.20;   // 120% = skip most things
    };
    
    /**
     * Statistics for the current/last frame.
     */
    struct FrameStats
    {
        double elapsedMs = 0.0;
        double budgetRatio = 0.0;           // elapsed / target
        int skippedWaveforms = 0;
        int skippedEffects = 0;
        int skippedShaderGroups = 0;
        bool budgetExceeded = false;        // true if ratio > 1.0
    };
    
    FrameBudgetGuard() = default;
    ~FrameBudgetGuard() = default;
    
    // ========================================================================
    // Lifecycle
    // ========================================================================
    
    /**
     * Start tracking a new frame. Call at the beginning of each render frame.
     */
    void beginFrame(const Config& config);
    
    /**
     * Finalize frame tracking. Call after blitToScreen().
     */
    void endFrame();
    
    // ========================================================================
    // Budget Queries (inline for speed)
    // ========================================================================
    
    /**
     * Get elapsed time since beginFrame() in milliseconds.
     */
    [[nodiscard]] double getElapsedMs() const
    {
        auto now = Clock::now();
        return std::chrono::duration<double, std::milli>(now - frameStart_).count();
    }
    
    /**
     * Get remaining budget in milliseconds (may be negative if over budget).
     */
    [[nodiscard]] double getRemainingMs() const
    {
        return config_.targetFrameMs - getElapsedMs();
    }
    
    /**
     * Get ratio of elapsed time to budget (1.0 = exactly at budget).
     */
    [[nodiscard]] double getBudgetRatio() const
    {
        double elapsed = getElapsedMs();
        return config_.targetFrameMs > 0 ? elapsed / config_.targetFrameMs : 0.0;
    }
    
    /**
     * Returns true if within budget (< warning threshold).
     */
    [[nodiscard]] bool isWithinBudget() const
    {
        return getBudgetRatio() < config_.warningThreshold;
    }
    
    /**
     * Returns true if in warning zone (80-100% of budget).
     * Action: Consider skipping post-processing on later waveforms.
     */
    [[nodiscard]] bool isWarning() const
    {
        double ratio = getBudgetRatio();
        return ratio >= config_.warningThreshold && ratio < config_.criticalThreshold;
    }
    
    /**
     * Returns true if critical (100-120% of budget).
     * Action: Skip waveforms with expensive post-processing.
     */
    [[nodiscard]] bool isCritical() const
    {
        double ratio = getBudgetRatio();
        return ratio >= config_.criticalThreshold && ratio < config_.emergencyThreshold;
    }
    
    /**
     * Returns true if in emergency (>120% of budget).
     * Action: Skip shader groups and global effects.
     */
    [[nodiscard]] bool isEmergency() const
    {
        return getBudgetRatio() >= config_.emergencyThreshold;
    }
    
    // ========================================================================
    // Recording (call when skipping work)
    // ========================================================================
    
    /**
     * Record that a waveform was skipped due to budget constraints.
     */
    void recordSkippedWaveform()
    {
        currentStats_.skippedWaveforms++;
    }
    
    /**
     * Record that an effect was skipped due to budget constraints.
     */
    void recordSkippedEffect()
    {
        currentStats_.skippedEffects++;
    }
    
    /**
     * Record that a shader group was skipped due to budget constraints.
     */
    void recordSkippedShaderGroup()
    {
        currentStats_.skippedShaderGroups++;
    }
    
    // ========================================================================
    // Statistics
    // ========================================================================
    
    /**
     * Get statistics for the current/last frame.
     */
    [[nodiscard]] FrameStats getFrameStats() const
    {
        return currentStats_;
    }
    
    /**
     * Get count of consecutive frames that exceeded budget.
     * Useful for adaptive quality decisions.
     */
    [[nodiscard]] int getConsecutiveOverBudget() const
    {
        return consecutiveOverBudget_;
    }
    
    /**
     * Get the current configuration.
     */
    [[nodiscard]] const Config& getConfig() const
    {
        return config_;
    }

private:
    TimePoint frameStart_;
    Config config_;
    FrameStats currentStats_;
    
    // For adaptive quality tracking
    int consecutiveOverBudget_ = 0;
    int consecutiveUnderBudget_ = 0;
};

} // namespace oscil

