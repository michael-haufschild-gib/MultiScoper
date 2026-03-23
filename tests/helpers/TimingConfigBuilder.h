/*
    Oscil - Timing Config Builder
    Fluent builder for creating EngineTimingConfig objects in tests
*/

#pragma once

#include "core/dsp/TimingConfig.h"
#include "core/dsp/TimingEngine.h"

namespace oscil::test
{

/**
 * Fluent builder for creating test EngineTimingConfig objects
 *
 * Example usage:
 *   auto config = TimingConfigBuilder()
 *       .withTimingMode(TimingMode::MELODIC)
 *       .withNoteInterval(EngineNoteInterval::NOTE_1_4TH)
 *       .withHostBPM(120.0f)
 *       .withHostSyncEnabled(true)
 *       .build();
 */
class TimingConfigBuilder
{
public:
    TimingConfigBuilder()
    {
        // Start with default config
        config_ = EngineTimingConfig();
    }

    /**
     * Set timing mode
     */
    TimingConfigBuilder& withTimingMode(TimingMode mode)
    {
        config_.timingMode = mode;
        return *this;
    }

    /**
     * Set as TIME mode
     */
    TimingConfigBuilder& asTimeMode()
    {
        config_.timingMode = TimingMode::TIME;
        return *this;
    }

    /**
     * Set as MELODIC mode
     */
    TimingConfigBuilder& asMelodicMode()
    {
        config_.timingMode = TimingMode::MELODIC;
        return *this;
    }

    /**
     * Set time interval in milliseconds (for TIME mode)
     */
    TimingConfigBuilder& withTimeIntervalMs(float ms)
    {
        config_.timeIntervalMs =
            juce::jlimit(EngineTimingConfig::MIN_TIME_INTERVAL_MS, EngineTimingConfig::MAX_TIME_INTERVAL_MS, ms);
        return *this;
    }

    /**
     * Set note interval (for MELODIC mode)
     */
    TimingConfigBuilder& withNoteInterval(EngineNoteInterval interval)
    {
        config_.noteInterval = interval;
        return *this;
    }

    /**
     * Set host sync enabled
     */
    TimingConfigBuilder& withHostSyncEnabled(bool enabled)
    {
        config_.hostSyncEnabled = enabled;
        return *this;
    }

    /**
     * Enable host sync
     */
    TimingConfigBuilder& withHostSync()
    {
        config_.hostSyncEnabled = true;
        return *this;
    }

    /**
     * Disable host sync (free-running)
     */
    TimingConfigBuilder& withFreeRunning()
    {
        config_.hostSyncEnabled = false;
        return *this;
    }

    /**
     * Set host BPM
     */
    TimingConfigBuilder& withHostBPM(float bpm)
    {
        config_.hostBPM = juce::jlimit(EngineTimingConfig::MIN_BPM, EngineTimingConfig::MAX_BPM, bpm);
        return *this;
    }

    /**
     * Set sync to playhead
     */
    TimingConfigBuilder& withSyncToPlayhead(bool enabled)
    {
        config_.syncToPlayhead = enabled;
        return *this;
    }

    /**
     * Set trigger mode
     */
    TimingConfigBuilder& withTriggerMode(WaveformTriggerMode mode)
    {
        config_.triggerMode = mode;
        return *this;
    }

    /**
     * Set as free-running (no triggering)
     */
    TimingConfigBuilder& withoutTriggering()
    {
        config_.triggerMode = WaveformTriggerMode::None;
        return *this;
    }

    /**
     * Set as rising edge trigger
     */
    TimingConfigBuilder& withRisingEdgeTrigger()
    {
        config_.triggerMode = WaveformTriggerMode::RisingEdge;
        return *this;
    }

    /**
     * Set as falling edge trigger
     */
    TimingConfigBuilder& withFallingEdgeTrigger()
    {
        config_.triggerMode = WaveformTriggerMode::FallingEdge;
        return *this;
    }

    /**
     * Set as level trigger
     */
    TimingConfigBuilder& withLevelTrigger()
    {
        config_.triggerMode = WaveformTriggerMode::Level;
        return *this;
    }

    /**
     * Set as manual trigger
     */
    TimingConfigBuilder& withManualTrigger()
    {
        config_.triggerMode = WaveformTriggerMode::Manual;
        return *this;
    }

    /**
     * Set as both-edges trigger
     */
    TimingConfigBuilder& withBothEdgesTrigger()
    {
        config_.triggerMode = WaveformTriggerMode::BothEdges;
        return *this;
    }

    /**
     * Set trigger threshold (0.0 to 1.0)
     */
    TimingConfigBuilder& withTriggerThreshold(float threshold)
    {
        config_.triggerThreshold = juce::jlimit(0.0f, 1.0f, threshold);
        return *this;
    }

    /**
     * Set trigger channel
     */
    TimingConfigBuilder& withTriggerChannel(int channel)
    {
        config_.triggerChannel = channel;
        return *this;
    }

    /**
     * Set trigger hysteresis
     */
    TimingConfigBuilder& withTriggerHysteresis(float hysteresis)
    {
        config_.triggerHysteresis = hysteresis;
        return *this;
    }

    /**
     * Build and return the EngineTimingConfig
     */
    EngineTimingConfig build() const { return config_; }

    /**
     * Build and apply to a TimingEngine
     */
    void applyTo(TimingEngine& engine) const { engine.setConfig(config_); }

private:
    EngineTimingConfig config_;
};

/**
 * Builder for HostTimingInfo (for testing host sync)
 *
 * Example usage:
 *   auto hostInfo = HostTimingInfoBuilder()
 *       .withBPM(140.0)
 *       .withPlaying(true)
 *       .withPPQPosition(16.5)
 *       .build();
 */
class HostTimingInfoBuilder
{
public:
    HostTimingInfoBuilder() { info_ = HostTimingInfo(); }

    /**
     * Set BPM
     */
    HostTimingInfoBuilder& withBPM(double bpm)
    {
        info_.bpm = bpm;
        return *this;
    }

    /**
     * Set PPQ position
     */
    HostTimingInfoBuilder& withPPQPosition(double ppq)
    {
        info_.ppqPosition = ppq;
        return *this;
    }

    /**
     * Set playing state
     */
    HostTimingInfoBuilder& withPlaying(bool playing)
    {
        info_.isPlaying = playing;
        return *this;
    }

    /**
     * Set as playing
     */
    HostTimingInfoBuilder& asPlaying()
    {
        info_.isPlaying = true;
        info_.transportState = HostTimingInfo::TransportState::PLAYING;
        return *this;
    }

    /**
     * Set as stopped
     */
    HostTimingInfoBuilder& asStopped()
    {
        info_.isPlaying = false;
        info_.transportState = HostTimingInfo::TransportState::STOPPED;
        return *this;
    }

    /**
     * Set sample rate
     */
    HostTimingInfoBuilder& withSampleRate(double sampleRate)
    {
        info_.sampleRate = sampleRate;
        return *this;
    }

    /**
     * Set time in samples
     */
    HostTimingInfoBuilder& withTimeInSamples(int64_t samples)
    {
        info_.timeInSamples = samples;
        return *this;
    }

    /**
     * Set time signature
     */
    HostTimingInfoBuilder& withTimeSignature(int numerator, int denominator)
    {
        info_.timeSigNumerator = numerator;
        info_.timeSigDenominator = denominator;
        return *this;
    }

    /**
     * Set transport state
     */
    HostTimingInfoBuilder& withTransportState(HostTimingInfo::TransportState state)
    {
        info_.transportState = state;
        return *this;
    }

    /**
     * Build and return the HostTimingInfo
     */
    HostTimingInfo build() const { return info_; }

private:
    HostTimingInfo info_;
};

} // namespace oscil::test
