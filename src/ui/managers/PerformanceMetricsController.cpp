/*
    Oscil - Performance Metrics Controller Implementation
*/

#include "ui/managers/PerformanceMetricsController.h"
#include "core/interfaces/IAudioDataProvider.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "core/OscilState.h"

namespace oscil
{

PerformanceMetricsController::PerformanceMetricsController(IAudioDataProvider& dataProvider, IInstanceRegistry& instanceRegistry, StatusBarComponent& statusBar)
    : dataProvider_(dataProvider)
    , instanceRegistry_(instanceRegistry)
    , statusBar_(statusBar)
{
    reset();
}

void PerformanceMetricsController::reset()
{
    frameCount_ = 0;
    lastFrameTime_ = juce::Time::getMillisecondCounterHiRes();
    currentFps_ = 0.0f;
}

void PerformanceMetricsController::update()
{
    auto currentTime = juce::Time::getMillisecondCounterHiRes();

    if (lastFrameTime_ <= 0.0)
        lastFrameTime_ = currentTime;

    // Calculate FPS on a stable 1-second window.
    frameCount_++;
    if (currentTime - lastFrameTime_ >= 1000.0)
    {
        currentFps_ = static_cast<float>(frameCount_) * 1000.0f / static_cast<float>(currentTime - lastFrameTime_);
        frameCount_ = 0;
        lastFrameTime_ = currentTime;

        statusBar_.setFps(currentFps_);
    }

    // Update non-FPS metrics every tick to avoid visible lag after state changes.
    statusBar_.setCpuUsage(dataProvider_.getCpuUsage());

    // Memory usage - estimate based on capture buffer and oscillators.
    size_t sourceCount = instanceRegistry_.getSourceCount();
    size_t oscillatorCount = dataProvider_.getState().getOscillators().size();
    float memoryMB = 0.5f * static_cast<float>(sourceCount) + 0.1f * static_cast<float>(oscillatorCount) + 5.0f;
    statusBar_.setMemoryUsage(memoryMB);

    // Oscillator and source counts.
    statusBar_.setOscillatorCount(static_cast<int>(oscillatorCount));
    statusBar_.setSourceCount(static_cast<int>(sourceCount));

    statusBar_.repaint();
}

} // namespace oscil
