/*
    Oscil - Render Stats
    Handles timing and diagnostic logging for the render engine.
*/

#pragma once

#include <juce_core/juce_core.h>

namespace oscil
{

// Release-mode logging macro for render engine debugging
#define RE_LOG(msg) DBG("[RenderEngine] " << msg)

// Rate-limited logging macro (logs every N frames)
#define RE_LOG_THROTTLED(interval_frames, msg) \
    do { \
        static int _log_counter = 0; \
        if (++_log_counter >= interval_frames) { \
            _log_counter = 0; \
            RE_LOG(msg); \
        } \
    } while (0)

class RenderStats
{
public:
    RenderStats() = default;
    ~RenderStats() = default;

    void update(float deltaTime)
    {
        deltaTime_ = deltaTime;
        accumulatedTime_ += deltaTime;
    }

    [[nodiscard]] float getDeltaTime() const { return deltaTime_; }
    [[nodiscard]] float getTime() const { return accumulatedTime_; }

    void reset()
    {
        deltaTime_ = 0.0f;
        accumulatedTime_ = 0.0f;
    }

private:
    float deltaTime_ = 0.0f;
    float accumulatedTime_ = 0.0f;
};

} // namespace oscil
