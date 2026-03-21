/*
    Oscil - Transient Detector Implementation
    Uses dual envelope follower for robust transient detection
*/

#include "core/analysis/TransientDetector.h"
#include <cmath>
#include <algorithm>

namespace oscil
{

TransientDetector::TransientDetector()
{
    reset();
}

void TransientDetector::reset()
{
    state_ = State::Idle;
    fastEnvelope_ = 0.0f;
    slowEnvelope_ = 0.0f;
    transientPeak_ = 0.0f;
    onsetLevel_ = 0.0f;
    samplesInState_ = 0;
    samplesSincePeakIncrease_ = 0;
    attackTimeMs_.store(0.0f, std::memory_order_relaxed);
    decayTimeMs_.store(0.0f, std::memory_order_relaxed);
}

void TransientDetector::updateCoefficients(double sampleRate)
{
    if (sampleRate <= 0.0 || std::abs(sampleRate - lastSampleRate_) < 0.01)
        return;

    lastSampleRate_ = sampleRate;

    // Convert time constants to coefficients: coef = exp(-1 / (time * sampleRate))
    // For time in ms: coef = exp(-1000 / (timeMs * sampleRate))
    auto msToCoef = [sampleRate](float timeMs) -> float {
        if (timeMs <= 0.0f) return 0.0f;
        return std::exp(-1000.0f / (timeMs * static_cast<float>(sampleRate)));
    };

    fastAttackCoef_ = msToCoef(FAST_ATTACK_MS);
    fastReleaseCoef_ = msToCoef(FAST_RELEASE_MS);
    slowAttackCoef_ = msToCoef(SLOW_ATTACK_MS);
    slowReleaseCoef_ = msToCoef(SLOW_RELEASE_MS);
}

void TransientDetector::updateEnvelopes(float absSample)
{
    if (absSample > fastEnvelope_)
        fastEnvelope_ = fastEnvelope_ * fastAttackCoef_ + absSample * (1.0f - fastAttackCoef_);
    else
        fastEnvelope_ = fastEnvelope_ * fastReleaseCoef_ + absSample * (1.0f - fastReleaseCoef_);

    if (absSample > slowEnvelope_)
        slowEnvelope_ = slowEnvelope_ * slowAttackCoef_ + absSample * (1.0f - slowAttackCoef_);
    else
        slowEnvelope_ = slowEnvelope_ * slowReleaseCoef_ + absSample * (1.0f - slowReleaseCoef_);
}

void TransientDetector::processIdleState()
{
    if (slowEnvelope_ > NOISE_FLOOR && fastEnvelope_ > slowEnvelope_ * ONSET_RATIO)
    {
        state_ = State::MeasuringAttack;
        onsetLevel_ = slowEnvelope_;
        transientPeak_ = fastEnvelope_;
        samplesInState_ = 0;
        samplesSincePeakIncrease_ = 0;
    }
}

float TransientDetector::processAttackState(double sampleRate, float currentAttack)
{
    samplesInState_++;

    if (fastEnvelope_ > transientPeak_)
    {
        transientPeak_ = fastEnvelope_;
        samplesSincePeakIncrease_ = 0;
    }
    else
    {
        samplesSincePeakIncrease_++;
    }

    bool envelopeDropped = fastEnvelope_ < transientPeak_ * PEAK_THRESHOLD;
    double plateauSamples = (PLATEAU_TIME_MS / 1000.0) * sampleRate;
    bool plateauReached = samplesSincePeakIncrease_ >= plateauSamples;

    if (envelopeDropped || plateauReached)
    {
        double attackMs = (samplesInState_ / sampleRate) * 1000.0;
        currentAttack = static_cast<float>(attackMs);
        state_ = State::MeasuringDecay;
        samplesInState_ = 0;
        samplesSincePeakIncrease_ = 0;
    }

    if (samplesInState_ > sampleRate * 0.5)
    {
        state_ = State::Idle;
        samplesSincePeakIncrease_ = 0;
    }

    return currentAttack;
}

float TransientDetector::processDecayState(double sampleRate, float currentDecay)
{
    samplesInState_++;

    float decayLevel = onsetLevel_ + (transientPeak_ - onsetLevel_) * DECAY_THRESHOLD;
    if (fastEnvelope_ < decayLevel || fastEnvelope_ < NOISE_FLOOR * 10.0f)
    {
        double decayMs = (samplesInState_ / sampleRate) * 1000.0;
        currentDecay = static_cast<float>(decayMs);
        state_ = State::Idle;
    }

    if (slowEnvelope_ > NOISE_FLOOR && fastEnvelope_ > slowEnvelope_ * ONSET_RATIO * 1.5f)
    {
        state_ = State::MeasuringAttack;
        onsetLevel_ = slowEnvelope_;
        transientPeak_ = fastEnvelope_;
        samplesInState_ = 0;
        samplesSincePeakIncrease_ = 0;
    }

    if (samplesInState_ > sampleRate * 5.0)
    {
        state_ = State::Idle;
    }

    return currentDecay;
}

void TransientDetector::process(const float* samples, int numSamples, double sampleRate)
{
    if (!samples || numSamples <= 0 || !std::isfinite(sampleRate) || sampleRate <= 0.0)
        return;

    updateCoefficients(sampleRate);

    if (!std::isfinite(fastEnvelope_)) fastEnvelope_ = 0.0f;
    if (!std::isfinite(slowEnvelope_)) slowEnvelope_ = 0.0f;
    if (!std::isfinite(transientPeak_)) transientPeak_ = 0.0f;
    if (!std::isfinite(onsetLevel_)) onsetLevel_ = 0.0f;

    float currentAttack = attackTimeMs_.load(std::memory_order_relaxed);
    float currentDecay = decayTimeMs_.load(std::memory_order_relaxed);

    for (int i = 0; i < numSamples; ++i)
    {
        float rawSample = samples[i];
        float absSample = std::isfinite(rawSample) ? std::abs(rawSample) : 0.0f;

        updateEnvelopes(absSample);

        switch (state_)
        {
            case State::Idle:
                processIdleState();
                break;
            case State::MeasuringAttack:
                currentAttack = processAttackState(sampleRate, currentAttack);
                break;
            case State::MeasuringDecay:
                currentDecay = processDecayState(sampleRate, currentDecay);
                break;
        }
    }

    attackTimeMs_.store(currentAttack, std::memory_order_relaxed);
    decayTimeMs_.store(currentDecay, std::memory_order_relaxed);
}

} // namespace oscil
