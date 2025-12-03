/*
    Oscil - Source Builder
    Fluent builder for creating Source objects in tests
*/

#pragma once

#include "core/Source.h"
#include <memory>

namespace oscil::test
{

/**
 * Fluent builder for creating test Source objects
 *
 * Example usage:
 *   auto source = SourceBuilder()
 *       .withId("test-source")
 *       .withState(SourceState::ACTIVE)
 *       .withOwner(instanceId)
 *       .withDisplayName("Test Track")
 *       .withSampleRate(48000.0f)
 *       .build();
 */
class SourceBuilder
{
public:
    SourceBuilder()
        : sourceId_(SourceId::generate())
        , state_(SourceState::DISCOVERED)
        , channelConfig_(ChannelConfig::STEREO)
        , sampleRate_(44100.0f)
        , bufferSize_(512)
    {
    }

    /**
     * Set a specific source ID (otherwise auto-generated)
     */
    SourceBuilder& withId(const juce::String& id)
    {
        sourceId_ = SourceId{ id };
        return *this;
    }

    /**
     * Set a specific source ID
     */
    SourceBuilder& withId(const SourceId& id)
    {
        sourceId_ = id;
        return *this;
    }

    /**
     * Set the source state
     */
    SourceBuilder& withState(SourceState state)
    {
        state_ = state;
        return *this;
    }

    /**
     * Set the owning instance ID
     */
    SourceBuilder& withOwner(const InstanceId& owner)
    {
        owner_ = owner;
        return *this;
    }

    /**
     * Add a backup instance ID
     */
    SourceBuilder& withBackupInstance(const InstanceId& backup)
    {
        backupInstances_.push_back(backup);
        return *this;
    }

    /**
     * Set multiple backup instances
     */
    SourceBuilder& withBackupInstances(const std::vector<InstanceId>& backups)
    {
        backupInstances_ = backups;
        return *this;
    }

    /**
     * Set the DAW track ID
     */
    SourceBuilder& withDawTrackId(const juce::String& trackId)
    {
        dawTrackId_ = trackId;
        return *this;
    }

    /**
     * Set the display name
     */
    SourceBuilder& withDisplayName(const juce::String& name)
    {
        displayName_ = name;
        return *this;
    }

    /**
     * Set the channel configuration
     */
    SourceBuilder& withChannelConfig(ChannelConfig config)
    {
        channelConfig_ = config;
        return *this;
    }

    /**
     * Set as mono
     */
    SourceBuilder& asMono()
    {
        channelConfig_ = ChannelConfig::MONO;
        return *this;
    }

    /**
     * Set as stereo (default)
     */
    SourceBuilder& asStereo()
    {
        channelConfig_ = ChannelConfig::STEREO;
        return *this;
    }

    /**
     * Set the sample rate
     */
    SourceBuilder& withSampleRate(float sampleRate)
    {
        sampleRate_ = sampleRate;
        return *this;
    }

    /**
     * Set the buffer size
     */
    SourceBuilder& withBufferSize(int bufferSize)
    {
        bufferSize_ = bufferSize;
        return *this;
    }

    /**
     * Set correlation metrics
     */
    SourceBuilder& withCorrelation(float coefficient)
    {
        correlationCoefficient_ = coefficient;
        hasCorrelation_ = true;
        return *this;
    }

    /**
     * Set signal metrics
     */
    SourceBuilder& withSignalMetrics(float rmsDb, float peakDb, float dcOffset)
    {
        rmsDb_ = rmsDb;
        peakDb_ = peakDb;
        dcOffset_ = dcOffset;
        hasSignalMetrics_ = true;
        return *this;
    }

    /**
     * Set capture buffer (optional)
     */
    SourceBuilder& withCaptureBuffer(std::shared_ptr<SharedCaptureBuffer> buffer)
    {
        captureBuffer_ = buffer;
        return *this;
    }

    /**
     * Build and return a unique_ptr to the Source
     */
    std::unique_ptr<Source> buildUnique()
    {
        auto source = std::make_unique<Source>(sourceId_);

        // Transition to desired state
        if (state_ != SourceState::DISCOVERED)
        {
            // Must go through ACTIVE first
            if (state_ == SourceState::ACTIVE)
            {
                (void)source->transitionTo(SourceState::ACTIVE);
            }
            else if (state_ == SourceState::INACTIVE)
            {
                (void)source->transitionTo(SourceState::ACTIVE);
                (void)source->transitionTo(SourceState::INACTIVE);
            }
            else if (state_ == SourceState::ORPHANED)
            {
                (void)source->transitionTo(SourceState::ACTIVE);
                (void)source->transitionTo(SourceState::ORPHANED);
            }
            else if (state_ == SourceState::STALE)
            {
                (void)source->transitionTo(SourceState::ACTIVE);
                (void)source->transitionTo(SourceState::STALE);
            }
        }

        // Set owner and backups
        if (owner_.has_value())
        {
            source->setOwningInstanceId(owner_.value());
        }

        for (const auto& backup : backupInstances_)
        {
            source->addBackupInstance(backup);
        }

        // Set properties
        if (dawTrackId_.has_value())
        {
            source->setDawTrackId(dawTrackId_.value());
        }

        if (displayName_.has_value())
        {
            source->setDisplayName(displayName_.value());
        }

        source->setChannelConfig(channelConfig_);
        source->setSampleRate(sampleRate_);
        source->setBufferSize(bufferSize_);

        // Set metrics if provided
        if (hasCorrelation_)
        {
            source->updateCorrelationMetrics(correlationCoefficient_);
        }

        if (hasSignalMetrics_)
        {
            source->updateSignalMetrics(rmsDb_, peakDb_, dcOffset_);
        }

        // Set capture buffer if provided
        if (captureBuffer_)
        {
            source->setCaptureBuffer(captureBuffer_);
        }

        return source;
    }

    /**
     * Build and return a Source by value
     * NOTE: Disabled because Source has deleted copy constructor (atomic member)
     */
    // Source build()
    // {
    //     auto ptr = buildUnique();
    //     return std::move(*ptr);
    // }

    /**
     * Build and return a shared_ptr to the Source
     */
    std::shared_ptr<Source> buildShared()
    {
        return std::shared_ptr<Source>(buildUnique().release());
    }

private:
    SourceId sourceId_;
    SourceState state_;
    std::optional<InstanceId> owner_;
    std::vector<InstanceId> backupInstances_;
    std::optional<juce::String> dawTrackId_;
    std::optional<juce::String> displayName_;
    ChannelConfig channelConfig_;
    float sampleRate_;
    int bufferSize_;

    // Metrics
    bool hasCorrelation_ = false;
    float correlationCoefficient_ = 0.0f;
    bool hasSignalMetrics_ = false;
    float rmsDb_ = 0.0f;
    float peakDb_ = 0.0f;
    float dcOffset_ = 0.0f;

    std::shared_ptr<SharedCaptureBuffer> captureBuffer_;
};

/**
 * Helper function to quickly create a SourceId
 */
inline SourceId makeSourceId(const juce::String& id)
{
    return SourceId{ id };
}

/**
 * Helper function to quickly create an InstanceId
 */
inline InstanceId makeInstanceId(const juce::String& id)
{
    return InstanceId{ id };
}

} // namespace oscil::test
