/*
    MultiScoper - Debug Logging Infrastructure

    Provides comprehensive debug logging for AI agent consumption.
    All MULTISCOPER_LOG output is captured by TestLogCapture and surfaced
    via the /diagnostic/snapshot HTTP endpoint.

    Build behavior:
    - Debug builds (JUCE_DEBUG): active, logs via juce::Logger::writeToLog.
    - Release builds: compiled out entirely, zero overhead.

    Thread safety:
    - MULTISCOPER_LOG uses juce::Logger::writeToLog which acquires a mutex.
    - Safe to call from the message thread and background threads.
    - NEVER call from the audio thread or OpenGL render thread.
    - For those threads, continue using DBG() which writes to stderr.

    Format: [CATEGORY] message with key=value pairs
    Example: [STATE] addOscillator: name="Osc 1" sourceId=abc paneId=def
*/

#pragma once

#include <juce_core/juce_core.h>

// Primary logging macro — captured by TestLogCapture in debug builds.
// Compiled out in release builds for zero overhead in distribution.
// Use on message thread and background threads only.
// category: unquoted tag (STATE, REGISTRY, PLUGIN, etc.)
// msg: stream expression using << operators
//
// Usage: MULTISCOPER_LOG(STATE, "addOscillator: name=" << osc.getName())
#if JUCE_DEBUG
    #define MULTISCOPER_LOG(category, msg)                                                                           \
        do                                                                                                           \
        {                                                                                                            \
            juce::String const _multiscoper_log_msg = [&]() {                                                        \
                juce::String _s;                                                                                     \
                _s << "[" #category "] " << msg; /* NOLINT(bugprone-macro-parentheses) stream expression fragment */ \
                return _s;                                                                                           \
            }();                                                                                                     \
            juce::Logger::writeToLog(_multiscoper_log_msg);                                                          \
        }                                                                                                            \
        while (0)
#else
    #define MULTISCOPER_LOG(category, msg) \
        do                                 \
        {                                  \
        }                                  \
        while (0)
#endif
