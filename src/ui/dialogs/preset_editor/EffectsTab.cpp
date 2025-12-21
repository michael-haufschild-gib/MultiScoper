#include "ui/dialogs/preset_editor/EffectsTab.h"

namespace oscil
{

EffectsTab::EffectsTab(IThemeService& themeService)
    : PresetEditorTab(themeService)
{
    setupComponents();
}

void EffectsTab::setupComponents()
{
    effectsAccordion_ = std::make_unique<OscilAccordion>(getThemeService());
    effectsAccordion_->setAllowMultiExpand(true);
    addAndMakeVisible(*effectsAccordion_);

    // Bloom effect
    bloomContainer_ = std::make_unique<juce::Component>();
    bloomEnabled_ = createToggle("", "preset_effect_bloom_enabled");
    bloomIntensity_ = createSlider("preset_bloom_intensity", 0.0, 2.0);
    bloomThreshold_ = createSlider("preset_bloom_threshold", 0.0, 1.0);
    bloomIterations_ = createSlider("preset_bloom_iterations", 2, 8, 1);
    bloomDownsample_ = createSlider("preset_bloom_downsample", 2, 8, 1);
    bloomSpread_ = createSlider("preset_bloom_spread", 0.5, 3.0);
    bloomSoftKnee_ = createSlider("preset_bloom_soft_knee", 0.0, 1.0);
    bloomContainer_->addAndMakeVisible(*bloomIntensity_);
    bloomContainer_->addAndMakeVisible(*bloomThreshold_);
    bloomContainer_->addAndMakeVisible(*bloomIterations_);
    bloomContainer_->addAndMakeVisible(*bloomDownsample_);
    bloomContainer_->addAndMakeVisible(*bloomSpread_);
    bloomContainer_->addAndMakeVisible(*bloomSoftKnee_);
    auto* bloomSection = effectsAccordion_->addSection("Bloom", bloomContainer_.get());
    bloomSection->setHeaderComponent(bloomEnabled_.get(), 40);

    // Radial Blur
    radialBlurContainer_ = std::make_unique<juce::Component>();
    radialBlurEnabled_ = createToggle("", "preset_effect_radial_blur_enabled");
    radialBlurAmount_ = createSlider("preset_radial_blur_amount", 0.0, 0.5);
    radialBlurGlow_ = createSlider("preset_radial_blur_glow", 0.0, 2.0);
    radialBlurSamples_ = createSlider("preset_radial_blur_samples", 2, 8, 1);
    radialBlurContainer_->addAndMakeVisible(*radialBlurAmount_);
    radialBlurContainer_->addAndMakeVisible(*radialBlurGlow_);
    radialBlurContainer_->addAndMakeVisible(*radialBlurSamples_);
    auto* radialSection = effectsAccordion_->addSection("Radial Blur", radialBlurContainer_.get());
    radialSection->setHeaderComponent(radialBlurEnabled_.get(), 40);

    // Trails
    trailsContainer_ = std::make_unique<juce::Component>();
    trailsEnabled_ = createToggle("", "preset_effect_trails_enabled");
    trailsDecay_ = createSlider("preset_trails_decay", 0.01, 0.5);
    trailsOpacity_ = createSlider("preset_trails_opacity", 0.0, 1.0);
    trailsContainer_->addAndMakeVisible(*trailsDecay_);
    trailsContainer_->addAndMakeVisible(*trailsOpacity_);
    auto* trailsSection = effectsAccordion_->addSection("Trails", trailsContainer_.get());
    trailsSection->setHeaderComponent(trailsEnabled_.get(), 40);

    // Color Grade
    colorGradeContainer_ = std::make_unique<juce::Component>();
    colorGradeEnabled_ = createToggle("", "preset_effect_color_grade_enabled");
    colorBrightness_ = createSlider("preset_color_brightness", -1.0, 1.0);
    colorContrast_ = createSlider("preset_color_contrast", 0.5, 2.0);
    colorSaturation_ = createSlider("preset_color_saturation", 0.0, 2.0);
    colorTemperature_ = createSlider("preset_color_temperature", -1.0, 1.0);
    colorTint_ = createSlider("preset_color_tint", -1.0, 1.0);
    colorShadowsButton_ = std::make_unique<OscilButton>(getThemeService(), "Shadows", "preset_color_shadows");
    colorHighlightsButton_ = std::make_unique<OscilButton>(getThemeService(), "Highlights", "preset_color_highlights");
    colorGradeContainer_->addAndMakeVisible(*colorBrightness_);
    colorGradeContainer_->addAndMakeVisible(*colorContrast_);
    colorGradeContainer_->addAndMakeVisible(*colorSaturation_);
    colorGradeContainer_->addAndMakeVisible(*colorTemperature_);
    colorGradeContainer_->addAndMakeVisible(*colorTint_);
    colorGradeContainer_->addAndMakeVisible(*colorShadowsButton_);
    colorGradeContainer_->addAndMakeVisible(*colorHighlightsButton_);
    auto* colorSection = effectsAccordion_->addSection("Color Grade", colorGradeContainer_.get());
    colorSection->setHeaderComponent(colorGradeEnabled_.get(), 40);

    // Vignette
    vignetteContainer_ = std::make_unique<juce::Component>();
    vignetteEnabled_ = createToggle("", "preset_effect_vignette_enabled");
    vignetteIntensity_ = createSlider("preset_vignette_intensity", 0.0, 1.0);
    vignetteSoftness_ = createSlider("preset_vignette_softness", 0.0, 1.0);
    vignetteColorButton_ = std::make_unique<OscilButton>(getThemeService(), "Color", "preset_vignette_color");
    vignetteContainer_->addAndMakeVisible(*vignetteIntensity_);
    vignetteContainer_->addAndMakeVisible(*vignetteSoftness_);
    vignetteContainer_->addAndMakeVisible(*vignetteColorButton_);
    auto* vignetteSection = effectsAccordion_->addSection("Vignette", vignetteContainer_.get());
    vignetteSection->setHeaderComponent(vignetteEnabled_.get(), 40);

    // Film Grain
    filmGrainContainer_ = std::make_unique<juce::Component>();
    filmGrainEnabled_ = createToggle("", "preset_effect_film_grain_enabled");
    grainIntensity_ = createSlider("preset_grain_intensity", 0.0, 0.5);
    grainSpeed_ = createSlider("preset_grain_speed", 1.0, 60.0);
    filmGrainContainer_->addAndMakeVisible(*grainIntensity_);
    filmGrainContainer_->addAndMakeVisible(*grainSpeed_);
    auto* grainSection = effectsAccordion_->addSection("Film Grain", filmGrainContainer_.get());
    grainSection->setHeaderComponent(filmGrainEnabled_.get(), 40);

    // Chromatic Aberration
    chromaticContainer_ = std::make_unique<juce::Component>();
    chromaticEnabled_ = createToggle("", "preset_effect_chromatic_aberration_enabled");
    chromaticIntensity_ = createSlider("preset_chromatic_intensity", 0.0, 0.02);
    chromaticContainer_->addAndMakeVisible(*chromaticIntensity_);
    auto* chromaticSection = effectsAccordion_->addSection("Chromatic Aberration", chromaticContainer_.get());
    chromaticSection->setHeaderComponent(chromaticEnabled_.get(), 40);

    // Scanlines
    scanlinesContainer_ = std::make_unique<juce::Component>();
    scanlinesEnabled_ = createToggle("", "preset_effect_scanlines_enabled");
    scanlinesIntensity_ = createSlider("preset_scanlines_intensity", 0.0, 1.0);
    scanlinesDensity_ = createSlider("preset_scanlines_density", 0.5, 4.0);
    scanlinesPhosphor_ = createToggle("Phosphor Glow", "preset_scanlines_phosphor");
    scanlinesContainer_->addAndMakeVisible(*scanlinesIntensity_);
    scanlinesContainer_->addAndMakeVisible(*scanlinesDensity_);
    scanlinesContainer_->addAndMakeVisible(*scanlinesPhosphor_);
    auto* scanlinesSection = effectsAccordion_->addSection("Scanlines", scanlinesContainer_.get());
    scanlinesSection->setHeaderComponent(scanlinesEnabled_.get(), 40);

    // Distortion
    distortionContainer_ = std::make_unique<juce::Component>();
    distortionEnabled_ = createToggle("", "preset_effect_distortion_enabled");
    distortionIntensity_ = createSlider("preset_distortion_intensity", 0.0, 0.5);
    distortionFrequency_ = createSlider("preset_distortion_frequency", 1.0, 20.0);
    distortionSpeed_ = createSlider("preset_distortion_speed", 0.1, 5.0);
    distortionContainer_->addAndMakeVisible(*distortionIntensity_);
    distortionContainer_->addAndMakeVisible(*distortionFrequency_);
    distortionContainer_->addAndMakeVisible(*distortionSpeed_);
    auto* distortionSection = effectsAccordion_->addSection("Distortion", distortionContainer_.get());
    distortionSection->setHeaderComponent(distortionEnabled_.get(), 40);

    // Glitch
    glitchContainer_ = std::make_unique<juce::Component>();
    glitchEnabled_ = createToggle("", "preset_effect_glitch_enabled");
    glitchIntensity_ = createSlider("preset_glitch_intensity", 0.0, 1.0);
    glitchBlockSize_ = createSlider("preset_glitch_block_size", 0.01, 0.2);
    glitchLineShift_ = createSlider("preset_glitch_line_shift", 0.0, 0.1);
    glitchColorSep_ = createSlider("preset_glitch_color_sep", 0.0, 0.05);
    glitchFlicker_ = createSlider("preset_glitch_flicker", 1.0, 30.0);
    glitchContainer_->addAndMakeVisible(*glitchIntensity_);
    glitchContainer_->addAndMakeVisible(*glitchBlockSize_);
    glitchContainer_->addAndMakeVisible(*glitchLineShift_);
    glitchContainer_->addAndMakeVisible(*glitchColorSep_);
    glitchContainer_->addAndMakeVisible(*glitchFlicker_);
    auto* glitchSection = effectsAccordion_->addSection("Glitch", glitchContainer_.get());
    glitchSection->setHeaderComponent(glitchEnabled_.get(), 40);

    // Tilt Shift
    tiltShiftContainer_ = std::make_unique<juce::Component>();
    tiltShiftEnabled_ = createToggle("", "preset_effect_tilt_shift_enabled");
    tiltPosition_ = createSlider("preset_tilt_position", 0.0, 1.0);
    tiltRange_ = createSlider("preset_tilt_range", 0.0, 1.0);
    tiltBlur_ = createSlider("preset_tilt_blur", 0.0, 10.0);
    tiltIterations_ = createSlider("preset_tilt_iterations", 1, 8, 1);
    tiltShiftContainer_->addAndMakeVisible(*tiltPosition_);
    tiltShiftContainer_->addAndMakeVisible(*tiltRange_);
    tiltShiftContainer_->addAndMakeVisible(*tiltBlur_);
    tiltShiftContainer_->addAndMakeVisible(*tiltIterations_);
    auto* tiltSection = effectsAccordion_->addSection("Tilt Shift", tiltShiftContainer_.get());
    tiltSection->setHeaderComponent(tiltShiftEnabled_.get(), 40);
}

std::unique_ptr<OscilSlider> EffectsTab::createSlider(const juce::String& testId, double min, double max, double step)
{
    auto slider = std::make_unique<OscilSlider>(getThemeService(), testId);
    slider->setRange(min, max);
    slider->setStep(step);
    slider->onValueChanged = [this](double) {
        notifyChanged();
    };
    return slider;
}

std::unique_ptr<OscilToggle> EffectsTab::createToggle(const juce::String& label, const juce::String& testId)
{
    auto toggle = std::make_unique<OscilToggle>(getThemeService(), label, testId);
    toggle->onValueChanged = [this](bool) {
        notifyChanged();
    };
    return toggle;
}

void EffectsTab::setConfiguration(const VisualConfiguration& config)
{
    // Bloom
    bloomEnabled_->setValue(config.bloom.enabled, false);
    bloomIntensity_->setValue(config.bloom.intensity, false);
    bloomThreshold_->setValue(config.bloom.threshold, false);
    bloomIterations_->setValue(config.bloom.iterations, false);
    bloomDownsample_->setValue(config.bloom.downsampleSteps, false);
    bloomSpread_->setValue(config.bloom.spread, false);
    bloomSoftKnee_->setValue(config.bloom.softKnee, false);

    // Radial Blur
    radialBlurEnabled_->setValue(config.radialBlur.enabled, false);
    radialBlurAmount_->setValue(config.radialBlur.amount, false);
    radialBlurGlow_->setValue(config.radialBlur.glow, false);
    radialBlurSamples_->setValue(config.radialBlur.samples, false);

    // Trails
    trailsEnabled_->setValue(config.trails.enabled, false);
    trailsDecay_->setValue(config.trails.decay, false);
    trailsOpacity_->setValue(config.trails.opacity, false);

    // Color Grade
    colorGradeEnabled_->setValue(config.colorGrade.enabled, false);
    colorBrightness_->setValue(config.colorGrade.brightness, false);
    colorContrast_->setValue(config.colorGrade.contrast, false);
    colorSaturation_->setValue(config.colorGrade.saturation, false);
    colorTemperature_->setValue(config.colorGrade.temperature, false);
    colorTint_->setValue(config.colorGrade.tint, false);

    // Vignette
    vignetteEnabled_->setValue(config.vignette.enabled, false);
    vignetteIntensity_->setValue(config.vignette.intensity, false);
    vignetteSoftness_->setValue(config.vignette.softness, false);

    // Film Grain
    filmGrainEnabled_->setValue(config.filmGrain.enabled, false);
    grainIntensity_->setValue(config.filmGrain.intensity, false);
    grainSpeed_->setValue(config.filmGrain.speed, false);

    // Chromatic Aberration
    chromaticEnabled_->setValue(config.chromaticAberration.enabled, false);
    chromaticIntensity_->setValue(config.chromaticAberration.intensity, false);

    // Scanlines
    scanlinesEnabled_->setValue(config.scanlines.enabled, false);
    scanlinesIntensity_->setValue(config.scanlines.intensity, false);
    scanlinesDensity_->setValue(config.scanlines.density, false);
    scanlinesPhosphor_->setValue(config.scanlines.phosphorGlow, false);

    // Distortion
    distortionEnabled_->setValue(config.distortion.enabled, false);
    distortionIntensity_->setValue(config.distortion.intensity, false);
    distortionFrequency_->setValue(config.distortion.frequency, false);
    distortionSpeed_->setValue(config.distortion.speed, false);

    // Glitch
    glitchEnabled_->setValue(config.glitch.enabled, false);
    glitchIntensity_->setValue(config.glitch.intensity, false);
    glitchBlockSize_->setValue(config.glitch.blockSize, false);
    glitchLineShift_->setValue(config.glitch.lineShift, false);
    glitchColorSep_->setValue(config.glitch.colorSeparation, false);
    glitchFlicker_->setValue(config.glitch.flickerRate, false);

    // Tilt Shift
    tiltShiftEnabled_->setValue(config.tiltShift.enabled, false);
    tiltPosition_->setValue(config.tiltShift.position, false);
    tiltRange_->setValue(config.tiltShift.range, false);
    tiltBlur_->setValue(config.tiltShift.blurRadius, false);
    tiltIterations_->setValue(config.tiltShift.iterations, false);
}

void EffectsTab::updateConfiguration(VisualConfiguration& config)
{
    // Bloom
    config.bloom.enabled = bloomEnabled_->getValue();
    config.bloom.intensity = static_cast<float>(bloomIntensity_->getValue());
    config.bloom.threshold = static_cast<float>(bloomThreshold_->getValue());
    config.bloom.iterations = static_cast<int>(bloomIterations_->getValue());
    config.bloom.downsampleSteps = static_cast<int>(bloomDownsample_->getValue());
    config.bloom.spread = static_cast<float>(bloomSpread_->getValue());
    config.bloom.softKnee = static_cast<float>(bloomSoftKnee_->getValue());

    // Radial Blur
    config.radialBlur.enabled = radialBlurEnabled_->getValue();
    config.radialBlur.amount = static_cast<float>(radialBlurAmount_->getValue());
    config.radialBlur.glow = static_cast<float>(radialBlurGlow_->getValue());
    config.radialBlur.samples = static_cast<int>(radialBlurSamples_->getValue());

    // Trails
    config.trails.enabled = trailsEnabled_->getValue();
    config.trails.decay = static_cast<float>(trailsDecay_->getValue());
    config.trails.opacity = static_cast<float>(trailsOpacity_->getValue());

    // Color Grade
    config.colorGrade.enabled = colorGradeEnabled_->getValue();
    config.colorGrade.brightness = static_cast<float>(colorBrightness_->getValue());
    config.colorGrade.contrast = static_cast<float>(colorContrast_->getValue());
    config.colorGrade.saturation = static_cast<float>(colorSaturation_->getValue());
    config.colorGrade.temperature = static_cast<float>(colorTemperature_->getValue());
    config.colorGrade.tint = static_cast<float>(colorTint_->getValue());

    // Vignette
    config.vignette.enabled = vignetteEnabled_->getValue();
    config.vignette.intensity = static_cast<float>(vignetteIntensity_->getValue());
    config.vignette.softness = static_cast<float>(vignetteSoftness_->getValue());

    // Film Grain
    config.filmGrain.enabled = filmGrainEnabled_->getValue();
    config.filmGrain.intensity = static_cast<float>(grainIntensity_->getValue());
    config.filmGrain.speed = static_cast<float>(grainSpeed_->getValue());

    // Chromatic Aberration
    config.chromaticAberration.enabled = chromaticEnabled_->getValue();
    config.chromaticAberration.intensity = static_cast<float>(chromaticIntensity_->getValue());

    // Scanlines
    config.scanlines.enabled = scanlinesEnabled_->getValue();
    config.scanlines.intensity = static_cast<float>(scanlinesIntensity_->getValue());
    config.scanlines.density = static_cast<float>(scanlinesDensity_->getValue());
    config.scanlines.phosphorGlow = scanlinesPhosphor_->getValue();

    // Distortion
    config.distortion.enabled = distortionEnabled_->getValue();
    config.distortion.intensity = static_cast<float>(distortionIntensity_->getValue());
    config.distortion.frequency = static_cast<float>(distortionFrequency_->getValue());
    config.distortion.speed = static_cast<float>(distortionSpeed_->getValue());

    // Glitch
    config.glitch.enabled = glitchEnabled_->getValue();
    config.glitch.intensity = static_cast<float>(glitchIntensity_->getValue());
    config.glitch.blockSize = static_cast<float>(glitchBlockSize_->getValue());
    config.glitch.lineShift = static_cast<float>(glitchLineShift_->getValue());
    config.glitch.colorSeparation = static_cast<float>(glitchColorSep_->getValue());
    config.glitch.flickerRate = static_cast<float>(glitchFlicker_->getValue());

    // Tilt Shift
    config.tiltShift.enabled = tiltShiftEnabled_->getValue();
    config.tiltShift.position = static_cast<float>(tiltPosition_->getValue());
    config.tiltShift.range = static_cast<float>(tiltRange_->getValue());
    config.tiltShift.blurRadius = static_cast<float>(tiltBlur_->getValue());
    config.tiltShift.iterations = static_cast<int>(tiltIterations_->getValue());
}

void EffectsTab::resized()
{
    auto bounds = getLocalBounds();
    effectsAccordion_->setBounds(bounds);
}

int EffectsTab::getPreferredHeight() const
{
    return effectsAccordion_->getPreferredHeight();
}

}
