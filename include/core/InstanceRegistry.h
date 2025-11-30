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

// Include Source.h for SourceId, InstanceId, and related types
// Must be before namespace oscil to avoid double-namespace
#include "Source.h"
#include "IInstanceRegistry.h"

namespace oscil
{

// Forward declarations
class SharedCaptureBuffer;
class OscilPluginProcessor;

/**
 * Information about a registered audio source.
 *
 * Thread safety note: The copy constructor and assignment operator read from
 * the `active` atomic without external synchronization. These operations are
 * only safe when called while holding InstanceRegistry::mutex_ (which is the
 * case for all internal usage). Direct access to SourceInfo outside the
 * registry should use const references from getAllSources() which returns
 * copies made under the lock.
 */
struct SourceInfo
{
    SourceId sourceId;
    juce::String name;                          // User-friendly name (DAW track name if available)
    int channelCount = 2;                       // Number of channels (1 = mono, 2 = stereo)
    double sampleRate = 44100.0;
    std::weak_ptr<SharedCaptureBuffer> buffer;  // Weak reference to capture buffer
    std::atomic<bool> active{ true };

    SourceInfo() = default;

    // Copy constructor - safe when called under InstanceRegistry::mutex_
    // Uses relaxed ordering since mutex provides synchronization
    SourceInfo(const SourceInfo& other)
        : sourceId(other.sourceId)
        , name(other.name)
        , channelCount(other.channelCount)
        , sampleRate(other.sampleRate)
        , buffer(other.buffer)
        , active(other.active.load(std::memory_order_relaxed))
    {}

    // Assignment operator - safe when called under InstanceRegistry::mutex_
    // Uses relaxed ordering since mutex provides synchronization
    SourceInfo& operator=(const SourceInfo& other)
    {
        if (this != &other)
        {
            sourceId = other.sourceId;
            name = other.name;
            channelCount = other.channelCount;
            sampleRate = other.sampleRate;
            buffer = other.buffer;
            active.store(other.active.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
        }
        return *this;
    }
};

/**
 * Listener interface for registry changes
 */
class InstanceRegistryListener
{
public:
    virtual ~InstanceRegistryListener() = default;
    virtual void sourceAdded(const SourceId& sourceId) = 0;
    virtual void sourceRemoved(const SourceId& sourceId) = 0;
    virtual void sourceUpdated(const SourceId& sourceId) = 0;
};

/**
 * Singleton registry for all active Oscil plugin instances.
 * Provides source discovery and deduplication for multi-instance coordination.
 *
 * Thread safety: All public methods are thread-safe.
 *
 * Implements IInstanceRegistry interface for dependency injection support.
 */
class InstanceRegistry : public IInstanceRegistry
{
public:
    /**
     * Get the singleton instance
     */
    [[nodiscard]] static InstanceRegistry& getInstance();

    /**
     * Register a new plugin instance as a signal source.
     * Returns the assigned SourceId (may be existing if deduplication applies).
     *
     * @param trackIdentifier DAW-provided track identifier for deduplication
     * @param captureBuffer The capture buffer for this instance
     * @param name User-friendly name for the source
     * @param channelCount Number of audio channels
     * @param sampleRate Current sample rate
     */
    [[nodiscard]] SourceId registerInstance(
        const juce::String& trackIdentifier,
        std::shared_ptr<SharedCaptureBuffer> captureBuffer,
        const juce::String& name = "Track",
        int channelCount = 2,
        double sampleRate = 44100.0) override;

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
     * Get the capture buffer for a source
     */
    [[nodiscard]] std::shared_ptr<SharedCaptureBuffer> getCaptureBuffer(const SourceId& sourceId) const override;

    /**
     * Update source metadata (name, sample rate, etc.)
     */
    void updateSource(const SourceId& sourceId, const juce::String& name, int channelCount, double sampleRate) override;

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

    // Prevent copying
    InstanceRegistry(const InstanceRegistry&) = delete;
    InstanceRegistry& operator=(const InstanceRegistry&) = delete;

private:
    InstanceRegistry() = default;
    ~InstanceRegistry() override = default;

    void notifySourceAdded(const SourceId& sourceId);
    void notifySourceRemoved(const SourceId& sourceId);
    void notifySourceUpdated(const SourceId& sourceId);

    mutable std::shared_mutex mutex_;
    std::unordered_map<SourceId, SourceInfo, SourceIdHash> sources_;
    std::unordered_map<juce::String, SourceId> trackToSourceMap_; // Deduplication map

    // Using JUCE ListenerList for safe listener management
    // ListenerList handles removal during iteration and prevents dangling pointer issues
    juce::ListenerList<InstanceRegistryListener> listeners_;
};

} // namespace oscil
