/*
    Oscil - Instance Registry Implementation
*/

#include "core/InstanceRegistry.h"

#include "core/OscilLog.h"
#include "core/SharedCaptureBuffer.h"

#include "Oscil.h"

namespace oscil
{

// Note: SourceId::generate() and InstanceId::generate() are implemented in Source.cpp

InstanceRegistry::InstanceRegistry()
{
    // Default dispatcher uses MessageManager::callAsync.
    // Constructor runs synchronously on the caller's thread; no other thread can
    // observe dispatcher_ yet, but we still take the lock to satisfy
    // thread-safety analysis (dispatcher_ is OSCIL_GUARDED_BY(dispatcherMutex_)).
    const oscil::ScopedLock lock(dispatcherMutex_);
    dispatcher_ = [](std::function<void()> f) {
        if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
        {
            juce::MessageManager::callAsync(std::move(f));
        }
        else if (f)
        {
            f();
        }
    };
}

InstanceRegistry::~InstanceRegistry() { shutdown(); }

void InstanceRegistry::setDispatcher(Dispatcher dispatcher)
{
    // Dispatcher replacement is a construction-time operation (typically test
    // setup or composition-root wiring). Enforce message-thread ownership to
    // match addListener/removeListener — the dispatcher feeds listener
    // callbacks, so changing it from another thread while callbacks are in
    // flight would be a layering violation.
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Reject empty callables so a caller passing `{}` or a default-constructed
    // std::function cannot brick all subsequent notifications — dispatchNotification
    // would otherwise throw std::bad_function_call and terminate.
    if (!dispatcher)
        return;

    const oscil::ScopedLock lock(dispatcherMutex_);
    dispatcher_ = std::move(dispatcher);
}

void InstanceRegistry::shutdown()
{
    // Set shutdown flag first to prevent new async notifications
    shuttingDown_.store(true, std::memory_order_release);

    // Clear all sources and listeners
    {
        const oscil::ScopedLock lock(mutex_);
        // Capture count into a local before logging — clang thread-safety
        // analysis does not propagate lock ownership into the OSCIL_LOG
        // lambda, so accessing guarded members from within the log message
        // fails -Wthread-safety even though the lock is held here.
        auto const sourceCount = sources_.size();
        OSCIL_LOG(REGISTRY, "shutdown: clearing " << sourceCount << " sources");
        sources_.clear();
        trackToSourceMap_.clear();
    }

    // Note: we don't clear listeners here because they might be static/global
    // or managed elsewhere, but we could if we wanted to be aggressive.
    // For now, just clearing sources breaks the reference cycles.
}

SourceId InstanceRegistry::tryReuseExistingSource(const juce::String& trackIdentifier,
                                                  std::shared_ptr<IAudioBuffer> captureBuffer, const juce::String& name,
                                                  int channelCount, double sampleRate,
                                                  std::shared_ptr<AnalysisEngine> analysisEngine)
{
    auto existingIt = trackToSourceMap_.find(trackIdentifier);
    if (existingIt == trackToSourceMap_.end())
        return SourceId::invalid();

    auto sourceIt = sources_.find(existingIt->second);
    if (sourceIt == sources_.end())
        return SourceId::invalid();

    sourceIt->second.buffer = captureBuffer;
    sourceIt->second.analysisEngine = analysisEngine;
    sourceIt->second.channelCount = channelCount;
    sourceIt->second.sampleRate = sampleRate;
    if (name.isNotEmpty())
        sourceIt->second.name = name;

    OSCIL_LOG(REGISTRY, "tryReuseExistingSource: reused sourceId="
                            << existingIt->second.id << " trackId=" << trackIdentifier << " name="
                            << sourceIt->second.name << " channels=" << channelCount << " sampleRate=" << sampleRate);
    return existingIt->second;
}

SourceId InstanceRegistry::insertNewSource(const juce::String& effectiveTrackId,
                                           std::shared_ptr<IAudioBuffer> captureBuffer, const juce::String& name,
                                           int channelCount, double sampleRate,
                                           std::shared_ptr<AnalysisEngine> analysisEngine)
{
    SourceId const sourceId = SourceId::generate();
    SourceInfo info;
    info.sourceId = sourceId;
    info.name = name.isEmpty() ? "Track " + juce::String(sources_.size() + 1) : name;
    info.trackIdentifier = effectiveTrackId;
    info.channelCount = channelCount;
    info.sampleRate = sampleRate;
    info.buffer = captureBuffer;
    info.analysisEngine = analysisEngine;
    info.active = true;
    sources_[sourceId] = info;
    trackToSourceMap_[effectiveTrackId] = sourceId;

    // Local for lock-analysis-friendly logging (OSCIL_LOG lambda does
    // not propagate lock ownership from enclosing scope).
    auto const totalSources = sources_.size();
    OSCIL_LOG(REGISTRY, "registerInstance: NEW id=" << sourceId.id << " name=" << info.name << " ch=" << channelCount
                                                    << " sr=" << sampleRate << " total=" << totalSources);
    return sourceId;
}

InstanceRegistry::RegisterOutcome InstanceRegistry::resolveOrInsertLocked(
    const juce::String& effectiveTrackId, const std::shared_ptr<IAudioBuffer>& captureBuffer, const juce::String& name,
    int validChannelCount, double validSampleRate, const std::shared_ptr<AnalysisEngine>& analysisEngine)
{
    RegisterOutcome outcome;
    outcome.sourceId = tryReuseExistingSource(effectiveTrackId, captureBuffer, name, validChannelCount, validSampleRate,
                                              analysisEngine);
    if (outcome.sourceId.isValid())
    {
        outcome.kind = RegisterOutcome::Kind::Updated;
        return outcome;
    }
    if (sources_.size() >= MAX_TRACKS)
    {
        OSCIL_LOG(REGISTRY, "registerInstance: REJECTED max=" << MAX_TRACKS);
        outcome.kind = RegisterOutcome::Kind::Rejected;
        return outcome;
    }
    outcome.sourceId =
        insertNewSource(effectiveTrackId, captureBuffer, name, validChannelCount, validSampleRate, analysisEngine);
    outcome.kind = RegisterOutcome::Kind::Added;
    return outcome;
}

SourceId InstanceRegistry::registerInstance(const juce::String& trackIdentifier,
                                            std::shared_ptr<IAudioBuffer> captureBuffer, const juce::String& name,
                                            int channelCount, double sampleRate,
                                            std::shared_ptr<AnalysisEngine> analysisEngine)
{
    // `captureBuffer` may be null so a later dedup re-registration can attach a
    // real buffer to the already-known source (see tryReuseExistingSource).
    jassert(channelCount > 0 && channelCount <= 2);
    jassert(sampleRate > 0.0);

    int const validChannelCount = juce::jlimit(1, 2, channelCount);
    double const validSampleRate = std::max(1.0, sampleRate);
    juce::String const effectiveTrackId =
        trackIdentifier.isEmpty() ? "__auto_" + juce::Uuid().toString() : trackIdentifier;

    RegisterOutcome outcome;
    {
        const oscil::ScopedLock lock(mutex_);
        outcome = resolveOrInsertLocked(effectiveTrackId, captureBuffer, name, validChannelCount, validSampleRate,
                                        analysisEngine);
    }

    switch (outcome.kind)
    {
        case RegisterOutcome::Kind::Added:
            notifySourceAdded(outcome.sourceId);
            break;
        case RegisterOutcome::Kind::Updated:
            notifySourceUpdated(outcome.sourceId);
            break;
        case RegisterOutcome::Kind::Rejected:
            return SourceId::invalid();
    }
    return outcome.sourceId;
}

void InstanceRegistry::unregisterInstance(const SourceId& sourceId)
{
    bool shouldNotify = false;

    {
        const oscil::ScopedLock lock(mutex_);

        auto it = sources_.find(sourceId);
        if (it == sources_.end())
        {
            OSCIL_LOG(REGISTRY, "unregisterInstance: sourceId=" << sourceId.id << " NOT FOUND");
            return;
        }

        auto const remaining = sources_.size() - 1;
        auto const nameCopy = it->second.name;
        auto const trackIdCopy = it->second.trackIdentifier;
        OSCIL_LOG(REGISTRY, "unregisterInstance: sourceId=" << sourceId.id << " name=" << nameCopy << " trackId="
                                                            << trackIdCopy << " remaining=" << remaining);
        // Remove from track map using stored identifier (O(1))
        trackToSourceMap_.erase(it->second.trackIdentifier);

        sources_.erase(it);
        shouldNotify = true;
    }

    // Notify listeners OUTSIDE the lock to prevent deadlock
    if (shouldNotify)
    {
        notifySourceRemoved(sourceId);
    }
}

std::vector<SourceInfo> InstanceRegistry::getAllSources() const
{
    const oscil::ScopedSharedLock lock(mutex_);

    std::vector<SourceInfo> result;
    result.reserve(sources_.size());

    for (const auto& [id, info] : sources_)
    {
        result.push_back(info);
    }

    return result;
}

std::optional<SourceInfo> InstanceRegistry::getSource(const SourceId& sourceId) const
{
    const oscil::ScopedSharedLock lock(mutex_);

    auto it = sources_.find(sourceId);
    if (it != sources_.end())
        return it->second;

    return std::nullopt;
}

std::shared_ptr<IAudioBuffer> InstanceRegistry::getCaptureBuffer(const SourceId& sourceId) const
{
    const oscil::ScopedSharedLock lock(mutex_);

    auto it = sources_.find(sourceId);
    if (it != sources_.end())
        return it->second.buffer.lock();

    return nullptr;
}

void InstanceRegistry::updateSource(const SourceId& sourceId, const juce::String& name, int channelCount,
                                    double sampleRate)
{
    // Sanitize inputs (jasserts omitted — sanitization is the real guard)
    int const validChannelCount = juce::jlimit(1, 2, channelCount);
    double const validSampleRate = std::max(1.0, sampleRate);

    bool shouldNotify = false;

    {
        const oscil::ScopedLock lock(mutex_);

        auto it = sources_.find(sourceId);
        if (it == sources_.end())
        {
            OSCIL_LOG(REGISTRY, "updateSource: sourceId=" << sourceId.id << " NOT FOUND");
            return;
        }

        OSCIL_LOG(REGISTRY, "updateSource: sourceId=" << sourceId.id << " name=" << name << " channels="
                                                      << validChannelCount << " sampleRate=" << validSampleRate);
        // Preserve the existing name on empty input, matching the dedup
        // re-registration path in tryReuseExistingSource. Empty strings are
        // treated as "leave unchanged" so a caller that only wants to update
        // channelCount/sampleRate cannot accidentally wipe the user-visible
        // track name.
        if (name.isNotEmpty())
            it->second.name = name;
        it->second.channelCount = validChannelCount;
        it->second.sampleRate = validSampleRate;
        shouldNotify = true;
    }

    // Notify listeners OUTSIDE the lock to prevent deadlock
    if (shouldNotify)
    {
        notifySourceUpdated(sourceId);
    }
}

size_t InstanceRegistry::getSourceCount() const
{
    const oscil::ScopedSharedLock lock(mutex_);
    return sources_.size();
}

void InstanceRegistry::addListener(InstanceRegistryListener* listener)
{
    // ListenerList::add() is NOT thread-safe. Must be called from message thread.
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    listeners_.add(listener);
}

void InstanceRegistry::removeListener(InstanceRegistryListener* listener)
{
    // ListenerList::remove() is NOT thread-safe. Must be called from message thread.
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    listeners_.remove(listener);
}

void InstanceRegistry::dispatchNotification(const char* eventName, const SourceId& sourceId,
                                            void (InstanceRegistryListener::*callback)(const SourceId&))
{
    OSCIL_LOG(REGISTRY, eventName << ": sourceId=" << sourceId.id);

    // Snapshot the dispatcher under lock so setDispatcher running concurrently
    // cannot produce a torn read of std::function (which has no thread-safe
    // copy guarantee). We invoke the snapshot outside the lock to avoid
    // holding dispatcherMutex_ across arbitrary user code.
    Dispatcher dispatcherSnapshot;
    {
        const oscil::ScopedLock lock(dispatcherMutex_);
        dispatcherSnapshot = dispatcher_;
    }
    if (!dispatcherSnapshot)
        return; // Defensive: setDispatcher guards this, but avoid UB if a future path bypasses it.
    auto weakThis = juce::WeakReference<InstanceRegistry>(this);

    dispatcherSnapshot([weakThis, sourceId, callback]() {
        auto* self = weakThis.get();
        if (!self)
            return;

        if (self->shuttingDown_.load(std::memory_order_acquire))
            return;
        self->listeners_.call([&sourceId, callback](InstanceRegistryListener& l) { (l.*callback)(sourceId); });
    });
}

void InstanceRegistry::notifySourceAdded(const SourceId& sourceId)
{
    dispatchNotification("notifySourceAdded", sourceId, &InstanceRegistryListener::sourceAdded);
}

void InstanceRegistry::notifySourceRemoved(const SourceId& sourceId)
{
    dispatchNotification("notifySourceRemoved", sourceId, &InstanceRegistryListener::sourceRemoved);
}

void InstanceRegistry::notifySourceUpdated(const SourceId& sourceId)
{
    dispatchNotification("notifySourceUpdated", sourceId, &InstanceRegistryListener::sourceUpdated);
}

} // namespace oscil
