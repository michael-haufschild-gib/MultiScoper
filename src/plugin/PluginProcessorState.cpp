/*
    Oscil - Plugin Processor State Serialization
    getStateInformation, setStateInformation, and updateCachedState
*/

#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"

namespace oscil
{

void OscilPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // CRITICAL: Some DAWs (Pro Tools, Reaper) may call this from audio thread
    // during save operations. We must not allocate memory in that case.
    if (!juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        // Audio thread path: return cached UTF-8 bytes (no allocation, no conversion)
        const juce::SpinLock::ScopedTryLockType sl(cachedStateLock_);
        if (sl.isLocked() && !cachedStateUtf8_.empty())
        {
            destData.replaceAll(cachedStateUtf8_.data(), cachedStateUtf8_.size());
        }
        // If lock failed or cache empty, return empty - host will retry
        return;
    }

    // Message thread path: perform full serialization (allocates memory)
    updateCachedState();

    const juce::SpinLock::ScopedLockType sl(cachedStateLock_);
    destData.replaceAll(cachedStateUtf8_.data(), cachedStateUtf8_.size());
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

    // Convert to UTF-8 bytes now (on message thread) to avoid any
    // potential allocation when audio thread reads the cached state.
    const char* utf8Ptr = xmlString.toRawUTF8();
    size_t utf8Size = xmlString.getNumBytesAsUTF8();

    // Update cache with lock
    const juce::SpinLock::ScopedLockType sl(cachedStateLock_);
    cachedStateUtf8_.assign(utf8Ptr, utf8Ptr + utf8Size);
}

void OscilPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Defer to message thread: fromXmlString allocates, state_ isn't thread-safe.
    // WeakReference handles processor destruction before callback runs.
    auto xmlString = std::make_shared<juce::String>(
        juce::String::createStringFromData(data, sizeInBytes));
    auto sampleRate = static_cast<int>(currentSampleRate_);

    auto doRestoreState = [weakThis = juce::WeakReference<OscilPluginProcessor>(this),
                           xmlString, sampleRate]() {
        auto* processor = weakThis.get();
        if (!processor) return;

        // Parse and apply state (now safely on message thread)
        if (!processor->state_.fromXmlString(*xmlString))
        {
            DBG("PluginProcessor::setStateInformation - Failed to parse state XML");
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
    };

    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        doRestoreState();
    else
        juce::MessageManager::callAsync(std::move(doRestoreState));
}

} // namespace oscil
