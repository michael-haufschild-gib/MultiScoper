/*
    Oscil - Audio Configuration Entity
    Stores audio system configuration from the host
    PRD aligned: Entities -> AudioConfig
*/

#pragma once

#include <juce_core/juce_core.h>

namespace oscil
{

/**
 * Plugin format type
 */
enum class PluginFormat
{
    VST3,
    CLAP,
    AU,
    STANDALONE
};

inline juce::String pluginFormatToString(PluginFormat format)
{
    switch (format)
    {
        case PluginFormat::VST3:       return "VST3";
        case PluginFormat::CLAP:       return "CLAP";
        case PluginFormat::AU:         return "AU";
        case PluginFormat::STANDALONE: return "STANDALONE";
        default:                       return "UNKNOWN";
    }
}

/**
 * Host threading model
 */
enum class HostThreadingModel
{
    SINGLE,            // Single-threaded host
    MULTI,             // Multi-threaded host
    REALTIME_SEPARATE  // Separate real-time audio thread
};

/**
 * Host timing precision
 */
enum class HostTimingPrecision
{
    SAMPLE_ACCURATE,  // Sample-accurate timing
    BLOCK_ACCURATE,   // Block-accurate timing
    APPROXIMATE       // Approximate timing
};

/**
 * Audio configuration structure containing host audio settings
 * PRD aligned with complete audio configuration
 */
struct AudioConfig
{
    // Audio format
    float sampleRate = 44100.0f;       // 44.1kHz-192kHz
    int bufferSize = 512;              // 32-2048 samples
    int channelCount = 2;              // 1-2 (mono/stereo)
    int bitDepth = 32;                 // 16, 24, 32

    // Processing state
    bool isRealtime = true;            // Real-time audio processing flag
    int reportedLatency = 0;           // 0-2048 samples - Latency reported to host
    bool bypassState = false;          // Current plugin bypass state from host

    // Constraints from PRD
    static constexpr float MIN_SAMPLE_RATE = 44100.0f;
    static constexpr float MAX_SAMPLE_RATE = 192000.0f;
    static constexpr int MIN_BUFFER_SIZE = 32;
    static constexpr int MAX_BUFFER_SIZE = 2048;
    static constexpr int MIN_CHANNELS = 1;
    static constexpr int MAX_CHANNELS = 2;
    static constexpr int MAX_LATENCY = 2048;

    /**
     * Validate sample rate is within supported range
     */
    bool isValidSampleRate() const
    {
        return sampleRate >= MIN_SAMPLE_RATE && sampleRate <= MAX_SAMPLE_RATE;
    }

    /**
     * Validate buffer size is within supported range
     */
    bool isValidBufferSize() const
    {
        return bufferSize >= MIN_BUFFER_SIZE && bufferSize <= MAX_BUFFER_SIZE;
    }

    /**
     * Validate channel count
     */
    bool isValidChannelCount() const
    {
        return channelCount >= MIN_CHANNELS && channelCount <= MAX_CHANNELS;
    }

    /**
     * Check if configuration is completely valid
     */
    bool isValid() const
    {
        return isValidSampleRate() && isValidBufferSize() && isValidChannelCount();
    }

    /**
     * Get buffer duration in milliseconds
     */
    double getBufferDurationMs() const
    {
        if (sampleRate <= 0.0f) return 0.0;
        return (static_cast<double>(bufferSize) / sampleRate) * 1000.0;
    }

    /**
     * Get buffer duration in seconds
     */
    double getBufferDurationSeconds() const
    {
        if (sampleRate <= 0.0f) return 0.0;
        return static_cast<double>(bufferSize) / sampleRate;
    }

    /**
     * Convert milliseconds to samples at current sample rate
     */
    int msToSamples(double ms) const
    {
        return static_cast<int>((ms / 1000.0) * sampleRate);
    }

    /**
     * Convert samples to milliseconds at current sample rate
     */
    double samplesToMs(int samples) const
    {
        if (sampleRate <= 0.0f) return 0.0;
        return (static_cast<double>(samples) / sampleRate) * 1000.0;
    }

    /**
     * Check if this is a stereo configuration
     */
    bool isStereo() const { return channelCount == 2; }

    /**
     * Check if this is a mono configuration
     */
    bool isMono() const { return channelCount == 1; }
};

/**
 * Host context information for advanced host integration
 * PRD aligned: Entities -> HostContext
 */
struct HostContext
{
    PluginFormat pluginFormat = PluginFormat::VST3;
    HostThreadingModel threadingModel = HostThreadingModel::REALTIME_SEPARATE;
    HostTimingPrecision timingPrecision = HostTimingPrecision::SAMPLE_ACCURATE;

    // Host capabilities
    bool supportsParameterAutomation = true;
    bool supportsStateChunks = true;
    bool supportsMidiMapping = false;
    bool supportsLatencyReporting = true;
    bool supportsBypassProcessing = true;
    bool supportsSampleRateChange = true;
    bool supportsBufferSizeChange = true;

    int maxParameterCount = 1000;  // Maximum parameters supported by host
};

} // namespace oscil
