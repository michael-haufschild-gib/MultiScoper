/*
    Oscil Test Harness - Performance Metrics Collection
    Monitors FPS, CPU usage, and memory for performance testing.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>

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
};

/**
 * Collects and provides performance metrics for the test harness.
 */
class TestMetrics : public juce::Timer
{
public:
    TestMetrics();
    ~TestMetrics() override;

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

private:
    void timerCallback() override;
    void collectSample();
    double calculateFps();
    double measureCpuUsage();
    double computeCpuDeltaPercent(int64_t cpuTime);
    int64_t measureMemoryUsage();

    std::atomic<bool> collecting_{false};
    std::atomic<int> oscillatorCount_{0};
    std::atomic<int> sourceCount_{0};

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestMetrics)
};

} // namespace oscil::test
