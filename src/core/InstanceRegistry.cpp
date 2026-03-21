/*
    Oscil - Instance Registry Implementation
*/

#include "core/InstanceRegistry.h"
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

InstanceRegistry::~InstanceRegistry()
{
    shutdown();
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
    }

    // Note: we don't clear listeners here because they might be static/global
    // or managed elsewhere, but we could if we wanted to be aggressive.
    // For now, just clearing sources breaks the reference cycles.
}

SourceId InstanceRegistry::tryReuseExistingSource(
    const juce::String& trackIdentifier,
    std::shared_ptr<IAudioBuffer> captureBuffer,
    const juce::String& name,
    int channelCount,
    double sampleRate,
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

    return existingIt->second;
}

SourceId InstanceRegistry::registerInstance(
    const juce::String& trackIdentifier,
    std::shared_ptr<IAudioBuffer> captureBuffer,
    const juce::String& name,
    int channelCount,
    double sampleRate,
    std::shared_ptr<AnalysisEngine> analysisEngine)
{
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    SourceId sourceId = SourceId::invalid();
    bool shouldNotifyAdded = false;
    bool shouldNotifyUpdated = false;

    {
        std::unique_lock<std::shared_mutex> lock(mutex_);

        sourceId = tryReuseExistingSource(trackIdentifier, captureBuffer, name,
                                           channelCount, sampleRate, analysisEngine);
        if (sourceId.isValid())
        {
            shouldNotifyUpdated = true;
        }
        else if (sources_.size() >= MAX_TRACKS)
        {
            return SourceId::invalid();
        }
        else
        {
            sourceId = SourceId::generate();
            SourceInfo info;
            info.sourceId = sourceId;
            info.name = name.isEmpty() ? "Track " + juce::String(sources_.size() + 1) : name;
            info.trackIdentifier = trackIdentifier;
            info.channelCount = channelCount;
            info.sampleRate = sampleRate;
            info.buffer = captureBuffer;
            info.analysisEngine = analysisEngine;
            info.active = true;
            sources_[sourceId] = info;
            trackToSourceMap_[trackIdentifier] = sourceId;
            shouldNotifyAdded = true;
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
    // NEVER call from audio thread - uses blocking locks.
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    bool shouldNotify = false;

    {
        std::unique_lock<std::shared_mutex> lock(mutex_);

        auto it = sources_.find(sourceId);
        if (it == sources_.end())
            return;

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

std::shared_ptr<IAudioBuffer> InstanceRegistry::getCaptureBuffer(const SourceId& sourceId) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = sources_.find(sourceId);
    if (it != sources_.end())
        return it->second.buffer.lock();

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

void InstanceRegistry::notifySourceAdded(const SourceId& sourceId)
{
    auto weakThis = juce::WeakReference<InstanceRegistry>(this);

    // Use injected dispatcher (defaults to MessageManager::callAsync)
    dispatcher_([weakThis, sourceId]() {
        auto* self = weakThis.get();
        if (!self)
            return;

        // Guard against use-after-free during shutdown
        if (self->shuttingDown_.load(std::memory_order_acquire))
            return;
        self->listeners_.call([&sourceId](InstanceRegistryListener& l) {
            l.sourceAdded(sourceId);
        });
    });
}

void InstanceRegistry::notifySourceRemoved(const SourceId& sourceId)
{
    auto weakThis = juce::WeakReference<InstanceRegistry>(this);

    dispatcher_([weakThis, sourceId]() {
        auto* self = weakThis.get();
        if (!self)
            return;

        // Guard against use-after-free during shutdown
        if (self->shuttingDown_.load(std::memory_order_acquire))
            return;
        self->listeners_.call([&sourceId](InstanceRegistryListener& l) {
            l.sourceRemoved(sourceId);
        });
    });
}

void InstanceRegistry::notifySourceUpdated(const SourceId& sourceId)
{
    auto weakThis = juce::WeakReference<InstanceRegistry>(this);

    dispatcher_([weakThis, sourceId]() {
        auto* self = weakThis.get();
        if (!self)
            return;

        // Guard against use-after-free during shutdown
        if (self->shuttingDown_.load(std::memory_order_acquire))
            return;
        self->listeners_.call([&sourceId](InstanceRegistryListener& l) {
            l.sourceUpdated(sourceId);
        });
    });
}

} // namespace oscil
