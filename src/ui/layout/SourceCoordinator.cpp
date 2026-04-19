/*
    MultiScoper - Source Coordinator Implementation
*/

#include "ui/layout/SourceCoordinator.h"

#include "core/MultiScoperLog.h"

#include <algorithm>

namespace multiscoper
{

SourceCoordinator::SourceCoordinator(IInstanceRegistry& registry, SourcesChangedCallback onSourcesChanged,
                                     SourceRemovedCallback onSourceRemoved)
    : registry_(registry)
    , onSourcesChanged_(std::move(onSourcesChanged))
    , onSourceRemoved_(std::move(onSourceRemoved))
{
    // Register as listener
    registry_.addListener(this);

    // Initialize source list
    refreshFromRegistry();
}

SourceCoordinator::~SourceCoordinator()
{
    // Mark as invalid to prevent pending callbacks from executing
    // The shared_ptr ensures the atomic outlives this object
    isValid_->store(false, std::memory_order_release);

    // Unregister from registry
    registry_.removeListener(this);
}

void SourceCoordinator::sourceAdded(const SourceId& sourceId)
{
    MULTISCOPER_LOG(SOURCE, "sourceAdded: id=" << sourceId.id << " totalSources=" << (availableSources_.size() + 1));
    // Capture by value for thread safety
    postToMessageThread([this, sourceId]() {
        availableSources_.push_back(sourceId);
        if (onSourcesChanged_)
            onSourcesChanged_();
    });
}

void SourceCoordinator::sourceRemoved(const SourceId& sourceId)
{
    MULTISCOPER_LOG(SOURCE, "sourceRemoved: id=" << sourceId.id << " remainingSources="
                                                 << (!availableSources_.empty() ? availableSources_.size() - 1 : 0));
    postToMessageThread([this, sourceId]() {
        std::erase(availableSources_, sourceId);
        // Fire the targeted remove callback BEFORE the aggregate "changed"
        // callback so the editor can clear oscillator bindings that specifically
        // referenced this sourceId — the aggregate callback no longer clears
        // unresolved bindings on its own (see onSourcesChanged).
        if (onSourceRemoved_)
            onSourceRemoved_(sourceId);
        if (onSourcesChanged_)
            onSourcesChanged_();
    });
}

void SourceCoordinator::sourceUpdated(const SourceId& sourceId)
{
    MULTISCOPER_LOG(SOURCE, "sourceUpdated: id=" << sourceId.id);
    juce::ignoreUnused(sourceId);
    postToMessageThread([this]() {
        if (onSourcesChanged_)
            onSourcesChanged_();
    });
}

void SourceCoordinator::refreshFromRegistry()
{
    availableSources_.clear();
    auto sources = registry_.getAllSources();
    for (const auto& source : sources)
    {
        availableSources_.push_back(source.sourceId);
    }
    MULTISCOPER_LOG(SOURCE, "refreshFromRegistry: " << availableSources_.size() << " sources");
}

void SourceCoordinator::postToMessageThread(std::function<void()> callback)
{
    // Capture shared_ptr to isValid_ flag - this keeps the atomic alive
    // even if SourceCoordinator is destroyed before the callback executes
    auto validFlag = isValid_;

    juce::MessageManager::callAsync([validFlag, cb = std::move(callback)]() {
        // Check if coordinator is still valid before executing callback
        if (validFlag->load(std::memory_order_acquire))
        {
            cb();
        }
    });
}

} // namespace multiscoper
