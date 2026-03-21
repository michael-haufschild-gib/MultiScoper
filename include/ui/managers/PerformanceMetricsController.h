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
    PerformanceMetricsController(IAudioDataProvider& dataProvider, IInstanceRegistry& instanceRegistry, StatusBarComponent& statusBar);
    ~PerformanceMetricsController() = default;

    void update();
    void reset();

private:
    IAudioDataProvider& dataProvider_;
    IInstanceRegistry& instanceRegistry_;
    StatusBarComponent& statusBar_;

    int frameCount_ = 0;
    double lastFrameTime_ = 0.0;
    float currentFps_ = 0.0f;
};

} // namespace oscil
