/*
    Oscil Test Harness - Transport
    Simulates DAW transport (play/stop/BPM/position)
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>

namespace oscil::test
{

/**
 * Simulates DAW transport for E2E testing.
 * Provides play/stop, BPM, and position tracking.
 */
class TestTransport
{
public:
    TestTransport();
    ~TestTransport() = default;

    /**
     * Prepare for given sample rate
     */
    void prepare(double sampleRate);

    /**
     * Start playback
     */
    void play();

    /**
     * Stop playback
     */
    void stop();

    /**
     * Check if playing
     */
    bool isPlaying() const { return playing_.load(); }

    /**
     * Set BPM
     */
    void setBpm(double bpm);
    double getBpm() const { return bpm_.load(); }

    /**
     * Set position in samples
     */
    void setPositionSamples(int64_t samples);
    int64_t getPositionSamples() const { return positionSamples_.load(); }

    /**
     * Get position in seconds
     */
    double getPositionSeconds() const;

    /**
     * Get position in beats
     */
    double getPositionBeats() const;

    /**
     * Advance position by given number of samples (call from audio callback)
     */
    void advancePosition(int numSamples);

    /**
     * Fill JUCE AudioPlayHead::PositionInfo for host sync simulation
     */
    juce::AudioPlayHead::PositionInfo getPositionInfo() const;

    /**
     * Get samples per beat at current BPM
     */
    double getSamplesPerBeat() const;

private:
    double sampleRate_ = 44100.0;
    std::atomic<bool> playing_{false};
    std::atomic<double> bpm_{120.0};
    std::atomic<int64_t> positionSamples_{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestTransport)
};

} // namespace oscil::test
