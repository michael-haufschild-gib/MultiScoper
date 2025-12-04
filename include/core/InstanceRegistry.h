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

// Forward declarations for test classes (global namespace)
class InstanceRegistryTest;
class InstanceRegistryCrudTest;
class InstanceRegistryLifecycleTest;
class InstanceRegistrySyncTest;
class PluginProcessorTest;
class PluginProcessorAudioTest;
class PluginProcessorLifecycleTest;
class PluginProcessorStateTest;
class SidebarThemePopulationTest;
class PluginEditorTest;
class PaneClosingBugTest;

namespace oscil
{

// Forward declarations for test namespace
namespace test {
class TestTrack;
class TestHttpServer;
class OscilPluginTestFixture;
class OscilComponentTestFixture;
class OscilAudioTestFixture;
}

// Forward declarations
class SharedCaptureBuffer;
class OscilPluginProcessor;
class PluginFactory;
class SidebarComponent;
class OscillatorListComponent;
class SourceSelectorComponent;
class OscillatorListItemComponent;
class OscilPluginEditor;
class AddOscillatorDialog;
class DialogManager;
class OptionsSection;
class TimingSidebarSection;
class PaneComponent;
class WaveformComponent;
class StateHandler;
class SourceHandler;

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
        std::shared_ptr<IAudioBuffer> captureBuffer,
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
    // Only these classes can access the singleton directly
    friend class oscil::OscilPluginProcessor;
    friend class oscil::PluginFactory; // Factory needs access to inject into processor

    // Test classes that need singleton access
    friend class ::InstanceRegistryTest;
    friend class ::InstanceRegistryCrudTest;
    friend class ::InstanceRegistryLifecycleTest;
    friend class ::InstanceRegistrySyncTest;
    friend class ::PluginProcessorTest;
    friend class ::PluginProcessorAudioTest;
    friend class ::PluginProcessorLifecycleTest;
    friend class ::PluginProcessorStateTest;
    friend class ::SidebarThemePopulationTest;
    friend class ::PluginEditorTest;
    friend class ::PaneClosingBugTest;

    // Also allow test harness access
    // Test fixtures need access to getInstance()
    friend class test::OscilPluginTestFixture;
    friend class test::OscilComponentTestFixture;
    friend class test::OscilAudioTestFixture;
    friend class test::TestTrack;
    friend class test::TestHttpServer;
    friend class StateHandler;
    friend class SourceHandler;

    /**
     * Get the singleton instance (Private/Friend access only)
     */
    [[nodiscard]] static InstanceRegistry& getInstance();

    InstanceRegistry();
    ~InstanceRegistry() override = default;

    void notifySourceAdded(const SourceId& sourceId);
    void notifySourceRemoved(const SourceId& sourceId);
    void notifySourceUpdated(const SourceId& sourceId);

    mutable std::shared_mutex mutex_;
    std::unordered_map<SourceId, SourceInfo, SourceIdHash> sources_;
    std::unordered_map<juce::String, SourceId> trackToSourceMap_; // Deduplication map

    // Dispatcher for notifications
    Dispatcher dispatcher_;

    // Using JUCE ListenerList for safe listener management
    // ListenerList handles removal during iteration and prevents dangling pointer issues
    juce::ListenerList<InstanceRegistryListener> listeners_;
};

} // namespace oscil
