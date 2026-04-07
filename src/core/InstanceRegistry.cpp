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
    // Default dispatcher uses MessageManager::callAsync
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

void InstanceRegistry::setDispatcher(Dispatcher dispatcher) { dispatcher_ = std::move(dispatcher); }

void InstanceRegistry::shutdown()
{
    OSCIL_LOG(REGISTRY, "shutdown: clearing " << sources_.size() << " sources");
    // Set shutdown flag first to prevent new async notifications
    shuttingDown_.store(true, std::memory_order_release);

    // Clear all sources and listeners
    {
        std::unique_lock<std::shared_mutex> const lock(mutex_);
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity,readability-function-size)
SourceId InstanceRegistry::registerInstance(const juce::String& trackIdentifier,
                                            std::shared_ptr<IAudioBuffer> captureBuffer, const juce::String& name,
                                            int channelCount, double sampleRate,
                                            std::shared_ptr<AnalysisEngine> analysisEngine)
{
    // Preconditions
    jassert(captureBuffer != nullptr);
    jassert(trackIdentifier.isNotEmpty());
    jassert(channelCount > 0 && channelCount <= 2);
    jassert(sampleRate > 0.0);

    // Sanitize inputs: clamp channelCount and sampleRate to valid ranges
    int const validChannelCount = juce::jlimit(1, 2, channelCount);
    double const validSampleRate = std::max(1.0, sampleRate);

    SourceId sourceId = SourceId::invalid();
    bool shouldNotifyAdded = false;
    bool shouldNotifyUpdated = false;

    {
        std::unique_lock<std::shared_mutex> const lock(mutex_);

        // Empty track identifiers bypass deduplication to prevent unrelated sources colliding
        juce::String const effectiveTrackId =
            trackIdentifier.isEmpty() ? "__auto_" + juce::Uuid().toString() : trackIdentifier;

        sourceId = tryReuseExistingSource(effectiveTrackId, captureBuffer, name, validChannelCount, validSampleRate,
                                          analysisEngine);
        if (sourceId.isValid())
        {
            shouldNotifyUpdated = true;
        }
        else if (sources_.size() >= MAX_TRACKS)
        {
            OSCIL_LOG(REGISTRY, "registerInstance: REJECTED max=" << MAX_TRACKS);
            return SourceId::invalid();
        }
        else
        {
            sourceId = SourceId::generate();
            SourceInfo info;
            info.sourceId = sourceId;
            info.name = name.isEmpty() ? "Track " + juce::String(sources_.size() + 1) : name;
            info.trackIdentifier = effectiveTrackId;
            info.channelCount = validChannelCount;
            info.sampleRate = validSampleRate;
            info.buffer = captureBuffer;
            info.analysisEngine = analysisEngine;
            info.active = true;
            sources_[sourceId] = info;
            trackToSourceMap_[effectiveTrackId] = sourceId;
            shouldNotifyAdded = true;
            OSCIL_LOG(REGISTRY, "registerInstance: NEW id=" << sourceId.id << " name=" << info.name
                                                            << " ch=" << validChannelCount << " sr=" << validSampleRate
                                                            << " total=" << sources_.size());
        }
    }

    if (shouldNotifyAdded)
        notifySourceAdded(sourceId);
    else if (shouldNotifyUpdated)
        notifySourceUpdated(sourceId);

    return sourceId;
}

void InstanceRegistry::unregisterInstance(const SourceId& sourceId)
{
    bool shouldNotify = false;

    {
        std::unique_lock<std::shared_mutex> const lock(mutex_);

        auto it = sources_.find(sourceId);
        if (it == sources_.end())
        {
            OSCIL_LOG(REGISTRY, "unregisterInstance: sourceId=" << sourceId.id << " NOT FOUND");
            return;
        }

        OSCIL_LOG(REGISTRY, "unregisterInstance: sourceId=" << sourceId.id << " name=" << it->second.name
                                                            << " trackId=" << it->second.trackIdentifier
                                                            << " remaining=" << (sources_.size() - 1));
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
    std::shared_lock<std::shared_mutex> const lock(mutex_);

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
    std::shared_lock<std::shared_mutex> const lock(mutex_);

    auto it = sources_.find(sourceId);
    if (it != sources_.end())
        return it->second;

    return std::nullopt;
}

std::shared_ptr<IAudioBuffer> InstanceRegistry::getCaptureBuffer(const SourceId& sourceId) const
{
    std::shared_lock<std::shared_mutex> const lock(mutex_);

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
        std::unique_lock<std::shared_mutex> const lock(mutex_);

        auto it = sources_.find(sourceId);
        if (it == sources_.end())
        {
            OSCIL_LOG(REGISTRY, "updateSource: sourceId=" << sourceId.id << " NOT FOUND");
            return;
        }

        OSCIL_LOG(REGISTRY, "updateSource: sourceId=" << sourceId.id << " name=" << name << " channels="
                                                      << validChannelCount << " sampleRate=" << validSampleRate);
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
    std::shared_lock<std::shared_mutex> const lock(mutex_);
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
    auto weakThis = juce::WeakReference<InstanceRegistry>(this);

    dispatcher_([weakThis, sourceId, callback]() {
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
