/*
    Oscil Test Harness - Performance Metrics Collection
    Monitors FPS, CPU usage, memory, GPU timing, and component stats for performance testing.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <vector>
#include <string>

namespace oscil::test
{

/**
 * Performance snapshot at a point in time
 */
struct PerformanceSnapshot
{
    double fps = 0.0;              // Frames per second
    double cpuPercent = 0.0;       // CPU usage percentage (0-100)
    int64_t memoryBytes = 0;       // Memory usage in bytes
    double memoryMB = 0.0;         // Memory usage in megabytes
    int oscillatorCount = 0;       // Current oscillator count
    int sourceCount = 0;           // Current source count
    int64_t timestamp = 0;         // Timestamp in milliseconds
    
    // Extended GPU metrics
    double gpuFrameTimeMs = 0.0;   // GPU frame time in milliseconds
    int waveformCount = 0;         // Active waveforms being rendered
    int effectCount = 0;           // Active post-processing effects
};

/**
 * Performance statistics over a time period
 */
struct PerformanceStats
{
    double avgFps = 0.0;
    double minFps = 0.0;
    double maxFps = 0.0;
    double avgCpu = 0.0;
    double maxCpu = 0.0;
    double avgMemoryMB = 0.0;
    double maxMemoryMB = 0.0;
    int sampleCount = 0;
    int64_t durationMs = 0;
    
    // Frame time percentiles
    double p50FrameTimeMs = 0.0;
    double p95FrameTimeMs = 0.0;
    double p99FrameTimeMs = 0.0;
};

/**
 * GPU-specific metrics for detailed analysis
 */
struct GpuMetrics
{
    double currentFps = 0.0;
    double avgFps = 0.0;
    double minFps = 0.0;
    double maxFps = 0.0;
    
    double avgFrameTimeMs = 0.0;
    double minFrameTimeMs = 0.0;
    double maxFrameTimeMs = 0.0;
    
    double p50FrameTimeMs = 0.0;
    double p95FrameTimeMs = 0.0;
    double p99FrameTimeMs = 0.0;
    
    // Per-pass timing
    double avgBeginFrameMs = 0.0;
    double avgWaveformRenderMs = 0.0;
    double avgEffectPipelineMs = 0.0;
    double avgCompositeMs = 0.0;
    double avgBlitMs = 0.0;
    
    // Operations
    int totalFboBinds = 0;
    int totalShaderSwitches = 0;
    
    int64_t totalFrames = 0;
};

/**
 * Thread-specific metrics
 */
struct ThreadMetrics
{
    std::string name;
    double avgExecutionTimeMs = 0.0;
    double maxExecutionTimeMs = 0.0;
    double minExecutionTimeMs = 0.0;
    int64_t invocationCount = 0;
    double loadPercent = 0.0;
    double totalLockWaitMs = 0.0;
    int64_t lockAcquisitionCount = 0;
};

/**
 * Component repaint statistics
 */
struct ComponentStats
{
    std::string name;
    int64_t repaintCount = 0;
    double totalRepaintTimeMs = 0.0;
    double avgRepaintTimeMs = 0.0;
    double maxRepaintTimeMs = 0.0;
    double repaintsPerSecond = 0.0;
};

/**
 * Performance hotspot identified during profiling
 */
struct PerformanceHotspot
{
    std::string category;
    std::string location;
    std::string description;
    double severity = 0.0;
    double impactMs = 0.0;
    std::string recommendation;
};

/**
 * Complete profiling session result
 */
struct ProfilingResult
{
    int64_t startTimestamp = 0;
    int64_t endTimestamp = 0;
    int64_t durationMs = 0;
    
    PerformanceStats stats;
    GpuMetrics gpuMetrics;
    ThreadMetrics audioThread;
    ThreadMetrics uiThread;
    ThreadMetrics openglThread;
    std::vector<ComponentStats> componentStats;
    std::vector<PerformanceHotspot> hotspots;
    
    double memoryGrowthMBPerSec = 0.0;
    double peakMemoryMB = 0.0;
};

/**
 * Collects and provides performance metrics for the test harness.
 * Extended with GPU profiling, thread metrics, and component tracking.
 */
class TestMetrics : public juce::Timer
{
public:
    TestMetrics();
    ~TestMetrics() override;

    // ========================================================================
    // Basic Collection
    // ========================================================================
    
    /**
     * Start collecting metrics at the given interval
     */
    void startCollection(int intervalMs = 100);

    /**
     * Stop collecting metrics
     */
    void stopCollection();

    /**
     * Check if collection is active
     */
    bool isCollecting() const { return collecting_.load(); }

    /**
     * Get the most recent performance snapshot
     */
    PerformanceSnapshot getCurrentSnapshot();

    /**
     * Get statistics over the collection period
     */
    PerformanceStats getStats();

    /**
     * Get statistics over the last N milliseconds
     */
    PerformanceStats getStatsForPeriod(int periodMs);

    /**
     * Reset all collected data
     */
    void reset();

    /**
     * Record a frame render (call this from the UI render loop)
     */
    void recordFrame();

    /**
     * Get current FPS based on recent frame times
     */
    double getCurrentFps();

    /**
     * Get current memory usage in bytes
     */
    int64_t getCurrentMemoryBytes();

    /**
     * Get current memory usage in MB
     */
    double getCurrentMemoryMB();

    /**
     * Get current CPU usage percentage
     */
    double getCurrentCpuPercent();

    /**
     * Set counts from the plugin state
     */
    void setOscillatorCount(int count);
    void setSourceCount(int count);
    
    // ========================================================================
    // Extended Profiling
    // ========================================================================
    
    /**
     * Start detailed profiling session
     * This enables GPU timing, thread metrics, and component tracking
     */
    void startProfiling();
    
    /**
     * Stop profiling and return results
     */
    ProfilingResult stopProfiling();
    
    /**
     * Check if profiling is active
     */
    bool isProfiling() const { return profiling_.load(); }
    
    /**
     * Get GPU-specific metrics
     */
    GpuMetrics getGpuMetrics();
    
    /**
     * Get thread metrics
     */
    ThreadMetrics getAudioThreadMetrics();
    ThreadMetrics getUiThreadMetrics();
    ThreadMetrics getOpenGLThreadMetrics();
    
    /**
     * Get component repaint statistics
     */
    std::vector<ComponentStats> getComponentStats();
    
    /**
     * Get identified performance hotspots
     */
    std::vector<PerformanceHotspot> getHotspots();
    
    /**
     * Get frame timeline data (last N frames)
     */
    juce::String getTimelineJson(int frameCount = 60);
    
    // ========================================================================
    // Stress Testing Support
    // ========================================================================
    
    void setWaveformCount(int count) { waveformCount_.store(count); }
    void setEffectCount(int count) { effectCount_.store(count); }

private:
    void timerCallback() override;
    void collectSample();
    double calculateFps();
    double measureCpuUsage();
    int64_t measureMemoryUsage();
    
    // Calculate frame time percentiles from snapshots
    void calculatePercentiles(std::vector<double>& frameTimes, 
                              double& p50, double& p95, double& p99);

    std::atomic<bool> collecting_{false};
    std::atomic<bool> profiling_{false};
    std::atomic<int> oscillatorCount_{0};
    std::atomic<int> sourceCount_{0};
    std::atomic<int> waveformCount_{0};
    std::atomic<int> effectCount_{0};

    // Frame timing for FPS calculation
    std::deque<int64_t> frameTimes_;
    mutable std::mutex frameTimeMutex_;
    static constexpr size_t MAX_FRAME_TIMES = 120;

    // Collected snapshots
    std::deque<PerformanceSnapshot> snapshots_;
    mutable std::mutex snapshotMutex_;
    static constexpr size_t MAX_SNAPSHOTS = 6000; // 10 minutes at 100ms intervals

    // CPU measurement state
    int64_t lastCpuTime_ = 0;
    int64_t lastSystemTime_ = 0;
    
    // Profiling session state
    int64_t profilingStartTime_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestMetrics)
};

} // namespace oscil::test
