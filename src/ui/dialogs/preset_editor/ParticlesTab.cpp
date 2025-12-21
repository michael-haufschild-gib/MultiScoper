#include "ui/dialogs/preset_editor/ParticlesTab.h"

namespace oscil
{

ParticlesTab::ParticlesTab(IThemeService& themeService)
    : PresetEditorTab(themeService)
{
    setupComponents();
}

void ParticlesTab::setupComponents()
{
    particlesEnabled_ = std::make_unique<OscilToggle>(getThemeService(), "Enable Particles", "preset_particles_enabled");
    particlesEnabled_->onValueChanged = [this](bool) {
        notifyChanged();
    };
    addAndMakeVisible(*particlesEnabled_);

    particlesModeDropdown_ = std::make_unique<OscilDropdown>(getThemeService(), "Emission Mode", "preset_particles_mode");
    populateParticleModeDropdown();
    particlesModeDropdown_->onSelectionChanged = [this](int) {
        notifyChanged();
    };
    addAndMakeVisible(*particlesModeDropdown_);

    particlesRate_ = createSlider("preset_particles_rate", 1.0, 1000.0);
    particlesLife_ = createSlider("preset_particles_life", 0.1, 10.0);
    particlesSize_ = createSlider("preset_particles_size", 1.0, 32.0);
    particlesColorButton_ = std::make_unique<OscilButton>(getThemeService(), "Color", "preset_particles_color");
    
    particlesBlendDropdown_ = std::make_unique<OscilDropdown>(getThemeService(), "Blend", "preset_particles_blend");
    particlesBlendDropdown_->addItem("Additive", "additive");
    particlesBlendDropdown_->addItem("Alpha", "alpha");
    particlesBlendDropdown_->addItem("Multiply", "multiply");
    particlesBlendDropdown_->addItem("Screen", "screen");
    particlesBlendDropdown_->onSelectionChanged = [this](int) {
        notifyChanged();
    };
    addAndMakeVisible(*particlesBlendDropdown_);

    particlesGravity_ = createSlider("preset_particles_gravity", -100.0, 100.0);
    particlesDrag_ = createSlider("preset_particles_drag", 0.0, 1.0);
    particlesRandomness_ = createSlider("preset_particles_randomness", 0.0, 1.0);
    particlesVelocity_ = createSlider("preset_particles_velocity", 0.0, 5.0);

    particlesAudioReactive_ = std::make_unique<OscilToggle>(getThemeService(), "Audio Reactive", "preset_particles_audio_reactive");
    particlesAudioReactive_->onValueChanged = [this](bool) {
        notifyChanged();
    };
    addAndMakeVisible(*particlesAudioReactive_);
    
    particlesAudioBoost_ = createSlider("preset_particles_audio_boost", 1.0, 10.0);

    particlesTextureDropdown_ = std::make_unique<OscilDropdown>(getThemeService(), "Texture", "preset_particles_texture");
    particlesTextureDropdown_->addItem("(none)", "");
    particlesTextureDropdown_->addItem("Sparkle", "sparkle");
    particlesTextureDropdown_->addItem("Dot", "dot");
    particlesTextureDropdown_->addItem("Star", "star");
    particlesTextureDropdown_->onSelectionChanged = [this](int) {
        notifyChanged();
    };
    addAndMakeVisible(*particlesTextureDropdown_);

    particlesTexRows_ = createSlider("preset_particles_tex_rows", 1, 16, 1);
    particlesTexCols_ = createSlider("preset_particles_tex_cols", 1, 16, 1);

    particlesSoft_ = std::make_unique<OscilToggle>(getThemeService(), "Soft Particles", "preset_particles_soft");
    particlesSoft_->onValueChanged = [this](bool) {
        notifyChanged();
    };
    addAndMakeVisible(*particlesSoft_);
    
    particlesSoftDepth_ = createSlider("preset_particles_soft_depth", 0.1, 5.0);

    particlesTurbulence_ = std::make_unique<OscilToggle>(getThemeService(), "Use Turbulence", "preset_particles_turbulence");
    particlesTurbulence_->onValueChanged = [this](bool) {
        notifyChanged();
    };
    addAndMakeVisible(*particlesTurbulence_);
    
    particlesTurbStrength_ = createSlider("preset_particles_turb_strength", 0.0, 10.0);
    particlesTurbScale_ = createSlider("preset_particles_turb_scale", 0.1, 2.0);
    particlesTurbSpeed_ = createSlider("preset_particles_turb_speed", 0.1, 2.0);

    // Add remaining components
    addAndMakeVisible(*particlesRate_);
    addAndMakeVisible(*particlesLife_);
    addAndMakeVisible(*particlesSize_);
    addAndMakeVisible(*particlesColorButton_);
    addAndMakeVisible(*particlesGravity_);
    addAndMakeVisible(*particlesDrag_);
    addAndMakeVisible(*particlesRandomness_);
    addAndMakeVisible(*particlesVelocity_);
    addAndMakeVisible(*particlesAudioBoost_);
    addAndMakeVisible(*particlesTexRows_);
    addAndMakeVisible(*particlesTexCols_);
    addAndMakeVisible(*particlesSoftDepth_);
    addAndMakeVisible(*particlesTurbStrength_);
    addAndMakeVisible(*particlesTurbScale_);
    addAndMakeVisible(*particlesTurbSpeed_);
}

std::unique_ptr<OscilSlider> ParticlesTab::createSlider(const juce::String& testId, double min, double max, double step)
{
    auto slider = std::make_unique<OscilSlider>(getThemeService(), testId);
    slider->setRange(min, max);
    slider->setStep(step);
    slider->onValueChanged = [this](double) {
        notifyChanged();
    };
    return slider;
}

void ParticlesTab::populateParticleModeDropdown()
{
    particlesModeDropdown_->addItem("Along Waveform", "alongwaveform");
    particlesModeDropdown_->addItem("At Peaks", "atpeaks");
    particlesModeDropdown_->addItem("At Zero Crossings", "atzerocrossings");
    particlesModeDropdown_->addItem("Continuous", "continuous");
    particlesModeDropdown_->addItem("Burst", "burst");
}

void ParticlesTab::setConfiguration(const VisualConfiguration& config)
{
    particlesEnabled_->setValue(config.particles.enabled, false);
    particlesRate_->setValue(config.particles.emissionRate, false);
    particlesLife_->setValue(config.particles.particleLife, false);
    particlesSize_->setValue(config.particles.particleSize, false);
    
    // Dropdowns - simplified selection logic for brevity, robust implementation would iterate
    // Mode
    juce::String modeId;
    switch (config.particles.emissionMode)
    {
        case ParticleEmissionMode::AlongWaveform: modeId = "alongwaveform"; break;
        case ParticleEmissionMode::AtPeaks: modeId = "atpeaks"; break;
        case ParticleEmissionMode::AtZeroCrossings: modeId = "atzerocrossings"; break;
        case ParticleEmissionMode::Continuous: modeId = "continuous"; break;
        case ParticleEmissionMode::Burst: modeId = "burst"; break;
    }
    // Find and select
    for (int i = 0; i < particlesModeDropdown_->getNumItems(); ++i)
    {
        if (particlesModeDropdown_->getItem(i).id == modeId)
        {
            particlesModeDropdown_->setSelectedIndex(i, false);
            break;
        }
    }

    // Blend
    juce::String blendId;
    switch (config.particles.blendMode)
    {
        case ParticleBlendMode::Additive: blendId = "additive"; break;
        case ParticleBlendMode::Alpha: blendId = "alpha"; break;
        case ParticleBlendMode::Multiply: blendId = "multiply"; break;
        case ParticleBlendMode::Screen: blendId = "screen"; break;
    }
    for (int i = 0; i < particlesBlendDropdown_->getNumItems(); ++i)
    {
        if (particlesBlendDropdown_->getItem(i).id == blendId)
        {
            particlesBlendDropdown_->setSelectedIndex(i, false);
            break;
        }
    }

    particlesGravity_->setValue(config.particles.gravity, false);
    particlesDrag_->setValue(config.particles.drag, false);
    particlesRandomness_->setValue(config.particles.randomness, false);
    particlesVelocity_->setValue(config.particles.velocityScale, false);

    particlesAudioReactive_->setValue(config.particles.audioReactive, false);
    particlesAudioBoost_->setValue(config.particles.audioEmissionBoost, false);

    // Texture
    for (int i = 0; i < particlesTextureDropdown_->getNumItems(); ++i)
    {
        if (particlesTextureDropdown_->getItem(i).id == config.particles.textureId)
        {
            particlesTextureDropdown_->setSelectedIndex(i, false);
            break;
        }
    }

    particlesTexRows_->setValue(config.particles.textureRows, false);
    particlesTexCols_->setValue(config.particles.textureCols, false);

    particlesSoft_->setValue(config.particles.softParticles, false);
    particlesSoftDepth_->setValue(config.particles.softDepthSensitivity, false);

    particlesTurbulence_->setValue(config.particles.useTurbulence, false);
    particlesTurbStrength_->setValue(config.particles.turbulenceStrength, false);
    particlesTurbScale_->setValue(config.particles.turbulenceScale, false);
    particlesTurbSpeed_->setValue(config.particles.turbulenceSpeed, false);
}

void ParticlesTab::updateConfiguration(VisualConfiguration& config)
{
    config.particles.enabled = particlesEnabled_->getValue();
    config.particles.emissionRate = static_cast<float>(particlesRate_->getValue());
    config.particles.particleLife = static_cast<float>(particlesLife_->getValue());
    config.particles.particleSize = static_cast<float>(particlesSize_->getValue());

    auto modeId = particlesModeDropdown_->getSelectedId();
    if (modeId == "alongwaveform") config.particles.emissionMode = ParticleEmissionMode::AlongWaveform;
    else if (modeId == "atpeaks") config.particles.emissionMode = ParticleEmissionMode::AtPeaks;
    else if (modeId == "atzerocrossings") config.particles.emissionMode = ParticleEmissionMode::AtZeroCrossings;
    else if (modeId == "continuous") config.particles.emissionMode = ParticleEmissionMode::Continuous;
    else if (modeId == "burst") config.particles.emissionMode = ParticleEmissionMode::Burst;

    auto blendId = particlesBlendDropdown_->getSelectedId();
    if (blendId == "additive") config.particles.blendMode = ParticleBlendMode::Additive;
    else if (blendId == "alpha") config.particles.blendMode = ParticleBlendMode::Alpha;
    else if (blendId == "multiply") config.particles.blendMode = ParticleBlendMode::Multiply;
    else if (blendId == "screen") config.particles.blendMode = ParticleBlendMode::Screen;

    config.particles.gravity = static_cast<float>(particlesGravity_->getValue());
    config.particles.drag = static_cast<float>(particlesDrag_->getValue());
    config.particles.randomness = static_cast<float>(particlesRandomness_->getValue());
    config.particles.velocityScale = static_cast<float>(particlesVelocity_->getValue());

    config.particles.audioReactive = particlesAudioReactive_->getValue();
    config.particles.audioEmissionBoost = static_cast<float>(particlesAudioBoost_->getValue());

    config.particles.textureId = particlesTextureDropdown_->getSelectedId();
    config.particles.textureRows = static_cast<int>(particlesTexRows_->getValue());
    config.particles.textureCols = static_cast<int>(particlesTexCols_->getValue());

    config.particles.softParticles = particlesSoft_->getValue();
    config.particles.softDepthSensitivity = static_cast<float>(particlesSoftDepth_->getValue());

    config.particles.useTurbulence = particlesTurbulence_->getValue();
    config.particles.turbulenceStrength = static_cast<float>(particlesTurbStrength_->getValue());
    config.particles.turbulenceScale = static_cast<float>(particlesTurbScale_->getValue());
    config.particles.turbulenceSpeed = static_cast<float>(particlesTurbSpeed_->getValue());
}

void ParticlesTab::resized()
{
    auto contentWidth = getWidth();
    int rowHeight = CONTROL_HEIGHT + 4;
    int particleY = 0;

    particlesEnabled_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight + 8;

    particlesModeDropdown_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight;

    particlesRate_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight;
    particlesLife_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight;
    particlesSize_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight;

    particlesColorButton_->setBounds(0, particleY, 100, CONTROL_HEIGHT);
    particlesBlendDropdown_->setBounds(110, particleY, contentWidth - 110, CONTROL_HEIGHT);
    particleY += rowHeight + 8;

    particlesGravity_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight;
    particlesDrag_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight;
    particlesRandomness_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight;
    particlesVelocity_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight + 8;

    particlesAudioReactive_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight;
    particlesAudioBoost_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight + 8;

    particlesTextureDropdown_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight;
    particlesTexRows_->setBounds(0, particleY, (contentWidth - 8) / 2, CONTROL_HEIGHT);
    particlesTexCols_->setBounds((contentWidth + 8) / 2, particleY, (contentWidth - 8) / 2, CONTROL_HEIGHT);
    particleY += rowHeight + 8;

    particlesSoft_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight;
    particlesSoftDepth_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight + 8;

    particlesTurbulence_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight;
    particlesTurbStrength_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight;
    particlesTurbScale_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight;
    particlesTurbSpeed_->setBounds(0, particleY, contentWidth, CONTROL_HEIGHT);
    particleY += rowHeight;
}

int ParticlesTab::getPreferredHeight() const
{
    int rowHeight = CONTROL_HEIGHT + 4;
    // 19 rows + gaps
    return 19 * rowHeight + 6 * 8; // Approx
}

}
