#pragma once
#include "ui/dialogs/preset_editor/PresetEditorTab.h"
#include "ui/components/OscilSlider.h"
#include "ui/components/OscilToggle.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilDropdown.h"

namespace oscil
{

class MaterialsTab : public PresetEditorTab
{
public:
    MaterialsTab(IThemeService& themeService);
    void setConfiguration(const VisualConfiguration& config) override;
    void updateConfiguration(VisualConfiguration& config) override;

    void resized() override;
    int getPreferredHeight() const;

private:
    void setupComponents();
    void populateEnvironmentMapDropdown();
    
    std::unique_ptr<OscilSlider> createSlider(const juce::String& testId, double min, double max, double step = 0.01);

    std::unique_ptr<OscilToggle> materialEnabled_;
    std::unique_ptr<OscilSlider> materialReflectivity_;
    std::unique_ptr<OscilSlider> materialIOR_;
    std::unique_ptr<OscilSlider> materialFresnel_;
    std::unique_ptr<OscilSlider> materialRoughness_;
    std::unique_ptr<OscilButton> materialTintButton_;
    std::unique_ptr<OscilToggle> materialUseEnv_;
    std::unique_ptr<OscilDropdown> materialEnvMapDropdown_;

    static constexpr int CONTROL_HEIGHT = 28;
};

}
