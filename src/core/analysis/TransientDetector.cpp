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
    if (sampleRate <= 0.0 || sampleRate == lastSampleRate_)
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

void TransientDetector::process(const float* samples, int numSamples, double sampleRate)
{
    // Guard against invalid parameters
    if (!samples || numSamples <= 0 || !std::isfinite(sampleRate) || sampleRate <= 0.0)
        return;

    // Update coefficients if sample rate changed
    updateCoefficients(sampleRate);

    if (!std::isfinite(fastEnvelope_))
        fastEnvelope_ = 0.0f;
    if (!std::isfinite(slowEnvelope_))
        slowEnvelope_ = 0.0f;
    if (!std::isfinite(transientPeak_))
        transientPeak_ = 0.0f;
    if (!std::isfinite(onsetLevel_))
        onsetLevel_ = 0.0f;

    float currentAttack = attackTimeMs_.load(std::memory_order_relaxed);
    float currentDecay = decayTimeMs_.load(std::memory_order_relaxed);

    for (int i = 0; i < numSamples; ++i)
    {
        float rawSample = samples[i];
        float absSample = std::isfinite(rawSample) ? std::abs(rawSample) : 0.0f;

        // Update fast envelope (peak follower)
        if (absSample > fastEnvelope_)
        {
            fastEnvelope_ = fastEnvelope_ * fastAttackCoef_ + absSample * (1.0f - fastAttackCoef_);
        }
        else
        {
            fastEnvelope_ = fastEnvelope_ * fastReleaseCoef_ + absSample * (1.0f - fastReleaseCoef_);
        }

        // Update slow envelope (average follower)
        if (absSample > slowEnvelope_)
        {
            slowEnvelope_ = slowEnvelope_ * slowAttackCoef_ + absSample * (1.0f - slowAttackCoef_);
        }
        else
        {
            slowEnvelope_ = slowEnvelope_ * slowReleaseCoef_ + absSample * (1.0f - slowReleaseCoef_);
        }

        // State machine for transient detection
        switch (state_)
        {
            case State::Idle:
            {
                // Check for transient onset: fast envelope significantly exceeds slow
                if (slowEnvelope_ > NOISE_FLOOR && fastEnvelope_ > slowEnvelope_ * ONSET_RATIO)
                {
                    // Transient detected - start measuring attack
                    state_ = State::MeasuringAttack;
                    onsetLevel_ = slowEnvelope_;
                    transientPeak_ = fastEnvelope_;
                    samplesInState_ = 0;
                    samplesSincePeakIncrease_ = 0;
                }
                break;
            }

            case State::MeasuringAttack:
            {
                samplesInState_++;

                // Track the peak during attack
                if (fastEnvelope_ > transientPeak_)
                {
                    transientPeak_ = fastEnvelope_;
                    samplesSincePeakIncrease_ = 0;
                }
                else
                {
                    samplesSincePeakIncrease_++;
                }

                // Check for attack completion: either envelope dropped or plateau detected
                bool envelopeDropped = fastEnvelope_ < transientPeak_ * PEAK_THRESHOLD;
                double plateauSamples = (PLATEAU_TIME_MS / 1000.0) * sampleRate;
                bool plateauReached = samplesSincePeakIncrease_ >= plateauSamples;

                if (envelopeDropped || plateauReached)
                {
                    // Attack phase complete - calculate attack time
                    double attackMs = (samplesInState_ / sampleRate) * 1000.0;
                    currentAttack = static_cast<float>(attackMs);

                    // Transition to decay measurement
                    state_ = State::MeasuringDecay;
                    samplesInState_ = 0;
                    samplesSincePeakIncrease_ = 0;
                }

                // Timeout: if attack takes too long (> 500ms), something's wrong
                if (samplesInState_ > sampleRate * 0.5)
                {
                    state_ = State::Idle;
                    samplesSincePeakIncrease_ = 0;
                }
                break;
            }

            case State::MeasuringDecay:
            {
                samplesInState_++;

                // Check if signal has decayed to threshold
                float decayLevel = onsetLevel_ + (transientPeak_ - onsetLevel_) * DECAY_THRESHOLD;
                if (fastEnvelope_ < decayLevel || fastEnvelope_ < NOISE_FLOOR * 10.0f)
                {
                    // Decay complete
                    double decayMs = (samplesInState_ / sampleRate) * 1000.0;
                    currentDecay = static_cast<float>(decayMs);

                    // Return to idle
                    state_ = State::Idle;
                }

                // Check for new transient during decay (re-trigger)
                if (slowEnvelope_ > NOISE_FLOOR && fastEnvelope_ > slowEnvelope_ * ONSET_RATIO * 1.5f)
                {
                    // New transient - restart attack measurement
                    state_ = State::MeasuringAttack;
                    onsetLevel_ = slowEnvelope_;
                    transientPeak_ = fastEnvelope_;
                    samplesInState_ = 0;
                    samplesSincePeakIncrease_ = 0;
                }

                // Timeout: if decay takes too long (> 5s), reset
                if (samplesInState_ > sampleRate * 5.0)
                {
                    state_ = State::Idle;
                }
                break;
            }
        }
    }

    // Store results
    attackTimeMs_.store(currentAttack, std::memory_order_relaxed);
    decayTimeMs_.store(currentDecay, std::memory_order_relaxed);
}

} // namespace oscil
