/*
    Oscil - Performance Metrics Controller
    Handles calculation and reporting of performance metrics (FPS, CPU, Memory)
*/

#pragma once

#include "ui/panels/StatusBarComponent.h"

namespace oscil
{

class IAudioDataProvider;
class IInstanceRegistry;

class PerformanceMetricsController
{
public:
    /// Construct the metrics controller wiring data sources to the status bar display.
    PerformanceMetricsController(IAudioDataProvider& dataProvider, IInstanceRegistry& instanceRegistry,
                                 StatusBarComponent& statusBar);
    ~PerformanceMetricsController() = default;

    void update();
    /// Reset all accumulated metrics (CPU, source count) to zero.
    void reset();

private:
    IAudioDataProvider& dataProvider_;
    IInstanceRegistry& instanceRegistry_;
    StatusBarComponent& statusBar_;

    int frameCount_ = 0;
    double lastFrameTime_ = 0.0;
    float currentFps_ = 0.0f;

    // Cached last-displayed values so we only repaint the status bar when
    // something the user would actually notice has changed.
    float lastCpuUsage_ = -1.0f;
    float lastMemoryMB_ = -1.0f;
    float lastFpsDisplayed_ = -1.0f;
    int lastOscillatorCount_ = -1;
    int lastSourceCount_ = -1;
};

} // namespace oscil
