/*
    Oscil - Frame Budget Guard Implementation
*/

#include "rendering/FrameBudgetGuard.h"

namespace oscil
{

void FrameBudgetGuard::beginFrame(const Config& config)
{
    // Store configuration for this frame
    config_ = config;
    
    // Reset per-frame statistics
    currentStats_ = FrameStats{};
    
    // Start timing
    frameStart_ = Clock::now();
}

void FrameBudgetGuard::endFrame()
{
    // Calculate final statistics
    currentStats_.elapsedMs = getElapsedMs();
    currentStats_.budgetRatio = config_.targetFrameMs > 0 
        ? currentStats_.elapsedMs / config_.targetFrameMs 
        : 0.0;
    currentStats_.budgetExceeded = currentStats_.budgetRatio > 1.0;
    
    // Update consecutive counters for adaptive quality
    if (currentStats_.budgetExceeded)
    {
        consecutiveOverBudget_++;
        consecutiveUnderBudget_ = 0;
    }
    else
    {
        consecutiveUnderBudget_++;
        // Only reset over-budget counter if we've had several good frames
        if (consecutiveUnderBudget_ >= 10)
        {
            consecutiveOverBudget_ = 0;
        }
    }
}

} // namespace oscil

