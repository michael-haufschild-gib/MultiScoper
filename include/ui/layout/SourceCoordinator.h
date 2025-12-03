/*
    Oscil - Source Coordinator
    Handles InstanceRegistryListener callbacks and source list management
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/InstanceRegistry.h"
#include "core/interfaces/IInstanceRegistry.h"
#include <functional>
#include <memory>
#include <vector>

namespace oscil
{

/**
 * Coordinates source registry events for the plugin editor.
 * Encapsulates InstanceRegistryListener implementation to reduce
 * editor responsibilities.
 */
class SourceCoordinator : public InstanceRegistryListener
{
public:
    using SourcesChangedCallback = std::function<void()>;

    /**
     * Create coordinator with registry reference and change callback.
     * Automatically registers as listener on construction.
     */
    SourceCoordinator(IInstanceRegistry& registry, SourcesChangedCallback onSourcesChanged);

    /**
     * Destructor automatically unregisters from registry.
     */
    ~SourceCoordinator() override;

    // InstanceRegistryListener interface
    void sourceAdded(const SourceId& sourceId) override;
    void sourceRemoved(const SourceId& sourceId) override;
    void sourceUpdated(const SourceId& sourceId) override;

    /**
     * Get the current list of available source IDs.
     */
    const std::vector<SourceId>& getAvailableSources() const { return availableSources_; }

    /**
     * Refresh the source list from the registry.
     * Call this to sync with current registry state.
     */
    void refreshFromRegistry();

    // Prevent copying
    SourceCoordinator(const SourceCoordinator&) = delete;
    SourceCoordinator& operator=(const SourceCoordinator&) = delete;

private:
    /**
     * Post callback to message thread with SafePointer guard.
     * Ensures callbacks don't execute if coordinator is deleted.
     */
    void postToMessageThread(std::function<void()> callback);

    IInstanceRegistry& registry_;
    SourcesChangedCallback onSourcesChanged_;
    std::vector<SourceId> availableSources_;

    // Flag to track if we're still valid for callbacks
    // Use shared_ptr so the flag outlives this object if async callbacks are pending
    std::shared_ptr<std::atomic<bool>> isValid_ = std::make_shared<std::atomic<bool>>(true);
};

} // namespace oscil
