/*
    MultiScoper Test Harness - Test DAW
    Main coordinator for multi-track DAW simulation
*/

#pragma once

#include "TestTrack.h"
#include "TestTransport.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace multiscoper::test
{

/**
 * Main DAW emulator that coordinates multiple tracks.
 *
 * Models multi-core DAW hosting: a juce::HighResolutionTimer running on its
 * own thread fires at block rate and dispatches one job per track to a
 * juce::ThreadPool. The dispatcher waits for all track jobs to complete
 * before advancing the shared transport and returning — this mirrors the
 * per-block barrier real DAWs (Logic, Reaper, Ableton) use between the
 * parallel audio graph passes.
 *
 * Threading model:
 *   - Audio dispatch runs on the HighResolutionTimer thread (NOT the message
 *     thread) so UI remains responsive under heavy per-track load.
 *   - TestTrack::processBlock invocations are parallelized across the pool.
 *   - Structural mutations (add/remove track, show/hide editor) take
 *     dispatchMutex_ and therefore serialize with any in-flight tick.
 */
class TestDAW : private juce::HighResolutionTimer
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

    /// Slot count of the internal track vector, including null slots left by
    /// removeTrack. Pair with `getTrack(i)` + nullptr skip when iterating.
    int getNumTracks() const { return static_cast<int>(tracks_.size()); }

    /// Live track count (non-null slots). Use for external "current tracks"
    /// reporting — health endpoint, scan-cycle post-condition, etc.
    /// Takes dispatchMutex_ so callers on any thread observe a coherent
    /// view — without the lock, unique_ptr slot writes from the message
    /// thread are not visible to HTTP-worker readers.
    int getActiveTrackCount() const
    {
        std::lock_guard<std::mutex> lock(dispatchMutex_);
        int n = 0;
        for (const auto& t : tracks_)
            if (t != nullptr)
                ++n;
        return n;
    }

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

    /**
     * Run one block on the given track synchronously from the caller's thread,
     * serialized with the audio dispatcher. Use this when a message-thread
     * handler needs to force a recompute (e.g., after changing BPM) without
     * racing the pool-dispatched tick.
     */
    void runSingleBlockSynchronously(TestTrack& track);

    /**
     * Diagnostic: number of audio ticks (pool dispatches) that have completed.
     * Monotonically increasing while the DAW is running.
     */
    uint64_t getProcessedTickCount() const { return processedTicks_.load(std::memory_order_relaxed); }

    /**
     * Diagnostic: number of worker threads in the audio thread pool.
     */
    int getPoolThreadCount() const;

    // ------------------------------------------------------------------
    //  Host-lifecycle simulation primitives
    //
    //  These reproduce DAW-initiated events that `prepareToPlay` / bus
    //  renegotiation / scanner cycles drive in production, and that the
    //  plain construct-once initialize() flow does not exercise.
    // ------------------------------------------------------------------

    /**
     * Reconfigure the session sample rate.
     *
     * Calls `prepare(newRate, bufferSize_)` on every live track — which in
     * turn invokes `prepareToPlay` on each processor — and updates the DAW's
     * own cached sample rate so later add/track uses the new value.
     *
     * If `audioThreadPrepareMode_` is true, the re-prepare is posted to the
     * audio thread pool instead of running inline on the caller's thread,
     * reproducing Pro Tools / Reaper behaviour where the host calls
     * `prepareToPlay` from the audio thread.
     *
     * Returns false if `newRate` is non-positive or non-finite.
     */
    bool setSampleRate(double newRate);

    /**
     * Reconfigure the session block size.
     *
     * Calls `prepare(sampleRate_, newSize)` on every live track and updates
     * the DAW's own cached size.  Honours `audioThreadPrepareMode_` the same
     * way `setSampleRate` does.
     *
     * Returns false if `newSize` is outside the accepted [16, 8192] range.
     */
    bool setBufferSize(int newSize);

    /**
     * Force the next `setSampleRate` / `setBufferSize` call to dispatch its
     * prepare work onto the audio thread pool instead of the caller's thread.
     *
     * This surfaces the non-message-thread branch of
     * `MultiScoperPluginProcessor::deferRegistration` that is otherwise unreachable
     * from harness tests.  A single flag — both host-triggered reprepares
     * happen on the same thread in real DAWs.
     */
    void setAudioThreadPrepareMode(bool enabled) { audioThreadPrepareMode_.store(enabled); }

    /**
     * True if audio-thread prepare mode is currently armed.
     */
    bool isAudioThreadPrepareMode() const { return audioThreadPrepareMode_.load(); }

    /**
     * Change the channel layout of a specific track.
     *
     * ``layout`` must be "mono" or "stereo".  Delegates to
     * `TestTrack::setChannelLayout` under the dispatch mutex so we can never
     * race a pool-dispatched tick.  Returns false for an unknown track index,
     * invalid layout name, or a processor that rejects the layout.
     */
    bool setChannelLayout(int trackIndex, const juce::String& layout);

    /**
     * Repeat an `addTrack → query state → removeTrack` cycle `cycles` times
     * without ever calling `processBlock` on the added track.
     *
     * Reproduces Ableton / Bitwig / Studio One plugin-scanner behaviour: the
     * host instantiates, reads names and layouts, then destroys — all without
     * feeding a single audio buffer.  Used for leak / UUID-collision tests.
     *
     * Returns the number of successful iterations (normally == `cycles`).
     */
    int scanCycle(const juce::String& trackName, int cycles);

    /**
     * Isolation mode: when enabled, every `addTrack` call constructs a fresh
     * `InstanceRegistry` dedicated to that one track, instead of sharing the
     * process-wide `PluginFactory::getInstance().getInstanceRegistry()`.
     *
     * This reproduces Logic Pro's AU sandbox model, where each plugin
     * instance runs in its own process and the registry singleton is NOT
     * shared across instances.  Without this, the in-process harness
     * structurally cannot fail cross-instance-discovery features — because
     * every track sees every other track's registered source through the
     * shared singleton.
     *
     * Existing tracks retain whatever registry they were constructed with.
     * Reset state + toggle the flag + re-add tracks to apply to a fresh
     * session.
     */
    void setIsolatedRegistriesEnabled(bool enabled) { isolatedRegistries_.store(enabled, std::memory_order_release); }
    bool isIsolatedRegistriesEnabled() const { return isolatedRegistries_.load(std::memory_order_acquire); }

private:
    void hiResTimerCallback() override;
    void processAudioTick();

    TestTransport transport_;
    std::vector<std::unique_ptr<TestTrack>> tracks_;

    double sampleRate_ = DEFAULT_SAMPLE_RATE;
    int bufferSize_ = DEFAULT_BUFFER_SIZE;
    bool running_ = false;
    bool initialized_ = false;

    // Multi-core DAW model: one pool shared by the whole graph, one job per track per tick.
    std::unique_ptr<juce::ThreadPool> audioThreadPool_;
    mutable std::mutex dispatchMutex_;
    std::atomic<uint64_t> processedTicks_{0};

    // When true, the next setSampleRate / setBufferSize posts its prepare
    // call onto a pool thread instead of running inline.  Reproduces the
    // Pro Tools / Reaper audio-thread prepareToPlay contract.
    std::atomic<bool> audioThreadPrepareMode_{false};

    // When true, addTrack() creates a per-track InstanceRegistry instead of
    // using the PluginFactory singleton's shared registry. Simulates Logic's
    // AU sandbox isolation — cross-instance source discovery will NOT work
    // because registries are not shared. See setIsolatedRegistriesEnabled.
    std::atomic<bool> isolatedRegistries_{false};

    // Per-track owned InstanceRegistry instances, created lazily by addTrack
    // when isolation mode is on. Entries pair with tracks_ by index; null
    // slots indicate tracks constructed against the shared factory registry.
    std::vector<std::unique_ptr<InstanceRegistry>> perTrackRegistries_;

    // Timer-based audio simulation (not real audio device)
    static constexpr int TIMER_INTERVAL_MS = 10; // ~100 callbacks per second

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestDAW)
};

} // namespace multiscoper::test
