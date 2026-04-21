/*
    MultiScoper Test Harness - Test DAW Implementation

    See TestDAW.h for the multi-core threading model.
*/

#include "TestDAW.h"

#include <algorithm>
#include <cmath>
#include <thread>

namespace multiscoper::test
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
    running_.store(true, std::memory_order_release);
    startTimer(TIMER_INTERVAL_MS);
}

void TestDAW::stop()
{
    // stopTimer() blocks until any currently-running hiResTimerCallback
    // returns; because that callback synchronously waits on all pool jobs
    // before returning, this implicitly drains the pool for in-flight work.
    stopTimer();
    running_.store(false, std::memory_order_release);
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

    // Keep perTrackRegistries_ sized in lockstep with tracks_ so slot
    // indices line up for add/remove/reuse. Null means "use factory".
    if (static_cast<size_t>(index) >= perTrackRegistries_.size())
        perTrackRegistries_.resize(static_cast<size_t>(index) + 1);

    IInstanceRegistry* overrideRegistry = nullptr;
    if (isolatedRegistries_.load(std::memory_order_acquire))
    {
        // Isolation mode: create a fresh registry for this track.  Replaces
        // any stale entry left by a previous remove on the same slot.
        perTrackRegistries_[static_cast<size_t>(index)] = std::make_unique<InstanceRegistry>();
        overrideRegistry = perTrackRegistries_[static_cast<size_t>(index)].get();
    }
    else
    {
        // Ensure no stale isolation registry from a prior cycle lingers
        // when isolation has been turned off mid-session.
        perTrackRegistries_[static_cast<size_t>(index)].reset();
    }

    juce::String trackName = name.isEmpty() ? ("Track " + juce::String(index + 1)) : name;
    auto track = std::make_unique<TestTrack>(index, trackName, transport_, overrideRegistry);
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
    // Destroy the paired per-track registry (if any) AFTER the track —
    // ~TestTrack runs the processor's ~MultiScoperPluginProcessor which calls
    // unregisterInstance on this registry. The registry must still be alive
    // during that call.
    if (static_cast<size_t>(trackIndex) < perTrackRegistries_.size())
        perTrackRegistries_[static_cast<size_t>(trackIndex)].reset();
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

namespace
{
// Blocking job used to dispatch prepare() onto a pool thread for the
// audio-thread-prepare simulation.  juce::ThreadPoolJob::runJob is the
// canonical hook to guarantee the callable executes on a pool worker; we
// signal completion via a WaitableEvent so the caller can bound the wait.
class PreparePoolJob final : public juce::ThreadPoolJob
{
public:
    PreparePoolJob(std::function<void()> fn, juce::WaitableEvent& done)
        : juce::ThreadPoolJob("MultiScoperTestDAW::prepare")
        , fn_(std::move(fn))
        , done_(done)
    {
    }

    JobStatus runJob() override
    {
        if (fn_)
            fn_();
        done_.signal();
        return jobHasFinished;
    }

private:
    std::function<void()> fn_;
    juce::WaitableEvent& done_;
};
} // namespace

bool TestDAW::setSampleRate(double newRate)
{
    // Accept only plausible audio rates. Real-world DAW support spans
    // roughly 8 kHz (voice-grade) to 384 kHz (DXD). Outside this range
    // downstream prepareToPlay computations (block-size in samples,
    // timing-engine coefficients) overflow or become useless.
    constexpr double kMinSampleRate = 8000.0;
    constexpr double kMaxSampleRate = 384000.0;
    if (!std::isfinite(newRate) || newRate < kMinSampleRate || newRate > kMaxSampleRate)
        return false;

    // Serialize with the audio dispatcher.  prepareToPlay mutates per-track
    // state that the pool reads — the dispatch mutex is the only place in
    // the harness that provides that ordering guarantee.
    std::lock_guard<std::mutex> lock(dispatchMutex_);

    sampleRate_.store(newRate, std::memory_order_release);
    transport_.prepare(newRate);

    const int currentBuffer = bufferSize_.load(std::memory_order_acquire);
    auto runPrepare = [this, newRate, currentBuffer]() {
        for (auto& track : tracks_)
        {
            if (track != nullptr)
                track->prepare(newRate, currentBuffer);
        }
    };

    if (audioThreadPrepareMode_.load() && audioThreadPool_ != nullptr)
    {
        juce::WaitableEvent done;
        // ThreadPool takes ownership of raw pointer (deleteJob=true).
        audioThreadPool_->addJob(new PreparePoolJob(std::move(runPrepare), done), true);
        // Return honestly: if the pool-dispatched prepare didn't complete
        // in 5s, the DAW's cached sample rate is already updated but the
        // tracks haven't been re-prepared. Callers who rely on "true means
        // the tracks are at the new rate" need that distinction.
        return done.wait(5000);
    }
    runPrepare();
    return true;
}

bool TestDAW::setBufferSize(int newSize)
{
    if (newSize < 16 || newSize > 8192)
        return false;

    std::lock_guard<std::mutex> lock(dispatchMutex_);

    bufferSize_.store(newSize, std::memory_order_release);

    const double currentRate = sampleRate_.load(std::memory_order_acquire);
    auto runPrepare = [this, newSize, currentRate]() {
        for (auto& track : tracks_)
        {
            if (track != nullptr)
                track->prepare(currentRate, newSize);
        }
    };

    if (audioThreadPrepareMode_.load() && audioThreadPool_ != nullptr)
    {
        juce::WaitableEvent done;
        audioThreadPool_->addJob(new PreparePoolJob(std::move(runPrepare), done), true);
        // Same timeout honesty as setSampleRate — return false so callers
        // can distinguish a completed prepare from a silent stall.
        return done.wait(5000);
    }
    runPrepare();
    return true;
}

bool TestDAW::setChannelLayout(int trackIndex, const juce::String& layout)
{
    std::lock_guard<std::mutex> lock(dispatchMutex_);

    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks_.size()))
        return false;

    auto& track = tracks_[static_cast<size_t>(trackIndex)];
    if (track == nullptr)
        return false;

    return track->setChannelLayout(layout);
}

int TestDAW::scanCycle(const juce::String& trackName, int cycles)
{
    if (cycles <= 0)
        return 0;

    int successful = 0;
    for (int i = 0; i < cycles; ++i)
    {
        const int index = addTrack(trackName);
        if (index < 0)
            continue;

        // Scanner-style probe: read the track's metadata without ever
        // processing a block.  These reads go through the same public API
        // a DAW scanner uses (getName + bus layout) so any ordering bug
        // between prepareToPlay and getSourceId still surfaces.  The
        // dispatch lock keeps the pool from racing us on the track slot;
        // asm-volatile-free reads prevent the optimizer from elidng them.
        {
            std::lock_guard<std::mutex> lock(dispatchMutex_);
            if (auto* t = (index >= 0 && index < static_cast<int>(tracks_.size()))
                              ? tracks_[static_cast<size_t>(index)].get()
                              : nullptr)
            {
                auto name = t->getName();
                auto channels = t->getProcessor().getTotalNumInputChannels();
                juce::ignoreUnused(name, channels);
            }
        }

        if (removeTrack(index))
            ++successful;
    }
    return successful;
}

void TestDAW::hiResTimerCallback()
{
    if (running_.load(std::memory_order_acquire))
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

    // Sync state lives on the heap under a shared_ptr — each pool lambda
    // captures its own copy. If done.wait(5000) times out and removeAllJobs
    // fails to actually cancel a still-running lambda (lambda jobs don't
    // check shouldExit()), the lambda's shared_ptr copy keeps the state
    // alive after processAudioTick returns. A stack-local WaitableEvent +
    // atomic would have been a use-after-free under that timeout path.
    struct TickState
    {
        juce::WaitableEvent done;
        std::atomic<int> remaining;
        explicit TickState(int n) : remaining(n) {}
    };
    auto state = std::make_shared<TickState>(static_cast<int>(live.size()));

    for (TestTrack* track : live)
    {
        audioThreadPool_->addJob([track, state]() {
            track->processBlock();
            if (state->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1)
                state->done.signal();
        });
    }

    // Synchronous wait. Timeout guards against a pathological stall
    // without hanging tests. The shared_ptr makes the state's lifetime
    // safe even if a pool job outlives the tick on the timeout path.
    if (!state->done.wait(5000))
    {
        // Drain the pool best-effort. juce's removeAllJobs(interruptRunningJobs)
        // is advisory — lambda jobs don't check shouldExit(), so a truly
        // stuck processBlock keeps running. The shared_ptr above ensures
        // that's safe wrt sync-state lifetime.
        audioThreadPool_->removeAllJobs(/*interruptRunningJobs=*/true, /*timeOutMs=*/2000);
    }

    // One transport advance per tick, regardless of track count — matches
    // how a real DAW advances the playhead exactly once per buffer.
    transport_.advancePosition(bufferSize_);
    processedTicks_.fetch_add(1, std::memory_order_relaxed);
}

} // namespace multiscoper::test
