/*
    Oscil - Status Bar Component Header
    Displays real-time performance metrics
*/

#pragma once

#include "ui/components/TestId.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace oscil
{

class IThemeService;

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
class StatusBarComponent
    : public juce::Component
    , public TestIdSupport
{
public:
    explicit StatusBarComponent(IThemeService& themeService);
    ~StatusBarComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setFps(float fps);
    void setCpuUsage(float cpu);
    void setMemoryUsage(float memory);
    void setOscillatorCount(int count);
    void setSourceCount(int count);
    void setRenderingMode(RenderingMode mode);

    RenderingMode getRenderingMode() const { return renderingMode_; }

    /**
     * Detect rendering mode based on compile-time options and runtime state
     */
    static RenderingMode detectRenderingMode();

private:
    void updateFpsLabel();
    void updateCpuLabel();
    void updateMemoryLabel();
    void updateOscillatorLabel();
    void updateSourceLabel();
    void updateRenderModeLabel();

    IThemeService& themeService_;
    float currentFps_ = 0.0f;
    float cpuUsage_ = 0.0f;
    float memoryUsage_ = 0.0f;
    int oscillatorCount_ = 0;
    int sourceCount_ = 0;
    RenderingMode renderingMode_ = RenderingMode::Software;

    std::unique_ptr<juce::Label> fpsLabel_;
    std::unique_ptr<juce::Label> cpuLabel_;
    std::unique_ptr<juce::Label> memoryLabel_;
    std::unique_ptr<juce::Label> oscillatorLabel_;
    std::unique_ptr<juce::Label> sourceLabel_;
    std::unique_ptr<juce::Label> renderModeLabel_;

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBarComponent)
};

} // namespace oscil
