/*
    Oscil Test Harness - Test DAW
    Main coordinator for multi-track DAW simulation
*/

#pragma once

#include "TestTrack.h"
#include "TestTransport.h"

#include <juce_audio_devices/juce_audio_devices.h>

#include <memory>
#include <vector>

namespace oscil::test
{

/**
 * Main DAW emulator that coordinates multiple tracks.
 * Runs an audio callback to continuously process audio.
 */
class TestDAW : private juce::Timer
{
public:
    static constexpr int DEFAULT_NUM_TRACKS = 3;
    static constexpr double DEFAULT_SAMPLE_RATE = 44100.0;
    static constexpr int DEFAULT_BUFFER_SIZE = 512;

    TestDAW();
    ~TestDAW() override;

    /**
     * Initialize with given number of tracks
     */
    void initialize(int numTracks = DEFAULT_NUM_TRACKS);

    /**
     * Start audio processing
     */
    void start();

    /**
     * Stop audio processing
     */
    void stop();

    /**
     * Check if running
     */
    bool isRunning() const { return running_; }

    /**
     * Get transport
     */
    TestTransport& getTransport() { return transport_; }
    const TestTransport& getTransport() const { return transport_; }

    /**
     * Get track by index
     */
    TestTrack* getTrack(int index);
    const TestTrack* getTrack(int index) const;

    /**
     * Get number of tracks
     */
    int getNumTracks() const { return static_cast<int>(tracks_.size()); }

    /**
     * Get all tracks
     */
    std::vector<TestTrack*> getTracks();

    /**
     * Add a new track at runtime.
     * Returns the track index.
     */
    int addTrack(const juce::String& name = {});

    /**
     * Remove a track at runtime.
     * Hides editor, nulls out the slot (indices remain stable).
     * Returns true if track existed and was removed.
     */
    bool removeTrack(int trackIndex);

    /**
     * Show editor for a specific track (0-based index)
     * Returns the main/first track's editor by default
     */
    void showTrackEditor(int trackIndex = 0);

    /**
     * Hide all editors
     */
    void hideAllEditors();

    /**
     * Get the primary editor (first track's editor)
     */
    juce::AudioProcessorEditor* getPrimaryEditor();

    /**
     * Get sample rate
     */
    double getSampleRate() const { return sampleRate_; }

    /**
     * Get buffer size
     */
    int getBufferSize() const { return bufferSize_; }

private:
    void timerCallback() override;
    void processAudioBlock();

    TestTransport transport_;
    std::vector<std::unique_ptr<TestTrack>> tracks_;

    double sampleRate_ = DEFAULT_SAMPLE_RATE;
    int bufferSize_ = DEFAULT_BUFFER_SIZE;
    bool running_ = false;
    bool initialized_ = false;

    // Timer-based audio simulation (not real audio device)
    static constexpr int TIMER_INTERVAL_MS = 10; // ~100 callbacks per second

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestDAW)
};

} // namespace oscil::test
