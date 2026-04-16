/*
    Oscil - SingleShaderEffect subclass tests

    Covers the non-GL surface of each effect: id/displayName stability
    (preset persistence relies on these strings), settings round-trip via
    setSettings/getSettings, configure() pulling from VisualConfiguration,
    and base PostProcessEffect intensity/enabled plumbing. GL-dependent
    paths (compile/apply) require a live context and are out of scope.
*/

#include "rendering/VisualConfiguration.h"
#include "rendering/effects/BloomEffect.h"
#include "rendering/effects/ChromaticAberrationEffect.h"
#include "rendering/effects/ColorGradeEffect.h"
#include "rendering/effects/FilmGrainEffect.h"
#include "rendering/effects/RadialBlurEffect.h"
#include "rendering/effects/ScanlineEffect.h"
#include "rendering/effects/TiltShiftEffect.h"
#include "rendering/effects/VignetteEffect.h"

#include <gtest/gtest.h>

#if OSCIL_ENABLE_OPENGL

using namespace oscil;

// -----------------------------------------------------------------------------
// Id / display name stability — preset files reference these by string.
// -----------------------------------------------------------------------------

TEST(SingleShaderEffectIds, EffectIdsAreStableStrings)
{
    EXPECT_EQ(VignetteEffect().getId(), juce::String("vignette"));
    EXPECT_EQ(FilmGrainEffect().getId(), juce::String("film_grain"));
    EXPECT_EQ(BloomEffect().getId(), juce::String("bloom"));
    EXPECT_EQ(ColorGradeEffect().getId(), juce::String("color_grade"));
    EXPECT_EQ(ChromaticAberrationEffect().getId(), juce::String("chromatic_aberration"));
    EXPECT_EQ(ScanlineEffect().getId(), juce::String("scanlines"));
    EXPECT_EQ(RadialBlurEffect().getId(), juce::String("radial_blur"));
    EXPECT_EQ(TiltShiftEffect().getId(), juce::String("tilt_shift"));
}

TEST(SingleShaderEffectIds, EffectDisplayNamesAreNonEmpty)
{
    EXPECT_FALSE(VignetteEffect().getDisplayName().isEmpty());
    EXPECT_FALSE(FilmGrainEffect().getDisplayName().isEmpty());
    EXPECT_FALSE(BloomEffect().getDisplayName().isEmpty());
    EXPECT_FALSE(ColorGradeEffect().getDisplayName().isEmpty());
    EXPECT_FALSE(ChromaticAberrationEffect().getDisplayName().isEmpty());
    EXPECT_FALSE(ScanlineEffect().getDisplayName().isEmpty());
    EXPECT_FALSE(RadialBlurEffect().getDisplayName().isEmpty());
    EXPECT_FALSE(TiltShiftEffect().getDisplayName().isEmpty());
}

// -----------------------------------------------------------------------------
// PostProcessEffect base contract — intensity clamping, enabled flag, and
// the compileFailedPermanently latch.
// -----------------------------------------------------------------------------

TEST(PostProcessEffectBase, IntensityClampsToUnitRange)
{
    VignetteEffect effect;

    effect.setIntensity(-1.0f);
    EXPECT_FLOAT_EQ(effect.getIntensity(), 0.0f);

    effect.setIntensity(2.5f);
    EXPECT_FLOAT_EQ(effect.getIntensity(), 1.0f);

    effect.setIntensity(0.5f);
    EXPECT_FLOAT_EQ(effect.getIntensity(), 0.5f);
}

TEST(PostProcessEffectBase, EnabledFlagRoundTrips)
{
    FilmGrainEffect effect;
    EXPECT_TRUE(effect.isEnabled()) << "Effects start enabled by default";

    effect.setEnabled(false);
    EXPECT_FALSE(effect.isEnabled());

    effect.setEnabled(true);
    EXPECT_TRUE(effect.isEnabled());
}

TEST(PostProcessEffectBase, CompileFailedPermanentlyStartsFalse)
{
    // Fresh instances must be eligible for a compile attempt. The latch is
    // set only by a failed compile() call.
    BloomEffect bloom;
    FilmGrainEffect grain;
    TiltShiftEffect tilt;

    EXPECT_FALSE(bloom.hasCompileFailedPermanently());
    EXPECT_FALSE(grain.hasCompileFailedPermanently());
    EXPECT_FALSE(tilt.hasCompileFailedPermanently());
}

// -----------------------------------------------------------------------------
// Settings round-trip — set, then verify getter returns the same struct.
// -----------------------------------------------------------------------------

TEST(SingleShaderEffectSettings, FilmGrainSettingsRoundTrip)
{
    FilmGrainEffect effect;

    FilmGrainSettings settings;
    settings.enabled = true;
    settings.intensity = 0.4f;
    settings.speed = 48.0f;

    effect.setSettings(settings);
    const auto& got = effect.getSettings();

    EXPECT_TRUE(got.enabled);
    EXPECT_FLOAT_EQ(got.intensity, 0.4f);
    EXPECT_FLOAT_EQ(got.speed, 48.0f);
}

TEST(SingleShaderEffectSettings, VignetteSettingsRoundTrip)
{
    VignetteEffect effect;

    VignetteSettings settings;
    settings.enabled = true;
    settings.intensity = 0.8f;
    settings.softness = 0.3f;
    settings.colour = juce::Colour(0xff112233);

    effect.setSettings(settings);
    const auto& got = effect.getSettings();

    EXPECT_TRUE(got.enabled);
    EXPECT_FLOAT_EQ(got.intensity, 0.8f);
    EXPECT_FLOAT_EQ(got.softness, 0.3f);
    EXPECT_EQ(got.colour, juce::Colour(0xff112233));
}

// -----------------------------------------------------------------------------
// configure() must copy the relevant substruct from VisualConfiguration.
// This is what EffectPipeline uses to apply per-preset settings.
// -----------------------------------------------------------------------------

TEST(SingleShaderEffectConfigure, FilmGrainConfigureCopiesFromVisualConfiguration)
{
    VisualConfiguration config;
    config.filmGrain.enabled = true;
    config.filmGrain.intensity = 0.25f;
    config.filmGrain.speed = 30.0f;

    FilmGrainEffect effect;
    effect.configure(config);

    EXPECT_TRUE(effect.getSettings().enabled);
    EXPECT_FLOAT_EQ(effect.getSettings().intensity, 0.25f);
    EXPECT_FLOAT_EQ(effect.getSettings().speed, 30.0f);
}

TEST(SingleShaderEffectConfigure, VignetteConfigureCopiesFromVisualConfiguration)
{
    VisualConfiguration config;
    config.vignette.enabled = true;
    config.vignette.intensity = 0.6f;
    config.vignette.softness = 0.2f;
    config.vignette.colour = juce::Colour(0xff445566);

    VignetteEffect effect;
    effect.configure(config);

    EXPECT_TRUE(effect.getSettings().enabled);
    EXPECT_FLOAT_EQ(effect.getSettings().intensity, 0.6f);
    EXPECT_FLOAT_EQ(effect.getSettings().softness, 0.2f);
    EXPECT_EQ(effect.getSettings().colour, juce::Colour(0xff445566));
}

TEST(SingleShaderEffectConfigure, ConfigureIsIdempotentAcrossRepeatedCalls)
{
    FilmGrainEffect effect;

    VisualConfiguration configA;
    configA.filmGrain.intensity = 0.1f;

    VisualConfiguration configB;
    configB.filmGrain.intensity = 0.45f;

    effect.configure(configA);
    EXPECT_FLOAT_EQ(effect.getSettings().intensity, 0.1f);

    effect.configure(configB);
    EXPECT_FLOAT_EQ(effect.getSettings().intensity, 0.45f);

    effect.configure(configA);
    EXPECT_FLOAT_EQ(effect.getSettings().intensity, 0.1f) << "configure must fully overwrite prior state, not merge";
}

#endif // OSCIL_ENABLE_OPENGL
