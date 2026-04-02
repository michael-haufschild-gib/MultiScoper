/*
    Oscil - Memory Budget Manager Read/Query Methods
    All read-only queries, snapshot generation, listener management,
    and internal helpers that support both the query and mutation paths.
*/

#include "core/DecimatingCaptureBuffer.h"
#include "core/MemoryBudgetManager.h"

namespace oscil
{

//==============================================================================
// Buffer Queries
//==============================================================================

int MemoryBudgetManager::getBufferCount() const
{
    std::scoped_lock lock(buffersMutex_);
    pruneExpiredBuffersLocked();
    return static_cast<int>(buffers_.size());
}

bool MemoryBudgetManager::isBufferRegistered(const juce::String& id) const
{
    std::scoped_lock lock(buffersMutex_);
    pruneExpiredBuffersLocked();
    return buffers_.find(id) != buffers_.end();
}

//==============================================================================
// Memory Tracking Queries
//==============================================================================

size_t MemoryBudgetManager::getTotalMemoryUsage() const
{
    std::scoped_lock lock(buffersMutex_);
    pruneExpiredBuffersLocked();

    if (usageCacheDirty_)
    {
        updateCachedUsage();
    }
    return cachedTotalUsage_;
}

size_t MemoryBudgetManager::getBufferMemoryUsage(const juce::String& id) const
{
    std::scoped_lock lock(buffersMutex_);
    pruneExpiredBuffersLocked();

    auto it = buffers_.find(id);
    if (it != buffers_.end())
    {
        if (auto buffer = it->second.buffer.lock())
            return buffer->getMemoryUsageBytes();
        return it->second.lastKnownMemoryBytes;
    }

    return 0;
}

MemoryUsageSnapshot MemoryBudgetManager::getMemorySnapshot() const
{
    MemoryUsageSnapshot snapshot;

    std::scoped_lock lock(buffersMutex_);
    pruneExpiredBuffersLocked();

    snapshot.budgetBytes = globalConfig_.memoryBudget.totalBudgetBytes;
    snapshot.numBuffers = static_cast<int>(buffers_.size());
    if (globalConfig_.autoAdjustQuality)
    {
        // Use internal method to avoid lock re-entry (we already hold buffersMutex_).
        snapshot.effectiveQuality = getRecommendedQualityForCount(snapshot.numBuffers);
    }
    else
    {
        snapshot.effectiveQuality = globalConfig_.qualityPreset;
    }

    for (const auto& [id, info] : buffers_)
    {
        MemoryUsageSnapshot::BufferUsage usage;
        usage.id = id;
        usage.hasOverride = (info.qualityOverride != QualityOverride::UseGlobal);

        if (auto buffer = info.buffer.lock())
        {
            usage.memoryBytes = buffer->getMemoryUsageBytes();
            usage.quality = buffer->getConfig().qualityPreset;
        }
        else
        {
            usage.memoryBytes = info.lastKnownMemoryBytes;
            usage.quality = globalConfig_.qualityPreset;
        }

        snapshot.totalUsageBytes += usage.memoryBytes;
        snapshot.perBufferUsage.push_back(usage);
    }

    if (snapshot.budgetBytes > 0)
    {
        snapshot.usagePercent =
            (static_cast<float>(snapshot.totalUsageBytes) / static_cast<float>(snapshot.budgetBytes)) * 100.0f;
    }

    snapshot.isOverBudget = snapshot.totalUsageBytes > snapshot.budgetBytes;

    return snapshot;
}

juce::String MemoryBudgetManager::getTotalMemoryUsageString() const { return formatBytes(getTotalMemoryUsage()); }

bool MemoryBudgetManager::isOverBudget() const
{
    return getTotalMemoryUsage() > globalConfig_.memoryBudget.totalBudgetBytes;
}

float MemoryBudgetManager::getUsagePercent() const
{
    size_t budget = globalConfig_.memoryBudget.totalBudgetBytes;
    if (budget == 0)
        return 0.0f;

    return (static_cast<float>(getTotalMemoryUsage()) / static_cast<float>(budget)) * 100.0f;
}

//==============================================================================
// Quality Queries
//==============================================================================

QualityPreset MemoryBudgetManager::getRecommendedQuality() const
{
    int numBuffers = getBufferCount();
    return getRecommendedQualityForCount(numBuffers);
}

QualityPreset MemoryBudgetManager::getRecommendedQualityForCount(int numBuffers) const
{
    float durationSec = bufferDurationToSeconds(globalConfig_.bufferDuration);
    return globalConfig_.memoryBudget.calculateRecommendedPreset(numBuffers, durationSec);
}

QualityPreset MemoryBudgetManager::getEffectiveQuality(const juce::String& id) const
{
    std::scoped_lock lock(buffersMutex_);
    pruneExpiredBuffersLocked();

    auto it = buffers_.find(id);
    if (it != buffers_.end())
    {
        if (it->second.qualityOverride != QualityOverride::UseGlobal)
        {
            return resolveQualityOverride(it->second.qualityOverride, globalConfig_.qualityPreset);
        }
    }

    // Use recommended quality (considering auto-adjust based on memory budget)
    if (globalConfig_.autoAdjustQuality)
    {
        return getRecommendedQualityForCount(static_cast<int>(buffers_.size()));
    }

    return globalConfig_.qualityPreset;
}

//==============================================================================
// Listeners
//==============================================================================

void MemoryBudgetManager::addListener(Listener* listener)
{
    // ListenerList::add() is NOT thread-safe. Must be called from message thread.
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    listeners_.add(listener);
}

void MemoryBudgetManager::removeListener(Listener* listener)
{
    // ListenerList::remove() is NOT thread-safe. Must be called from message thread.
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    listeners_.remove(listener);
}

//==============================================================================
// Maintenance
//==============================================================================

void MemoryBudgetManager::pruneExpiredBuffers()
{
    std::scoped_lock lock(buffersMutex_);
    pruneExpiredBuffersLocked();
}

void MemoryBudgetManager::refreshMemoryUsage()
{
    {
        std::scoped_lock lock(buffersMutex_);
        usageCacheDirty_ = true;
        updateCachedUsage();
    }
    notifyMemoryUsageChanged();
}

//==============================================================================
// Internal Helpers
//==============================================================================

bool MemoryBudgetManager::pruneExpiredBuffersLocked() const
{
    bool removed = false;

    for (auto it = buffers_.begin(); it != buffers_.end();)
    {
        if (it->second.buffer.expired())
        {
            it = buffers_.erase(it);
            removed = true;
        }
        else
        {
            ++it;
        }
    }

    if (removed)
        usageCacheDirty_ = true;

    return removed;
}

void MemoryBudgetManager::updateCachedUsage() const
{
    // REQUIRES: buffersMutex_ must already be held by caller
    // This is an internal helper that assumes the lock is already acquired.

    cachedTotalUsage_ = 0;

    for (auto& [id, info] : buffers_)
    {
        if (auto buffer = info.buffer.lock())
        {
            info.lastKnownMemoryBytes = buffer->getMemoryUsageBytes();
        }
        cachedTotalUsage_ += info.lastKnownMemoryBytes;
    }

    usageCacheDirty_ = false;
}

void MemoryBudgetManager::postNotification(std::function<void(MemoryBudgetManager&)> callback)
{
    if (shuttingDown_.load(std::memory_order_acquire))
        return;

    auto* messageManager = juce::MessageManager::getInstanceWithoutCreating();
    if (messageManager == nullptr)
        return;

    auto dispatch = [weakThis = juce::WeakReference<MemoryBudgetManager>(this), cb = std::move(callback)]() mutable {
        auto* self = weakThis.get();
        if (self == nullptr || self->shuttingDown_.load(std::memory_order_acquire))
            return;

        cb(*self);
    };

    if (messageManager->isThisTheMessageThread())
        dispatch();
    else
        juce::MessageManager::callAsync(std::move(dispatch));
}

void MemoryBudgetManager::notifyMemoryUsageChanged()
{
    postNotification([](MemoryBudgetManager& self) {
        auto snapshot = self.getMemorySnapshot();
        self.listeners_.call([&snapshot](Listener& listener) { listener.memoryUsageChanged(snapshot); });
    });
}

void MemoryBudgetManager::notifyBufferCountChanged()
{
    postNotification([](MemoryBudgetManager& self) {
        int count = self.getBufferCount();
        self.listeners_.call([count](Listener& listener) { listener.bufferCountChanged(count); });
    });
}

void MemoryBudgetManager::notifyEffectiveQualityChanged(QualityPreset newQuality)
{
    postNotification([newQuality](MemoryBudgetManager& self) {
        self.listeners_.call([newQuality](Listener& listener) { listener.effectiveQualityChanged(newQuality); });
    });
}

} // namespace oscil
