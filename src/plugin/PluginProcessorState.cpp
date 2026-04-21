/*
    MultiScoper - Plugin Processor State Serialization
    getStateInformation, setStateInformation, and updateCachedState
*/

#include "core/MultiScoperLog.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"

namespace multiscoper
{

void MultiScoperPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // CRITICAL: Some DAWs (Pro Tools, Reaper) may call this from audio thread
    // during save operations. We must not allocate memory in that case.
    if (!juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        // Non-message-thread path: grab latest published state via non-blocking tryLock.
        // If the lock is contended (updateCachedState running), skip — DAW will retry.
        // NOTE: Cannot log here — audio thread.
        const juce::SpinLock::ScopedTryLockType tryLock(stateLock_);
        if (tryLock.isLocked())
        {
            auto state = publishedState_;
            if (state && !state->empty())
                destData.replaceAll(state->data(), state->size());
        }
        return;
    }

    MULTISCOPER_LOG(PLUGIN, "getStateInformation: serializing on message thread");
    // Message thread path: perform full serialization then read the result.
    updateCachedState();

    std::shared_ptr<const std::vector<char>> state;
    {
        const juce::SpinLock::ScopedLockType lock(stateLock_);
        state = publishedState_;
    }
    if (state && !state->empty())
        destData.replaceAll(state->data(), state->size());
}

void MultiScoperPluginProcessor::updateCachedState()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Persist the current trackIdentifier so cross-instance oscillator bindings
    // can resolve after DAW save/reload. See PluginProcessor.cpp::ctor and
    // reRegisterUnderPersistedTrackIdentifier.
    state_.setTrackIdentifier(trackIdentifier_);

    // Sync timing engine state to MultiScoperState before saving
    auto timingState = timingEngine_.toValueTree();
    auto& stateTree = state_.getState();

    // Remove existing Timing node if present
    auto existingTiming = stateTree.getChildWithName(StateIds::Timing);
    if (existingTiming.isValid())
        stateTree.removeChild(existingTiming, nullptr);

    // Add current timing state
    stateTree.appendChild(timingState, nullptr);

    auto xmlString = state_.toXmlString();

    // Convert to UTF-8 bytes on message thread. The resulting vector is
    // wrapped in a const shared_ptr so the data is immutable once published.
    const char* utf8Ptr = xmlString.toRawUTF8();
    auto utf8Size = xmlString.getNumBytesAsUTF8();

    auto newState = std::make_shared<const std::vector<char>>(utf8Ptr, utf8Ptr + utf8Size);
    {
        const juce::SpinLock::ScopedLockType lock(stateLock_);
        publishedState_ = std::move(newState);
    }
}

void MultiScoperPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Defer to message thread: fromXmlString allocates, state_ isn't thread-safe.
    // WeakReference handles processor destruction before callback runs.
    auto xmlString = std::make_shared<juce::String>(juce::String::createStringFromData(data, sizeInBytes));
    auto sampleRate = static_cast<int>(currentSampleRate_);

    auto doRestoreState = [weakThis = juce::WeakReference<MultiScoperPluginProcessor>(this), xmlString, sampleRate]() {
        auto* processor = weakThis.get();
        if (!processor)
            return;

        // Parse and apply state (now safely on message thread)
        MULTISCOPER_LOG(PLUGIN, "setStateInformation: restoring state (" << xmlString->length() << " chars)");
        if (!processor->state_.fromXmlString(*xmlString))
        {
            MULTISCOPER_LOG(PLUGIN,
                            "setStateInformation: FAILED to parse state XML (" << xmlString->length() << " chars)");
            return;
        }

        // Restore persisted trackIdentifier before any source-binding work so
        // the instance re-registers (or will register on prepareToPlay) under
        // its stable identity, letting oscillators with persisted sourceIds
        // resolve to the same sources across DAW save/reload.
        const auto persistedTrackId = processor->state_.getTrackIdentifier();
        processor->reRegisterUnderPersistedTrackIdentifier(persistedTrackId);

        // Apply restored timing state. `fromValueTree` treats an invalid tree
        // as "reset to defaults + clear runtime latches", which is the correct
        // behavior when the state payload omits Timing entirely.
        auto timingTree = processor->state_.getState().getChildWithName(StateIds::Timing);
        processor->timingEngine_.fromValueTree(timingTree);

        // Sync restored capture quality config to MemoryBudgetManager
        auto captureConfig = processor->state_.getCaptureQualityConfig();
        processor->setCaptureQualityConfig(captureConfig);
        processor->memoryBudgetManager_.setGlobalConfig(captureConfig, sampleRate);

        // Update capture buffer config (allocates memory)
        processor->captureBuffer_->configure(captureConfig, sampleRate);

        // Update cached state for real-time safe getStateInformation()
        processor->updateCachedState();

        MULTISCOPER_LOG(PLUGIN, "setStateInformation: restored successfully, " << processor->state_.getOscillatorCount()
                                                                               << " oscillators");
    };

    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        doRestoreState();
    else
        juce::MessageManager::callAsync(std::move(doRestoreState));
}

} // namespace multiscoper
