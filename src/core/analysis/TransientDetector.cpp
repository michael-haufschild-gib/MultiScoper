/*
    Oscil - Transient Detector Implementation
*/

#include "core/analysis/TransientDetector.h"
#include <cmath>

namespace oscil
{

TransientDetector::TransientDetector()
{
}

void TransientDetector::reset()
{
    state_ = State::Idle;
    envelope_ = 0.0f;
    peakLevel_ = 0.0f;
    samplesInState_ = 0;
    attackTimeMs_ = 0.0f;
    decayTimeMs_ = 0.0f;
}

void TransientDetector::process(const float* samples, int numSamples, double sampleRate)
{
    if (sampleRate <= 0.0) return;
    
    float currentAttack = attackTimeMs_.load(std::memory_order_relaxed);
    float currentDecay = decayTimeMs_.load(std::memory_order_relaxed);
    
    for (int i = 0; i < numSamples; ++i)
    {
        float absSample = std::abs(samples[i]);
        
        // Track overall peak envelope for dynamic thresholds
        if (absSample > peakLevel_)
        {
            peakLevel_ = absSample;
        }
        else
        {
            peakLevel_ *= ENVELOPE_DECAY;
        }
        
        // Avoid division by zero or noise floor issues
        if (peakLevel_ < 0.001f) 
        {
            state_ = State::Idle;
            continue;
        }
        
        float lowThresh = peakLevel_ * ATTACK_THRESHOLD_LOW;
        float highThresh = peakLevel_ * ATTACK_THRESHOLD_HIGH;
        
        switch (state_)
        {
            case State::Idle:
                if (absSample < lowThresh)
                {
                    // Ready for attack
                }
                else if (absSample > lowThresh && absSample < highThresh)
                {
                    // Entered attack phase
                    state_ = State::MeasuringAttack;
                    samplesInState_ = 0;
                }
                break;
                
            case State::MeasuringAttack:
                samplesInState_++;
                
                if (absSample >= highThresh)
                {
                    // Attack complete
                    double ms = (samplesInState_ / sampleRate) * 1000.0;
                    currentAttack = static_cast<float>(ms);
                    state_ = State::MeasuringDecay;
                    samplesInState_ = 0;
                }
                else if (absSample < lowThresh)
                {
                    // False start, dropped back down
                    state_ = State::Idle;
                }
                break;
                
            case State::MeasuringDecay:
                samplesInState_++;
                
                if (absSample <= lowThresh)
                {
                    // Decay complete
                    double ms = (samplesInState_ / sampleRate) * 1000.0;
                    currentDecay = static_cast<float>(ms);
                    state_ = State::Idle;
                }
                else if (absSample > peakLevel_) // New peak during decay
                {
                    // Reset decay measurement or restart attack?
                    // Usually implies a new attack or sustain
                    // For now, assume re-attack if it jumps significantly, but simple logic:
                    if (absSample > highThresh)
                    {
                        // Still high, continue decay measurement
                    }
                }
                break;
        }
    }
    
    attackTimeMs_.store(currentAttack, std::memory_order_relaxed);
    decayTimeMs_.store(currentDecay, std::memory_order_relaxed);
}

} // namespace oscil