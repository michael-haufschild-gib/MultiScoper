/*
    MultiScoper - Performance Metrics Controller Implementation
*/

#include "ui/managers/PerformanceMetricsController.h"

#include "core/MultiScoperState.h"
#include "core/dsp/CaptureQualityConfig.h"
#include "core/interfaces/IAudioDataProvider.h"
#include "core/interfaces/IInstanceRegistry.h"

#include <cmath>

namespace multiscoper
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

    // Restore sentinel values so the first update() after reset unconditionally
    // repaints the status bar even if the new metric lands within the change
    // threshold of the previously displayed value.
    lastCpuUsage_ = -1.0f;
    lastMemoryMB_ = -1.0f;
    lastFpsDisplayed_ = -1.0f;
    lastOscillatorCount_ = -1;
    lastSourceCount_ = -1;
}

void PerformanceMetricsController::update()
{
    auto currentTime = juce::Time::getMillisecondCounterHiRes();

    if (lastFrameTime_ <= 0.0)
        lastFrameTime_ = currentTime;

    bool changed = false;

    // Calculate FPS on a stable 1-second window.
    frameCount_++;
    if (currentTime - lastFrameTime_ >= 1000.0)
    {
        currentFps_ = static_cast<float>(frameCount_) * 1000.0f / static_cast<float>(currentTime - lastFrameTime_);
        frameCount_ = 0;
        lastFrameTime_ = currentTime;

        // Repaint FPS when it drifts >= 2 (users can't tell 58 vs 58.4) or
        // on the first post-reset sample — the -1.0f sentinel alone isn't
        // enough (abs(0 - -1) = 1 < 2 would keep the stale pre-reset FPS).
        if (lastFpsDisplayed_ < 0.0f || std::abs(currentFps_ - lastFpsDisplayed_) >= 2.0f)
        {
            statusBar_.setFps(currentFps_);
            lastFpsDisplayed_ = currentFps_;
            changed = true;
        }
    }

    // CPU usage.  1 percentage point is the smallest visible change in the
    // status bar's single-digit display; anything finer is just noise.
    float const cpu = dataProvider_.getCpuUsage();
    if (std::abs(cpu - lastCpuUsage_) >= 1.0f)
    {
        statusBar_.setCpuUsage(cpu);
        lastCpuUsage_ = cpu;
        changed = true;
    }

    // Memory usage — computed from capture quality config and source count.
    // Each source gets one DecimatingCaptureBuffer sized per the quality config.
    auto captureConfig = dataProvider_.getState().getCaptureQualityConfig();
    size_t const perBufferBytes =
        captureConfig.calculateMemoryUsageBytes(static_cast<int>(dataProvider_.getSampleRate()));
    size_t const sourceCount = instanceRegistry_.getSourceCount();
    size_t const totalBytes = perBufferBytes * sourceCount;
    float const memoryMB = static_cast<float>(totalBytes) / (1024.0f * 1024.0f);
    if (std::abs(memoryMB - lastMemoryMB_) >= 0.1f)
    {
        statusBar_.setMemoryUsage(memoryMB);
        lastMemoryMB_ = memoryMB;
        changed = true;
    }

    // Oscillator and source counts.
    int const oscillatorCount = dataProvider_.getState().getOscillatorCount();
    if (oscillatorCount != lastOscillatorCount_)
    {
        statusBar_.setOscillatorCount(oscillatorCount);
        lastOscillatorCount_ = oscillatorCount;
        changed = true;
    }

    int const sourceCountInt = static_cast<int>(sourceCount);
    if (sourceCountInt != lastSourceCount_)
    {
        statusBar_.setSourceCount(sourceCountInt);
        lastSourceCount_ = sourceCountInt;
        changed = true;
    }

    // Only repaint if a value actually moved — saves ~60 repaints/sec per
    // editor during idle (which, with 16 open editors, is ~1 full core).
    if (changed)
        statusBar_.repaint();
}

} // namespace multiscoper
