/*
    Oscil - Instance Registry
    Singleton managing all active Oscil instances for multi-instance coordination
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <atomic>

#include "core/interfaces/IInstanceRegistry.h"

namespace oscil
{

/**
 * Registry for all active Oscil plugin instances.
 * Provides source discovery and deduplication for multi-instance coordination.
 *
 * Thread safety: All public methods are thread-safe.
 *
 * Implements IInstanceRegistry interface for dependency injection.
 * Owned by OscilPluginProcessor - do not create directly except in tests.
 */
class InstanceRegistry : public IInstanceRegistry
{
public:
    InstanceRegistry();
    ~InstanceRegistry() override = default;
    /**
     * Register a new plugin instance as a signal source.
     * Returns the assigned SourceId (may be existing if deduplication applies).
     *
     * @param trackIdentifier DAW-provided track identifier for deduplication
     * @param captureBuffer The capture buffer for this instance
     * @param name User-friendly name for the source
     * @param channelCount Number of audio channels
     * @param sampleRate Current sample rate
     * @param analysisEngine Analysis engine for this source
     * @param instanceUUID Persistent UUID that survives DAW session reload
     */
    [[nodiscard]] SourceId registerInstance(
        const juce::String& trackIdentifier,
        std::shared_ptr<IAudioBuffer> captureBuffer,
        const juce::String& name = "Track",
        int channelCount = 2,
        double sampleRate = 44100.0,
        std::shared_ptr<AnalysisEngine> analysisEngine = nullptr,
        const juce::String& instanceUUID = juce::String()) override;

    /**
     * Unregister a plugin instance.
     * If another instance exists for the same track, it becomes the primary source.
     */
    void unregisterInstance(const SourceId& sourceId) override;

    /**
     * Get all available sources
     */
    [[nodiscard]] std::vector<SourceInfo> getAllSources() const override;

    /**
     * Get a specific source by ID
     */
    [[nodiscard]] std::optional<SourceInfo> getSource(const SourceId& sourceId) const override;

    /**
     * Get a source by name (for resolving persisted oscillator references)
     * Returns the first source with a matching name, or nullopt if not found.
     * Name matching is case-insensitive.
     */
    [[nodiscard]] std::optional<SourceInfo> getSourceByName(const juce::String& name) const override;

    /**
     * Get a source by instanceUUID (for resolving persisted oscillator references)
     * This is the most reliable method as instanceUUIDs are unique and persist across sessions.
     * Returns the source with matching UUID, or nullopt if not found.
     */
    [[nodiscard]] std::optional<SourceInfo> getSourceByInstanceUUID(const juce::String& uuid) const override;

    /**
     * Get the capture buffer for a source
     */
    [[nodiscard]] std::shared_ptr<IAudioBuffer> getCaptureBuffer(const SourceId& sourceId) const override;

    /**
     * Update source metadata (name, sample rate, etc.)
     */
    void updateSource(const SourceId& sourceId, const juce::String& name, int channelCount, double sampleRate) override;

    /**
     * Shutdown the registry, clearing all sources and listeners.
     * Call during plugin/application teardown.
     */
    void shutdown();

    /**
     * Get the number of registered sources
     */
    [[nodiscard]] size_t getSourceCount() const override;

    /**
     * Add a listener for registry changes
     */
    void addListener(InstanceRegistryListener* listener) override;

    /**
     * Remove a listener
     */
    void removeListener(InstanceRegistryListener* listener) override;

    /**
     * Dispatcher function type for asynchronous notification handling.
     */
    using Dispatcher = std::function<void(std::function<void()>)>;

    /**
     * Set the dispatcher used for listener notifications.
     * Default uses juce::MessageManager::callAsync.
     * Use this to inject synchronous dispatching for unit tests.
     */
    void setDispatcher(Dispatcher dispatcher);

    // Prevent copying
    InstanceRegistry(const InstanceRegistry&) = delete;
    InstanceRegistry& operator=(const InstanceRegistry&) = delete;

private:
    void notifySourceAdded(const SourceId& sourceId);
    void notifySourceRemoved(const SourceId& sourceId);
    void notifySourceUpdated(const SourceId& sourceId);

    mutable std::shared_mutex mutex_;
    std::unordered_map<SourceId, SourceInfo, SourceIdHash> sources_;
    std::unordered_map<juce::String, SourceId> trackToSourceMap_;      // Deduplication map (by trackIdentifier)
    std::unordered_map<juce::String, SourceId> instanceUUIDToSourceMap_; // Persistent UUID lookup map

    // Dispatcher for notifications
    Dispatcher dispatcher_;

    // Using JUCE ListenerList for safe listener management
    // ListenerList handles removal during iteration and prevents dangling pointer issues
    juce::ListenerList<InstanceRegistryListener> listeners_;

    // Shutdown flag to prevent async notifications from accessing destroyed object
    std::atomic<bool> shuttingDown_{false};

    // Sequence number for detecting stale async notifications
    // Incremented on each source registration/unregistration/update
    // Captured by async callbacks to detect if notification is stale
    std::atomic<uint64_t> operationSequence_{0};

    JUCE_DECLARE_WEAK_REFERENCEABLE(InstanceRegistry)
};

} // namespace oscil
