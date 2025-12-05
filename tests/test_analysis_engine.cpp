/*
    Oscil - Analysis Engine Tests
*/

#include <gtest/gtest.h>
#include "core/analysis/AnalysisEngine.h"
#include <vector>
#include <cmath>

using namespace oscil;

class AnalysisEngineTest : public ::testing::Test
{
protected:
    AnalysisEngine engine;
    double sampleRate = 44100.0;
    
    // Helper to generate sine wave
    void generateSine(juce::AudioBuffer<float>& buffer, int channel, float frequency, float amplitude)
    {
        auto* data = buffer.getWritePointer(channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            data[i] = amplitude * std::sin(2.0f * juce::MathConstants<float>::pi * frequency * i / sampleRate);
        }
    }
    
    // Helper to generate DC offset
    void generateDC(juce::AudioBuffer<float>& buffer, int channel, float offset)
    {
        auto* data = buffer.getWritePointer(channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            data[i] = offset;
        }
    }
    
    // Helper to generate step for transient testing
    void generateStep(juce::AudioBuffer<float>& buffer, int channel, float startAmp, float endAmp, int transitionSample)
    {
        auto* data = buffer.getWritePointer(channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            data[i] = (i < transitionSample) ? startAmp : endAmp;
        }
    }
};

TEST_F(AnalysisEngineTest, InitialState)
{
    const auto& metrics = engine.getMetrics();
    EXPECT_EQ(metrics.left.rmsDb.load(), -100.0f);
    EXPECT_EQ(metrics.left.peakDb.load(), -100.0f);
    EXPECT_EQ(metrics.left.crestFactorDb.load(), 0.0f);
    EXPECT_EQ(metrics.left.dcOffset.load(), 0.0f);
}

TEST_F(AnalysisEngineTest, RMS_SineWave)
{
    juce::AudioBuffer<float> buffer(1, 1024);
    // Full scale sine wave: RMS = 1/sqrt(2) = 0.707 (-3.01 dB)
    generateSine(buffer, 0, 1000.0f, 1.0f);
    
    // Run multiple blocks to let smoothing settle
    for (int i = 0; i < 100; ++i)
        engine.process(buffer, sampleRate);
    
    const auto& metrics = engine.getMetrics();
    
    // Allow small margin for float precision
    EXPECT_NEAR(metrics.left.rmsDb.load(), -3.01f, 0.1f);
    EXPECT_NEAR(metrics.left.peakDb.load(), 0.0f, 0.01f); // Peak is 1.0 (0 dB)
}

TEST_F(AnalysisEngineTest, RMS_Silence)
{
    juce::AudioBuffer<float> buffer(1, 1024);
    buffer.clear();
    
    // Run multiple blocks to let smoothing settle
    for (int i = 0; i < 50; ++i)
        engine.process(buffer, sampleRate);
    
    const auto& metrics = engine.getMetrics();
    EXPECT_LE(metrics.left.rmsDb.load(), -99.0f);
    EXPECT_LE(metrics.left.peakDb.load(), -99.0f);
}

TEST_F(AnalysisEngineTest, CrestFactor_Sine)
{
    juce::AudioBuffer<float> buffer(1, 1024);
    generateSine(buffer, 0, 1000.0f, 1.0f);
    
    // Run multiple blocks to let smoothing settle
    for (int i = 0; i < 100; ++i)
        engine.process(buffer, sampleRate);
    
    const auto& metrics = engine.getMetrics();
    // Crest factor for sine = Peak / RMS = 1 / 0.707 = 1.414 (3.01 dB)
    EXPECT_NEAR(metrics.left.crestFactorDb.load(), 3.01f, 0.1f);
}

TEST_F(AnalysisEngineTest, DC_Offset)
{
    juce::AudioBuffer<float> buffer(1, 1024);
    generateDC(buffer, 0, 0.5f);
    
    // Run multiple blocks to let smoothing settle
    // DC smoothing is slow (0.05), so we need more iterations
    for (int i = 0; i < 200; ++i)
        engine.process(buffer, sampleRate);
    
    const auto& metrics = engine.getMetrics();
    EXPECT_NEAR(metrics.left.dcOffset.load(), 0.5f, 0.001f);
}

TEST_F(AnalysisEngineTest, MaxPeakHold)
{
    juce::AudioBuffer<float> buffer(1, 1024);
    
    // First block: Peak 0.5 (-6 dB)
    generateDC(buffer, 0, 0.5f);
    engine.process(buffer, sampleRate);
    
    const auto& metrics = engine.getMetrics();
    EXPECT_NEAR(metrics.left.maxPeakDb.load(), -6.02f, 0.1f);
    
    // Second block: Peak 0.25 (-12 dB) - Max should hold
    generateDC(buffer, 0, 0.25f);
    engine.process(buffer, sampleRate);
    EXPECT_NEAR(metrics.left.maxPeakDb.load(), -6.02f, 0.1f);
    
    // Third block: Peak 1.0 (0 dB) - Max should update
    generateDC(buffer, 0, 1.0f);
    engine.process(buffer, sampleRate);
    EXPECT_NEAR(metrics.left.maxPeakDb.load(), 0.0f, 0.1f);
    
    // Reset accumulated
    engine.resetAccumulated();
    EXPECT_EQ(metrics.left.maxPeakDb.load(), -100.0f);
}

TEST_F(AnalysisEngineTest, MidSideProcessing)
{
    juce::AudioBuffer<float> buffer(2, 1024);
    
    // Left = 1.0, Right = 1.0 (Mono center)
    // Mid = (L+R)/2 = 1.0
    // Side = (L-R)/2 = 0.0
    generateDC(buffer, 0, 1.0f);
    generateDC(buffer, 1, 1.0f);
    
    engine.process(buffer, sampleRate);
    
    const auto& metrics = engine.getMetrics();
    EXPECT_NEAR(metrics.mid.peakDb.load(), 0.0f, 0.1f);
    EXPECT_LE(metrics.side.peakDb.load(), -99.0f);
    
    // Left = 1.0, Right = -1.0 (Full wide)
    // Mid = 0.0
    // Side = 1.0
    generateDC(buffer, 0, 1.0f);
    generateDC(buffer, 1, -1.0f);
    
    engine.process(buffer, sampleRate);
    EXPECT_LE(metrics.mid.peakDb.load(), -99.0f);
    EXPECT_NEAR(metrics.side.peakDb.load(), 0.0f, 0.1f);
}

TEST_F(AnalysisEngineTest, AttackDetection)
{
    // Need continuous processing to drive state machine
    // Create a buffer with a rising edge (attack)
    juce::AudioBuffer<float> buffer(1, 1024);
    
    // 1. Prime detector with a full-scale signal to establish "Recent Max Peak" (1.0)
    juce::AudioBuffer<float> primeBuffer(1, 1024);
    generateDC(primeBuffer, 0, 1.0f);
    engine.process(primeBuffer, sampleRate);
    
    // 2. Silence to reset state to Idle (signal drops below 10%)
    // But peak envelope decays slowly (0.9999), so it stays near 1.0 for a while
    buffer.clear();
    engine.process(buffer, sampleRate);
    
    // 3. Step up to 1.0 (Ramp)
    // Should trigger attack measurement against the established peak (~1.0)
    
    // Simulate 10ms attack ramp
    // 44.1 samples per ms -> 441 samples
    auto* data = buffer.getWritePointer(0);
    for (int i = 0; i < 441; ++i)
    {
        data[i] = static_cast<float>(i) / 441.0f; // 0 to 1.0 linear ramp
    }
    for (int i = 441; i < 1024; ++i)
    {
        data[i] = 1.0f;
    }
    
    engine.process(buffer, sampleRate);
    
    const auto& metrics = engine.getMetrics();
    // Attack is time from 10% (0.1) to 90% (0.9) of Peak (1.0)
    // That's 80% of the 10ms ramp = 8ms
    
    float attack = metrics.left.attackTimeMs.load();
    EXPECT_GT(attack, 0.0f);
    // Allow wider margin (2.0ms) due to block boundaries and envelope decay drift
    EXPECT_NEAR(attack, 8.0f, 2.0f); 
}
