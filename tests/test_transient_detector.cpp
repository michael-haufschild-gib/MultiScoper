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
