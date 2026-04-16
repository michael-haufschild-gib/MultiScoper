/*
    Oscil - DecimatingCaptureBuffer Property-Based Tests (rapidcheck)

    Invariants across quality-preset / source-rate combinations.
    See docs/decisions/011-property-based-dsp-tests.md.
*/

#include "core/DecimatingCaptureBuffer.h"
#include "core/dsp/CaptureQualityConfig.h"
#include "core/dsp/QualityPreset.h"

#include <juce_core/juce_core.h>

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

using namespace oscil;

namespace
{

// Generator: realistic audio source sample rates.
rc::Gen<int> genSourceRate() { return rc::gen::element<int>(22050, 44100, 48000, 88200, 96000, 192000); }

// Generator: any quality preset.
rc::Gen<QualityPreset> genQualityPreset()
{
    return rc::gen::element<QualityPreset>(QualityPreset::Eco, QualityPreset::Standard, QualityPreset::High,
                                           QualityPreset::Ultra);
}

// Generator: any buffer duration.
rc::Gen<BufferDuration> genBufferDuration()
{
    return rc::gen::element<BufferDuration>(BufferDuration::Short, BufferDuration::Medium, BufferDuration::Long,
                                            BufferDuration::VeryLong);
}

} // namespace

// ---------------------------------------------------------------------------
// Property: Decimation ratio matches CaptureQualityConfig::getDecimationRatio
// for every (preset, source-rate) combination. This is the contract that
// configure() must preserve. The reachable ratios include {1, 2, 4, 8} across
// Eco/Standard/High at 44.1k/88.2k; the property asserts exact equality over
// the full generated space, which is a stronger claim than the hand-written
// tests' enumeration.
// ---------------------------------------------------------------------------

RC_GTEST_PROP(DecimatingCaptureBufferProperties, DecimationRatioMatchesConfig, ())
{
    int const sourceRate = *genSourceRate();
    QualityPreset const preset = *genQualityPreset();
    BufferDuration const duration = *genBufferDuration();

    CaptureQualityConfig cfg;
    cfg.qualityPreset = preset;
    cfg.bufferDuration = duration;
    cfg.autoAdjustQuality = false; // isolate the preset's ratio, don't let auto-adjust retune it

    DecimatingCaptureBuffer buffer(cfg, sourceRate);
    int const expectedRatio = cfg.getDecimationRatio(sourceRate);

    RC_ASSERT(buffer.getDecimationRatio() == expectedRatio);
    RC_ASSERT(buffer.getSourceRate() == sourceRate);
    // The stored capture rate is the integer-exact post-decimation rate
    // (srcRate / ratio), not necessarily the preset's nominal rate. E.g.,
    // Standard preset (nominal 22050) at srcRate=192000 stores 24000 because
    // ratio is the truncated 192000/22050 = 8. This integer-exact contract
    // is what the decimator actually produces and is the right invariant.
    RC_ASSERT(buffer.getCaptureRate() == juce::jmax(1, sourceRate / expectedRatio));
    RC_ASSERT(buffer.getDecimationRatio() >= 1);
}

// ---------------------------------------------------------------------------
// Property: configure() is idempotent.
// Calling configure(cfg, rate) twice produces observable state identical to
// calling it once. We compare the visible rate getters and the config
// round-trip. Memory usage is intentionally excluded — filter coefficient
// storage from juce::dsp::FIR may reallocate identically but is not an
// externally-specified invariant.
// ---------------------------------------------------------------------------

RC_GTEST_PROP(DecimatingCaptureBufferProperties, ConfigureIsIdempotent, ())
{
    int const sourceRate = *genSourceRate();
    QualityPreset const preset = *genQualityPreset();
    BufferDuration const duration = *genBufferDuration();

    CaptureQualityConfig cfg;
    cfg.qualityPreset = preset;
    cfg.bufferDuration = duration;
    cfg.autoAdjustQuality = false;

    DecimatingCaptureBuffer buffer;
    buffer.configure(cfg, sourceRate);

    int const ratio1 = buffer.getDecimationRatio();
    int const capture1 = buffer.getCaptureRate();
    int const src1 = buffer.getSourceRate();
    size_t const capacity1 = buffer.getCapacity();
    CaptureQualityConfig const cfg1 = buffer.getConfig();

    buffer.configure(cfg, sourceRate);

    RC_ASSERT(buffer.getDecimationRatio() == ratio1);
    RC_ASSERT(buffer.getCaptureRate() == capture1);
    RC_ASSERT(buffer.getSourceRate() == src1);
    RC_ASSERT(buffer.getCapacity() == capacity1);
    RC_ASSERT(buffer.getConfig() == cfg1);
}
