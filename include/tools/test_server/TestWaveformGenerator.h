/*
    Oscil - Test Waveform Generator
    Shared waveform generation utilities for test server handlers
*/

#pragma once

#include "core/SharedCaptureBuffer.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <string>

namespace oscil
{

/**
 * Generate a test waveform into the given audio buffer.
 *
 * Supported waveform types: "sine", "square", "triangle", "sawtooth",
 * "noise", "dc", "silence" (or any unknown type).
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
 * Create standard test metadata for audio injection.
 *
 * @param sampleRate  Sample rate in Hz
 * @param numSamples  Number of samples in the frame
 */
CaptureFrameMetadata makeTestMetadata(float sampleRate, int numSamples);

} // namespace oscil
