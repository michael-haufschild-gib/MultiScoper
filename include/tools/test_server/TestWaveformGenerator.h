/*
    MultiScoper - Test Waveform Generator
    Shared waveform generation utilities for test server handlers
*/

#pragma once

#include "core/SharedCaptureBuffer.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <string>

namespace multiscoper
{

/**
 * Generate a test waveform into the given audio buffer.
 *
 * Supported waveform types: "sine", "square", "triangle", "sawtooth",
 * "noise", "dc", "silence". Unknown types fall back to silence, which
 * is fine for generate-as-best-effort usage. Callers that need to
 * reject typos should check isValidWaveformType() before calling.
 *
 * @param buffer      Output buffer (must be pre-allocated with desired channel/sample count)
 * @param waveformType One of the supported type strings
 * @param frequency   Waveform frequency in Hz
 * @param amplitude   Peak amplitude [0.0, 1.0]
 * @param sampleRate  Sample rate in Hz (clamped to 44100 if <= 0)
 */
void generateTestWaveform(juce::AudioBuffer<float>& buffer, const std::string& waveformType, float frequency,
                          float amplitude, float sampleRate);

/**
 * Check whether a string names a supported waveform type.
 *
 * Use this before generateTestWaveform when a typo'd type should be
 * rejected with an error rather than silently degraded to silence —
 * the HTTP handlers that echo waveformType back in their success
 * response can mislead callers into thinking their data was injected
 * when in fact it was zero-filled.
 */
bool isValidWaveformType(const std::string& waveformType);

/**
 * Create standard test metadata for audio injection.
 *
 * @param sampleRate  Sample rate in Hz
 * @param numSamples  Number of samples in the frame
 */
CaptureFrameMetadata makeTestMetadata(float sampleRate, int numSamples);

} // namespace multiscoper
