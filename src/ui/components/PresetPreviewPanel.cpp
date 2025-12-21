/*
    Oscil - Preset Preview Panel Implementation
*/

#include "ui/components/PresetPreviewPanel.h"
#include <cmath>

namespace oscil
{

PresetPreviewPanel::PresetPreviewPanel(IThemeService& themeService, const juce::String& testId)
    : ThemedComponent(themeService)
{
    setSize(MIN_WIDTH, MIN_HEIGHT);
    waveformData_.resize(WAVEFORM_SAMPLES, 0.0f);

    if (testId.isNotEmpty())
    {
        OSCIL_REGISTER_TEST_ID(testId);
    }

    // Start animation timer
    startTimerHz(60);
}

PresetPreviewPanel::~PresetPreviewPanel()
{
    stopTimer();
}

void PresetPreviewPanel::setConfiguration(const VisualConfiguration& config)
{
    config_ = config;
    configDirty_ = true;
    debounceCounter_ = DEBOUNCE_FRAMES;
}

void PresetPreviewPanel::setAnimating(bool animate)
{
    animating_ = animate;
    if (animate && !isTimerRunning())
        startTimerHz(60);
}

void PresetPreviewPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    auto& theme = getTheme();

    // Background
    g.setColour(theme.backgroundPrimary);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Border
    g.setColour(theme.controlBorder);
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 4.0f, 1.0f);

    // Draw waveform preview
    auto waveformBounds = bounds.reduced(8);
    auto centerY = waveformBounds.getCentreY();
    auto amplitude = waveformBounds.getHeight() * 0.35f;

    // Generate waveform path
    juce::Path wavePath;
    bool pathStarted = false;

    for (int i = 0; i < WAVEFORM_SAMPLES; ++i)
    {
        float x = waveformBounds.getX() + (static_cast<float>(i) / WAVEFORM_SAMPLES) * waveformBounds.getWidth();
        float y = centerY - waveformData_[i] * amplitude;

        if (!pathStarted)
        {
            wavePath.startNewSubPath(x, y);
            pathStarted = true;
        }
        else
        {
            wavePath.lineTo(x, y);
        }
    }

    // Apply visual effects based on configuration

    // Base waveform color
    juce::Colour waveColor = theme.controlActive;

    // Apply bloom effect simulation
    if (config_.bloom.enabled)
    {
        // Draw glow behind the waveform
        float glowIntensity = config_.bloom.intensity * 0.5f;
        for (int pass = 3; pass > 0; --pass)
        {
            g.setColour(waveColor.withAlpha(glowIntensity / static_cast<float>(pass + 1)));
            g.strokePath(wavePath, juce::PathStrokeType(2.0f + pass * 4.0f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    // Apply color grading simulation
    if (config_.colorGrade.enabled)
    {
        float brightness = config_.colorGrade.brightness;
        float saturation = config_.colorGrade.saturation;

        // Adjust waveform color
        float hue, sat, light;
        waveColor.getHSL(hue, sat, light);
        waveColor = juce::Colour::fromHSL(
            hue,
            juce::jlimit(0.0f, 1.0f, sat * saturation),
            juce::jlimit(0.0f, 1.0f, light + brightness * 0.3f),
            1.0f
        );
    }

    // Main waveform stroke
    float strokeWidth = 2.0f;
    if (config_.shaderType == ShaderType::NeonGlow)
        strokeWidth = 3.0f;

    g.setColour(waveColor);
    g.strokePath(wavePath, juce::PathStrokeType(strokeWidth,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Apply scanlines effect
    if (config_.scanlines.enabled)
    {
        // Clamp intensity to valid alpha range (0.0-1.0) to prevent JUCE assertions
        float scanlinesAlpha = juce::jlimit(0.0f, 1.0f, config_.scanlines.intensity * 0.3f);
        g.setColour(theme.backgroundPrimary.withAlpha(scanlinesAlpha));
        float lineSpacing = 4.0f / config_.scanlines.density;
        for (float y = static_cast<float>(waveformBounds.getY()); y < waveformBounds.getBottom(); y += lineSpacing)
        {
            g.drawHorizontalLine(static_cast<int>(y), static_cast<float>(waveformBounds.getX()),
                static_cast<float>(waveformBounds.getRight()));
        }
    }

    // Apply vignette effect
    if (config_.vignette.enabled)
    {
        // Clamp intensity to valid alpha range (0.0-1.0) to prevent JUCE assertions
        float vignetteAlpha = juce::jlimit(0.0f, 1.0f, config_.vignette.intensity);
        juce::ColourGradient vignette(
            juce::Colours::transparentBlack,
            bounds.getCentreX(), bounds.getCentreY(),
            config_.vignette.colour.withAlpha(vignetteAlpha),
            bounds.getX(), bounds.getY(),
            true
        );
        g.setGradientFill(vignette);
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    }

    // Apply film grain simulation
    if (config_.filmGrain.enabled)
    {
        juce::Random rng;
        rng.setSeedRandomly();
        // Clamp intensity to valid alpha range (0.0-1.0) to prevent JUCE assertions
        float grainAlpha = juce::jlimit(0.0f, 1.0f, config_.filmGrain.intensity * 0.3f);
        g.setColour(theme.textPrimary.withAlpha(grainAlpha));
        for (int i = 0; i < static_cast<int>(bounds.getWidth() * bounds.getHeight() * 0.01f * config_.filmGrain.intensity); ++i)
        {
            int x = bounds.getX() + rng.nextInt(bounds.getWidth());
            int y = bounds.getY() + rng.nextInt(bounds.getHeight());
            g.fillRect(x, y, 1, 1);
        }
    }

    // Draw particles preview
    if (config_.particles.enabled)
    {
        juce::Random particleRng(static_cast<int>(animationTime_ * 1000));
        g.setColour(config_.particles.particleColor);

        int particleCount = static_cast<int>(config_.particles.emissionRate * 0.1f);
        for (int i = 0; i < particleCount; ++i)
        {
            float t = static_cast<float>(i) / particleCount;
            int sampleIndex = static_cast<int>(t * WAVEFORM_SAMPLES) % WAVEFORM_SAMPLES;

            float x = waveformBounds.getX() + t * waveformBounds.getWidth();
            float baseY = centerY - waveformData_[sampleIndex] * amplitude;

            // Add some randomness
            float offsetY = (particleRng.nextFloat() - 0.5f) * 20.0f * config_.particles.randomness;
            float y = baseY + offsetY;

            float size = config_.particles.particleSize * 0.5f;
            g.fillEllipse(x - size / 2, y - size / 2, size, size);
        }
    }

    // Shader type indicator text
    g.setColour(theme.textSecondary.withAlpha(0.5f));
    g.setFont(10.0f);
    juce::String shaderName = shaderTypeToId(config_.shaderType);
    g.drawText(shaderName, bounds.reduced(8).removeFromBottom(16),
               juce::Justification::bottomRight, false);
}

void PresetPreviewPanel::resized()
{
    // No child components
}

void PresetPreviewPanel::timerCallback()
{
    // Handle debounced config updates
    if (configDirty_)
    {
        if (--debounceCounter_ <= 0)
        {
            configDirty_ = false;
            updatePreview();
        }
    }

    // Update animation
    if (animating_)
    {
        animationTime_ += 1.0f / 60.0f;
        generateWaveformData();
        repaint();
    }
}

void PresetPreviewPanel::updatePreview()
{
    generateWaveformData();
    repaint();
}

void PresetPreviewPanel::generateWaveformData()
{
    // Generate synthetic waveform based on animation time
    float baseFreq = 440.0f; // 440 Hz
    float sampleRate = 44100.0f;
    float timeOffset = animationTime_;

    for (int i = 0; i < WAVEFORM_SAMPLES; ++i)
    {
        float t = static_cast<float>(i) / WAVEFORM_SAMPLES;

        // Base sine wave
        float value = std::sin(t * juce::MathConstants<float>::twoPi * 3.0f + timeOffset * 2.0f);

        // Add harmonics for more interesting shape
        value += 0.3f * std::sin(t * juce::MathConstants<float>::twoPi * 6.0f + timeOffset * 3.0f);
        value += 0.2f * std::sin(t * juce::MathConstants<float>::twoPi * 9.0f + timeOffset * 4.0f);

        // Normalize
        value /= 1.5f;

        // Apply some distortion if glitch shader
        if (config_.shaderType == ShaderType::DigitalGlitch)
        {
            if (std::fmod(t + timeOffset * 0.1f, 0.2f) < 0.05f)
                value *= 0.5f;
        }

        waveformData_[i] = value;
    }
}

void PresetPreviewPanel::registerTestId()
{
    // Test ID is set in constructor
}

} // namespace oscil
