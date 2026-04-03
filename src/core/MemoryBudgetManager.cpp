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

    bool configChanged = false;
    {
        std::scoped_lock const lock(buffersMutex_);
        configChanged = (globalConfig_ != config) || (sourceRate_ != sourceRate);
        globalConfig_ = config;
        sourceRate_ = sourceRate;
    }

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
    notifyMemoryUsageChanged();

    // Check if quality needs adjustment
    QualityPreset const recommended = getRecommendedQuality();
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
        std::scoped_lock const lock(buffersMutex_);
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

void MemoryBudgetManager::applyRecommendedQuality()
{
    struct ReconfigJob
    {
        juce::String id;
        std::shared_ptr<DecimatingCaptureBuffer> buffer;
    };

    std::vector<ReconfigJob> jobs;
    CaptureQualityConfig bufferConfig;
    int srcRate = 0;
    QualityPreset recommended;

    {
        std::scoped_lock const lock(buffersMutex_);
        pruneExpiredBuffersLocked();
        recommended = getRecommendedQualityForCount(static_cast<int>(buffers_.size()));
        bufferConfig = globalConfig_;
        bufferConfig.qualityPreset = recommended;
        srcRate = sourceRate_;

        for (auto& [id, info] : buffers_)
        {
            if (info.qualityOverride != QualityOverride::UseGlobal)
                continue;

            if (auto buffer = info.buffer.lock())
                jobs.push_back({.id = id, .buffer = std::move(buffer)});
        }
    }

    // Configure outside the lock — this allocates memory and may be slow.
    for (auto& job : jobs)
        job.buffer->configure(bufferConfig, srcRate);

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

    if (recommended != lastEffectiveQuality_)
    {
        lastEffectiveQuality_ = recommended;
        notifyEffectiveQualityChanged(recommended);
    }
}

void MemoryBudgetManager::reconfigureAllBuffers()
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
            auto buffer = info.buffer.lock();
            if (!buffer)
                continue;

            QualityPreset effectivePreset;

            if (info.qualityOverride != QualityOverride::UseGlobal)
            {
                effectivePreset = resolveQualityOverride(info.qualityOverride, globalConfig_.qualityPreset);
            }
            else if (globalConfig_.autoAdjustQuality)
            {
                effectivePreset = globalConfig_.getEffectiveQuality(static_cast<int>(buffers_.size()), sourceRate_);
            }
            else
            {
                effectivePreset = globalConfig_.qualityPreset;
            }

            CaptureQualityConfig bufferConfig = globalConfig_;
            bufferConfig.qualityPreset = effectivePreset;
            jobs.push_back({.id = id, .buffer = std::move(buffer), .config = bufferConfig});
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

} // namespace oscil
