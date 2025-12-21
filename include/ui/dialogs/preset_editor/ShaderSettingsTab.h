#pragma once
#include "ui/dialogs/preset_editor/PresetEditorTab.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/OscilSlider.h"

namespace oscil
{

class ShaderSettingsTab : public PresetEditorTab
{
public:
    ShaderSettingsTab(IThemeService& themeService);
    void setConfiguration(const VisualConfiguration& config) override;
    void updateConfiguration(VisualConfiguration& config) override;

    void resized() override;

    int getPreferredHeight() const;

private:
    void setupComponents();
    void populateShaderDropdown();
    void populateBlendModeDropdown();

    std::unique_ptr<juce::Label> shaderTypeLabel_;
    std::unique_ptr<OscilDropdown> shaderTypeDropdown_;
    std::unique_ptr<juce::Label> blendModeLabel_;
    std::unique_ptr<OscilDropdown> blendModeDropdown_;
    std::unique_ptr<juce::Label> opacityLabel_;
    std::unique_ptr<OscilSlider> opacitySlider_;

    static constexpr int CONTROL_HEIGHT = 28;
};

}
