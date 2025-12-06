/*
    Oscil - Source Entity
    Represents an audio source from a DAW track with state management and ownership tracking
    PRD aligned: Entities -> Source
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "core/analysis/AnalysisEngine.h"
#include <atomic>
#include <vector>
#include <optional>

namespace oscil
{

// Forward declarations
class SharedCaptureBuffer;

/**
 * Source state machine states per PRD
 * Tracks lifecycle of an audio source from discovery to removal
 */
enum class SourceState
{
    DISCOVERED,  // New source detected, awaiting audio data
    ACTIVE,      // Receiving audio samples, owning instance operational
    INACTIVE,    // No audio samples received for >1 second, but owning instance alive
    ORPHANED,    // Owning instance terminated, no backup instances available
    STALE        // No audio samples for >30 seconds, ownership questionable
};

/**
 * Convert SourceState to string for serialization/display
 */
inline juce::String sourceStateToString(SourceState state)
{
    switch (state)
    {
        case SourceState::DISCOVERED: return "DISCOVERED";
        case SourceState::ACTIVE:     return "ACTIVE";
        case SourceState::INACTIVE:   return "INACTIVE";
        case SourceState::ORPHANED:   return "ORPHANED";
        case SourceState::STALE:      return "STALE";
        default:                      return "UNKNOWN";
    }
}

/**
 * Convert string to SourceState for deserialization
 */
inline SourceState stringToSourceState(const juce::String& str)
{
    if (str == "DISCOVERED") return SourceState::DISCOVERED;
    if (str == "ACTIVE")     return SourceState::ACTIVE;
    if (str == "INACTIVE")   return SourceState::INACTIVE;
    if (str == "ORPHANED")   return SourceState::ORPHANED;
    if (str == "STALE")      return SourceState::STALE;
    return SourceState::DISCOVERED;
}

/**
 * Channel configuration for a source
 */
enum class ChannelConfig
{
    MONO,
    STEREO
};

inline juce::String channelConfigToString(ChannelConfig config)
{
    return config == ChannelConfig::MONO ? "MONO" : "STEREO";
}

inline ChannelConfig stringToChannelConfig(const juce::String& str)
{
    return str == "MONO" ? ChannelConfig::MONO : ChannelConfig::STEREO;
}

/**
 * Unique identifier for a Source
 */
struct SourceId
{
    juce::String id;

    bool operator==(const SourceId& other) const { return id == other.id; }
    bool operator!=(const SourceId& other) const { return !(*this == other); }
    bool operator<(const SourceId& other) const { return id < other.id; }

    [[nodiscard]] static SourceId generate();
    static SourceId invalid() { return SourceId{ "" }; }
    static SourceId noSource() { return SourceId{ "NO_SOURCE" }; }
    bool isValid() const { return id.isNotEmpty() && id != "NO_SOURCE"; }
    bool isNoSource() const { return id == "NO_SOURCE"; }
};

/**
 * Hash function for SourceId
 */
struct SourceIdHash
{
    std::size_t operator()(const SourceId& sid) const
    {
        return std::hash<std::string>{}(sid.id.toStdString());
    }
};

/**
 * Unique identifier for a Plugin Instance
 */
struct InstanceId
{
    juce::String id;

    bool operator==(const InstanceId& other) const { return id == other.id; }
    bool operator!=(const InstanceId& other) const { return !(*this == other); }

    [[nodiscard]] static InstanceId generate();
    static InstanceId invalid() { return InstanceId{ "" }; }
    bool isValid() const { return id.isNotEmpty(); }
};

/**
 * Hash function for InstanceId
 */
struct InstanceIdHash
{
    std::size_t operator()(const InstanceId& iid) const
    {
        return std::hash<std::string>{}(iid.id.toStdString());
    }
};

/**
 * Stereo correlation metrics for STEREO sources
 */
struct CorrelationMetrics
{
    float correlationCoefficient = 0.0f;  // -1.0 to 1.0, Pearson correlation
    bool isValid = false;
};

/**
 * Signal metrics for monitoring
 */
struct SignalMetrics
{
    float rmsLevel = 0.0f;       // RMS level in dBFS
    float peakLevel = 0.0f;      // Peak level in dBFS
    float dcOffset = 0.0f;       // DC offset measurement
    bool hasSignal = false;      // True if signal detected above noise floor
};

/**
 * Source entity representing an audio source from a DAW track.
 * PRD aligned with full state machine and ownership tracking.
 *
 * Key responsibilities:
 * - Track audio source lifecycle (state machine)
 * - Manage instance ownership and backup instances
 * - Provide audio data access via capture buffer
 * - Track signal metrics and correlation
 */
class Source
{
public:
    // Timing constants for state transitions (PRD defined)
    static constexpr int INACTIVE_THRESHOLD_MS = 1000;    // 1 second to INACTIVE
    static constexpr int STALE_THRESHOLD_MS = 30000;      // 30 seconds to STALE
    static constexpr int ORPHAN_CLEANUP_MS = 300000;      // 5 minutes for orphan cleanup
    static constexpr int HEARTBEAT_INTERVAL_MS = 100;     // 100ms heartbeat

    // Display name constraints
    static constexpr int MAX_DISPLAY_NAME_LENGTH = 64;

    /**
     * Create a new source with generated ID
     */
    Source();

    /**
     * Create a source with specified ID
     */
    explicit Source(const SourceId& sourceId);

    /**
     * Create a source from serialized ValueTree
     */
    explicit Source(const juce::ValueTree& state);

    /**
     * Serialize to ValueTree for persistence
     */
    juce::ValueTree toValueTree() const;

    /**
     * Load state from ValueTree
     */
    void fromValueTree(const juce::ValueTree& state);

    // === Identity ===
    [[nodiscard]] SourceId getId() const noexcept { return id_; }
    [[nodiscard]] juce::String getDawTrackId() const noexcept { return dawTrackId_; }
    void setDawTrackId(const juce::String& trackId) { dawTrackId_ = trackId; }

    // === Ownership ===
    [[nodiscard]] InstanceId getOwningInstanceId() const noexcept { return owningInstanceId_; }
    void setOwningInstanceId(const InstanceId& instanceId);

    [[nodiscard]] const std::vector<InstanceId>& getBackupInstanceIds() const noexcept { return backupInstanceIds_; }
    void addBackupInstance(const InstanceId& instanceId);
    void removeBackupInstance(const InstanceId& instanceId);
    [[nodiscard]] bool hasBackupInstances() const noexcept { return !backupInstanceIds_.empty(); }
    [[nodiscard]] std::optional<InstanceId> getNextBackupInstance() const;

    /**
     * Transfer ownership to the next backup instance
     * @return true if transfer successful, false if no backup available
     */
    [[nodiscard]] bool transferOwnership();

    // === Audio Configuration ===
    [[nodiscard]] float getSampleRate() const noexcept { return sampleRate_; }
    void setSampleRate(float rate) noexcept { sampleRate_ = rate; }

    [[nodiscard]] ChannelConfig getChannelConfig() const noexcept { return channelConfig_; }
    void setChannelConfig(ChannelConfig config) noexcept { channelConfig_ = config; }

    [[nodiscard]] int getBufferSize() const noexcept { return bufferSize_; }
    void setBufferSize(int size) noexcept { bufferSize_ = size; }

    // === State Machine ===
    [[nodiscard]] SourceState getState() const noexcept { return state_; }

    /**
     * Transition to a new state with validation
     * @return true if transition is valid and performed
     */
    [[nodiscard]] bool transitionTo(SourceState newState);

    /**
     * Check if transition to target state is valid
     */
    [[nodiscard]] bool canTransitionTo(SourceState targetState) const;

    // === Activity Tracking ===
    [[nodiscard]] bool isActive() const noexcept { return state_ == SourceState::ACTIVE; }

    [[nodiscard]] juce::String getDisplayName() const noexcept { return displayName_; }
    void setDisplayName(const juce::String& name);

    /**
     * Update last audio time (call when audio samples received)
     */
    void updateLastAudioTime();

    /**
     * Get time since last audio in milliseconds
     */
    [[nodiscard]] int64_t getTimeSinceLastAudio() const;

    /**
     * Check and update state based on audio activity timing
     * Call periodically to handle INACTIVE/STALE transitions
     */
    void updateActivityState();

    // === Metrics ===
    [[nodiscard]] const CorrelationMetrics& getCorrelationMetrics() const noexcept { return correlationMetrics_; }
    void updateCorrelationMetrics(float correlation) noexcept;

    [[nodiscard]] const SignalMetrics& getSignalMetrics() const noexcept { return signalMetrics_; }
    void updateSignalMetrics(float rms, float peak, float dcOffset) noexcept;
    
    // === Analysis ===
    [[nodiscard]] AnalysisEngine& getAnalysisEngine() { return analysisEngine_; }
    [[nodiscard]] const AnalysisEngine& getAnalysisEngine() const { return analysisEngine_; }

    // === Capture Buffer ===
    [[nodiscard]] std::shared_ptr<SharedCaptureBuffer> getCaptureBuffer() const noexcept { return captureBuffer_; }
    void setCaptureBuffer(std::shared_ptr<SharedCaptureBuffer> buffer) noexcept { captureBuffer_ = buffer; }

    // === Timestamps ===
    [[nodiscard]] juce::Time getCreatedAt() const noexcept { return createdAt_; }
    [[nodiscard]] juce::Time getLastHeartbeat() const noexcept
    {
        return juce::Time(lastHeartbeatMs_.load(std::memory_order_relaxed));
    }
    void updateHeartbeat() noexcept
    {
        lastHeartbeatMs_.store(juce::Time::currentTimeMillis(), std::memory_order_relaxed);
    }

    // Schema version for migration
    static constexpr int CURRENT_SCHEMA_VERSION = 1;

private:
    // Identity
    SourceId id_;
    juce::String dawTrackId_;

    // Ownership
    InstanceId owningInstanceId_;
    std::vector<InstanceId> backupInstanceIds_;

    // Audio configuration
    float sampleRate_ = 44100.0f;
    ChannelConfig channelConfig_ = ChannelConfig::STEREO;
    int bufferSize_ = 512;

    // State
    std::atomic<SourceState> state_{ SourceState::DISCOVERED };
    std::atomic<bool> isActiveFlag_{ false };
    juce::String displayName_;

    // Timestamps
    juce::Time createdAt_;
    std::atomic<int64_t> lastHeartbeatMs_{ 0 };  // Thread-safe heartbeat timestamp
    std::atomic<int64_t> lastAudioTimeMs_{ 0 };

    // Metrics
    CorrelationMetrics correlationMetrics_;
    SignalMetrics signalMetrics_;
    
    // Analysis Engine
    AnalysisEngine analysisEngine_;

    // Audio data
    std::shared_ptr<SharedCaptureBuffer> captureBuffer_;

    int schemaVersion_ = CURRENT_SCHEMA_VERSION;

    // State transition validation
    static bool isValidTransition(SourceState from, SourceState to);
};

} // namespace oscil