#pragma once
#include "ui/dialogs/preset_editor/PresetEditorTab.h"
#include "ui/components/OscilSlider.h"
#include "ui/components/OscilToggle.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilDropdown.h"

namespace oscil
{

class ParticlesTab : public PresetEditorTab
{
public:
    ParticlesTab(IThemeService& themeService);
    void setConfiguration(const VisualConfiguration& config) override;
    void updateConfiguration(VisualConfiguration& config) override;

    void resized() override;
    int getPreferredHeight() const;

private:
    void setupComponents();
    void populateParticleModeDropdown();
    
    // Helpers
    std::unique_ptr<OscilSlider> createSlider(const juce::String& testId, double min, double max, double step = 0.01);

    std::unique_ptr<OscilToggle> particlesEnabled_;
    std::unique_ptr<OscilDropdown> particlesModeDropdown_;
    std::unique_ptr<OscilSlider> particlesRate_;
    std::unique_ptr<OscilSlider> particlesLife_;
    std::unique_ptr<OscilSlider> particlesSize_;
    std::unique_ptr<OscilButton> particlesColorButton_;
    std::unique_ptr<OscilDropdown> particlesBlendDropdown_;
    std::unique_ptr<OscilSlider> particlesGravity_;
    std::unique_ptr<OscilSlider> particlesDrag_;
    std::unique_ptr<OscilSlider> particlesRandomness_;
    std::unique_ptr<OscilSlider> particlesVelocity_;
    std::unique_ptr<OscilToggle> particlesAudioReactive_;
    std::unique_ptr<OscilSlider> particlesAudioBoost_;
    std::unique_ptr<OscilDropdown> particlesTextureDropdown_;
    std::unique_ptr<OscilSlider> particlesTexRows_;
    std::unique_ptr<OscilSlider> particlesTexCols_;
    std::unique_ptr<OscilToggle> particlesSoft_;
    std::unique_ptr<OscilSlider> particlesSoftDepth_;
    std::unique_ptr<OscilToggle> particlesTurbulence_;
    std::unique_ptr<OscilSlider> particlesTurbStrength_;
    std::unique_ptr<OscilSlider> particlesTurbScale_;
    std::unique_ptr<OscilSlider> particlesTurbSpeed_;

    static constexpr int CONTROL_HEIGHT = 28;
};

}
