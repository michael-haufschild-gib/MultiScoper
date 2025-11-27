/*
    Oscil - Status Bar Component Header
    Displays real-time performance metrics
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace oscil
{

/**
 * Rendering mode enumeration
 */
enum class RenderingMode
{
    Software,
    OpenGL
};

/**
 * Status bar showing FPS, CPU usage, memory usage, and rendering mode
 */
class StatusBarComponent : public juce::Component
{
public:
    StatusBarComponent();
    ~StatusBarComponent() override = default;

    void paint(juce::Graphics& g) override;

    void setFps(float fps) { currentFps_ = fps; }
    void setCpuUsage(float cpu) { cpuUsage_ = cpu; }
    void setMemoryUsage(float memory) { memoryUsage_ = memory; }
    void setOscillatorCount(int count) { oscillatorCount_ = count; }
    void setSourceCount(int count) { sourceCount_ = count; }
    void setRenderingMode(RenderingMode mode) { renderingMode_ = mode; }

    RenderingMode getRenderingMode() const { return renderingMode_; }

    /**
     * Detect rendering mode based on compile-time options and runtime state
     */
    static RenderingMode detectRenderingMode();

private:
    float currentFps_ = 0.0f;
    float cpuUsage_ = 0.0f;
    float memoryUsage_ = 0.0f;
    int oscillatorCount_ = 0;
    int sourceCount_ = 0;
    RenderingMode renderingMode_ = RenderingMode::Software;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBarComponent)
};

} // namespace oscil
