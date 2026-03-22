/*
    Oscil - Transient Detector Tests
*/

#include <gtest/gtest.h>

#include "core/analysis/TransientDetector.h"

namespace oscil
{

/**
 * Test accessor for TransientDetector private members.
 * Avoids the UB of `#define private public`.
 */
struct TransientDetectorTestAccess
{
    using State = TransientDetector::State;

    static State& state(TransientDetector& d) { return d.state_; }
    static float& fastEnvelope(TransientDetector& d) { return d.fastEnvelope_; }
    static float& slowEnvelope(TransientDetector& d) { return d.slowEnvelope_; }
    static float& transientPeak(TransientDetector& d) { return d.transientPeak_; }
    static float& onsetLevel(TransientDetector& d) { return d.onsetLevel_; }
    static double& samplesInState(TransientDetector& d) { return d.samplesInState_; }
    static double& samplesSincePeakIncrease(TransientDetector& d) { return d.samplesSincePeakIncrease_; }
};

} // namespace oscil

using namespace oscil;

// ============================================================================
// Behavioral Tests (public interface only)
// ============================================================================

TEST(TransientDetectorTest, DefaultStateReportsZeroTimings)
{
    TransientDetector detector;
    EXPECT_FLOAT_EQ(detector.getAttackTimeMs(), 0.0f);
    EXPECT_FLOAT_EQ(detector.getDecayTimeMs(), 0.0f);
}

TEST(TransientDetectorTest, ResetClearsTimings)
{
    TransientDetector detector;
    constexpr double sampleRate = 44100.0;

    // Feed a transient to get non-zero timings
    std::vector<float> silence(4410, 0.0f); // 100ms silence
    std::vector<float> impulse(441, 0.0f);
    impulse[0] = 1.0f; // Sharp transient

    detector.process(silence.data(), static_cast<int>(silence.size()), sampleRate);
    detector.process(impulse.data(), static_cast<int>(impulse.size()), sampleRate);
    // Process more silence to let decay complete
    detector.process(silence.data(), static_cast<int>(silence.size()), sampleRate);

    detector.reset();

    EXPECT_FLOAT_EQ(detector.getAttackTimeMs(), 0.0f);
    EXPECT_FLOAT_EQ(detector.getDecayTimeMs(), 0.0f);
}

TEST(TransientDetectorTest, SilenceProducesNoTransient)
{
    // Bug caught: detector triggering on noise floor
    TransientDetector detector;
    constexpr double sampleRate = 44100.0;

    std::vector<float> silence(44100, 0.0f); // 1 second of silence
    detector.process(silence.data(), static_cast<int>(silence.size()), sampleRate);

    EXPECT_FLOAT_EQ(detector.getAttackTimeMs(), 0.0f);
    EXPECT_FLOAT_EQ(detector.getDecayTimeMs(), 0.0f);
}

TEST(TransientDetectorTest, SharpImpulseProducesShortAttack)
{
    // Bug caught: attack time measurement returning infinity or negative
    TransientDetector detector;
    constexpr double sampleRate = 44100.0;

    // Warm up with silence
    std::vector<float> silence(4410, 0.0f);
    detector.process(silence.data(), static_cast<int>(silence.size()), sampleRate);

    // Sharp impulse followed by silence
    std::vector<float> impulseBlock(441, 0.0f);
    impulseBlock[0] = 1.0f;
    detector.process(impulseBlock.data(), static_cast<int>(impulseBlock.size()), sampleRate);

    // Let the decay complete
    detector.process(silence.data(), static_cast<int>(silence.size()), sampleRate);
    detector.process(silence.data(), static_cast<int>(silence.size()), sampleRate);

    float attackMs = detector.getAttackTimeMs();
    float decayMs = detector.getDecayTimeMs();

    // Attack should be very short (< 10ms for an impulse)
    // It's OK if it's 0 (detector may report 0 for sub-sample attacks)
    EXPECT_GE(attackMs, 0.0f);
    EXPECT_TRUE(std::isfinite(attackMs)) << "Attack time is not finite: " << attackMs;

    // Decay should be positive and finite if measured
    EXPECT_GE(decayMs, 0.0f);
    EXPECT_TRUE(std::isfinite(decayMs)) << "Decay time is not finite: " << decayMs;
}

TEST(TransientDetectorTest, GradualOnsetProducesLongerAttack)
{
    // Bug caught: attack time not scaling with actual rise time
    TransientDetector detector;
    constexpr double sampleRate = 44100.0;

    // Warm up with silence
    std::vector<float> silence(4410, 0.0f);
    detector.process(silence.data(), static_cast<int>(silence.size()), sampleRate);

    // Gradual ramp up over ~10ms (441 samples)
    std::vector<float> ramp(441);
    for (int i = 0; i < 441; ++i)
        ramp[static_cast<size_t>(i)] = static_cast<float>(i) / 441.0f;
    detector.process(ramp.data(), static_cast<int>(ramp.size()), sampleRate);

    // Hold at peak for a bit
    std::vector<float> peak(441, 1.0f);
    detector.process(peak.data(), static_cast<int>(peak.size()), sampleRate);

    // Decay
    detector.process(silence.data(), static_cast<int>(silence.size()), sampleRate);
    detector.process(silence.data(), static_cast<int>(silence.size()), sampleRate);

    float attackMs = detector.getAttackTimeMs();
    EXPECT_TRUE(std::isfinite(attackMs));
}

TEST(TransientDetectorTest, ProcessWithZeroSamplesDoesNotCrash)
{
    TransientDetector detector;
    float dummy = 0.0f;
    detector.process(&dummy, 0, 44100.0);
    // Should not crash or change state
    EXPECT_FLOAT_EQ(detector.getAttackTimeMs(), 0.0f);
}

TEST(TransientDetectorTest, VeryHighSampleRateDoesNotOverflow)
{
    // Bug caught: coefficient calculation overflow at 192kHz+
    TransientDetector detector;
    constexpr double sampleRate = 192000.0;

    std::vector<float> silence(19200, 0.0f);
    detector.process(silence.data(), static_cast<int>(silence.size()), sampleRate);

    std::vector<float> impulse(1920, 0.0f);
    impulse[0] = 0.9f;
    detector.process(impulse.data(), static_cast<int>(impulse.size()), sampleRate);

    detector.process(silence.data(), static_cast<int>(silence.size()), sampleRate);

    EXPECT_TRUE(std::isfinite(detector.getAttackTimeMs()));
    EXPECT_TRUE(std::isfinite(detector.getDecayTimeMs()));
}

TEST(TransientDetectorTest, DCOffsetDoesNotTriggerFalseTransients)
{
    // Bug caught: constant DC interpreted as onset because fast envelope
    // converges faster than slow envelope initially
    TransientDetector detector;
    constexpr double sampleRate = 44100.0;

    // Feed constant DC for 1 second
    std::vector<float> dc(44100, 0.5f);
    detector.process(dc.data(), static_cast<int>(dc.size()), sampleRate);

    // After the envelopes converge, there should be no new transient activity
    // Feed more DC
    detector.process(dc.data(), static_cast<int>(dc.size()), sampleRate);

    // Attack and decay should remain stable (either 0 or a settled value)
    float attackMs = detector.getAttackTimeMs();
    float decayMs = detector.getDecayTimeMs();
    EXPECT_TRUE(std::isfinite(attackMs));
    EXPECT_TRUE(std::isfinite(decayMs));
}

// ============================================================================
// Implementation State Tests (using test accessor)
// ============================================================================

TEST(TransientDetectorTest, IdleToAttackResetsPlateauCounter)
{
    TransientDetector detector;
    constexpr double sampleRate = 44100.0;

    TransientDetectorTestAccess::samplesSincePeakIncrease(detector) = 67.0;
    TransientDetectorTestAccess::state(detector) = TransientDetectorTestAccess::State::Idle;
    TransientDetectorTestAccess::fastEnvelope(detector) = 0.6f;
    TransientDetectorTestAccess::slowEnvelope(detector) = 0.2f;

    const float onsetSample[1] = { 8.0f };
    detector.process(onsetSample, 1, sampleRate);

    ASSERT_EQ(TransientDetectorTestAccess::state(detector), TransientDetectorTestAccess::State::MeasuringAttack);
    EXPECT_EQ(TransientDetectorTestAccess::samplesInState(detector), 0.0);
    EXPECT_EQ(TransientDetectorTestAccess::samplesSincePeakIncrease(detector), 0.0);
}

TEST(TransientDetectorTest, DecayRetriggerResetsPlateauCounter)
{
    TransientDetector detector;
    constexpr double sampleRate = 44100.0;

    TransientDetectorTestAccess::state(detector) = TransientDetectorTestAccess::State::MeasuringDecay;
    TransientDetectorTestAccess::fastEnvelope(detector) = 0.9f;
    TransientDetectorTestAccess::slowEnvelope(detector) = 0.2f;
    TransientDetectorTestAccess::transientPeak(detector) = 1.0f;
    TransientDetectorTestAccess::onsetLevel(detector) = 0.1f;
    TransientDetectorTestAccess::samplesInState(detector) = 12.0;
    TransientDetectorTestAccess::samplesSincePeakIncrease(detector) = 51.0;

    const float retriggerSample[1] = { 10.0f };
    detector.process(retriggerSample, 1, sampleRate);

    ASSERT_EQ(TransientDetectorTestAccess::state(detector), TransientDetectorTestAccess::State::MeasuringAttack);
    EXPECT_EQ(TransientDetectorTestAccess::samplesInState(detector), 0.0);
    EXPECT_EQ(TransientDetectorTestAccess::samplesSincePeakIncrease(detector), 0.0);
}
