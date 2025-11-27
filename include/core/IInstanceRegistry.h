/*
    Oscil - Instance Registry Interface
    Abstract interface for source registry to enable dependency injection
*/

#pragma once

#include <juce_core/juce_core.h>
#include <memory>
#include <vector>
#include <optional>

namespace oscil
{

// Forward declarations
class SharedCaptureBuffer;
class InstanceRegistryListener;
struct SourceInfo;
struct SourceId;

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
