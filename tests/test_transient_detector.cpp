/*
    Oscil - Transient Detector Tests
*/

#include <gtest/gtest.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#define private public
#include "core/analysis/TransientDetector.h"
#undef private
#pragma clang diagnostic pop

using namespace oscil;

TEST(TransientDetectorTest, IdleToAttackResetsPlateauCounter)
{
    TransientDetector detector;
    constexpr double sampleRate = 44100.0;

    detector.samplesSincePeakIncrease_ = 67.0;
    detector.state_ = TransientDetector::State::Idle;
    detector.fastEnvelope_ = 0.6f;
    detector.slowEnvelope_ = 0.2f;

    const float onsetSample[1] = { 8.0f };
    detector.process(onsetSample, 1, sampleRate);

    ASSERT_EQ(detector.state_, TransientDetector::State::MeasuringAttack);
    EXPECT_EQ(detector.samplesInState_, 0.0);
    EXPECT_EQ(detector.samplesSincePeakIncrease_, 0.0);
}

TEST(TransientDetectorTest, DecayRetriggerResetsPlateauCounter)
{
    TransientDetector detector;
    constexpr double sampleRate = 44100.0;

    detector.state_ = TransientDetector::State::MeasuringDecay;
    detector.fastEnvelope_ = 0.9f;
    detector.slowEnvelope_ = 0.2f;
    detector.transientPeak_ = 1.0f;
    detector.onsetLevel_ = 0.1f;
    detector.samplesInState_ = 12.0;
    detector.samplesSincePeakIncrease_ = 51.0;

    const float retriggerSample[1] = { 10.0f };
    detector.process(retriggerSample, 1, sampleRate);

    ASSERT_EQ(detector.state_, TransientDetector::State::MeasuringAttack);
    EXPECT_EQ(detector.samplesInState_, 0.0);
    EXPECT_EQ(detector.samplesSincePeakIncrease_, 0.0);
}
