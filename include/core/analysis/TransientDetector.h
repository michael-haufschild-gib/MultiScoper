/*
    Oscil - Transient Detector
    Detects attack and decay phases in audio signals
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

namespace oscil
{

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
        Idle,
        MeasuringAttack,
        MeasuringDecay
    };
    
    State state_ = State::Idle;
    
    float envelope_ = 0.0f;
    float peakLevel_ = 0.0f;
    
    // Timing tracking
    double samplesInState_ = 0;
    
    std::atomic<float> attackTimeMs_{ 0.0f };
    std::atomic<float> decayTimeMs_{ 0.0f };
    
    // Constants
    static constexpr float ENVELOPE_DECAY = 0.9999f; // Slow decay for peak tracking
    static constexpr float ATTACK_THRESHOLD_LOW = 0.1f;
    static constexpr float ATTACK_THRESHOLD_HIGH = 0.9f;
};

} // namespace oscil
