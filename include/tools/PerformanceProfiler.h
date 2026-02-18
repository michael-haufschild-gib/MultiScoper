/*
    Oscil - Performance Profiler
    Comprehensive performance profiling infrastructure for GPU timing,
    thread metrics, and lock contention detection.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <limits>
#include <mutex>
#include <unordered_map>
#include <string>
#include <vector>
#include <array>

namespace oscil
{

// ============================================================================
// Timing Helpers
// ============================================================================

/**
 * High-resolution timer for precise timing measurements
 */
class ScopedTimer
{
public:
    using Clock = std::chrono::high_resolution_clock;
    
    explicit ScopedTimer(double& target)
        : target_(target)
        , start_(Clock::now())
    {}
    
    ~ScopedTimer()
    {
        auto end = Clock::now();
        target_ = std::chrono::duration<double, std::milli>(end - start_).count();
    }
    
private:
    double& target_;
    Clock::time_point start_;
};

// ============================================================================
// Frame Timing Data
// ============================================================================

/**
 * Detailed timing breakdown for a single frame
 */
struct FrameTimingData
{
    int64_t frameNumber = 0;
    int64_t timestamp = 0;  // Unix timestamp in milliseconds
    
    // Total frame time
    double totalFrameMs = 0.0;
    
    // GPU timing breakdown
    double beginFrameMs = 0.0;
    double waveformRenderMs = 0.0;
    double effectPipelineMs = 0.0;
    double compositeMs = 0.0;
    double blitToScreenMs = 0.0;
    double endFrameMs = 0.0;
    
    // Per-waveform timing
    std::vector<std::pair<int, double>> waveformTimings;  // (waveformId, ms)
    
    // FBO operations
    double fboBindUnbindMs = 0.0;
    int fboBindCount = 0;
    
    // Shader operations
    double shaderSwitchMs = 0.0;
    int shaderSwitchCount = 0;
    
    // Waveform count this frame
    int waveformCount = 0;
};

/**
 * Snapshot of histogram data (copyable)
 */
struct FrameTimeHistogramData
{
    static constexpr int NUM_BUCKETS = 20;
    static constexpr double MAX_FRAME_TIME_MS = 100.0;
    
    std::array<int, NUM_BUCKETS> buckets{};
    int totalFrames = 0;
    double totalTimeMs = 0.0;
    double minMs = std::numeric_limits<double>::max();
    double maxMs = 0.0;
    
    double getAverageMs() const { return totalFrames > 0 ? totalTimeMs / totalFrames : 0.0; }
    double getMinMs() const { return minMs; }
    double getMaxMs() const { return maxMs; }
    double getPercentile(double percentile) const;
};

/**
 * Histogram for frame time distribution
 */
class FrameTimeHistogram
{
public:
    static constexpr int NUM_BUCKETS = 20;
    static constexpr double MAX_FRAME_TIME_MS = 100.0;  // Frames above this go in last bucket
    
    void recordFrame(double frameTimeMs);
    void reset();
    
    // Get percentile value (0-100)
    double getPercentile(double percentile) const;
    
    // Get bucket counts for visualization
    std::array<int, NUM_BUCKETS> getBuckets() const;
    
    int getTotalFrames() const;
    double getAverageMs() const;
    double getMinMs() const;
    double getMaxMs() const;
    
    // Get a copyable snapshot of the data
    FrameTimeHistogramData getSnapshot() const;
    
private:
    std::array<int, NUM_BUCKETS> buckets_{};
    int totalFrames_ = 0;
    double totalTimeMs_ = 0.0;
    double minMs_ = std::numeric_limits<double>::max();
    double maxMs_ = 0.0;
    mutable std::mutex mutex_;
};

// ============================================================================
// Thread Metrics
// ============================================================================

/**
 * Metrics for a specific thread
 */
struct ThreadMetrics
{
    std::string threadName;
    
    // Timing
    double avgExecutionTimeMs = 0.0;
    double maxExecutionTimeMs = 0.0;
    double minExecutionTimeMs = std::numeric_limits<double>::max();
    
    // Invocation count
    int64_t invocationCount = 0;
    
    // Load percentage (time spent busy / total time)
    double loadPercent = 0.0;
    
    // Lock wait time
    double totalLockWaitMs = 0.0;
    int64_t lockAcquisitionCount = 0;
    
    void reset();
    void recordExecution(double durationMs);
    void recordLockWait(double waitMs);
};

/**
 * Lock-free metrics for audio thread (real-time safe)
 * Uses atomic operations to avoid blocking the audio thread
 */
struct AudioThreadMetricsAtomic
{
    std::atomic<double> maxExecutionTimeMs{0.0};
    std::atomic<double> minExecutionTimeMs{std::numeric_limits<double>::max()};
    std::atomic<double> avgExecutionTimeMs{0.0};
    std::atomic<int64_t> invocationCount{0};
    std::atomic<double> totalLockWaitMs{0.0};
    std::atomic<int64_t> lockAcquisitionCount{0};

    void recordExecution(double durationMs);
    void recordLockWait(double waitMs);
    void reset();
    ThreadMetrics toThreadMetrics() const;
};

/**
 * Thread-specific profiling data
 */
class ThreadProfiler
{
public:
    // Audio thread methods are lock-free (real-time safe)
    void recordAudioThreadExecution(double durationMs);
    void recordAudioThreadLockWait(double waitMs);

    // UI/OpenGL thread methods use mutex (not called from audio thread)
    void recordUIThreadExecution(double durationMs);
    void recordOpenGLThreadExecution(double durationMs);
    void recordUIThreadLockWait(double waitMs);
    void recordOpenGLThreadLockWait(double waitMs);

    ThreadMetrics getAudioThreadMetrics() const;
    ThreadMetrics getUIThreadMetrics() const;
    ThreadMetrics getOpenGLThreadMetrics() const;

    void reset();

private:
    // Lock-free metrics for audio thread
    AudioThreadMetricsAtomic audioMetricsAtomic_;

    // Mutex-protected metrics for non-audio threads
    mutable std::mutex mutex_;
    ThreadMetrics uiMetrics_{"UI Thread"};
    ThreadMetrics openglMetrics_{"OpenGL Thread"};
};

// ============================================================================
// Component Repaint Tracking
// ============================================================================

/**
 * Tracks repaint frequency and cost for UI components
 */
struct ComponentRepaintStats
{
    std::string componentName;
    int64_t repaintCount = 0;
    double totalRepaintTimeMs = 0.0;
    double avgRepaintTimeMs = 0.0;
    double maxRepaintTimeMs = 0.0;
    double repaintsPerSecond = 0.0;
    
    void recordRepaint(double durationMs);
    void reset();
};

/**
 * Tracks UI component repaint statistics
 */
class ComponentProfiler
{
public:
    void recordRepaint(const std::string& componentName, double durationMs);
    
    std::vector<ComponentRepaintStats> getStats() const;
    ComponentRepaintStats getStats(const std::string& componentName) const;
    
    // Get top N components by repaint frequency
    std::vector<ComponentRepaintStats> getTopByFrequency(int n) const;
    
    // Get top N components by total repaint time
    std::vector<ComponentRepaintStats> getTopByTime(int n) const;
    
    void reset();
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ComponentRepaintStats> stats_;
    std::chrono::steady_clock::time_point startTime_;
};

// ============================================================================
// Memory Profiling
// ============================================================================

/**
 * Memory usage snapshot
 */
struct MemorySnapshot
{
    int64_t timestamp = 0;
    int64_t residentBytes = 0;
    int64_t virtualBytes = 0;
    double residentMB = 0.0;
    double virtualMB = 0.0;
};

/**
 * Tracks memory allocation patterns
 */
class MemoryProfiler
{
public:
    void recordSnapshot();
    
    MemorySnapshot getCurrentSnapshot() const;
    std::vector<MemorySnapshot> getHistory(int maxEntries = 100) const;
    
    // Growth analysis
    double getGrowthRateMBPerSecond() const;
    double getPeakMemoryMB() const;
    
    void reset();
    
private:
    mutable std::mutex mutex_;
    std::deque<MemorySnapshot> history_;
    static constexpr size_t MAX_HISTORY = 1000;
};

// ============================================================================
// GPU Profiler
// ============================================================================

/**
 * GPU-specific profiling data (copyable snapshot)
 */
struct GpuProfilingData
{
    // Frame statistics (use snapshot, not the histogram itself)
    FrameTimeHistogramData frameHistogram;
    
    // Per-pass timing averages
    double avgBeginFrameMs = 0.0;
    double avgWaveformRenderMs = 0.0;
    double avgEffectPipelineMs = 0.0;
    double avgCompositeMs = 0.0;
    double avgBlitMs = 0.0;
    double avgEndFrameMs = 0.0;
    
    // FBO statistics
    double avgFboBindTimeMs = 0.0;
    int totalFboBinds = 0;
    
    // Shader statistics
    double avgShaderSwitchTimeMs = 0.0;
    int totalShaderSwitches = 0;
    
    // Frame rate
    double currentFps = 0.0;
    double avgFps = 0.0;
    double minFps = std::numeric_limits<double>::max();
    double maxFps = 0.0;
    
    int64_t totalFrames = 0;
    
    // Frame budget tracking
    int budgetExceededCount = 0;
    int totalSkippedWaveforms = 0;
    int totalSkippedEffects = 0;
    double avgBudgetRatio = 0.0;
    double maxBudgetRatio = 0.0;
};

/**
 * GPU performance profiler
 */
class GpuProfiler
{
public:
    void beginFrame();
    void endFrame();
    
    // Record specific operations
    void recordBeginFrame(double ms);
    void recordWaveformRender(int waveformId, double ms);
    void recordEffectPipeline(double ms);
    void recordComposite(double ms);
    void recordBlitToScreen(double ms);
    void recordEndFrame(double ms);
    
    // FBO tracking
    void recordFboBind(double ms);
    
    // Shader tracking
    void recordShaderSwitch(double ms);
    
    // Frame budget tracking
    void recordBudgetStats(double elapsedMs, double budgetMs, int skippedWaveforms, int skippedEffects);
    
    GpuProfilingData getData() const;
    FrameTimingData getLastFrameData() const;
    std::vector<FrameTimingData> getRecentFrames(int count = 60) const;
    
    void reset();
    
private:
    void updateAverages();
    GpuProfilingData buildDataSnapshot() const;
    
    mutable std::mutex mutex_;
    
    // Internal histogram (non-copyable, with mutex)
    FrameTimeHistogram frameHistogram_;
    
    // Accumulated stats
    double avgBeginFrameMs_ = 0.0;
    double avgWaveformRenderMs_ = 0.0;
    double avgEffectPipelineMs_ = 0.0;
    double avgCompositeMs_ = 0.0;
    double avgBlitMs_ = 0.0;
    double avgEndFrameMs_ = 0.0;
    double avgFboBindTimeMs_ = 0.0;
    int totalFboBinds_ = 0;
    double avgShaderSwitchTimeMs_ = 0.0;
    int totalShaderSwitches_ = 0;
    double currentFps_ = 0.0;
    double avgFps_ = 0.0;
    double minFps_ = std::numeric_limits<double>::max();
    double maxFps_ = 0.0;
    int64_t totalFrames_ = 0;
    
    // Frame budget tracking
    int budgetExceededCount_ = 0;
    int totalSkippedWaveforms_ = 0;
    int totalSkippedEffects_ = 0;
    double avgBudgetRatio_ = 0.0;
    double maxBudgetRatio_ = 0.0;
    
    FrameTimingData currentFrame_;
    std::deque<FrameTimingData> frameHistory_;
    static constexpr size_t MAX_FRAME_HISTORY = 300;  // 5 seconds at 60fps
    
    std::chrono::steady_clock::time_point frameStart_;
    std::chrono::steady_clock::time_point profilingStart_;
    int64_t frameCounter_ = 0;
};

// ============================================================================
// Hotspot Detection
// ============================================================================

/**
 * Performance hotspot information
 */
struct PerformanceHotspot
{
    std::string category;     // "GPU", "UI", "Lock", "Memory"
    std::string location;     // Function/component name
    std::string description;  // Human-readable description
    double severity;          // 0.0-1.0, higher = more severe
    double impactMs;          // Estimated impact in milliseconds per frame
    std::string recommendation;  // Suggested fix
};

/**
 * Detects performance hotspots from profiling data
 */
class HotspotDetector
{
public:
    std::vector<PerformanceHotspot> detectHotspots(
        const GpuProfilingData& gpuData,
        const ThreadProfiler& threadProfiler,
        const ComponentProfiler& componentProfiler,
        const MemoryProfiler& memoryProfiler) const;
    
private:
    std::vector<PerformanceHotspot> detectGpuHotspots(const GpuProfilingData& data) const;
    std::vector<PerformanceHotspot> detectThreadHotspots(const ThreadProfiler& profiler) const;
    std::vector<PerformanceHotspot> detectComponentHotspots(const ComponentProfiler& profiler) const;
    std::vector<PerformanceHotspot> detectMemoryHotspots(const MemoryProfiler& profiler) const;
};

// ============================================================================
// Main Performance Profiler
// ============================================================================

/**
 * Profiling session configuration
 */
struct ProfilingConfig
{
    bool enableGpuProfiling = true;
    bool enableThreadProfiling = true;
    bool enableComponentProfiling = true;
    bool enableMemoryProfiling = true;
    bool enableHotspotDetection = true;
    
    int memorySnapshotIntervalMs = 1000;  // How often to snapshot memory
    int historyDurationSeconds = 60;       // How much history to keep
};

/**
 * Complete profiling session data
 */
struct ProfilingSnapshot
{
    int64_t timestamp = 0;
    int64_t durationMs = 0;
    
    GpuProfilingData gpuData;
    ThreadMetrics audioThreadMetrics;
    ThreadMetrics uiThreadMetrics;
    ThreadMetrics openglThreadMetrics;
    std::vector<ComponentRepaintStats> componentStats;
    MemorySnapshot memorySnapshot;
    std::vector<PerformanceHotspot> hotspots;
    
    // Summary metrics
    double avgFps = 0.0;
    double p50FrameTimeMs = 0.0;
    double p95FrameTimeMs = 0.0;
    double p99FrameTimeMs = 0.0;
    double avgCpuPercent = 0.0;
    double peakMemoryMB = 0.0;
};

/**
 * Central performance profiler
 * Aggregates all profiling subsystems and provides unified access
 */
class PerformanceProfiler
{
public:
    static PerformanceProfiler& getInstance();
    
    // Lifecycle
    void startProfiling(const ProfilingConfig& config = {});
    void stopProfiling();
    bool isProfiling() const { return profiling_.load(); }
    
    // Access subsystems
    GpuProfiler& getGpuProfiler() { return gpuProfiler_; }
    ThreadProfiler& getThreadProfiler() { return threadProfiler_; }
    ComponentProfiler& getComponentProfiler() { return componentProfiler_; }
    MemoryProfiler& getMemoryProfiler() { return memoryProfiler_; }
    
    // Get snapshot of all profiling data
    ProfilingSnapshot getSnapshot() const;
    
    // Get detected hotspots
    std::vector<PerformanceHotspot> getHotspots() const;
    
    // Reset all profiling data
    void reset();
    
    // JSON export for HTTP API
    juce::String toJson() const;
    juce::String getGpuDataJson() const;
    juce::String getThreadDataJson() const;
    juce::String getComponentDataJson() const;
    juce::String getHotspotsJson() const;
    juce::String getTimelineJson(int frameCount = 60) const;
    
private:
    PerformanceProfiler() = default;
    ~PerformanceProfiler() = default;
    PerformanceProfiler(const PerformanceProfiler&) = delete;
    PerformanceProfiler& operator=(const PerformanceProfiler&) = delete;
    
    void memorySnapshotTimerCallback();
    
    std::atomic<bool> profiling_{false};
    ProfilingConfig config_;
    std::chrono::steady_clock::time_point startTime_;
    
    GpuProfiler gpuProfiler_;
    ThreadProfiler threadProfiler_;
    ComponentProfiler componentProfiler_;
    MemoryProfiler memoryProfiler_;
    HotspotDetector hotspotDetector_;
    
    // Memory snapshot timer (juce::Timer would create header dependency, use atomic flag)
    std::atomic<bool> memoryTimerRunning_{false};
};

// ============================================================================
// RAII Helpers for Automatic Profiling
// ============================================================================

/**
 * RAII helper for profiling a scope
 */
class ScopedGpuProfile
{
public:
    enum class Phase
    {
        BeginFrame,
        WaveformRender,
        EffectPipeline,
        Composite,
        BlitToScreen,
        EndFrame
    };
    
    ScopedGpuProfile(Phase phase, int waveformId = -1);
    ~ScopedGpuProfile();
    
private:
    Phase phase_;
    int waveformId_;
    std::chrono::steady_clock::time_point start_;
};

/**
 * RAII helper for profiling component repaints
 */
class ScopedComponentProfile
{
public:
    explicit ScopedComponentProfile(const std::string& componentName);
    ~ScopedComponentProfile();
    
private:
    std::string componentName_;
    std::chrono::steady_clock::time_point start_;
};

/**
 * RAII helper for profiling thread execution
 */
class ScopedThreadProfile
{
public:
    enum class Thread { Audio, UI, OpenGL };
    
    explicit ScopedThreadProfile(Thread thread);
    ~ScopedThreadProfile();
    
private:
    Thread thread_;
    std::chrono::steady_clock::time_point start_;
};

// ============================================================================
// Macros for Easy Profiling
// ============================================================================

#if OSCIL_ENABLE_PROFILING
    #define PROFILE_GPU_SCOPE(phase) \
        oscil::ScopedGpuProfile _gpuProfile##__LINE__(oscil::ScopedGpuProfile::Phase::phase)
    
    #define PROFILE_GPU_WAVEFORM(waveformId) \
        oscil::ScopedGpuProfile _gpuProfile##__LINE__(oscil::ScopedGpuProfile::Phase::WaveformRender, waveformId)
    
    #define PROFILE_COMPONENT(name) \
        oscil::ScopedComponentProfile _componentProfile##__LINE__(name)
    
    #define PROFILE_AUDIO_THREAD() \
        oscil::ScopedThreadProfile _threadProfile##__LINE__(oscil::ScopedThreadProfile::Thread::Audio)
    
    #define PROFILE_UI_THREAD() \
        oscil::ScopedThreadProfile _threadProfile##__LINE__(oscil::ScopedThreadProfile::Thread::UI)
    
    #define PROFILE_OPENGL_THREAD() \
        oscil::ScopedThreadProfile _threadProfile##__LINE__(oscil::ScopedThreadProfile::Thread::OpenGL)
#else
    #define PROFILE_GPU_SCOPE(phase) ((void)0)
    #define PROFILE_GPU_WAVEFORM(waveformId) ((void)0)
    #define PROFILE_COMPONENT(name) ((void)0)
    #define PROFILE_AUDIO_THREAD() ((void)0)
    #define PROFILE_UI_THREAD() ((void)0)
    #define PROFILE_OPENGL_THREAD() ((void)0)
#endif

} // namespace oscil

