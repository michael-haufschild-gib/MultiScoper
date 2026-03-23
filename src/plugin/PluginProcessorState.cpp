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
        // Audio thread path: read from active buffer (lock-free, no allocation)
        const auto& buf = cachedStateBuffers_[cachedStateActiveIndex_.load(std::memory_order_acquire)];
        if (!buf.empty())
            destData.replaceAll(buf.data(), buf.size());
        return;
    }

    // Message thread path: perform full serialization (allocates memory)
    updateCachedState();

    const auto& buf = cachedStateBuffers_[cachedStateActiveIndex_.load(std::memory_order_acquire)];
    destData.replaceAll(buf.data(), buf.size());
}

void OscilPluginProcessor::updateCachedState()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // CONTRACT CHECK: verify double-buffer safety invariant.
    // Consecutive swaps must be separated by >= 1ms so that any in-flight
    // audio-thread read from the previously-active buffer completes first.
    // The first call (lastCachedStateSwapTimeMs_ == 0) is exempt.
    auto now = juce::Time::currentTimeMillis();
    if (lastCachedStateSwapTimeMs_ > 0)
    {
        auto elapsed = now - lastCachedStateSwapTimeMs_;
        // Log a warning if swaps happen too rapidly (< 1ms apart).
        // This would indicate a potential torn-read risk on the audio thread.
        jassert(elapsed >= 1);  // Double-buffer swap interval too short
    }

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

    // Write to the inactive buffer, then swap atomically
    int active = cachedStateActiveIndex_.load(std::memory_order_relaxed);
    int inactive = 1 - active;
    cachedStateBuffers_[inactive].assign(utf8Ptr, utf8Ptr + utf8Size);
    cachedStateActiveIndex_.store(inactive, std::memory_order_release);
    lastCachedStateSwapTimeMs_ = now;
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
            // Log to both DBG (debug builds) and Logger (all builds) so that
            // state restoration failures are always diagnosable in production.
            DBG("PluginProcessor::setStateInformation - Failed to parse state XML");
            juce::Logger::writeToLog("OscilPluginProcessor: setStateInformation failed to parse XML ("
                                     + juce::String(xmlString->length()) + " chars)");
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
