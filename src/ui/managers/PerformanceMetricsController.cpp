/*
    Oscil - Performance Metrics Controller Implementation
*/

#include "ui/managers/PerformanceMetricsController.h"

#include "core/OscilState.h"
#include "core/dsp/CaptureQualityConfig.h"
#include "core/interfaces/IAudioDataProvider.h"
#include "core/interfaces/IInstanceRegistry.h"

namespace oscil
{

PerformanceMetricsController::PerformanceMetricsController(IAudioDataProvider& dataProvider,
                                                           IInstanceRegistry& instanceRegistry,
                                                           StatusBarComponent& statusBar)
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

    // Memory usage — computed from capture quality config and source count.
    // Each source gets one DecimatingCaptureBuffer sized per the quality config.
    auto captureConfig = dataProvider_.getState().getCaptureQualityConfig();
    size_t const perBufferBytes =
        captureConfig.calculateMemoryUsageBytes(static_cast<int>(dataProvider_.getSampleRate()));
    size_t const sourceCount = instanceRegistry_.getSourceCount();
    size_t const totalBytes = perBufferBytes * sourceCount;
    float const memoryMB = static_cast<float>(totalBytes) / (1024.0f * 1024.0f);
    statusBar_.setMemoryUsage(memoryMB);

    // Oscillator and source counts.
    size_t const oscillatorCount = dataProvider_.getState().getOscillators().size();
    statusBar_.setOscillatorCount(static_cast<int>(oscillatorCount));
    statusBar_.setSourceCount(static_cast<int>(sourceCount));

    statusBar_.repaint();
}

} // namespace oscil
