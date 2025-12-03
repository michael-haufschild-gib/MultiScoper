/*
    Oscil - Instance Registry Interface
    Abstract interface for source registry to enable dependency injection
*/

#pragma once

#include <juce_core/juce_core.h>
#include <memory>
#include <vector>
#include <optional>
#include <atomic>
#include "core/Source.h"

namespace oscil
{

// Forward declarations
class SharedCaptureBuffer;

/**
 * Information about a registered audio source.
 */
struct SourceInfo
{
    SourceId sourceId;
    juce::String name;                          // User-friendly name (DAW track name if available)
    juce::String trackIdentifier;               // DAW-provided track identifier
    int channelCount = 2;                       // Number of channels (1 = mono, 2 = stereo)
    double sampleRate = 44100.0;
    std::weak_ptr<SharedCaptureBuffer> buffer;  // Weak reference to capture buffer
    std::atomic<bool> active{ true };

    SourceInfo() = default;

    // Copy constructor
    SourceInfo(const SourceInfo& other)
        : sourceId(other.sourceId)
        , name(other.name)
        , trackIdentifier(other.trackIdentifier)
        , channelCount(other.channelCount)
        , sampleRate(other.sampleRate)
        , buffer(other.buffer)
        , active(other.active.load(std::memory_order_relaxed))
    {}

    // Assignment operator
    SourceInfo& operator=(const SourceInfo& other)
    {
        if (this != &other)
        {
            sourceId = other.sourceId;
            name = other.name;
            trackIdentifier = other.trackIdentifier;
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
 * Abstract interface for the instance registry.
 * Enables dependency injection and testability by decoupling
 * UI components from the concrete InstanceRegistry singleton.
 */
class IInstanceRegistry
{
public:
    virtual ~IInstanceRegistry() = default;

    /**
     * Register a new plugin instance as a signal source.
     */
    virtual SourceId registerInstance(
        const juce::String& trackIdentifier,
        std::shared_ptr<SharedCaptureBuffer> captureBuffer,
        const juce::String& name = "Track",
        int channelCount = 2,
        double sampleRate = 44100.0) = 0;

    /**
     * Unregister a plugin instance.
     */
    virtual void unregisterInstance(const SourceId& sourceId) = 0;

    /**
     * Get all available sources
     */
    virtual std::vector<SourceInfo> getAllSources() const = 0;

    /**
     * Get a specific source by ID
     */
    virtual std::optional<SourceInfo> getSource(const SourceId& sourceId) const = 0;

    /**
     * Get the capture buffer for a source
     */
    virtual std::shared_ptr<SharedCaptureBuffer> getCaptureBuffer(const SourceId& sourceId) const = 0;

    /**
     * Update source metadata
     */
    virtual void updateSource(const SourceId& sourceId, const juce::String& name,
                              int channelCount, double sampleRate) = 0;

    /**
     * Get the number of registered sources
     */
    virtual size_t getSourceCount() const = 0;

    /**
     * Add a listener for registry changes
     */
    virtual void addListener(InstanceRegistryListener* listener) = 0;

    /**
     * Remove a listener
     */
    virtual void removeListener(InstanceRegistryListener* listener) = 0;
};

} // namespace oscil
