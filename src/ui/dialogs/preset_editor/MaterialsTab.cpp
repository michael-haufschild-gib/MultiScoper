#include "ui/dialogs/preset_editor/MaterialsTab.h"

namespace oscil
{

MaterialsTab::MaterialsTab(IThemeService& themeService)
    : PresetEditorTab(themeService)
{
    setupComponents();
}

void MaterialsTab::setupComponents()
{
    materialEnabled_ = std::make_unique<OscilToggle>(getThemeService(), "Enable Materials", "preset_material_enabled");
    materialEnabled_->onValueChanged = [this](bool) {
        notifyChanged();
    };
    addAndMakeVisible(*materialEnabled_);

    materialReflectivity_ = createSlider("preset_material_reflectivity", 0.0, 1.0);
    materialIOR_ = createSlider("preset_material_ior", 1.0, 3.0);
    materialFresnel_ = createSlider("preset_material_fresnel", 0.5, 5.0);
    materialRoughness_ = createSlider("preset_material_roughness", 0.0, 1.0);

    materialTintButton_ = std::make_unique<OscilButton>(getThemeService(), "Tint", "preset_material_tint");

    materialUseEnv_ = std::make_unique<OscilToggle>(getThemeService(), "Use Environment Map", "preset_material_use_env");
    materialUseEnv_->onValueChanged = [this](bool) {
        notifyChanged();
    };

    materialEnvMapDropdown_ = std::make_unique<OscilDropdown>(getThemeService(), "Environment", "preset_material_env_map");
    populateEnvironmentMapDropdown();
    materialEnvMapDropdown_->onSelectionChanged = [this](int) {
        notifyChanged();
    };

    // Add all
    addAndMakeVisible(*materialReflectivity_);
    addAndMakeVisible(*materialIOR_);
    addAndMakeVisible(*materialFresnel_);
    addAndMakeVisible(*materialRoughness_);
    addAndMakeVisible(*materialTintButton_);
    addAndMakeVisible(*materialUseEnv_);
    addAndMakeVisible(*materialEnvMapDropdown_);
}

std::unique_ptr<OscilSlider> MaterialsTab::createSlider(const juce::String& testId, double min, double max, double step)
{
    auto slider = std::make_unique<OscilSlider>(getThemeService(), testId);
    slider->setRange(min, max);
    slider->setStep(step);
    slider->onValueChanged = [this](double) {
        notifyChanged();
    };
    return slider;
}

void MaterialsTab::populateEnvironmentMapDropdown()
{
    materialEnvMapDropdown_->addItem("Default Studio", "default_studio");
    materialEnvMapDropdown_->addItem("Sunset", "sunset");
    materialEnvMapDropdown_->addItem("Night", "night");
    materialEnvMapDropdown_->addItem("Abstract", "abstract");
}

void MaterialsTab::setConfiguration(const VisualConfiguration& config)
{
    materialEnabled_->setValue(config.material.enabled, false);
    materialReflectivity_->setValue(config.material.reflectivity, false);
    materialIOR_->setValue(config.material.refractiveIndex, false);
    materialFresnel_->setValue(config.material.fresnelPower, false);
    materialRoughness_->setValue(config.material.roughness, false);

    materialUseEnv_->setValue(config.material.useEnvironmentMap, false);
    
    for (int i = 0; i < materialEnvMapDropdown_->getNumItems(); ++i)
    {
        if (materialEnvMapDropdown_->getItem(i).id == config.material.environmentMapId)
        {
            materialEnvMapDropdown_->setSelectedIndex(i, false);
            break;
        }
    }
}

void MaterialsTab::updateConfiguration(VisualConfiguration& config)
{
    config.material.enabled = materialEnabled_->getValue();
    config.material.reflectivity = static_cast<float>(materialReflectivity_->getValue());
    config.material.refractiveIndex = static_cast<float>(materialIOR_->getValue());
    config.material.fresnelPower = static_cast<float>(materialFresnel_->getValue());
    config.material.roughness = static_cast<float>(materialRoughness_->getValue());
    
    config.material.useEnvironmentMap = materialUseEnv_->getValue();
    config.material.environmentMapId = materialEnvMapDropdown_->getSelectedId();
}

void MaterialsTab::resized()
{
    auto contentWidth = getWidth();
    int rowHeight = CONTROL_HEIGHT + 4;
    int yMat = 0;

    materialEnabled_->setBounds(0, yMat, contentWidth, CONTROL_HEIGHT);
    yMat += rowHeight + 8;

    materialReflectivity_->setBounds(0, yMat, contentWidth, CONTROL_HEIGHT);
    yMat += rowHeight;
    materialIOR_->setBounds(0, yMat, contentWidth, CONTROL_HEIGHT);
    yMat += rowHeight;
    materialFresnel_->setBounds(0, yMat, contentWidth, CONTROL_HEIGHT);
    yMat += rowHeight;
    materialRoughness_->setBounds(0, yMat, contentWidth, CONTROL_HEIGHT);
    yMat += rowHeight + 8;

    materialTintButton_->setBounds(0, yMat, 100, CONTROL_HEIGHT);
    yMat += rowHeight + 8;

    materialUseEnv_->setBounds(0, yMat, contentWidth, CONTROL_HEIGHT);
    yMat += rowHeight;
    materialEnvMapDropdown_->setBounds(0, yMat, contentWidth, CONTROL_HEIGHT);
    yMat += rowHeight;
}

int MaterialsTab::getPreferredHeight() const
{
    int rowHeight = CONTROL_HEIGHT + 4;
    // 8 rows + gaps
    return 8 * rowHeight + 4 * 8; // Approx
}

}
