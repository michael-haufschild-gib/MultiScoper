/*
    Oscil Test Harness - Test DAW Implementation

    See TestDAW.h for the multi-core threading model.
*/

#include "TestDAW.h"

#include <algorithm>
#include <thread>

namespace oscil::test
{

namespace
{
// Pool size mirrors what a real DAW does: roughly hardware concurrency, but
// clamped so tiny CI VMs still get real parallelism and huge machines don't
// spawn absurd pools in the test harness. Real DAWs (Logic, Reaper) expose
// this as a user-configurable "processing threads" setting.
int chooseAudioPoolSize()
{
    const auto hw = std::thread::hardware_concurrency();
    const unsigned clamped = std::clamp(hw, 2u, 16u);
    return static_cast<int>(clamped);
}
} // namespace

TestDAW::TestDAW() {}

TestDAW::~TestDAW()
{
    stop();
    hideAllEditors();

    // Clear tracks under the dispatch mutex so we can never collide with an
    // in-flight pool job (stop() above will have drained, but this is belt-
    // and-suspenders for teardown ordering).
    std::lock_guard<std::mutex> lock(dispatchMutex_);
    for (int i = static_cast<int>(tracks_.size()) - 1; i >= 0; --i)
        tracks_[static_cast<size_t>(i)].reset();
    tracks_.clear();
}

void TestDAW::initialize(int numTracks)
{
    if (initialized_)
        return;

    // Build the audio thread pool before any tracks exist so addTrack paths
    // after initialize() observe a live pool.
    audioThreadPool_ = std::make_unique<juce::ThreadPool>(chooseAudioPoolSize());

    // Prepare transport
    transport_.prepare(sampleRate_);

    // Create tracks
    for (int i = 0; i < numTracks; ++i)
    {
        juce::String trackName = "Track " + juce::String(i + 1);
        auto track = std::make_unique<TestTrack>(i, trackName, transport_);
        track->prepare(sampleRate_, bufferSize_);

        // Set default audio - each track gets a different LFO frequency for visual testing
        constexpr float trackFrequencies[] = {2.0f, 6.0f, 3.0f};
        float freq = (i < 3) ? trackFrequencies[i] : 0.5f;
        track->getAudioGenerator().setFrequency(freq);
        track->getAudioGenerator().setWaveform(Waveform::Sine);
        track->getAudioGenerator().setAmplitude(0.5f);

        tracks_.push_back(std::move(track));
    }

    initialized_ = true;
}

void TestDAW::start()
{
    if (!initialized_)
        initialize();

    transport_.play();
    running_ = true;
    startTimer(TIMER_INTERVAL_MS);
}

void TestDAW::stop()
{
    // stopTimer() blocks until any currently-running hiResTimerCallback
    // returns; because that callback synchronously waits on all pool jobs
    // before returning, this implicitly drains the pool for in-flight work.
    stopTimer();
    running_ = false;
    transport_.stop();

    // Explicit safety drain: if a caller invoked runSingleBlockSynchronously
    // right as we were stopping, we don't want leftover jobs to run after
    // teardown begins.
    if (audioThreadPool_)
        audioThreadPool_->removeAllJobs(/*interruptRunningJobs=*/false, /*timeOutMs=*/2000);
}

TestTrack* TestDAW::getTrack(int index)
{
    if (index >= 0 && index < static_cast<int>(tracks_.size()))
        return tracks_[static_cast<size_t>(index)].get();
    return nullptr;
}

const TestTrack* TestDAW::getTrack(int index) const
{
    if (index >= 0 && index < static_cast<int>(tracks_.size()))
        return tracks_[static_cast<size_t>(index)].get();
    return nullptr;
}

std::vector<TestTrack*> TestDAW::getTracks()
{
    std::vector<TestTrack*> result;
    for (auto& track : tracks_)
    {
        if (track != nullptr)
            result.push_back(track.get());
    }
    return result;
}

void TestDAW::showTrackEditor(int trackIndex)
{
    // Editor lifetime is observed by the audio dispatcher indirectly (via
    // track state), and showEditor() must not overlap with a pool job on the
    // same track: the editor caches source-view state the processor mutates
    // during processBlock.
    std::lock_guard<std::mutex> lock(dispatchMutex_);
    if (auto* track = getTrack(trackIndex))
    {
        track->showEditor();
    }
}

void TestDAW::hideAllEditors()
{
    std::lock_guard<std::mutex> lock(dispatchMutex_);
    for (auto& track : tracks_)
    {
        if (track != nullptr)
            track->hideEditor();
    }
}

juce::AudioProcessorEditor* TestDAW::getPrimaryEditor()
{
    if (!tracks_.empty())
    {
        return tracks_[0]->getEditor();
    }
    return nullptr;
}

int TestDAW::addTrack(const juce::String& name)
{
    // Serialize with the audio dispatcher. Real DAWs do the same: adding a
    // track on a running session drops a graph-rebuild barrier so the audio
    // thread pool sees a consistent track list.
    std::lock_guard<std::mutex> lock(dispatchMutex_);

    // Reuse a freed slot (from removeTrack) if available so track indices
    // stay stable across add/remove cycles — essential for tests that remove
    // a middle track and rely on subsequent tests still addressing track N.
    int index = -1;
    for (size_t i = 0; i < tracks_.size(); ++i)
    {
        if (tracks_[i] == nullptr)
        {
            index = static_cast<int>(i);
            break;
        }
    }
    if (index < 0)
        index = static_cast<int>(tracks_.size());

    juce::String trackName = name.isEmpty() ? ("Track " + juce::String(index + 1)) : name;
    auto track = std::make_unique<TestTrack>(index, trackName, transport_);
    track->prepare(sampleRate_, bufferSize_);
    track->getAudioGenerator().setFrequency(440.0f);
    track->getAudioGenerator().setWaveform(Waveform::Sine);
    track->getAudioGenerator().setAmplitude(0.5f);

    if (index < static_cast<int>(tracks_.size()))
        tracks_[static_cast<size_t>(index)] = std::move(track);
    else
        tracks_.push_back(std::move(track));
    return index;
}

bool TestDAW::removeTrack(int trackIndex)
{
    std::lock_guard<std::mutex> lock(dispatchMutex_);

    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks_.size()))
        return false;
    if (tracks_[static_cast<size_t>(trackIndex)] == nullptr)
        return false;

    tracks_[static_cast<size_t>(trackIndex)]->hideEditor();
    tracks_[static_cast<size_t>(trackIndex)].reset();
    return true;
}

void TestDAW::runSingleBlockSynchronously(TestTrack& track)
{
    std::lock_guard<std::mutex> lock(dispatchMutex_);
    track.processBlock();
    // Advance the transport once for this forced block, matching the
    // dispatcher's own per-tick discipline.
    transport_.advancePosition(bufferSize_);
}

int TestDAW::getPoolThreadCount() const { return audioThreadPool_ ? audioThreadPool_->getNumThreads() : 0; }

void TestDAW::hiResTimerCallback()
{
    if (running_)
    {
        processAudioTick();
    }
}

void TestDAW::processAudioTick()
{
    // Hold the mutex for the whole tick: keeps the track list stable while we
    // enumerate it, and blocks add/remove until pool jobs finish so the pool
    // never touches a track that has been destroyed.
    std::lock_guard<std::mutex> lock(dispatchMutex_);

    if (audioThreadPool_ == nullptr)
        return;

    // Snapshot live tracks into a local vector. Raw pointers are safe under
    // the lock; the tracks_ vector cannot mutate, and the pool jobs complete
    // before we unlock.
    std::vector<TestTrack*> live;
    live.reserve(tracks_.size());
    for (auto& t : tracks_)
    {
        if (t != nullptr)
            live.push_back(t.get());
    }

    if (live.empty())
    {
        // Still advance the transport so tests relying on position continue
        // to see time passing when no tracks are present.
        transport_.advancePosition(bufferSize_);
        processedTicks_.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    juce::WaitableEvent done;
    std::atomic<int> remaining{static_cast<int>(live.size())};

    for (TestTrack* track : live)
    {
        audioThreadPool_->addJob([track, &remaining, &done]() {
            track->processBlock();
            if (remaining.fetch_sub(1, std::memory_order_acq_rel) == 1)
                done.signal();
        });
    }

    // Synchronous wait — done lives on this stack and outlives all jobs.
    // Timeout guards against a pathological stall without hanging tests.
    if (!done.wait(5000))
    {
        // If we ever time out, drain the pool so no lambdas race our stack.
        audioThreadPool_->removeAllJobs(/*interruptRunningJobs=*/true, /*timeOutMs=*/2000);
    }

    // One transport advance per tick, regardless of track count — matches
    // how a real DAW advances the playhead exactly once per buffer.
    transport_.advancePosition(bufferSize_);
    processedTicks_.fetch_add(1, std::memory_order_relaxed);
}

} // namespace oscil::test
