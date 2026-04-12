/*
    Oscil - Memory Budget Manager Implementation
*/

#include "core/MemoryBudgetManager.h"

#include "core/DecimatingCaptureBuffer.h"

namespace oscil
{

MemoryBudgetManager::MemoryBudgetManager() = default;

MemoryBudgetManager::~MemoryBudgetManager() { shuttingDown_.store(true, std::memory_order_release); }

void MemoryBudgetManager::setGlobalConfig(const CaptureQualityConfig& config, int sourceRate)
{
    // NEVER call from audio thread - triggers buffer reconfiguration with heap allocation.
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    bool const configChanged = [&]() {
        std::scoped_lock const lock(buffersMutex_);
        bool const changed = (globalConfig_ != config) || (sourceRate_ != sourceRate);
        globalConfig_ = config;
        sourceRate_ = sourceRate;
        return changed;
    }();

    if (configChanged)
    {
        reconfigureAllBuffers();
        // reconfigureAllBuffers() already sets usageCacheDirty_ under lock.
        notifyMemoryUsageChanged();
    }
}

void MemoryBudgetManager::registerBuffer(const juce::String& id, std::shared_ptr<DecimatingCaptureBuffer> buffer)
{
    // NEVER call from audio thread - uses blocking locks.
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (!buffer || id.isEmpty())
        return;

    {
        std::scoped_lock const lock(buffersMutex_);
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

    // Notify memory usage *after* auto-quality adjustment so listeners see the
    // final buffer sizes, not a stale pre-adjustment snapshot.
    if (globalConfig_.autoAdjustQuality)
        applyRecommendedQuality();

    notifyMemoryUsageChanged();
}

void MemoryBudgetManager::unregisterBuffer(const juce::String& id)
{
    // NEVER call from audio thread - uses blocking locks.
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    bool const removed = [&]() {
        std::scoped_lock const lock(buffersMutex_);
        bool const erased = buffers_.erase(id) > 0;
        if (erased)
            usageCacheDirty_ = true;
        return erased;
    }();

    if (removed)
    {
        notifyBufferCountChanged();

        if (globalConfig_.autoAdjustQuality)
            applyRecommendedQuality();

        notifyMemoryUsageChanged();
    }
}

void MemoryBudgetManager::setBufferQualityOverride(const juce::String& id, QualityOverride override)
{
    // NEVER call from audio thread - uses blocking locks and triggers buffer reconfiguration.
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    std::shared_ptr<DecimatingCaptureBuffer> bufferToReconfigure;
    CaptureQualityConfig bufferConfig;
    int srcRate = 0;

    {
        std::scoped_lock const lock(buffersMutex_);
        pruneExpiredBuffersLocked();

        auto it = buffers_.find(id);
        if (it == buffers_.end())
            return;

        it->second.qualityOverride = override;

        bufferToReconfigure = it->second.buffer.lock();
        if (!bufferToReconfigure)
            return;

        QualityPreset const effectivePreset =
            (override != QualityOverride::UseGlobal)
                ? resolveQualityOverride(override, globalConfig_.qualityPreset)
                : globalConfig_.getEffectiveQuality(static_cast<int>(buffers_.size()), sourceRate_);

        bufferConfig = globalConfig_;
        bufferConfig.qualityPreset = effectivePreset;
        srcRate = sourceRate_;
    }

    // Configure outside the lock — this allocates memory and may be slow.
    bufferToReconfigure->configure(bufferConfig, srcRate);

    {
        std::scoped_lock const lock(buffersMutex_);
        auto it = buffers_.find(id);
        if (it != buffers_.end())
            it->second.lastKnownMemoryBytes = bufferToReconfigure->getMemoryUsageBytes();
        usageCacheDirty_ = true;
    }

    notifyMemoryUsageChanged();
}

QualityOverride MemoryBudgetManager::getBufferQualityOverride(const juce::String& id) const
{
    std::scoped_lock const lock(buffersMutex_);

    auto it = buffers_.find(id);
    if (it != buffers_.end())
        return it->second.qualityOverride;

    return QualityOverride::UseGlobal;
}

void MemoryBudgetManager::reconfigureBuffersImpl(
    const std::function<bool(const BufferInfo&)>& filter,
    const std::function<CaptureQualityConfig(const BufferInfo&)>& configFor)
{
    struct ReconfigJob
    {
        juce::String id;
        std::shared_ptr<DecimatingCaptureBuffer> buffer;
        CaptureQualityConfig config;
    };

    std::vector<ReconfigJob> jobs;
    int srcRate = 0;

    {
        std::scoped_lock const lock(buffersMutex_);
        pruneExpiredBuffersLocked();
        srcRate = sourceRate_;

        for (auto& [id, info] : buffers_)
        {
            if (!filter(info))
                continue;

            auto buffer = info.buffer.lock();
            if (!buffer)
                continue;

            jobs.push_back({.id = id, .buffer = std::move(buffer), .config = configFor(info)});
        }
    }

    // Configure outside the lock — this allocates memory and may be slow.
    for (auto& job : jobs)
        job.buffer->configure(job.config, srcRate);

    {
        std::scoped_lock const lock(buffersMutex_);
        for (auto& job : jobs)
        {
            auto it = buffers_.find(job.id);
            if (it != buffers_.end())
                it->second.lastKnownMemoryBytes = job.buffer->getMemoryUsageBytes();
        }
        usageCacheDirty_ = true;
    }
}

void MemoryBudgetManager::applyRecommendedQuality()
{
    QualityPreset recommended = QualityPreset::Standard;
    CaptureQualityConfig sharedConfig;
    {
        std::scoped_lock const lock(buffersMutex_);
        pruneExpiredBuffersLocked();
        recommended = globalConfig_.getEffectiveQuality(static_cast<int>(buffers_.size()), sourceRate_);
        sharedConfig = globalConfig_;
        sharedConfig.qualityPreset = recommended;
    }

    reconfigureBuffersImpl([](const BufferInfo& info) { return info.qualityOverride == QualityOverride::UseGlobal; },
                           [&sharedConfig](const BufferInfo&) { return sharedConfig; });

    if (recommended != lastEffectiveQuality_)
    {
        lastEffectiveQuality_ = recommended;
        notifyEffectiveQualityChanged(recommended);
    }
}

void MemoryBudgetManager::reconfigureAllBuffers()
{
    reconfigureBuffersImpl(
        [](const BufferInfo&) { return true; },
        [this](const BufferInfo& info) -> CaptureQualityConfig {
            QualityPreset effectivePreset = globalConfig_.qualityPreset;

            if (info.qualityOverride != QualityOverride::UseGlobal)
                effectivePreset = resolveQualityOverride(info.qualityOverride, globalConfig_.qualityPreset);
            else if (globalConfig_.autoAdjustQuality)
                effectivePreset = globalConfig_.getEffectiveQuality(static_cast<int>(buffers_.size()), sourceRate_);

            CaptureQualityConfig bufferConfig = globalConfig_;
            bufferConfig.qualityPreset = effectivePreset;
            return bufferConfig;
        });
}

} // namespace oscil
