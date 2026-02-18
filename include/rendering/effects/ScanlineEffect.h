/*
    Oscil - Scanline Effect
    CRT-style scanline overlay with optional phosphor glow
*/

#pragma once

#include "PostProcessEffect.h"
#include "rendering/VisualConfiguration.h"
#include <memory>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Scanline post-processing effect.
 * Creates a CRT monitor aesthetic with horizontal scanlines.
 */
class ScanlineEffect : public PostProcessEffect
{
public:
    ScanlineEffect();
    ~ScanlineEffect() override;

    [[nodiscard]] juce::String getId() const override { return "scanlines"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Scanlines"; }

    bool compile(juce::OpenGLContext& context) override;
    void release(juce::OpenGLContext& context) override;
    [[nodiscard]] bool isCompiled() const override;

    void apply(
        juce::OpenGLContext& context,
        Framebuffer* source,
        Framebuffer* destination,
        FramebufferPool& pool,
        float deltaTime
    ) override;

    void configure(const VisualConfiguration& config) override { settings_ = config.scanlines; }

    /**
     * Configure the scanline effect.
     * Parameters are clamped to valid ranges to prevent undefined behavior.
     */
    void setSettings(const ScanlineSettings& settings)
    {
        settings_ = settings;
        // Validate and clamp parameters to safe ranges
        settings_.intensity = juce::jlimit(0.0f, 1.0f, settings_.intensity);
        settings_.density = juce::jlimit(0.5f, 10.0f, settings_.density);
    }
    [[nodiscard]] const ScanlineSettings& getSettings() const { return settings_; }

private:
    ScanlineSettings settings_;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;

    GLint textureLoc_ = -1;
    GLint intensityLoc_ = -1;
    GLint densityLoc_ = -1;
    GLint widthLoc_ = -1;
    GLint heightLoc_ = -1;
    GLint phosphorGlowLoc_ = -1;

    bool compiled_ = false;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
