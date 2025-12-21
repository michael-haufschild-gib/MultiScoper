#pragma once
#include "ui/dialogs/preset_editor/PresetEditorTab.h"
#include "ui/components/OscilSlider.h"
#include "ui/components/OscilToggle.h"
#include "ui/components/OscilButton.h"

namespace oscil
{

class Settings3DTab : public PresetEditorTab
{
public:
    Settings3DTab(IThemeService& themeService);
    void setConfiguration(const VisualConfiguration& config) override;
    void updateConfiguration(VisualConfiguration& config) override;

    void resized() override;
    int getPreferredHeight() const;

private:
    void setupComponents();
    
    std::unique_ptr<OscilSlider> createSlider(const juce::String& testId, double min, double max, double step = 0.01);

    std::unique_ptr<OscilToggle> enable3D_;
    std::unique_ptr<OscilSlider> cameraDistance_;
    std::unique_ptr<OscilSlider> cameraAngleX_;
    std::unique_ptr<OscilSlider> cameraAngleY_;
    std::unique_ptr<OscilToggle> autoRotate_;
    std::unique_ptr<OscilSlider> rotateSpeed_;
    std::unique_ptr<OscilSlider> meshResX_;
    std::unique_ptr<OscilSlider> meshResZ_;
    std::unique_ptr<OscilSlider> meshScale_;
    std::unique_ptr<OscilSlider> lightDirX_;
    std::unique_ptr<OscilSlider> lightDirY_;
    std::unique_ptr<OscilSlider> lightDirZ_;
    std::unique_ptr<OscilButton> lightAmbientButton_;
    std::unique_ptr<OscilButton> lightDiffuseButton_;
    std::unique_ptr<OscilButton> lightSpecularButton_;
    std::unique_ptr<OscilSlider> lightSpecPower_;
    std::unique_ptr<OscilSlider> lightSpecIntensity_;

    static constexpr int CONTROL_HEIGHT = 28;
};

}
