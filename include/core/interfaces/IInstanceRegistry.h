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
#include "core/interfaces/IAudioBuffer.h"

namespace oscil
{

// Forward declarations
class SharedCaptureBuffer;
class AnalysisEngine;

/**
 * Information about a registered audio source.
 */
struct SourceInfo
{
    SourceId sourceId;
    juce::String name;                          // User-friendly name (DAW track name if available)
    juce::String trackIdentifier;               // DAW-provided track identifier
    juce::String instanceUUID;                  // Persistent UUID (survives DAW session reload)
    int channelCount = 2;                       // Number of channels (1 = mono, 2 = stereo)
    double sampleRate = 44100.0;
    std::weak_ptr<IAudioBuffer> buffer;         // Weak reference to capture buffer
    std::shared_ptr<AnalysisEngine> analysisEngine; // Analysis engine for this source
    std::atomic<bool> active{ true };

    SourceInfo() = default;

    // Copy constructor
    SourceInfo(const SourceInfo& other)
        : sourceId(other.sourceId)
        , name(other.name)
        , trackIdentifier(other.trackIdentifier)
        , instanceUUID(other.instanceUUID)
        , channelCount(other.channelCount)
        , sampleRate(other.sampleRate)
        , buffer(other.buffer)
        , analysisEngine(other.analysisEngine)
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
            instanceUUID = other.instanceUUID;
            channelCount = other.channelCount;
            sampleRate = other.sampleRate;
            buffer = other.buffer;
            analysisEngine = other.analysisEngine;
            active.store(other.active.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
        }
        return *this;
    }
};

/**
 * Listener interface for registry changes
 */
class IInstanceRegistryListener
{
public:
    virtual ~IInstanceRegistryListener() = default;
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
        std::shared_ptr<IAudioBuffer> captureBuffer,
        const juce::String& name = "Track",
        int channelCount = 2,
        double sampleRate = 44100.0,
        std::shared_ptr<AnalysisEngine> analysisEngine = nullptr,
        const juce::String& instanceUUID = juce::String()) = 0;

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
     * Get a source by name (for resolving persisted oscillator references)
     * Returns the first source with a matching name, or nullopt if not found.
     * Name matching is case-insensitive.
     */
    virtual std::optional<SourceInfo> getSourceByName(const juce::String& name) const = 0;

    /**
     * Get a source by instanceUUID (for resolving persisted oscillator references)
     * This is the most reliable method as instanceUUIDs are unique and persist across sessions.
     * Returns the source with matching UUID, or nullopt if not found.
     */
    virtual std::optional<SourceInfo> getSourceByInstanceUUID(const juce::String& uuid) const = 0;

    /**
     * Get the capture buffer for a source
     */
    virtual std::shared_ptr<IAudioBuffer> getCaptureBuffer(const SourceId& sourceId) const = 0;

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
    virtual void addListener(IInstanceRegistryListener* listener) = 0;

    /**
     * Remove a listener
     */
    virtual void removeListener(IInstanceRegistryListener* listener) = 0;
};

} // namespace oscil
