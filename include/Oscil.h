/*
    Oscil - Multi-Track Audio Oscilloscope Plugin
    Main header file
*/

#pragma once

// JUCE includes
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#ifdef OSCIL_ENABLE_OPENGL
    #include <juce_opengl/juce_opengl.h>
#endif

// Core includes
#include "core/GlobalPreferences.h"
#include "core/InstanceRegistry.h"
#include "core/OscilState.h"
#include "core/Oscillator.h"
#include "core/Pane.h"
#include "core/SharedCaptureBuffer.h"

// DSP includes
#include "core/dsp/SignalProcessor.h"
#include "core/dsp/TimingEngine.h"

// UI includes
#include "ui/theme/ThemeManager.h"

namespace oscil
{
// Version information
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;

// Performance limits from PRD
constexpr int MAX_TRACKS = 64;
constexpr int MAX_OSCILLATORS = 128;
constexpr int DEFAULT_MAX_OSCILLATORS = 32;
constexpr int MIN_SAMPLE_RATE = 44100;
constexpr int MAX_SAMPLE_RATE = 192000;
constexpr int MIN_BUFFER_SIZE = 32;
constexpr int MAX_BUFFER_SIZE = 2048;
constexpr int TARGET_FPS = 60;
constexpr int MAX_FPS = 120;
constexpr size_t MAX_MEMORY_PER_TRACK_BYTES = static_cast<size_t>(10) * 1024 * 1024; // 10MB
constexpr size_t CAPTURE_BUFFER_SIZE_BYTES = static_cast<size_t>(1024) * 1024;       // 1MB per instance

} // namespace oscil
