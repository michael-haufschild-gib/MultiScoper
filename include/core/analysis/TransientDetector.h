/*
    Oscil - Transient Detector
    Detects attack and decay phases in audio signals using envelope following
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

namespace oscil
{

/**
 * Transient detector using dual-envelope approach.
 *
 * Measures attack and decay times by tracking:
 * - Fast envelope: quickly follows signal peaks
 * - Slow envelope: tracks average signal level
 *
 * Attack is measured from transient onset to peak.
 * Decay is measured from peak to return to baseline.
 */
class TransientDetector
{
public:
    TransientDetector();

    void process(const float* samples, int numSamples, double sampleRate);
    void reset();

    float getAttackTimeMs() const { return attackTimeMs_.load(std::memory_order_relaxed); }
    float getDecayTimeMs() const { return decayTimeMs_.load(std::memory_order_relaxed); }

private:
    enum class State
    {
        Idle,           // Waiting for transient onset
        MeasuringAttack, // Transient detected, measuring rise to peak
        MeasuringDecay   // Peak passed, measuring fall to baseline
    };

    State state_ = State::Idle;

    // Dual envelope followers
    float fastEnvelope_ = 0.0f;   // Quick attack, quick release
    float slowEnvelope_ = 0.0f;   // Slow attack, slow release

    // Transient measurement
    float transientPeak_ = 0.0f;       // Peak level during current transient
    float onsetLevel_ = 0.0f;          // Level at transient onset
    double samplesInState_ = 0;        // Sample counter for timing
    double samplesSincePeakIncrease_ = 0; // Counter for plateau detection

    // Results (atomic for thread-safe UI read)
    std::atomic<float> attackTimeMs_{ 0.0f };
    std::atomic<float> decayTimeMs_{ 0.0f };

    // Envelope coefficients (set based on sample rate)
    float fastAttackCoef_ = 0.0f;
    float fastReleaseCoef_ = 0.0f;
    float slowAttackCoef_ = 0.0f;
    float slowReleaseCoef_ = 0.0f;

    // Last sample rate for coefficient calculation
    double lastSampleRate_ = 0.0;

    void updateCoefficients(double sampleRate);
    void updateEnvelopes(float absSample);
    void processIdleState();
    float processAttackState(double sampleRate, float currentAttack);
    float processDecayState(double sampleRate, float currentDecay);

    // Detection thresholds
    static constexpr float ONSET_RATIO = 2.0f;      // Fast/slow ratio to detect onset
    static constexpr float PEAK_THRESHOLD = 0.9f;   // Fraction of peak to consider "at peak"
    static constexpr float DECAY_THRESHOLD = 0.2f;  // Fraction of peak to consider "decayed"
    static constexpr float NOISE_FLOOR = 0.001f;    // Minimum level to consider signal present

    // Envelope time constants (in milliseconds)
    static constexpr float FAST_ATTACK_MS = 0.5f;
    static constexpr float FAST_RELEASE_MS = 5.0f;
    static constexpr float SLOW_ATTACK_MS = 50.0f;
    static constexpr float SLOW_RELEASE_MS = 200.0f;

    // Plateau detection: if peak hasn't increased for this long, consider attack complete
    static constexpr float PLATEAU_TIME_MS = 2.0f;
};

} // namespace oscil
