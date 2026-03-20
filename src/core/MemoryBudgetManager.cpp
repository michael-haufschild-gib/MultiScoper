/*
    Oscil - Memory Budget Manager Implementation
*/

#include "core/MemoryBudgetManager.h"
#include "core/DecimatingCaptureBuffer.h"

namespace oscil
{

MemoryBudgetManager::MemoryBudgetManager() = default;

MemoryBudgetManager::~MemoryBudgetManager()
{
    shuttingDown_.store(true, std::memory_order_release);
}

void MemoryBudgetManager::setGlobalConfig(const CaptureQualityConfig& config, int sourceRate)
{
    // NEVER call from audio thread - triggers buffer reconfiguration with heap allocation.
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    bool configChanged = (globalConfig_ != config) || (sourceRate_ != sourceRate);

    globalConfig_ = config;
    sourceRate_ = sourceRate;

    if (configChanged)
    {
        reconfigureAllBuffers();
        usageCacheDirty_ = true;
        notifyMemoryUsageChanged();
    }
}

void MemoryBudgetManager::registerBuffer(const juce::String& id,
                                          std::shared_ptr<DecimatingCaptureBuffer> buffer)
{
    // NEVER call from audio thread - uses blocking locks.
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (!buffer || id.isEmpty())
        return;

    {
        std::scoped_lock lock(buffersMutex_);
        pruneExpiredBuffersLocked();

        BufferInfo info;
        info.id = id;
        info.buffer = buffer;
        info.qualityOverride = QualityOverride::UseGlobal;
        info.lastKnownMemoryBytes = buffer->getMemoryUsageBytes();

        buffers_[id] = info;
        usageCacheDirty_ = true;
    }

    notifyBufferCountChanged();
    notifyMemoryUsageChanged();

    // Check if quality needs adjustment
    QualityPreset recommended = getRecommendedQuality();
    if (recommended != lastEffectiveQuality_ && globalConfig_.autoAdjustQuality)
    {
        lastEffectiveQuality_ = recommended;
        notifyEffectiveQualityChanged(recommended);
    }
}

void MemoryBudgetManager::unregisterBuffer(const juce::String& id)
{
    // NEVER call from audio thread - uses blocking locks.
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    bool removed = false;

    {
        std::scoped_lock lock(buffersMutex_);
        removed = buffers_.erase(id) > 0;
        if (removed)
            usageCacheDirty_ = true;
    }

    if (removed)
    {
        notifyBufferCountChanged();
        notifyMemoryUsageChanged();
    }
}

void MemoryBudgetManager::setBufferQualityOverride(const juce::String& id, QualityOverride override)
{
    // NEVER call from audio thread - uses blocking locks and triggers buffer reconfiguration.
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    std::scoped_lock lock(buffersMutex_);
    pruneExpiredBuffersLocked();

    auto it = buffers_.find(id);
    if (it != buffers_.end())
    {
        it->second.qualityOverride = override;

        // Reconfigure this buffer with new quality
        if (auto buffer = it->second.buffer.lock())
        {
            QualityPreset effectivePreset;
            if (override != QualityOverride::UseGlobal)
            {
                effectivePreset = resolveQualityOverride(override, globalConfig_.qualityPreset);
            }
            else if (globalConfig_.autoAdjustQuality)
            {
                effectivePreset = getRecommendedQualityForCount(static_cast<int>(buffers_.size()));
            }
            else
            {
                effectivePreset = globalConfig_.qualityPreset;
            }

            CaptureQualityConfig bufferConfig = globalConfig_;
            bufferConfig.qualityPreset = effectivePreset;
            buffer->configure(bufferConfig, sourceRate_);

            it->second.lastKnownMemoryBytes = buffer->getMemoryUsageBytes();
            usageCacheDirty_ = true;
        }
    }
}

QualityOverride MemoryBudgetManager::getBufferQualityOverride(const juce::String& id) const
{
    std::scoped_lock lock(buffersMutex_);

    auto it = buffers_.find(id);
    if (it != buffers_.end())
        return it->second.qualityOverride;

    return QualityOverride::UseGlobal;
}

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
        snapshot.usagePercent = (static_cast<float>(snapshot.totalUsageBytes) /
                                  static_cast<float>(snapshot.budgetBytes)) * 100.0f;
    }

    snapshot.isOverBudget = snapshot.totalUsageBytes > snapshot.budgetBytes;

    return snapshot;
}

juce::String MemoryBudgetManager::getTotalMemoryUsageString() const
{
    size_t bytes = getTotalMemoryUsage();

    if (bytes >= 1024 * 1024)
    {
        float mb = static_cast<float>(bytes) / (1024.0f * 1024.0f);
        return juce::String(mb, 1) + " MB";
    }
    else if (bytes >= 1024)
    {
        float kb = static_cast<float>(bytes) / 1024.0f;
        return juce::String(kb, 0) + " KB";
    }
    else
    {
        return juce::String(bytes) + " B";
    }
}

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

void MemoryBudgetManager::applyRecommendedQuality()
{
    std::scoped_lock lock(buffersMutex_);
    pruneExpiredBuffersLocked();
    QualityPreset recommended = getRecommendedQualityForCount(static_cast<int>(buffers_.size()));

    for (auto& [id, info] : buffers_)
    {
        // Skip buffers with explicit overrides
        if (info.qualityOverride != QualityOverride::UseGlobal)
            continue;

        if (auto buffer = info.buffer.lock())
        {
            CaptureQualityConfig bufferConfig = globalConfig_;
            bufferConfig.qualityPreset = recommended;
            buffer->configure(bufferConfig, sourceRate_);

            info.lastKnownMemoryBytes = buffer->getMemoryUsageBytes();
        }
    }

    usageCacheDirty_ = true;

    if (recommended != lastEffectiveQuality_)
    {
        lastEffectiveQuality_ = recommended;
        notifyEffectiveQualityChanged(recommended);
    }
}

void MemoryBudgetManager::reconfigureAllBuffers()
{
    std::scoped_lock lock(buffersMutex_);
    pruneExpiredBuffersLocked();

    for (auto& [id, info] : buffers_)
    {
        if (auto buffer = info.buffer.lock())
        {
            QualityPreset effectivePreset;

            if (info.qualityOverride != QualityOverride::UseGlobal)
            {
                effectivePreset = resolveQualityOverride(info.qualityOverride, globalConfig_.qualityPreset);
            }
            else if (globalConfig_.autoAdjustQuality)
            {
                effectivePreset = globalConfig_.getEffectiveQuality(
                    static_cast<int>(buffers_.size()), sourceRate_);
            }
            else
            {
                effectivePreset = globalConfig_.qualityPreset;
            }

            CaptureQualityConfig bufferConfig = globalConfig_;
            bufferConfig.qualityPreset = effectivePreset;
            buffer->configure(bufferConfig, sourceRate_);

            info.lastKnownMemoryBytes = buffer->getMemoryUsageBytes();
        }
    }

    usageCacheDirty_ = true;
}

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

bool MemoryBudgetManager::pruneExpiredBuffersLocked() const
{
    bool removed = false;

    for (auto it = buffers_.begin(); it != buffers_.end(); )
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

    auto dispatch = [weakThis = juce::WeakReference<MemoryBudgetManager>(this),
                     callback = std::move(callback)]() mutable {
        auto* self = weakThis.get();
        if (self == nullptr || self->shuttingDown_.load(std::memory_order_acquire))
            return;

        callback(*self);
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
