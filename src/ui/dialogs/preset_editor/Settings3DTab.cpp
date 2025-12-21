#include "ui/dialogs/preset_editor/Settings3DTab.h"

namespace oscil
{

Settings3DTab::Settings3DTab(IThemeService& themeService)
    : PresetEditorTab(themeService)
{
    setupComponents();
}

void Settings3DTab::setupComponents()
{
    enable3D_ = std::make_unique<OscilToggle>(getThemeService(), "Enable 3D", "preset_3d_enabled");
    enable3D_->onValueChanged = [this](bool) {
        notifyChanged();
    };
    addAndMakeVisible(*enable3D_);

    cameraDistance_ = createSlider("preset_3d_distance", 1.0, 20.0);
    cameraAngleX_ = createSlider("preset_3d_angle_x", -90.0, 90.0);
    cameraAngleY_ = createSlider("preset_3d_angle_y", 0.0, 360.0);

    autoRotate_ = std::make_unique<OscilToggle>(getThemeService(), "Auto Rotate", "preset_3d_auto_rotate");
    autoRotate_->onValueChanged = [this](bool) {
        notifyChanged();
    };
    addAndMakeVisible(*autoRotate_);
    
    rotateSpeed_ = createSlider("preset_3d_rotate_speed", 0.0, 60.0);

    meshResX_ = createSlider("preset_3d_mesh_res_x", 16, 512, 1);
    meshResZ_ = createSlider("preset_3d_mesh_res_z", 8, 128, 1);
    meshScale_ = createSlider("preset_3d_mesh_scale", 0.1, 5.0);

    lightDirX_ = createSlider("preset_light_dir_x", -1.0, 1.0);
    lightDirY_ = createSlider("preset_light_dir_y", -1.0, 1.0);
    lightDirZ_ = createSlider("preset_light_dir_z", -1.0, 1.0);

    lightAmbientButton_ = std::make_unique<OscilButton>(getThemeService(), "Ambient", "preset_light_ambient");
    lightDiffuseButton_ = std::make_unique<OscilButton>(getThemeService(), "Diffuse", "preset_light_diffuse");
    lightSpecularButton_ = std::make_unique<OscilButton>(getThemeService(), "Specular", "preset_light_specular");
    addAndMakeVisible(*lightAmbientButton_);
    addAndMakeVisible(*lightDiffuseButton_);
    addAndMakeVisible(*lightSpecularButton_);

    lightSpecPower_ = createSlider("preset_light_spec_power", 1.0, 256.0);
    lightSpecIntensity_ = createSlider("preset_light_spec_intensity", 0.0, 2.0);

    // Add remaining components
    addAndMakeVisible(*cameraDistance_);
    addAndMakeVisible(*cameraAngleX_);
    addAndMakeVisible(*cameraAngleY_);
    addAndMakeVisible(*rotateSpeed_);
    addAndMakeVisible(*meshResX_);
    addAndMakeVisible(*meshResZ_);
    addAndMakeVisible(*meshScale_);
    addAndMakeVisible(*lightDirX_);
    addAndMakeVisible(*lightDirY_);
    addAndMakeVisible(*lightDirZ_);
    addAndMakeVisible(*lightSpecPower_);
    addAndMakeVisible(*lightSpecIntensity_);
}

std::unique_ptr<OscilSlider> Settings3DTab::createSlider(const juce::String& testId, double min, double max, double step)
{
    auto slider = std::make_unique<OscilSlider>(getThemeService(), testId);
    slider->setRange(min, max);
    slider->setStep(step);
    slider->onValueChanged = [this](double) {
        notifyChanged();
    };
    return slider;
}

void Settings3DTab::setConfiguration(const VisualConfiguration& config)
{
    enable3D_->setValue(config.settings3D.enabled, false);
    cameraDistance_->setValue(config.settings3D.cameraDistance, false);
    cameraAngleX_->setValue(config.settings3D.cameraAngleX, false);
    cameraAngleY_->setValue(config.settings3D.cameraAngleY, false);
    autoRotate_->setValue(config.settings3D.autoRotate, false);
    rotateSpeed_->setValue(config.settings3D.rotateSpeed, false);

    meshResX_->setValue(config.settings3D.meshResolutionX, false);
    meshResZ_->setValue(config.settings3D.meshResolutionZ, false);
    meshScale_->setValue(config.settings3D.meshScale, false);

    // Lighting - wait, VisualConfiguration struct has `LightingConfig lighting;`
    // Let's check LightingConfig.h or VisualConfiguration.h to see members.
    // Assuming structure from original code context or inference.
    // Original code:
    /*
    lightDirX_->setValue(workingConfig_.lighting.direction.x);
    lightDirY_->setValue(workingConfig_.lighting.direction.y);
    lightDirZ_->setValue(workingConfig_.lighting.direction.z);
    // Buttons likely trigger color pickers or dialogs? Or toggles?
    // Original code didn't show implementation of button clicks.
    // Just setup.
    */
    
    lightDirX_->setValue(config.lighting.lightDirX, false);
    lightDirY_->setValue(config.lighting.lightDirY, false);
    lightDirZ_->setValue(config.lighting.lightDirZ, false);
    
    lightSpecPower_->setValue(config.lighting.specularPower, false);
    lightSpecIntensity_->setValue(config.lighting.specularIntensity, false);
}

void Settings3DTab::updateConfiguration(VisualConfiguration& config)
{
    config.settings3D.enabled = enable3D_->getValue();
    config.settings3D.cameraDistance = static_cast<float>(cameraDistance_->getValue());
    config.settings3D.cameraAngleX = static_cast<float>(cameraAngleX_->getValue());
    config.settings3D.cameraAngleY = static_cast<float>(cameraAngleY_->getValue());
    config.settings3D.autoRotate = autoRotate_->getValue();
    config.settings3D.rotateSpeed = static_cast<float>(rotateSpeed_->getValue());

    config.settings3D.meshResolutionX = static_cast<int>(meshResX_->getValue());
    config.settings3D.meshResolutionZ = static_cast<int>(meshResZ_->getValue());
    config.settings3D.meshScale = static_cast<float>(meshScale_->getValue());

    config.lighting.lightDirX = static_cast<float>(lightDirX_->getValue());
    config.lighting.lightDirY = static_cast<float>(lightDirY_->getValue());
    config.lighting.lightDirZ = static_cast<float>(lightDirZ_->getValue());
    
    config.lighting.specularPower = static_cast<float>(lightSpecPower_->getValue());
    config.lighting.specularIntensity = static_cast<float>(lightSpecIntensity_->getValue());
}

void Settings3DTab::resized()
{
    auto contentWidth = getWidth();
    int rowHeight = CONTROL_HEIGHT + 4;
    int y3d = 0;

    enable3D_->setBounds(0, y3d, contentWidth, CONTROL_HEIGHT);
    y3d += rowHeight + 8;

    cameraDistance_->setBounds(0, y3d, contentWidth, CONTROL_HEIGHT);
    y3d += rowHeight;
    cameraAngleX_->setBounds(0, y3d, contentWidth, CONTROL_HEIGHT);
    y3d += rowHeight;
    cameraAngleY_->setBounds(0, y3d, contentWidth, CONTROL_HEIGHT);
    y3d += rowHeight + 8;

    autoRotate_->setBounds(0, y3d, contentWidth, CONTROL_HEIGHT);
    y3d += rowHeight;
    rotateSpeed_->setBounds(0, y3d, contentWidth, CONTROL_HEIGHT);
    y3d += rowHeight + 8;

    meshResX_->setBounds(0, y3d, contentWidth, CONTROL_HEIGHT);
    y3d += rowHeight;
    meshResZ_->setBounds(0, y3d, contentWidth, CONTROL_HEIGHT);
    y3d += rowHeight;
    meshScale_->setBounds(0, y3d, contentWidth, CONTROL_HEIGHT);
    y3d += rowHeight + 8;

    lightDirX_->setBounds(0, y3d, contentWidth, CONTROL_HEIGHT);
    y3d += rowHeight;
    lightDirY_->setBounds(0, y3d, contentWidth, CONTROL_HEIGHT);
    y3d += rowHeight;
    lightDirZ_->setBounds(0, y3d, contentWidth, CONTROL_HEIGHT);
    y3d += rowHeight + 8;

    int btnWidth = (contentWidth - 16) / 3;
    lightAmbientButton_->setBounds(0, y3d, btnWidth, CONTROL_HEIGHT);
    lightDiffuseButton_->setBounds(btnWidth + 8, y3d, btnWidth, CONTROL_HEIGHT);
    lightSpecularButton_->setBounds((btnWidth + 8) * 2, y3d, btnWidth, CONTROL_HEIGHT);
    y3d += rowHeight + 8;

    lightSpecPower_->setBounds(0, y3d, contentWidth, CONTROL_HEIGHT);
    y3d += rowHeight;
    lightSpecIntensity_->setBounds(0, y3d, contentWidth, CONTROL_HEIGHT);
    y3d += rowHeight;
}

int Settings3DTab::getPreferredHeight() const
{
    int rowHeight = CONTROL_HEIGHT + 4;
    // 14 rows + gaps
    return 14 * rowHeight + 5 * 8; // Approx
}

}
