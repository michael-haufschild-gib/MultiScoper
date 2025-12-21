#pragma once
#include "ui/dialogs/preset_editor/PresetEditorTab.h"
#include "ui/components/OscilAccordion.h"
#include "ui/components/OscilSlider.h"
#include "ui/components/OscilToggle.h"
#include "ui/components/OscilButton.h"

namespace oscil
{

class EffectsTab : public PresetEditorTab
{
public:
    EffectsTab(IThemeService& themeService);
    void setConfiguration(const VisualConfiguration& config) override;
    void updateConfiguration(VisualConfiguration& config) override;

    void resized() override;
    int getPreferredHeight() const;

private:
    void setupComponents();
    
    // Helpers
    std::unique_ptr<OscilSlider> createSlider(const juce::String& testId, double min, double max, double step = 0.01);
    std::unique_ptr<OscilToggle> createToggle(const juce::String& label, const juce::String& testId);

    std::unique_ptr<OscilAccordion> effectsAccordion_;

    // Bloom
    std::unique_ptr<OscilToggle> bloomEnabled_;
    std::unique_ptr<OscilSlider> bloomIntensity_;
    std::unique_ptr<OscilSlider> bloomThreshold_;
    std::unique_ptr<OscilSlider> bloomIterations_;
    std::unique_ptr<OscilSlider> bloomDownsample_;
    std::unique_ptr<OscilSlider> bloomSpread_;
    std::unique_ptr<OscilSlider> bloomSoftKnee_;
    std::unique_ptr<juce::Component> bloomContainer_;

    // Radial Blur
    std::unique_ptr<OscilToggle> radialBlurEnabled_;
    std::unique_ptr<OscilSlider> radialBlurAmount_;
    std::unique_ptr<OscilSlider> radialBlurGlow_;
    std::unique_ptr<OscilSlider> radialBlurSamples_;
    std::unique_ptr<juce::Component> radialBlurContainer_;

    // Trails
    std::unique_ptr<OscilToggle> trailsEnabled_;
    std::unique_ptr<OscilSlider> trailsDecay_;
    std::unique_ptr<OscilSlider> trailsOpacity_;
    std::unique_ptr<juce::Component> trailsContainer_;

    // Color Grade
    std::unique_ptr<OscilToggle> colorGradeEnabled_;
    std::unique_ptr<OscilSlider> colorBrightness_;
    std::unique_ptr<OscilSlider> colorContrast_;
    std::unique_ptr<OscilSlider> colorSaturation_;
    std::unique_ptr<OscilSlider> colorTemperature_;
    std::unique_ptr<OscilSlider> colorTint_;
    std::unique_ptr<OscilButton> colorShadowsButton_;
    std::unique_ptr<OscilButton> colorHighlightsButton_;
    std::unique_ptr<juce::Component> colorGradeContainer_;

    // Vignette
    std::unique_ptr<OscilToggle> vignetteEnabled_;
    std::unique_ptr<OscilSlider> vignetteIntensity_;
    std::unique_ptr<OscilSlider> vignetteSoftness_;
    std::unique_ptr<OscilButton> vignetteColorButton_;
    std::unique_ptr<juce::Component> vignetteContainer_;

    // Film Grain
    std::unique_ptr<OscilToggle> filmGrainEnabled_;
    std::unique_ptr<OscilSlider> grainIntensity_;
    std::unique_ptr<OscilSlider> grainSpeed_;
    std::unique_ptr<juce::Component> filmGrainContainer_;

    // Chromatic Aberration
    std::unique_ptr<OscilToggle> chromaticEnabled_;
    std::unique_ptr<OscilSlider> chromaticIntensity_;
    std::unique_ptr<juce::Component> chromaticContainer_;

    // Scanlines
    std::unique_ptr<OscilToggle> scanlinesEnabled_;
    std::unique_ptr<OscilSlider> scanlinesIntensity_;
    std::unique_ptr<OscilSlider> scanlinesDensity_;
    std::unique_ptr<OscilToggle> scanlinesPhosphor_;
    std::unique_ptr<juce::Component> scanlinesContainer_;

    // Distortion
    std::unique_ptr<OscilToggle> distortionEnabled_;
    std::unique_ptr<OscilSlider> distortionIntensity_;
    std::unique_ptr<OscilSlider> distortionFrequency_;
    std::unique_ptr<OscilSlider> distortionSpeed_;
    std::unique_ptr<juce::Component> distortionContainer_;

    // Glitch
    std::unique_ptr<OscilToggle> glitchEnabled_;
    std::unique_ptr<OscilSlider> glitchIntensity_;
    std::unique_ptr<OscilSlider> glitchBlockSize_;
    std::unique_ptr<OscilSlider> glitchLineShift_;
    std::unique_ptr<OscilSlider> glitchColorSep_;
    std::unique_ptr<OscilSlider> glitchFlicker_;
    std::unique_ptr<juce::Component> glitchContainer_;

    // Tilt Shift
    std::unique_ptr<OscilToggle> tiltShiftEnabled_;
    std::unique_ptr<OscilSlider> tiltPosition_;
    std::unique_ptr<OscilSlider> tiltRange_;
    std::unique_ptr<OscilSlider> tiltBlur_;
    std::unique_ptr<OscilSlider> tiltIterations_;
    std::unique_ptr<juce::Component> tiltShiftContainer_;
};

}
