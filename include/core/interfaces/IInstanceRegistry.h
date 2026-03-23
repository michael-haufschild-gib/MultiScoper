/*
    Oscil - Instance Registry Interface
    Abstract interface for source registry to enable dependency injection
*/

#pragma once

#include <juce_core/juce_core.h>
#include <memory>
#include <vector>
#include <optional>
#include "core/Source.h"
#include "core/interfaces/IAudioBuffer.h"

namespace oscil
{

// Forward declarations
class SharedCaptureBuffer;
class AnalysisEngine;

/**
 * Information about a registered audio source.
 *
 * Thread safety: all access to SourceInfo instances stored in InstanceRegistry
 * is protected by the registry's shared_mutex. No atomic members are needed.
 *
 * Ownership contract for `buffer`:
 *   The PluginProcessor owns the SharedCaptureBuffer via shared_ptr and passes it
 *   to registerInstance(). The registry stores only a weak_ptr so that buffer
 *   lifetime is tied to the PluginProcessor, not the registry. When a plugin
 *   instance is destroyed (unregisterInstance), the shared_ptr in PluginProcessor
 *   is released and the buffer is freed — even if stale SourceInfo copies exist
 *   elsewhere. Callers that need the buffer must lock() the weak_ptr and handle
 *   the nullptr case (source already removed).
 */
struct SourceInfo
{
    SourceId sourceId;
    juce::String name;                          // User-friendly name (DAW track name if available)
    juce::String trackIdentifier;               // DAW-provided track identifier
    int channelCount = 2;                       // Number of channels (1 = mono, 2 = stereo)
    double sampleRate = 44100.0;
    std::weak_ptr<IAudioBuffer> buffer;         // Non-owning ref; see ownership contract above
    std::shared_ptr<AnalysisEngine> analysisEngine; // Analysis engine for this source
    bool active = true;
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
 * UI components from the concrete InstanceRegistry implementation.
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
        std::shared_ptr<AnalysisEngine> analysisEngine = nullptr) = 0;

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
    virtual void addListener(InstanceRegistryListener* listener) = 0;

    /**
     * Remove a listener
     */
    virtual void removeListener(InstanceRegistryListener* listener) = 0;
};

} // namespace oscil
