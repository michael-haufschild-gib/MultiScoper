#include "ui/dialogs/preset_editor/ShaderSettingsTab.h"

namespace oscil
{

ShaderSettingsTab::ShaderSettingsTab(IThemeService& themeService)
    : PresetEditorTab(themeService)
{
    setupComponents();
}

void ShaderSettingsTab::setupComponents()
{
    // Shader type
    shaderTypeLabel_ = std::make_unique<juce::Label>("", "Shader Type:");
    shaderTypeLabel_->setFont(juce::FontOptions(13.0f));
    addAndMakeVisible(*shaderTypeLabel_);

    shaderTypeDropdown_ = std::make_unique<OscilDropdown>(getThemeService(), "Select shader...", "preset_shader_type_dropdown");
    populateShaderDropdown();
    shaderTypeDropdown_->onSelectionChanged = [this](int) {
        notifyChanged();
    };
    addAndMakeVisible(*shaderTypeDropdown_);

    // Blend mode
    blendModeLabel_ = std::make_unique<juce::Label>("", "Blend Mode:");
    blendModeLabel_->setFont(juce::FontOptions(13.0f));
    addAndMakeVisible(*blendModeLabel_);

    blendModeDropdown_ = std::make_unique<OscilDropdown>(getThemeService(), "Alpha", "preset_blend_mode_dropdown");
    populateBlendModeDropdown();
    blendModeDropdown_->onSelectionChanged = [this](int) {
        notifyChanged();
    };
    addAndMakeVisible(*blendModeDropdown_);

    // Opacity
    opacityLabel_ = std::make_unique<juce::Label>("", "Opacity:");
    opacityLabel_->setFont(juce::FontOptions(13.0f));
    addAndMakeVisible(*opacityLabel_);

    opacitySlider_ = std::make_unique<OscilSlider>(getThemeService(), "preset_opacity_slider");
    opacitySlider_->setRange(0.0, 1.0);
    opacitySlider_->setStep(0.01);
    opacitySlider_->setValue(1.0);
    opacitySlider_->onValueChanged = [this](double) {
        notifyChanged();
    };
    addAndMakeVisible(*opacitySlider_);
}

void ShaderSettingsTab::populateShaderDropdown()
{
    // 2D Shaders
    shaderTypeDropdown_->addItem("Basic 2D", "basic2d");
    shaderTypeDropdown_->addItem("Neon Glow", "neonglow");
    shaderTypeDropdown_->addItem("Gradient Fill", "gradientfill");
    shaderTypeDropdown_->addItem("Dual Outline", "dualoutline");
    shaderTypeDropdown_->addItem("Plasma Sine", "plasmasine");
    // 3D Shaders
    shaderTypeDropdown_->addItem("Volumetric Ribbon", "volumetricribbon");
    shaderTypeDropdown_->addItem("Wireframe Mesh", "wireframemesh");
    shaderTypeDropdown_->addItem("Vector Flow", "vectorflow");
    shaderTypeDropdown_->addItem("String Theory", "stringtheory");
    shaderTypeDropdown_->addItem("Electric Flower", "electricflower");
    shaderTypeDropdown_->addItem("Electric Filigree", "electricfiligree");
    // Material Shaders
    shaderTypeDropdown_->addItem("Glass Refraction", "glassrefraction");
    shaderTypeDropdown_->addItem("Liquid Chrome", "liquidchrome");
    shaderTypeDropdown_->addItem("Crystalline", "crystalline");
}

void ShaderSettingsTab::populateBlendModeDropdown()
{
    blendModeDropdown_->addItem("Alpha", "alpha");
    blendModeDropdown_->addItem("Additive", "additive");
    blendModeDropdown_->addItem("Multiply", "multiply");
    blendModeDropdown_->addItem("Screen", "screen");
}

void ShaderSettingsTab::setConfiguration(const VisualConfiguration& config)
{
    // Shader type
    auto shaderId = shaderTypeToId(config.shaderType);
    
    // Helper to find index by ID
    for (int i = 0; i < shaderTypeDropdown_->getNumItems(); ++i)
    {
        if (shaderTypeDropdown_->getItem(i).id == shaderId)
        {
            shaderTypeDropdown_->setSelectedIndex(i, false);
            break;
        }
    }

    juce::String blendId;
    switch (config.compositeBlendMode)
    {
        case BlendMode::Alpha: blendId = "alpha"; break;
        case BlendMode::Additive: blendId = "additive"; break;
        case BlendMode::Multiply: blendId = "multiply"; break;
        case BlendMode::Screen: blendId = "screen"; break;
    }
    
    for (int i = 0; i < blendModeDropdown_->getNumItems(); ++i)
    {
        if (blendModeDropdown_->getItem(i).id == blendId)
        {
            blendModeDropdown_->setSelectedIndex(i, false);
            break;
        }
    }
    
    opacitySlider_->setValue(config.compositeOpacity, false);
}

void ShaderSettingsTab::updateConfiguration(VisualConfiguration& config)
{
    auto shaderIdStr = shaderTypeDropdown_->getSelectedId();
    config.shaderType = idToShaderType(shaderIdStr);

    auto blendId = blendModeDropdown_->getSelectedId();
    if (blendId == "alpha") config.compositeBlendMode = BlendMode::Alpha;
    else if (blendId == "additive") config.compositeBlendMode = BlendMode::Additive;
    else if (blendId == "multiply") config.compositeBlendMode = BlendMode::Multiply;
    else if (blendId == "screen") config.compositeBlendMode = BlendMode::Screen;

    config.compositeOpacity = static_cast<float>(opacitySlider_->getValue());
}

void ShaderSettingsTab::resized()
{
    auto bounds = getLocalBounds();
    int y = 0;
    int rowHeight = CONTROL_HEIGHT + 4;
    int labelWidth = 100;
    int contentWidth = bounds.getWidth();

    shaderTypeLabel_->setBounds(0, y, labelWidth, CONTROL_HEIGHT);
    shaderTypeDropdown_->setBounds(labelWidth + 10, y, contentWidth - (labelWidth + 10), CONTROL_HEIGHT);
    y += rowHeight + 8;

    blendModeLabel_->setBounds(0, y, labelWidth, CONTROL_HEIGHT);
    blendModeDropdown_->setBounds(labelWidth + 10, y, contentWidth - (labelWidth + 10), CONTROL_HEIGHT);
    y += rowHeight + 8;

    opacityLabel_->setBounds(0, y, labelWidth, CONTROL_HEIGHT);
    opacitySlider_->setBounds(labelWidth + 10, y, contentWidth - (labelWidth + 10), CONTROL_HEIGHT);
}

int ShaderSettingsTab::getPreferredHeight() const
{
    // 3 rows + spacing + padding
    return 3 * (CONTROL_HEIGHT + 8) + 20;
}

}
