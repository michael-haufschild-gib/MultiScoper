/*
    Oscil - Performance Metrics Controller Implementation
*/

#include "ui/managers/PerformanceMetricsController.h"

namespace oscil
{

PerformanceMetricsController::PerformanceMetricsController(OscilPluginProcessor& processor, StatusBarComponent& statusBar)
    : processor_(processor)
    , statusBar_(statusBar)
{
}

void PerformanceMetricsController::reset()
{
    frameCount_ = 0;
    lastFrameTime_ = juce::Time::getMillisecondCounterHiRes();
    currentFps_ = 0.0f;
}

void PerformanceMetricsController::update()
{
    // Calculate FPS
    auto currentTime = juce::Time::getMillisecondCounterHiRes();
    frameCount_++;

    if (currentTime - lastFrameTime_ >= 1000.0)
    {
        currentFps_ = static_cast<float>(frameCount_) * 1000.0f / static_cast<float>(currentTime - lastFrameTime_);
        frameCount_ = 0;
        lastFrameTime_ = currentTime;

        // Update status bar with all metrics
        statusBar_.setFps(currentFps_);

        // CPU usage from processor (approximation based on audio callback time)
        statusBar_.setCpuUsage(processor_.getCpuUsage());

        // Memory usage - estimate based on capture buffer and oscillators
        // Each capture buffer ~0.5MB, plus UI overhead
        size_t sourceCount = processor_.getInstanceRegistry().getSourceCount();
        size_t oscillatorCount = processor_.getState().getOscillators().size();
        float memoryMB = 0.5f * static_cast<float>(sourceCount) + 0.1f * static_cast<float>(oscillatorCount) + 5.0f; // Base UI overhead
        statusBar_.setMemoryUsage(memoryMB);

        // Oscillator and source counts
        statusBar_.setOscillatorCount(static_cast<int>(oscillatorCount));
        statusBar_.setSourceCount(static_cast<int>(sourceCount));

        // Trigger repaint to display updated values
        statusBar_.repaint();
    }
}

} // namespace oscil
