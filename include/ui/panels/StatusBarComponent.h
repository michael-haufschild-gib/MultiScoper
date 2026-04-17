/*
    Oscil - Status Bar Component Header
    Two-zone layout: left contextual hint text, right metrics/mode group.
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
 * Status bar with a two-zone layout: contextual hint on the left and
 * performance metrics plus render mode on the right, divided by a hairline.
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

    // Hint zone API. Empty string clears. Repaints on change.
    void setHintText(const juce::String& text);
    juce::String getHintText() const { return hintText_; }

    /**
     * Detect rendering mode based on compile-time options and runtime state
     */
    static RenderingMode detectRenderingMode();

private:
    /**
     * Return the hint text as it would render in `availableWidth` pixels,
     * elided at the end with `...` when the full string would not fit.
     * Private implementation detail; exposed to tests through the friend
     * declaration below so unit tests can assert layout behaviour without
     * scraping rendered pixels.
     */
    juce::String getElidedHintText(float availableWidth) const;

    /**
     * True when the vertical separator between the hint zone and the
     * metrics zone should be drawn (i.e. the hint is non-empty and the
     * layout has space for both zones). Private for the same reason as
     * `getElidedHintText`.
     */
    bool shouldDrawSeparator() const;

    friend class StatusBarComponentTestAccess;

    void updateFpsLabel();
    void updateCpuLabel();
    void updateMemoryLabel();
    void updateOscillatorLabel();
    void updateSourceLabel();
    void updateRenderModeLabel();

    // Right-aligned metrics group width, including inter-item spacing.
    int getRightZoneWidth() const;
    // Hint zone width after deducting right zone, separator, and outer margins.
    int getLeftZoneWidth() const;

    IThemeService& themeService_;
    float currentFps_ = 0.0f;
    float cpuUsage_ = 0.0f;
    float memoryUsage_ = 0.0f;
    int oscillatorCount_ = 0;
    int sourceCount_ = 0;
    RenderingMode renderingMode_ = RenderingMode::Software;

    juce::String hintText_;

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
