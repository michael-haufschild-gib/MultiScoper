/*
    Oscil - Preset Preview Panel Component
    Live waveform preview with current preset applied
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include "ui/components/ThemedComponent.h"
#include "ui/components/TestId.h"
#include "rendering/VisualConfiguration.h"

namespace oscil
{

/**
 * Live preview panel for visual preset editing.
 * Displays an animated waveform with the current visual configuration applied.
 *
 * Features:
 * - Real-time waveform preview (60fps)
 * - Updates within 100ms of parameter change
 * - Synthetic sine wave or generated pattern
 * - Debounced parameter updates (50ms)
 */
class PresetPreviewPanel : public ThemedComponent,
                           public TestIdSupport,
                           private juce::Timer
{
public:
    PresetPreviewPanel(IThemeService& themeService, const juce::String& testId = "");
    ~PresetPreviewPanel() override;

    /**
     * Set the visual configuration to preview.
     * Changes are debounced by 50ms before updating the preview.
     */
    void setConfiguration(const VisualConfiguration& config);
    const VisualConfiguration& getConfiguration() const { return config_; }

    /**
     * Set whether the preview should animate.
     */
    void setAnimating(bool animate);
    bool isAnimating() const { return animating_; }

    // Size hints
    static constexpr int MIN_WIDTH = 320;
    static constexpr int MIN_HEIGHT = 240;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void updatePreview();
    void generateWaveformData();

    VisualConfiguration config_;
    bool animating_ = true;
    float animationTime_ = 0.0f;

    // Debounce for configuration changes
    bool configDirty_ = false;
    int debounceCounter_ = 0;
    static constexpr int DEBOUNCE_FRAMES = 3; // ~50ms at 60fps

    // Generated waveform data
    std::vector<float> waveformData_;
    static constexpr int WAVEFORM_SAMPLES = 512;

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetPreviewPanel)
};

} // namespace oscil
