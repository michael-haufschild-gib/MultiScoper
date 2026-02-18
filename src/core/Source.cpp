/*
    Oscil - Source Entity Implementation
*/

#include "core/Source.h"
#include <juce_core/juce_core.h>

namespace oscil
{

// === SourceId Implementation ===

SourceId SourceId::generate()
{
    return SourceId{ juce::Uuid().toString() };
}

// === InstanceId Implementation ===

InstanceId InstanceId::generate()
{
    return InstanceId{ juce::Uuid().toString() };
}

// === Source Implementation ===

Source::Source()
    : id_(SourceId::generate())
    , createdAt_(juce::Time::getCurrentTime())
{
    auto now = juce::Time::currentTimeMillis();
    lastHeartbeatMs_.store(now, std::memory_order_relaxed);
    lastAudioTimeMs_.store(now, std::memory_order_relaxed);
}

Source::Source(const SourceId& sourceId)
    : id_(sourceId)
    , createdAt_(juce::Time::getCurrentTime())
{
    auto now = juce::Time::currentTimeMillis();
    lastHeartbeatMs_.store(now, std::memory_order_relaxed);
    lastAudioTimeMs_.store(now, std::memory_order_relaxed);
}

Source::Source(const juce::ValueTree& state)
    : createdAt_(juce::Time::getCurrentTime())
{
    auto now = juce::Time::currentTimeMillis();
    lastHeartbeatMs_.store(now, std::memory_order_relaxed);
    lastAudioTimeMs_.store(now, std::memory_order_relaxed);
    fromValueTree(state);
}

juce::ValueTree Source::toValueTree() const
{
    juce::ValueTree tree("Source");

    tree.setProperty("id", id_.id, nullptr);
    tree.setProperty("dawTrackId", dawTrackId_, nullptr);
    tree.setProperty("owningInstanceId", owningInstanceId_.id, nullptr);
    tree.setProperty("sampleRate", sampleRate_, nullptr);
    tree.setProperty("channelConfig", channelConfigToString(channelConfig_), nullptr);
    tree.setProperty("bufferSize", bufferSize_, nullptr);
    tree.setProperty("state", sourceStateToString(state_), nullptr);
    tree.setProperty("displayName", displayName_, nullptr);
    tree.setProperty("schemaVersion", schemaVersion_, nullptr);

    // Serialize backup instance IDs (thread-safe access)
    {
        const juce::SpinLock::ScopedLockType lock(backupMutex_);
        juce::ValueTree backupsTree("BackupInstances");
        for (const auto& backup : backupInstanceIds_)
        {
            juce::ValueTree backupTree("Instance");
            backupTree.setProperty("id", backup.id, nullptr);
            backupsTree.appendChild(backupTree, nullptr);
        }
        tree.appendChild(backupsTree, nullptr);
    }

    return tree;
}

void Source::fromValueTree(const juce::ValueTree& state)
{
    if (!state.isValid() || state.getType().toString() != "Source")
        return;

    id_.id = state.getProperty("id", juce::String()).toString();
    dawTrackId_ = state.getProperty("dawTrackId", juce::String()).toString();
    owningInstanceId_.id = state.getProperty("owningInstanceId", juce::String()).toString();
    sampleRate_ = static_cast<float>(state.getProperty("sampleRate", 44100.0));
    channelConfig_ = stringToChannelConfig(state.getProperty("channelConfig", "STEREO").toString());
    bufferSize_ = static_cast<int>(state.getProperty("bufferSize", 512));
    state_ = stringToSourceState(state.getProperty("state", "DISCOVERED").toString());
    displayName_ = state.getProperty("displayName", juce::String()).toString();
    schemaVersion_ = static_cast<int>(state.getProperty("schemaVersion", 1));

    // Deserialize backup instance IDs (thread-safe access)
    {
        const juce::SpinLock::ScopedLockType lock(backupMutex_);
        backupInstanceIds_.clear();
        auto backupsTree = state.getChildWithName("BackupInstances");
        if (backupsTree.isValid())
        {
            for (int i = 0; i < backupsTree.getNumChildren(); ++i)
            {
                auto backupTree = backupsTree.getChild(i);
                InstanceId backupId;
                backupId.id = backupTree.getProperty("id", juce::String()).toString();
                if (backupId.isValid())
                {
                    backupInstanceIds_.push_back(backupId);
                }
            }
        }
    }
}

void Source::setOwningInstanceId(const InstanceId& instanceId)
{
    owningInstanceId_ = instanceId;

    // Remove from backup list if present
    removeBackupInstance(instanceId);
}

void Source::addBackupInstance(const InstanceId& instanceId)
{
    if (!instanceId.isValid())
        return;

    // Don't add if it's the current owner
    if (instanceId == owningInstanceId_)
        return;

    const juce::SpinLock::ScopedLockType lock(backupMutex_);

    // Don't add duplicates
    for (const auto& backup : backupInstanceIds_)
    {
        if (backup == instanceId)
            return;
    }

    backupInstanceIds_.push_back(instanceId);
}

void Source::removeBackupInstance(const InstanceId& instanceId)
{
    const juce::SpinLock::ScopedLockType lock(backupMutex_);

    backupInstanceIds_.erase(
        std::remove_if(backupInstanceIds_.begin(), backupInstanceIds_.end(),
            [&instanceId](const InstanceId& id) { return id == instanceId; }),
        backupInstanceIds_.end());
}

std::optional<InstanceId> Source::getNextBackupInstance() const
{
    const juce::SpinLock::ScopedLockType lock(backupMutex_);

    if (backupInstanceIds_.empty())
        return std::nullopt;

    return backupInstanceIds_.front();
}

bool Source::transferOwnership()
{
    const juce::SpinLock::ScopedLockType lock(backupMutex_);

    if (backupInstanceIds_.empty())
        return false;

    // Get and remove the next backup in one atomic operation
    auto nextBackup = backupInstanceIds_.front();

    // Set new owner
    owningInstanceId_ = nextBackup;

    // Remove from backup list
    backupInstanceIds_.erase(
        std::remove_if(backupInstanceIds_.begin(), backupInstanceIds_.end(),
            [&nextBackup](const InstanceId& id) { return id == nextBackup; }),
        backupInstanceIds_.end());

    return true;
}

bool Source::transitionTo(SourceState newState)
{
    // Use compare_exchange to atomically check and update state (avoids TOCTOU race)
    SourceState currentState = state_.load(std::memory_order_acquire);

    // Loop to handle spurious failures from compare_exchange_weak
    while (isValidTransition(currentState, newState))
    {
        if (state_.compare_exchange_weak(currentState, newState,
                                          std::memory_order_acq_rel,
                                          std::memory_order_acquire))
        {
            lastHeartbeatMs_.store(juce::Time::currentTimeMillis(), std::memory_order_relaxed);
            return true;
        }
        // compare_exchange_weak failed, currentState is updated with actual value
        // Loop will re-check if the new current state allows the transition
    }

    return false;
}

bool Source::canTransitionTo(SourceState targetState) const
{
    return isValidTransition(state_, targetState);
}

bool Source::isValidTransition(SourceState from, SourceState to)
{
    // State transition matrix per PRD
    switch (from)
    {
        case SourceState::DISCOVERED:
            return to == SourceState::ACTIVE;

        case SourceState::ACTIVE:
            return to == SourceState::INACTIVE ||
                   to == SourceState::ORPHANED ||
                   to == SourceState::STALE;

        case SourceState::INACTIVE:
            return to == SourceState::ACTIVE ||
                   to == SourceState::ORPHANED;

        case SourceState::ORPHANED:
            return to == SourceState::ACTIVE;

        case SourceState::STALE:
            return to == SourceState::ACTIVE ||
                   to == SourceState::ORPHANED;

        default:
            return false;
    }
}

void Source::setDisplayName(const juce::String& name)
{
    if (name.length() > MAX_DISPLAY_NAME_LENGTH)
    {
        displayName_ = name.substring(0, MAX_DISPLAY_NAME_LENGTH);
    }
    else
    {
        displayName_ = name;
    }
}

void Source::updateLastAudioTime()
{
    // Just update the timestamp atomically. 
    // State transition is handled in updateActivityState() on the message thread
    // to avoid race conditions and complex locking on the audio thread.
    lastAudioTimeMs_.store(juce::Time::currentTimeMillis(), std::memory_order_relaxed);
}

int64_t Source::getTimeSinceLastAudio() const
{
    // Guard against system clock adjustments that could produce negative values
    int64_t delta = juce::Time::currentTimeMillis() - lastAudioTimeMs_.load(std::memory_order_relaxed);
    return delta >= 0 ? delta : 0;
}

void Source::updateActivityState()
{
    auto timeSinceAudio = getTimeSinceLastAudio();
    SourceState currentState = state_.load(std::memory_order_relaxed);

    // Check for transition TO Active
    // If we have received audio recently and are not ACTIVE, become ACTIVE
    // Note: Do not auto-transition from ORPHANED state (must be explicitly re-adopted)
    if (timeSinceAudio < INACTIVE_THRESHOLD_MS)
    {
        if (currentState != SourceState::ACTIVE && currentState != SourceState::ORPHANED)
        {
            (void)transitionTo(SourceState::ACTIVE);
        }
        // Already active or explicitly orphaned, nothing to do
    }
    // Check for transitions FROM Active
    else if (currentState == SourceState::ACTIVE)
    {
        if (timeSinceAudio > STALE_THRESHOLD_MS)
        {
            (void)transitionTo(SourceState::STALE);
        }
        else if (timeSinceAudio > INACTIVE_THRESHOLD_MS)
        {
            (void)transitionTo(SourceState::INACTIVE);
        }
    }
    // Check for transitions from Inactive
    else if (currentState == SourceState::INACTIVE)
    {
        if (timeSinceAudio > STALE_THRESHOLD_MS)
        {
            (void)transitionTo(SourceState::STALE);
        }
    }
}

void Source::updateCorrelationMetrics(float correlation) noexcept
{
    correlationMetrics_.correlationCoefficient = juce::jlimit(-1.0f, 1.0f, correlation);
    correlationMetrics_.isValid = true;
}

void Source::updateSignalMetrics(float rms, float peak, float dcOffset) noexcept
{
    signalMetrics_.rmsLevel = rms;
    signalMetrics_.peakLevel = peak;
    signalMetrics_.dcOffset = dcOffset;
    signalMetrics_.hasSignal = rms > -60.0f; // Signal detected if above -60 dBFS
}

} // namespace oscil