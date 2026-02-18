/*
    Oscil - Instance Registry Implementation
*/

#include "core/InstanceRegistry.h"
#include "core/SharedCaptureBuffer.h"
#include "core/DebugLogger.h"
#include "Oscil.h"

namespace oscil
{

// Note: SourceId::generate() and InstanceId::generate() are implemented in Source.cpp

InstanceRegistry::InstanceRegistry()
{
    // Default dispatcher uses MessageManager::callAsync
    dispatcher_ = [](std::function<void()> f) {
        juce::MessageManager::callAsync(std::move(f));
    };
}

void InstanceRegistry::setDispatcher(Dispatcher dispatcher)
{
    dispatcher_ = std::move(dispatcher);
}

void InstanceRegistry::shutdown()
{
    // Set shutdown flag first to prevent new async notifications
    shuttingDown_.store(true, std::memory_order_release);

    // Clear all sources and listeners
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        sources_.clear();
        trackToSourceMap_.clear();
        instanceUUIDToSourceMap_.clear();
    }

    // Note: we don't clear listeners here because they might be static/global
    // or managed elsewhere, but we could if we wanted to be aggressive.
    // For now, just clearing sources breaks the reference cycles.
}

SourceId InstanceRegistry::registerInstance(
    const juce::String& trackIdentifier,
    std::shared_ptr<IAudioBuffer> captureBuffer,
    const juce::String& name,
    int channelCount,
    double sampleRate,
    std::shared_ptr<AnalysisEngine> analysisEngine,
    const juce::String& instanceUUID)
{
    // Registry mutations should happen on message thread or during initialization.
    // NEVER call from audio thread - uses blocking locks and heap allocation.
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    SourceId sourceId;
    bool shouldNotify = false;
    bool maxLimitReached = false;

    {
        std::unique_lock<std::shared_mutex> lock(mutex_);

        // Check if we already have a source for this track (deduplication)
        auto existingIt = trackToSourceMap_.find(trackIdentifier);
        if (existingIt != trackToSourceMap_.end())
        {
            // Return existing source ID, update buffer reference if needed
            auto sourceIt = sources_.find(existingIt->second);
            if (sourceIt != sources_.end())
            {
                sourceIt->second.buffer = captureBuffer;
                sourceIt->second.analysisEngine = analysisEngine;
                sourceIt->second.channelCount = channelCount;
                sourceIt->second.sampleRate = sampleRate;
                if (name.isNotEmpty())
                    sourceIt->second.name = name;
                // CRITICAL FIX: Always update instanceUUID when provided, not just when empty.
                // This handles the case where a plugin restores its UUID after initial registration
                // (setStateInformation may run after prepareToPlay with provisional UUID).
                if (instanceUUID.isNotEmpty())
                {
                    // Remove old UUID mapping if it exists and is different
                    if (sourceIt->second.instanceUUID.isNotEmpty() &&
                        sourceIt->second.instanceUUID != instanceUUID)
                    {
                        instanceUUIDToSourceMap_.erase(sourceIt->second.instanceUUID);
                        OSCIL_LOG("SOURCE", "registerInstance: Replacing UUID '" + sourceIt->second.instanceUUID
                                  + "' with '" + instanceUUID + "' for sourceId=" + juce::String(existingIt->second.id));
                    }
                    sourceIt->second.instanceUUID = instanceUUID;
                    instanceUUIDToSourceMap_[instanceUUID] = existingIt->second;
                }
                return existingIt->second;
            }
        }

        // Check if we've reached the maximum number of sources
        if (sources_.size() >= MAX_TRACKS)
        {
            maxLimitReached = true;
        }
        else
        {
            // Create new source
            sourceId = SourceId::generate();

            SourceInfo info;
            info.sourceId = sourceId;
            info.name = name.isEmpty() ? "Track " + juce::String(sources_.size() + 1) : name;
            info.trackIdentifier = trackIdentifier;
            info.instanceUUID = instanceUUID;
            info.channelCount = channelCount;
            info.sampleRate = sampleRate;
            info.buffer = captureBuffer;
            info.analysisEngine = analysisEngine;
            info.active = true;

            sources_[sourceId] = info;
            trackToSourceMap_[trackIdentifier] = sourceId;

            // Add to UUID map if provided, with collision detection
            if (instanceUUID.isNotEmpty())
            {
                auto existingUUID = instanceUUIDToSourceMap_.find(instanceUUID);
                if (existingUUID != instanceUUIDToSourceMap_.end() && existingUUID->second != sourceId)
                {
                    // UUID collision detected - log warning and reject duplicate
                    OSCIL_LOG_ERROR("InstanceRegistry::registerInstance",
                                    "UUID collision for " + instanceUUID + " - already registered to sourceId="
                                    + juce::String(existingUUID->second.id) + ", rejecting new registration");
                    // Clean up the source we just added
                    trackToSourceMap_.erase(trackIdentifier);
                    sources_.erase(sourceId);
                    return SourceId::invalid();
                }
                instanceUUIDToSourceMap_[instanceUUID] = sourceId;
            }

            shouldNotify = true;

            OSCIL_LOG_SOURCE("REGISTERED", juce::String(sourceId.id), info.name,
                             captureBuffer != nullptr, static_cast<int>(sources_.size()));
        }
    }

    // Return invalid ID if max limit reached
    if (maxLimitReached)
    {
        OSCIL_LOG_ERROR("InstanceRegistry::registerInstance", "MAX_TRACKS limit reached");
        return SourceId::invalid();
    }

    // Notify listeners OUTSIDE the lock to prevent deadlock
    if (shouldNotify)
    {
        notifySourceAdded(sourceId);
    }

    return sourceId;
}

void InstanceRegistry::unregisterInstance(const SourceId& sourceId)
{
    // NEVER call from audio thread - uses blocking locks.
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    bool shouldNotify = false;
    juce::String removedName;

    {
        std::unique_lock<std::shared_mutex> lock(mutex_);

        auto it = sources_.find(sourceId);
        if (it == sources_.end())
        {
            OSCIL_LOG("SOURCE", "unregisterInstance: sourceId=" + juce::String(sourceId.id) + " NOT FOUND");
            return;
        }

        removedName = it->second.name;

        // Remove from track map using stored identifier (O(1))
        trackToSourceMap_.erase(it->second.trackIdentifier);

        // Remove from UUID map if present
        if (it->second.instanceUUID.isNotEmpty())
        {
            instanceUUIDToSourceMap_.erase(it->second.instanceUUID);
        }

        sources_.erase(it);
        shouldNotify = true;

        OSCIL_LOG_SOURCE("UNREGISTERED", juce::String(sourceId.id), removedName,
                         false, static_cast<int>(sources_.size()));
    }

    // Notify listeners OUTSIDE the lock to prevent deadlock
    if (shouldNotify)
    {
        notifySourceRemoved(sourceId);
    }
}

std::vector<SourceInfo> InstanceRegistry::getAllSources() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);

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
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = sources_.find(sourceId);
    if (it != sources_.end())
        return it->second;

    return std::nullopt;
}

std::optional<SourceInfo> InstanceRegistry::getSourceByName(const juce::String& name) const
{
    if (name.isEmpty())
        return std::nullopt;

    std::shared_lock<std::shared_mutex> lock(mutex_);

    // Case-insensitive search for matching name
    for (const auto& [id, info] : sources_)
    {
        if (info.name.equalsIgnoreCase(name))
        {
            OSCIL_LOG("SOURCE", "getSourceByName: FOUND name='" + name + "' -> sourceId=" + juce::String(id.id));
            return info;
        }
    }

    OSCIL_LOG("SOURCE", "getSourceByName: NOT FOUND name='" + name + "' (searched " + juce::String(static_cast<int>(sources_.size())) + " sources)");
    return std::nullopt;
}

std::optional<SourceInfo> InstanceRegistry::getSourceByInstanceUUID(const juce::String& uuid) const
{
    if (uuid.isEmpty())
        return std::nullopt;

    std::shared_lock<std::shared_mutex> lock(mutex_);

    // O(1) lookup using the UUID map
    auto it = instanceUUIDToSourceMap_.find(uuid);
    if (it != instanceUUIDToSourceMap_.end())
    {
        auto sourceIt = sources_.find(it->second);
        if (sourceIt != sources_.end())
        {
            OSCIL_LOG("SOURCE", "getSourceByInstanceUUID: FOUND uuid='" + uuid + "' -> sourceId=" + juce::String(it->second.id));
            return sourceIt->second;
        }
    }

    OSCIL_LOG("SOURCE", "getSourceByInstanceUUID: NOT FOUND uuid='" + uuid + "'");
    return std::nullopt;
}

std::shared_ptr<IAudioBuffer> InstanceRegistry::getCaptureBuffer(const SourceId& sourceId) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = sources_.find(sourceId);
    if (it != sources_.end())
    {
        auto buffer = it->second.buffer.lock();
        bool lockSuccess = (buffer != nullptr);
        OSCIL_LOG_BUFFER("InstanceRegistry::getCaptureBuffer", juce::String(sourceId.id),
                         true, lockSuccess, "name=" + it->second.name);
        if (!buffer)
        {
            OSCIL_LOG_ERROR("InstanceRegistry::getCaptureBuffer",
                           "weak_ptr.lock() FAILED for sourceId=" + juce::String(sourceId.id)
                           + " (name=" + it->second.name + ") - buffer was destroyed");
        }
        return buffer;
    }

    OSCIL_LOG_BUFFER("InstanceRegistry::getCaptureBuffer", juce::String(sourceId.id),
                     false, false, "source NOT FOUND in registry");
    return nullptr;
}

void InstanceRegistry::updateSource(const SourceId& sourceId, const juce::String& name, int channelCount, double sampleRate)
{
    // NEVER call from audio thread - uses blocking locks.
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    bool shouldNotify = false;

    {
        std::unique_lock<std::shared_mutex> lock(mutex_);

        auto it = sources_.find(sourceId);
        if (it == sources_.end())
            return;

        it->second.name = name;
        it->second.channelCount = channelCount;
        it->second.sampleRate = sampleRate;
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
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return sources_.size();
}

void InstanceRegistry::addListener(IInstanceRegistryListener* listener)
{
    // ListenerList::add() is NOT thread-safe. Must be called from message thread.
    // Runtime check - not just debug assertion
    if (!juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        jassertfalse; // Still assert in Debug for development
        return;       // But also guard in Release
    }
    listeners_.add(listener);
}

void InstanceRegistry::removeListener(IInstanceRegistryListener* listener)
{
    // ListenerList::remove() is NOT thread-safe. Must be called from message thread.
    // Runtime check - not just debug assertion
    if (!juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        jassertfalse; // Still assert in Debug for development
        return;       // But also guard in Release
    }
    listeners_.remove(listener);
}

void InstanceRegistry::notifySourceAdded(const SourceId& sourceId)
{
    // Increment sequence number to track this operation
    uint64_t capturedSeq = operationSequence_.fetch_add(1, std::memory_order_acq_rel);

    // Use injected dispatcher (defaults to MessageManager::callAsync)
    // Capture WeakReference to prevent use-after-free if registry is destroyed before callback runs
    // Capture sequence number to detect stale notifications
    dispatcher_([weakThis = juce::WeakReference<InstanceRegistry>(this), sourceId, capturedSeq]() {
        if (auto* self = weakThis.get())
        {
            // Guard against use-after-free during shutdown
            if (self->shuttingDown_.load(std::memory_order_acquire))
                return;

            // Check if source still exists (stale notification detection)
            // The sequence number ensures we don't process notifications for sources
            // that were added then removed before the async callback ran
            {
                std::shared_lock<std::shared_mutex> lock(self->mutex_);
                if (self->sources_.find(sourceId) == self->sources_.end())
                    return;  // Source was removed before notification could be delivered
            }

            self->listeners_.call([&sourceId](IInstanceRegistryListener& l) {
                l.sourceAdded(sourceId);
            });
        }
    });
}

void InstanceRegistry::notifySourceRemoved(const SourceId& sourceId)
{
    // Increment sequence number to track this operation
    operationSequence_.fetch_add(1, std::memory_order_acq_rel);

    // Capture WeakReference to prevent use-after-free if registry is destroyed before callback runs
    // For removal notifications, we don't check if source exists (it was just removed)
    // but we still use sequence tracking for consistency
    dispatcher_([weakThis = juce::WeakReference<InstanceRegistry>(this), sourceId]() {
        if (auto* self = weakThis.get())
        {
            // Guard against use-after-free during shutdown
            if (self->shuttingDown_.load(std::memory_order_acquire))
                return;
            self->listeners_.call([&sourceId](IInstanceRegistryListener& l) {
                l.sourceRemoved(sourceId);
            });
        }
    });
}

void InstanceRegistry::notifySourceUpdated(const SourceId& sourceId)
{
    // Increment sequence number to track this operation
    operationSequence_.fetch_add(1, std::memory_order_acq_rel);

    // Capture WeakReference to prevent use-after-free if registry is destroyed before callback runs
    dispatcher_([weakThis = juce::WeakReference<InstanceRegistry>(this), sourceId]() {
        if (auto* self = weakThis.get())
        {
            // Guard against use-after-free during shutdown
            if (self->shuttingDown_.load(std::memory_order_acquire))
                return;

            // Check if source still exists (stale notification detection)
            {
                std::shared_lock<std::shared_mutex> lock(self->mutex_);
                if (self->sources_.find(sourceId) == self->sources_.end())
                    return;  // Source was removed before notification could be delivered
            }

            self->listeners_.call([&sourceId](IInstanceRegistryListener& l) {
                l.sourceUpdated(sourceId);
            });
        }
    });
}

} // namespace oscil
