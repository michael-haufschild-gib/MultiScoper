/*
    Oscil - Plugin Processor State Serialization
    getStateInformation, setStateInformation, and updateCachedState
*/

#include "core/OscilLog.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"

namespace oscil
{

void OscilPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // CRITICAL: Some DAWs (Pro Tools, Reaper) may call this from audio thread
    // during save operations. We must not allocate memory in that case.
    if (!juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        // Non-message-thread path: try to grab latest published state (non-blocking).
        // If the lock is contended, fall back to this thread's previously cached copy.
        // thread_local avoids a data race if multiple non-message threads call concurrently.
        // NOTE: Cannot log here — audio thread.
        static thread_local std::shared_ptr<const std::vector<char>> threadLocalState;
        {
            const juce::SpinLock::ScopedTryLockType tryLock(stateLock_);
            if (tryLock.isLocked())
                threadLocalState = publishedState_;
        }
        // Read from this thread's cached copy — no lock held, data is immutable.
        auto state = threadLocalState;
        if (state && !state->empty())
            destData.replaceAll(state->data(), state->size());
        return;
    }

    OSCIL_LOG(PLUGIN, "getStateInformation: serializing on message thread");
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

void OscilPluginProcessor::updateCachedState()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Sync timing engine state to OscilState before saving
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

void OscilPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Defer to message thread: fromXmlString allocates, state_ isn't thread-safe.
    // WeakReference handles processor destruction before callback runs.
    auto xmlString = std::make_shared<juce::String>(juce::String::createStringFromData(data, sizeInBytes));
    auto sampleRate = static_cast<int>(currentSampleRate_);

    auto doRestoreState = [weakThis = juce::WeakReference<OscilPluginProcessor>(this), xmlString, sampleRate]() {
        auto* processor = weakThis.get();
        if (!processor)
            return;

        // Parse and apply state (now safely on message thread)
        OSCIL_LOG(PLUGIN, "setStateInformation: restoring state (" << xmlString->length() << " chars)");
        if (!processor->state_.fromXmlString(*xmlString))
        {
            OSCIL_LOG(PLUGIN, "setStateInformation: FAILED to parse state XML (" << xmlString->length() << " chars)");
            return;
        }

        // Apply restored timing state (TimingEngine uses atomics internally)
        auto timingTree = processor->state_.getState().getChildWithName(StateIds::Timing);
        if (timingTree.isValid())
        {
            processor->timingEngine_.fromValueTree(timingTree);
        }
        else
        {
            // Legacy/malformed payloads may omit Timing node.
            // Re-apply current persisted config to clear runtime-only timing latches.
            processor->timingEngine_.fromValueTree(processor->timingEngine_.toValueTree());
        }

        // Sync restored capture quality config to MemoryBudgetManager
        auto captureConfig = processor->state_.getCaptureQualityConfig();
        processor->setCaptureQualityConfig(captureConfig);
        processor->memoryBudgetManager_.setGlobalConfig(captureConfig, sampleRate);

        // Update capture buffer config (allocates memory)
        processor->captureBuffer_->configure(captureConfig, sampleRate);

        // Update cached state for real-time safe getStateInformation()
        processor->updateCachedState();

        OSCIL_LOG(PLUGIN, "setStateInformation: restored successfully, " << processor->state_.getOscillators().size()
                                                                         << " oscillators");
    };

    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        doRestoreState();
    else
        juce::MessageManager::callAsync(std::move(doRestoreState));
}

} // namespace oscil
