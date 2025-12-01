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
        juce::MessageManager::callAsync(std::move(f));
    };
}

void InstanceRegistry::setDispatcher(Dispatcher dispatcher)
{
    dispatcher_ = std::move(dispatcher);
}

InstanceRegistry& InstanceRegistry::getInstance()
{
    static InstanceRegistry instance;
    return instance;
}

SourceId InstanceRegistry::registerInstance(
    const juce::String& trackIdentifier,
    std::shared_ptr<SharedCaptureBuffer> captureBuffer,
    const juce::String& name,
    int channelCount,
    double sampleRate)
{
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
                sourceIt->second.channelCount = channelCount;
                sourceIt->second.sampleRate = sampleRate;
                if (name.isNotEmpty())
                    sourceIt->second.name = name;
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
            info.channelCount = channelCount;
            info.sampleRate = sampleRate;
            info.buffer = captureBuffer;
            info.active = true;

            sources_[sourceId] = info;
            trackToSourceMap_[trackIdentifier] = sourceId;
            shouldNotify = true;
        }
    }

    // Log outside mutex to avoid I/O while holding lock
    if (maxLimitReached)
    {
        DBG("InstanceRegistry: Maximum source limit (" << MAX_TRACKS << ") reached");
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
    bool shouldNotify = false;

    {
        std::unique_lock<std::shared_mutex> lock(mutex_);

        auto it = sources_.find(sourceId);
        if (it == sources_.end())
            return;

        // Remove from track map
        for (auto trackIt = trackToSourceMap_.begin(); trackIt != trackToSourceMap_.end(); ++trackIt)
        {
            if (trackIt->second == sourceId)
            {
                trackToSourceMap_.erase(trackIt);
                break;
            }
        }

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

std::shared_ptr<SharedCaptureBuffer> InstanceRegistry::getCaptureBuffer(const SourceId& sourceId) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = sources_.find(sourceId);
    if (it != sources_.end())
        return it->second.buffer.lock();

    return nullptr;
}

void InstanceRegistry::updateSource(const SourceId& sourceId, const juce::String& name, int channelCount, double sampleRate)
{
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
    // Use injected dispatcher (defaults to MessageManager::callAsync)
    dispatcher_([this, sourceId]() {
        listeners_.call([&sourceId](InstanceRegistryListener& l) {
            l.sourceAdded(sourceId);
        });
    });
}

void InstanceRegistry::notifySourceRemoved(const SourceId& sourceId)
{
    dispatcher_([this, sourceId]() {
        listeners_.call([&sourceId](InstanceRegistryListener& l) {
            l.sourceRemoved(sourceId);
        });
    });
}

void InstanceRegistry::notifySourceUpdated(const SourceId& sourceId)
{
    dispatcher_([this, sourceId]() {
        listeners_.call([&sourceId](InstanceRegistryListener& l) {
            l.sourceUpdated(sourceId);
        });
    });
}

} // namespace oscil
