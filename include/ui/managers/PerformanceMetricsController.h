/*
    Oscil - Performance Metrics Controller
    Handles calculation and reporting of performance metrics (FPS, CPU, Memory)
*/

#pragma once

#include "ui/panels/StatusBarComponent.h"

#include "plugin/PluginProcessor.h"

namespace oscil
{

class PerformanceMetricsController
{
public:
    PerformanceMetricsController(OscilPluginProcessor& processor, StatusBarComponent& statusBar);
    ~PerformanceMetricsController() = default;

    void update();
    void reset();

private:
    OscilPluginProcessor& processor_;
    StatusBarComponent& statusBar_;

    int frameCount_ = 0;
    double lastFrameTime_ = 0.0;
    float currentFps_ = 0.0f;
};

} // namespace oscil
