/*
    Oscil - Scanline Effect
    CRT-style scanline overlay with optional phosphor glow
*/

#pragma once

#include "SingleShaderEffect.h"
#include "rendering/VisualConfiguration.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Scanline post-processing effect.
 * Creates a CRT monitor aesthetic with horizontal scanlines.
 */
class ScanlineEffect : public SingleShaderEffect
{
public:
    /// Create a scanline effect with default settings.
    ScanlineEffect();
    ~ScanlineEffect() override;

    [[nodiscard]] juce::String getId() const override { return "scanlines"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "Scanlines"; }

    void configure(const VisualConfiguration& config) override { settings_ = config.scanlines; }
    void setSettings(const ScanlineSettings& settings) { settings_ = settings; }
    [[nodiscard]] const ScanlineSettings& getSettings() const { return settings_; }

protected:
    [[nodiscard]] const char* getFragmentSource() const override;
    bool resolveUniforms() override;
    void setUniforms(const Framebuffer& source, float deltaTime) override;

private:
    ScanlineSettings settings_;

    GLint intensityLoc_ = -1;
    GLint densityLoc_ = -1;
    GLint widthLoc_ = -1;
    GLint heightLoc_ = -1;
    GLint phosphorGlowLoc_ = -1;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
