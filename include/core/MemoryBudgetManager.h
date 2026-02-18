/*
    Oscil - Memory Budget Manager
    Centralized tracking and management of waveform capture buffer memory
    Provides auto-quality adjustment and memory usage monitoring
*/

#pragma once

#include "core/dsp/CaptureQualityConfig.h"
#include <juce_core/juce_core.h>
#include <memory>
#include <map>
#include <mutex>

namespace oscil
{

// Forward declaration
class DecimatingCaptureBuffer;

/**
 * Information about a single tracked buffer
 */
struct BufferInfo
{
    juce::String id;
    std::weak_ptr<DecimatingCaptureBuffer> buffer;
    QualityOverride qualityOverride = QualityOverride::UseGlobal;
    size_t lastKnownMemoryBytes = 0;
};

/**
 * Memory usage snapshot for UI display
 */
struct MemoryUsageSnapshot
{
    size_t totalUsageBytes = 0;
    size_t budgetBytes = 0;
    int numBuffers = 0;
    float usagePercent = 0.0f;
    QualityPreset effectiveQuality = QualityPreset::Standard;
    bool isOverBudget = false;

    struct BufferUsage
    {
        juce::String id;
        size_t memoryBytes = 0;
        QualityPreset quality = QualityPreset::Standard;
        bool hasOverride = false;
    };
    std::vector<BufferUsage> perBufferUsage;
};

/**
 * Central manager for capture buffer memory budgets
 *
 * Responsibilities:
 * - Track all DecimatingCaptureBuffer instances
 * - Calculate total and per-buffer memory usage
 * - Recommend quality presets based on track count
 * - Notify listeners when memory usage changes
 *
 * Thread Safety:
 * - Registration/unregistration are mutex-protected
 * - Memory calculations may be called from any thread
 *
 * Ownership:
 * - Does NOT own buffers (uses weak_ptr)
 * - Should be owned by PluginProcessor or similar
 */
class MemoryBudgetManager
{
public:
    /**
     * Listener interface for memory budget changes
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when total memory usage changes significantly */
        virtual void memoryUsageChanged(const MemoryUsageSnapshot& snapshot) = 0;

        /** Called when a buffer is added or removed */
        virtual void bufferCountChanged(int newCount) = 0;

        /** Called when effective quality changes due to auto-adjustment */
        virtual void effectiveQualityChanged(QualityPreset newQuality) = 0;
    };

    /**
     * Constructor - creates a new MemoryBudgetManager instance.
     * Typically owned by PluginFactory and passed via dependency injection.
     */
    MemoryBudgetManager();
    ~MemoryBudgetManager() = default;

    // Non-copyable
    MemoryBudgetManager(const MemoryBudgetManager&) = delete;
    MemoryBudgetManager& operator=(const MemoryBudgetManager&) = delete;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * Set the global capture quality configuration
     * This affects all buffers without explicit overrides
     * @param config New global configuration
     * @param sourceRate Source audio sample rate
     */
    void setGlobalConfig(const CaptureQualityConfig& config, int sourceRate);

    /**
     * Get current global configuration
     */
    [[nodiscard]] const CaptureQualityConfig& getGlobalConfig() const { return globalConfig_; }

    /**
     * Get current source sample rate
     */
    [[nodiscard]] int getSourceRate() const { return sourceRate_; }

    //==========================================================================
    // Buffer Registration
    //==========================================================================

    /**
     * Register a buffer for tracking
     * @param id Unique identifier for the buffer
     * @param buffer Shared pointer to the buffer
     */
    void registerBuffer(const juce::String& id, std::shared_ptr<DecimatingCaptureBuffer> buffer);

    /**
     * Unregister a buffer
     * @param id Buffer identifier to remove
     */
    void unregisterBuffer(const juce::String& id);

    /**
     * Set quality override for a specific buffer
     * @param id Buffer identifier
     * @param override Quality override setting
     */
    void setBufferQualityOverride(const juce::String& id, QualityOverride override);

    /**
     * Get quality override for a specific buffer
     * @param id Buffer identifier
     * @return Current override setting, or UseGlobal if not found
     */
    [[nodiscard]] QualityOverride getBufferQualityOverride(const juce::String& id) const;

    /**
     * Get number of registered buffers
     */
    [[nodiscard]] int getBufferCount() const;

    /**
     * Check if a buffer is registered
     */
    [[nodiscard]] bool isBufferRegistered(const juce::String& id) const;

    //==========================================================================
    // Memory Tracking
    //==========================================================================

    /**
     * Get total memory usage across all buffers
     * @return Total bytes used
     */
    [[nodiscard]] size_t getTotalMemoryUsage() const;

    /**
     * Get memory usage for a specific buffer
     * @param id Buffer identifier
     * @return Bytes used, or 0 if not found
     */
    [[nodiscard]] size_t getBufferMemoryUsage(const juce::String& id) const;

    /**
     * Get complete memory usage snapshot
     * Useful for UI display
     */
    [[nodiscard]] MemoryUsageSnapshot getMemorySnapshot() const;

    /**
     * Get memory usage as a formatted string
     */
    [[nodiscard]] juce::String getTotalMemoryUsageString() const;

    /**
     * Check if currently over budget
     */
    [[nodiscard]] bool isOverBudget() const;

    /**
     * Get usage percentage (0-100+)
     */
    [[nodiscard]] float getUsagePercent() const;

    //==========================================================================
    // Auto-Quality Adjustment
    //==========================================================================

    /**
     * Get recommended quality preset based on current buffer count
     * Takes into account memory budget and number of tracks
     */
    [[nodiscard]] QualityPreset getRecommendedQuality() const;

    /**
     * Get effective quality for a specific buffer
     * Considers global setting, auto-adjust, and per-buffer override
     * @param id Buffer identifier
     * @return Effective quality preset
     */
    [[nodiscard]] QualityPreset getEffectiveQuality(const juce::String& id) const;

    /**
     * Apply recommended quality to all buffers without overrides
     * Call this when track count changes significantly
     */
    void applyRecommendedQuality();

    /**
     * Reconfigure all buffers with current settings
     * Useful after global config change
     */
    void reconfigureAllBuffers();

    //==========================================================================
    // Listeners
    //==========================================================================

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    //==========================================================================
    // Utility
    //==========================================================================

    /**
     * Clean up expired weak_ptr entries
     * Called automatically during operations, but can be called manually
     */
    void pruneExpiredBuffers();

    /**
     * Force recalculation of all memory usage
     */
    void refreshMemoryUsage();

private:
    void notifyMemoryUsageChanged();
    void notifyBufferCountChanged();
    void notifyEffectiveQualityChanged(QualityPreset newQuality);
    void updateCachedUsage() const;

    // Internal helper to calculate recommended quality without locking
    // Use this when you already hold buffersMutex_
    [[nodiscard]] QualityPreset getRecommendedQualityForCount(int numBuffers) const;

    CaptureQualityConfig globalConfig_;
    int sourceRate_ = 44100;

    mutable std::mutex buffersMutex_;
    mutable std::map<juce::String, BufferInfo> buffers_; // mutable for cache updates in const methods

    // Cached values for quick access
    mutable size_t cachedTotalUsage_ = 0;
    mutable bool usageCacheDirty_ = true;
    QualityPreset lastEffectiveQuality_ = QualityPreset::Standard;

    juce::ListenerList<Listener> listeners_;

    JUCE_DECLARE_WEAK_REFERENCEABLE(MemoryBudgetManager)
};

} // namespace oscil
