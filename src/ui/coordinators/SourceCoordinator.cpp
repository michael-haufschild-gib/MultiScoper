/*
    Oscil - Source Coordinator Implementation
*/

#include "ui/coordinators/SourceCoordinator.h"

namespace oscil
{

SourceCoordinator::SourceCoordinator(IInstanceRegistry& registry, SourcesChangedCallback onSourcesChanged)
    : registry_(registry)
    , onSourcesChanged_(std::move(onSourcesChanged))
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
    // Capture by value for thread safety
    postToMessageThread([this, sourceId]()
    {
        availableSources_.push_back(sourceId);
        if (onSourcesChanged_)
            onSourcesChanged_();
    });
}

void SourceCoordinator::sourceRemoved(const SourceId& sourceId)
{
    postToMessageThread([this, sourceId]()
    {
        availableSources_.erase(
            std::remove(availableSources_.begin(), availableSources_.end(), sourceId),
            availableSources_.end());
        if (onSourcesChanged_)
            onSourcesChanged_();
    });
}

void SourceCoordinator::sourceUpdated(const SourceId& /*sourceId*/)
{
    postToMessageThread([this]()
    {
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
}

void SourceCoordinator::postToMessageThread(std::function<void()> callback)
{
    // Capture shared_ptr to isValid_ flag - this keeps the atomic alive
    // even if SourceCoordinator is destroyed before the callback executes
    auto validFlag = isValid_;

    juce::MessageManager::callAsync([validFlag, callback = std::move(callback)]()
    {
        // Check if coordinator is still valid before executing callback
        if (validFlag->load(std::memory_order_acquire))
        {
            callback();
        }
    });
}

} // namespace oscil
