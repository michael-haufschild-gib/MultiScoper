/*
    Oscil - Audio Data Provider Interface
    High-level facade for accessing audio data and plugin state
*/

#pragma once

#include <memory>

namespace oscil
{

// Forward declarations
class IAudioBuffer;
class OscilState;
class TimingEngine;
struct SourceId;

/**
 * High-level interface for accessing audio data and plugin state.
 * Acts as a facade that decouples UI/visualization components from
 * the concrete PluginProcessor implementation.
 *
 * This enables:
 * - Dependency injection for testability
 * - Clear abstraction boundary between audio and UI layers
 * - Simplified access patterns for visualization components
 */
class IAudioDataProvider
{
public:
    virtual ~IAudioDataProvider() = default;

    /**
     * Get the audio buffer for a specific source.
     * @param sourceId The source to get the buffer for
     * @return Shared pointer to the audio buffer, or nullptr if not found
     */
    virtual std::shared_ptr<IAudioBuffer> getBuffer(const SourceId& sourceId) = 0;

    /**
     * Get the plugin state for accessing oscillators, panes, etc.
     */
    virtual OscilState& getState() = 0;

    /**
     * Get the current CPU usage percentage.
     * Based on time spent in audio processing relative to available time.
     */
    virtual float getCpuUsage() const = 0;

    /**
     * Get the current source sample rate (DAW's sample rate).
     */
    virtual double getSampleRate() const = 0;

    /**
     * Get the capture rate used for display buffers (after decimation).
     * This is the effective sample rate for display data.
     * Use this for timing calculations related to displayed waveforms.
     */
    virtual int getCaptureRate() const = 0;

    /**
     * Get the timing engine for synchronization and timing queries.
     */
    virtual TimingEngine& getTimingEngine() = 0;

    /**
     * Get this processor instance's registered source ID.
     */
    virtual SourceId getSourceId() const = 0;
};

} // namespace oscil
