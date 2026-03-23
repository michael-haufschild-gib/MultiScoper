/*
    Oscil - Host Information Entity
    Stores DAW/host information for proper integration and timing
    PRD aligned: Entities -> HostInfo
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include <cmath>

namespace oscil
{

/**
 * Transport state from the host DAW
 */
enum class TransportState
{
    STOPPED,
    PLAYING,
    RECORDING,
    PAUSED
};

inline juce::String transportStateToString(TransportState state)
{
    switch (state)
    {
        case TransportState::STOPPED:
            return "STOPPED";
        case TransportState::PLAYING:
            return "PLAYING";
        case TransportState::RECORDING:
            return "RECORDING";
        case TransportState::PAUSED:
            return "PAUSED";
    }
    jassertfalse; // Unhandled TransportState enum value
    return "STOPPED";
}

inline TransportState stringToTransportState(const juce::String& str)
{
    if (str == "PLAYING")
        return TransportState::PLAYING;
    if (str == "RECORDING")
        return TransportState::RECORDING;
    if (str == "PAUSED")
        return TransportState::PAUSED;
    return TransportState::STOPPED;
}

/**
 * Time signature representation
 */
struct TimeSignature
{
    int numerator = 4;   // Beats per bar
    int denominator = 4; // Note value that gets one beat

    bool operator==(const TimeSignature& other) const
    {
        return numerator == other.numerator && denominator == other.denominator;
    }

    juce::String toString() const { return juce::String(numerator) + "/" + juce::String(denominator); }

    static TimeSignature fromString(const juce::String& str)
    {
        TimeSignature ts;
        auto parts = juce::StringArray::fromTokens(str, "/", "");
        if (parts.size() == 2)
        {
            ts.numerator = parts[0].getIntValue();
            ts.denominator = parts[1].getIntValue();
        }
        return ts;
    }
};

/**
 * Host information structure containing DAW-provided metadata
 * PRD aligned with full host integration data
 */
struct HostInfo
{
    // DAW identification
    juce::String dawName;    // Max 64 chars - DAW application name
    juce::String dawVersion; // Max 32 chars - DAW version string
    int processId = 0;       // > 0 - Operating system process ID

    // Track information
    juce::String trackName; // Max 128 chars - DAW track name (if available)
    int trackIndex = 0;     // >= 0 - DAW track number/index

    // Tempo and timing
    float bpm = 120.0f;          // 20.0-999.0 - Current host BPM
    TimeSignature timeSignature; // Current time signature (4/4, 3/4, etc.)
    TransportState transportState = TransportState::STOPPED;

    // Playhead position
    double ppqPosition = 0.0;   // Position in pulses per quarter note
    double timeInSeconds = 0.0; // Position in seconds
    int64_t timeInSamples = 0;  // Position in samples

    // Loop information
    bool isLooping = false;
    double loopStartPpq = 0.0;
    double loopEndPpq = 0.0;

    // Validation
    static constexpr int MAX_DAW_NAME_LENGTH = 64;
    static constexpr int MAX_DAW_VERSION_LENGTH = 32;
    static constexpr int MAX_TRACK_NAME_LENGTH = 128;
    static constexpr float MIN_BPM = 20.0f;
    static constexpr float MAX_BPM = 999.0f;

    /**
     * Update from JUCE AudioPlayHead position info
     */
    void updateFromPlayHead(const juce::AudioPlayHead::PositionInfo& posInfo)
    {
        if (auto bpmOpt = posInfo.getBpm())
        {
            double bpmValue = *bpmOpt;
            if (std::isfinite(bpmValue))
            {
                bpm = static_cast<float>(
                    juce::jlimit(static_cast<double>(MIN_BPM), static_cast<double>(MAX_BPM), bpmValue));
            }
        }

        if (auto tsOpt = posInfo.getTimeSignature())
        {
            timeSignature.numerator = tsOpt->numerator;
            timeSignature.denominator = tsOpt->denominator;
        }

        if (auto ppqOpt = posInfo.getPpqPosition())
        {
            if (std::isfinite(*ppqOpt))
                ppqPosition = *ppqOpt;
        }

        if (auto timeOpt = posInfo.getTimeInSeconds())
        {
            if (std::isfinite(*timeOpt))
                timeInSeconds = *timeOpt;
        }

        if (auto samplesOpt = posInfo.getTimeInSamples())
            timeInSamples = *samplesOpt;

        transportState = posInfo.getIsPlaying() ? TransportState::PLAYING : TransportState::STOPPED;
        if (posInfo.getIsRecording())
            transportState = TransportState::RECORDING;

        isLooping = posInfo.getIsLooping();
        if (auto loopPoints = posInfo.getLoopPoints())
        {
            if (std::isfinite(loopPoints->ppqStart))
                loopStartPpq = loopPoints->ppqStart;
            if (std::isfinite(loopPoints->ppqEnd))
                loopEndPpq = loopPoints->ppqEnd;
        }
    }

    /**
     * Check if BPM is within valid range
     */
    bool isValidBpm() const { return bpm >= MIN_BPM && bpm <= MAX_BPM; }

    /**
     * Get bar position from PPQ
     */
    double getBarPosition() const
    {
        if (timeSignature.numerator <= 0 || !std::isfinite(ppqPosition))
            return 0.0;
        return ppqPosition / timeSignature.numerator;
    }

    /**
     * Get beat position within current bar (0 to numerator-1)
     */
    double getBeatInBar() const
    {
        if (timeSignature.numerator <= 0 || !std::isfinite(ppqPosition))
            return 0.0;
        return std::fmod(ppqPosition, static_cast<double>(timeSignature.numerator));
    }

    /**
     * Calculate milliseconds per beat at current BPM
     */
    double getMsPerBeat() const
    {
        if (!std::isfinite(bpm) || bpm <= 0.0f)
            return 500.0; // Default to 120 BPM
        return 60000.0 / bpm;
    }

    /**
     * Calculate milliseconds per bar at current BPM and time signature
     */
    double getMsPerBar() const
    {
        if (timeSignature.numerator <= 0)
            return 0.0;
        return getMsPerBeat() * timeSignature.numerator;
    }
};

} // namespace oscil
