/*
    Oscil - Render Stats
    Handles timing and diagnostic logging for the render engine.
    Extended with profiling integration for detailed GPU timing.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <chrono>
#include <array>
#include <atomic>

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

/**
 * Render pass identifiers for timing breakdown
 */
enum class RenderPass
{
    BeginFrame = 0,
    GridRender,
    WaveformGeometry,
    EffectPipeline,
    Composite,
    GlobalEffects,
    BlitToScreen,
    EndFrame,
    COUNT
};

/**
 * Timing data for a single render pass
 */
struct RenderPassTiming
{
    double lastMs = 0.0;
    double avgMs = 0.0;
    double maxMs = 0.0;
    double minMs = std::numeric_limits<double>::max();
    int64_t count = 0;
    
    void record(double ms)
    {
        lastMs = ms;
        count++;
        maxMs = std::max(maxMs, ms);
        minMs = std::min(minMs, ms);
        // Exponential moving average (alpha = 0.1)
        avgMs = count == 1 ? ms : avgMs * 0.9 + ms * 0.1;
    }
    
    void reset()
    {
        lastMs = 0.0;
        avgMs = 0.0;
        maxMs = 0.0;
        minMs = std::numeric_limits<double>::max();
        count = 0;
    }
};

/**
 * RAII timer for render pass profiling
 */
class ScopedRenderPassTimer
{
public:
    using Clock = std::chrono::high_resolution_clock;
    
    ScopedRenderPassTimer(RenderPassTiming& target)
        : target_(target)
        , start_(Clock::now())
    {}
    
    ~ScopedRenderPassTimer()
    {
        auto end = Clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start_).count();
        target_.record(ms);
    }
    
private:
    RenderPassTiming& target_;
    Clock::time_point start_;
};

/**
 * Extended render statistics with per-pass timing and profiling integration
 */
class RenderStats
{
public:
    RenderStats() = default;
    ~RenderStats() = default;

    // ========================================================================
    // Basic Timing (unchanged API)
    // ========================================================================
    
    void update(float deltaTime)
    {
        deltaTime_ = deltaTime;
        accumulatedTime_ += deltaTime;
        
        // Update frame rate calculations
        frameCount_++;
        double frameTimeMs = deltaTime * 1000.0;
        totalFrameTimeMs_ += frameTimeMs;
        maxFrameTimeMs_ = std::max(maxFrameTimeMs_, static_cast<double>(frameTimeMs));
        minFrameTimeMs_ = std::min(minFrameTimeMs_, static_cast<double>(frameTimeMs));
        
        // Update FPS calculation (exponential moving average)
        if (deltaTime > 0.0f)
        {
            double instantFps = 1.0 / deltaTime;
            avgFps_ = frameCount_ == 1 ? instantFps : avgFps_ * 0.95 + instantFps * 0.05;
        }
    }

    [[nodiscard]] float getDeltaTime() const { return deltaTime_; }
    [[nodiscard]] float getTime() const { return accumulatedTime_; }

    void reset()
    {
        deltaTime_ = 0.0f;
        accumulatedTime_ = 0.0f;
        frameCount_ = 0;
        totalFrameTimeMs_ = 0.0;
        avgFps_ = 0.0;
        maxFrameTimeMs_ = 0.0;
        minFrameTimeMs_ = std::numeric_limits<double>::max();
        waveformCount_ = 0;
        effectCount_ = 0;
        
        for (auto& timing : passTiming_)
            timing.reset();
    }

    // ========================================================================
    // Extended Frame Statistics
    // ========================================================================
    
    [[nodiscard]] int64_t getFrameCount() const { return frameCount_; }
    [[nodiscard]] double getAvgFps() const { return avgFps_; }
    [[nodiscard]] double getCurrentFps() const { return deltaTime_ > 0 ? 1.0 / deltaTime_ : 0.0; }
    [[nodiscard]] double getAvgFrameTimeMs() const 
    { 
        return frameCount_ > 0 ? totalFrameTimeMs_ / frameCount_ : 0.0; 
    }
    [[nodiscard]] double getMaxFrameTimeMs() const { return maxFrameTimeMs_; }
    [[nodiscard]] double getMinFrameTimeMs() const 
    { 
        return minFrameTimeMs_ == std::numeric_limits<double>::max() ? 0.0 : minFrameTimeMs_; 
    }
    
    // ========================================================================
    // Per-Pass Timing
    // ========================================================================
    
    /**
     * Get timing for a specific render pass
     */
    [[nodiscard]] const RenderPassTiming& getPassTiming(RenderPass pass) const
    {
        return passTiming_[static_cast<size_t>(pass)];
    }
    
    /**
     * Get mutable reference for recording (used by ScopedRenderPassTimer)
     */
    RenderPassTiming& getPassTimingMutable(RenderPass pass)
    {
        return passTiming_[static_cast<size_t>(pass)];
    }
    
    /**
     * Create a scoped timer for a render pass
     */
    ScopedRenderPassTimer timePass(RenderPass pass)
    {
        return ScopedRenderPassTimer(passTiming_[static_cast<size_t>(pass)]);
    }
    
    /**
     * Get timing breakdown summary as a string (for debugging)
     */
    [[nodiscard]] juce::String getTimingBreakdown() const
    {
        juce::String result;
        result << "Frame: " << juce::String(deltaTime_ * 1000.0f, 2) << "ms (";
        result << juce::String(getCurrentFps(), 1) << " FPS)\n";
        
        const char* passNames[] = {
            "BeginFrame", "GridRender", "WaveformGeometry", "EffectPipeline",
            "Composite", "GlobalEffects", "BlitToScreen", "EndFrame"
        };
        
        for (size_t i = 0; i < static_cast<size_t>(RenderPass::COUNT); ++i)
        {
            const auto& timing = passTiming_[i];
            if (timing.count > 0)
            {
                result << "  " << passNames[i] << ": ";
                result << juce::String(timing.avgMs, 2) << "ms avg, ";
                result << juce::String(timing.maxMs, 2) << "ms max\n";
            }
        }
        
        return result;
    }
    
    // ========================================================================
    // Render Complexity Tracking
    // ========================================================================
    
    void setWaveformCount(int count) { waveformCount_ = count; }
    [[nodiscard]] int getWaveformCount() const { return waveformCount_; }
    
    void setEffectCount(int count) { effectCount_ = count; }
    [[nodiscard]] int getEffectCount() const { return effectCount_; }
    
    void incrementFboBindCount() { fboBindCount_++; }
    [[nodiscard]] int getFboBindCount() const { return fboBindCount_; }
    void resetFboBindCount() { fboBindCount_ = 0; }
    
    void incrementShaderSwitchCount() { shaderSwitchCount_++; }
    [[nodiscard]] int getShaderSwitchCount() const { return shaderSwitchCount_; }
    void resetShaderSwitchCount() { shaderSwitchCount_ = 0; }
    
    // ========================================================================
    // JSON Export (for HTTP API)
    // ========================================================================
    
    [[nodiscard]] juce::String toJson() const
    {
        juce::String json = "{";
        json << "\"frameCount\":" << frameCount_ << ",";
        json << "\"deltaTimeMs\":" << juce::String(deltaTime_ * 1000.0f, 3) << ",";
        json << "\"avgFps\":" << juce::String(avgFps_, 2) << ",";
        json << "\"currentFps\":" << juce::String(getCurrentFps(), 2) << ",";
        json << "\"avgFrameTimeMs\":" << juce::String(getAvgFrameTimeMs(), 3) << ",";
        json << "\"maxFrameTimeMs\":" << juce::String(maxFrameTimeMs_, 3) << ",";
        json << "\"minFrameTimeMs\":" << juce::String(getMinFrameTimeMs(), 3) << ",";
        json << "\"waveformCount\":" << waveformCount_ << ",";
        json << "\"effectCount\":" << effectCount_ << ",";
        json << "\"fboBindCount\":" << fboBindCount_ << ",";
        json << "\"shaderSwitchCount\":" << shaderSwitchCount_ << ",";
        json << "\"passTiming\":{";
        
        const char* passNames[] = {
            "beginFrame", "gridRender", "waveformGeometry", "effectPipeline",
            "composite", "globalEffects", "blitToScreen", "endFrame"
        };
        
        bool first = true;
        for (size_t i = 0; i < static_cast<size_t>(RenderPass::COUNT); ++i)
        {
            const auto& timing = passTiming_[i];
            if (timing.count > 0)
            {
                if (!first) json << ",";
                first = false;
                json << "\"" << passNames[i] << "\":{";
                json << "\"avgMs\":" << juce::String(timing.avgMs, 3) << ",";
                json << "\"maxMs\":" << juce::String(timing.maxMs, 3) << ",";
                json << "\"minMs\":" << juce::String(timing.minMs == std::numeric_limits<double>::max() ? 0.0 : timing.minMs, 3) << ",";
                json << "\"count\":" << timing.count;
                json << "}";
            }
        }
        
        json << "}}";
        return json;
    }

private:
    // Basic timing
    float deltaTime_ = 0.0f;
    float accumulatedTime_ = 0.0f;
    
    // Frame statistics
    int64_t frameCount_ = 0;
    double totalFrameTimeMs_ = 0.0;
    double avgFps_ = 0.0;
    double maxFrameTimeMs_ = 0.0;
    double minFrameTimeMs_ = std::numeric_limits<double>::max();
    
    // Per-pass timing
    std::array<RenderPassTiming, static_cast<size_t>(RenderPass::COUNT)> passTiming_;
    
    // Render complexity
    int waveformCount_ = 0;
    int effectCount_ = 0;
    int fboBindCount_ = 0;
    int shaderSwitchCount_ = 0;
};

// Macros for easy pass timing
#define RE_TIME_PASS(stats, pass) \
    auto _scopedTimer##pass = (stats).timePass(oscil::RenderPass::pass)

} // namespace oscil
