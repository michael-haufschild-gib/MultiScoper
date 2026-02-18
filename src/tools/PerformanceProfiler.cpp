/*
    Oscil - Performance Profiler Implementation
*/

#include "tools/PerformanceProfiler.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <cmath>

#if JUCE_MAC
    #include <mach/mach.h>
    #include <mach/task.h>
#elif JUCE_WINDOWS
    #include <windows.h>
    #include <psapi.h>
#elif JUCE_LINUX
    #include <unistd.h>
    #include <fstream>
#endif

namespace oscil
{

// ============================================================================
// FrameTimeHistogram
// ============================================================================

void FrameTimeHistogram::recordFrame(double frameTimeMs)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    totalFrames_++;
    totalTimeMs_ += frameTimeMs;
    minMs_ = std::min(minMs_, frameTimeMs);
    maxMs_ = std::max(maxMs_, frameTimeMs);
    
    // Calculate bucket index
    double bucketSize = MAX_FRAME_TIME_MS / NUM_BUCKETS;
    int bucketIndex = static_cast<int>(frameTimeMs / bucketSize);
    bucketIndex = std::clamp(bucketIndex, 0, NUM_BUCKETS - 1);
    buckets_[static_cast<size_t>(bucketIndex)]++;
}

void FrameTimeHistogram::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    buckets_.fill(0);
    totalFrames_ = 0;
    totalTimeMs_ = 0.0;
    minMs_ = std::numeric_limits<double>::max();
    maxMs_ = 0.0;
}

double FrameTimeHistogram::getPercentile(double percentile) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (totalFrames_ == 0)
        return 0.0;
    
    int targetCount = static_cast<int>(totalFrames_ * percentile / 100.0);
    int accumulated = 0;
    double bucketSize = MAX_FRAME_TIME_MS / NUM_BUCKETS;
    
    for (int i = 0; i < NUM_BUCKETS; ++i)
    {
        accumulated += buckets_[static_cast<size_t>(i)];
        if (accumulated >= targetCount)
        {
            // Return the upper bound of this bucket
            return (i + 1) * bucketSize;
        }
    }
    
    return MAX_FRAME_TIME_MS;
}

std::array<int, FrameTimeHistogram::NUM_BUCKETS> FrameTimeHistogram::getBuckets() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return buckets_;
}

int FrameTimeHistogram::getTotalFrames() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return totalFrames_;
}

double FrameTimeHistogram::getAverageMs() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return totalFrames_ > 0 ? totalTimeMs_ / totalFrames_ : 0.0;
}

double FrameTimeHistogram::getMinMs() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return minMs_;
}

double FrameTimeHistogram::getMaxMs() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return maxMs_;
}

FrameTimeHistogramData FrameTimeHistogram::getSnapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    FrameTimeHistogramData data;
    data.buckets = buckets_;
    data.totalFrames = totalFrames_;
    data.totalTimeMs = totalTimeMs_;
    data.minMs = minMs_;
    data.maxMs = maxMs_;
    return data;
}

// ============================================================================
// FrameTimeHistogramData
// ============================================================================

double FrameTimeHistogramData::getPercentile(double percentile) const
{
    if (totalFrames == 0)
        return 0.0;
    
    int targetCount = static_cast<int>(totalFrames * percentile / 100.0);
    int accumulated = 0;
    double bucketSize = MAX_FRAME_TIME_MS / NUM_BUCKETS;
    
    for (int i = 0; i < NUM_BUCKETS; ++i)
    {
        accumulated += buckets[static_cast<size_t>(i)];
        if (accumulated >= targetCount)
        {
            return (i + 1) * bucketSize;
        }
    }
    
    return MAX_FRAME_TIME_MS;
}

// ============================================================================
// ThreadMetrics
// ============================================================================

void ThreadMetrics::reset()
{
    avgExecutionTimeMs = 0.0;
    maxExecutionTimeMs = 0.0;
    minExecutionTimeMs = std::numeric_limits<double>::max();
    invocationCount = 0;
    loadPercent = 0.0;
    totalLockWaitMs = 0.0;
    lockAcquisitionCount = 0;
}

void ThreadMetrics::recordExecution(double durationMs)
{
    invocationCount++;
    maxExecutionTimeMs = std::max(maxExecutionTimeMs, durationMs);
    minExecutionTimeMs = std::min(minExecutionTimeMs, durationMs);
    
    // Running average
    avgExecutionTimeMs = avgExecutionTimeMs + (durationMs - avgExecutionTimeMs) / invocationCount;
}

void ThreadMetrics::recordLockWait(double waitMs)
{
    lockAcquisitionCount++;
    totalLockWaitMs += waitMs;
}

// ============================================================================
// AudioThreadMetricsAtomic (lock-free for real-time safety)
// ============================================================================

void AudioThreadMetricsAtomic::recordExecution(double durationMs)
{
    // Lock-free atomic update of max
    double prevMax = maxExecutionTimeMs.load(std::memory_order_relaxed);
    while (durationMs > prevMax &&
           !maxExecutionTimeMs.compare_exchange_weak(prevMax, durationMs,
               std::memory_order_relaxed, std::memory_order_relaxed))
    {
        // prevMax is updated by compare_exchange_weak on failure
    }

    // Lock-free atomic update of min
    double prevMin = minExecutionTimeMs.load(std::memory_order_relaxed);
    while (durationMs < prevMin &&
           !minExecutionTimeMs.compare_exchange_weak(prevMin, durationMs,
               std::memory_order_relaxed, std::memory_order_relaxed))
    {
        // prevMin is updated by compare_exchange_weak on failure
    }

    // Increment count first (for running average calculation)
    int64_t count = invocationCount.fetch_add(1, std::memory_order_relaxed) + 1;

    // Lock-free running average update
    // Note: This is an approximation that may have slight inaccuracy under contention,
    // but is acceptable for profiling data and avoids blocking the audio thread
    double prevAvg = avgExecutionTimeMs.load(std::memory_order_relaxed);
    double newAvg = prevAvg + (durationMs - prevAvg) / static_cast<double>(count);
    avgExecutionTimeMs.store(newAvg, std::memory_order_relaxed);
}

void AudioThreadMetricsAtomic::recordLockWait(double waitMs)
{
    lockAcquisitionCount.fetch_add(1, std::memory_order_relaxed);

    // Lock-free atomic addition for total
    double prevTotal = totalLockWaitMs.load(std::memory_order_relaxed);
    while (!totalLockWaitMs.compare_exchange_weak(prevTotal, prevTotal + waitMs,
           std::memory_order_relaxed, std::memory_order_relaxed))
    {
        // prevTotal is updated by compare_exchange_weak on failure
    }
}

void AudioThreadMetricsAtomic::reset()
{
    maxExecutionTimeMs.store(0.0, std::memory_order_relaxed);
    minExecutionTimeMs.store(std::numeric_limits<double>::max(), std::memory_order_relaxed);
    avgExecutionTimeMs.store(0.0, std::memory_order_relaxed);
    invocationCount.store(0, std::memory_order_relaxed);
    totalLockWaitMs.store(0.0, std::memory_order_relaxed);
    lockAcquisitionCount.store(0, std::memory_order_relaxed);
}

ThreadMetrics AudioThreadMetricsAtomic::toThreadMetrics() const
{
    ThreadMetrics metrics;
    metrics.threadName = "Audio Thread";
    metrics.maxExecutionTimeMs = maxExecutionTimeMs.load(std::memory_order_relaxed);
    metrics.minExecutionTimeMs = minExecutionTimeMs.load(std::memory_order_relaxed);
    metrics.avgExecutionTimeMs = avgExecutionTimeMs.load(std::memory_order_relaxed);
    metrics.invocationCount = invocationCount.load(std::memory_order_relaxed);
    metrics.totalLockWaitMs = totalLockWaitMs.load(std::memory_order_relaxed);
    metrics.lockAcquisitionCount = lockAcquisitionCount.load(std::memory_order_relaxed);
    return metrics;
}

// ============================================================================
// ThreadProfiler
// ============================================================================

void ThreadProfiler::recordAudioThreadExecution(double durationMs)
{
    // Lock-free - safe to call from audio thread
    audioMetricsAtomic_.recordExecution(durationMs);
}

void ThreadProfiler::recordAudioThreadLockWait(double waitMs)
{
    // Lock-free - safe to call from audio thread
    audioMetricsAtomic_.recordLockWait(waitMs);
}

void ThreadProfiler::recordUIThreadExecution(double durationMs)
{
    std::lock_guard<std::mutex> lock(mutex_);
    uiMetrics_.recordExecution(durationMs);
}

void ThreadProfiler::recordOpenGLThreadExecution(double durationMs)
{
    std::lock_guard<std::mutex> lock(mutex_);
    openglMetrics_.recordExecution(durationMs);
}

void ThreadProfiler::recordUIThreadLockWait(double waitMs)
{
    std::lock_guard<std::mutex> lock(mutex_);
    uiMetrics_.recordLockWait(waitMs);
}

void ThreadProfiler::recordOpenGLThreadLockWait(double waitMs)
{
    std::lock_guard<std::mutex> lock(mutex_);
    openglMetrics_.recordLockWait(waitMs);
}

ThreadMetrics ThreadProfiler::getAudioThreadMetrics() const
{
    // Lock-free read from atomic members
    return audioMetricsAtomic_.toThreadMetrics();
}

ThreadMetrics ThreadProfiler::getUIThreadMetrics() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return uiMetrics_;
}

ThreadMetrics ThreadProfiler::getOpenGLThreadMetrics() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return openglMetrics_;
}

void ThreadProfiler::reset()
{
    // Reset audio thread metrics (lock-free)
    audioMetricsAtomic_.reset();

    // Reset other thread metrics (with lock)
    std::lock_guard<std::mutex> lock(mutex_);
    uiMetrics_.reset();
    openglMetrics_.reset();
}

// ============================================================================
// ComponentRepaintStats
// ============================================================================

void ComponentRepaintStats::recordRepaint(double durationMs)
{
    repaintCount++;
    totalRepaintTimeMs += durationMs;
    avgRepaintTimeMs = totalRepaintTimeMs / repaintCount;
    maxRepaintTimeMs = std::max(maxRepaintTimeMs, durationMs);
}

void ComponentRepaintStats::reset()
{
    repaintCount = 0;
    totalRepaintTimeMs = 0.0;
    avgRepaintTimeMs = 0.0;
    maxRepaintTimeMs = 0.0;
    repaintsPerSecond = 0.0;
}

// ============================================================================
// ComponentProfiler
// ============================================================================

void ComponentProfiler::recordRepaint(const std::string& componentName, double durationMs)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& stats = stats_[componentName];
    if (stats.componentName.empty())
    {
        stats.componentName = componentName;
        if (startTime_ == std::chrono::steady_clock::time_point{})
            startTime_ = std::chrono::steady_clock::now();
    }
    stats.recordRepaint(durationMs);
    
    // Update repaints per second
    auto elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - startTime_).count();
    if (elapsed > 0)
        stats.repaintsPerSecond = stats.repaintCount / elapsed;
}

std::vector<ComponentRepaintStats> ComponentProfiler::getStats() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ComponentRepaintStats> result;
    result.reserve(stats_.size());
    for (const auto& [name, stats] : stats_)
        result.push_back(stats);
    return result;
}

ComponentRepaintStats ComponentProfiler::getStats(const std::string& componentName) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = stats_.find(componentName);
    if (it != stats_.end())
        return it->second;
    return {};
}

std::vector<ComponentRepaintStats> ComponentProfiler::getTopByFrequency(int n) const
{
    auto all = getStats();
    std::sort(all.begin(), all.end(),
              [](const auto& a, const auto& b) { return a.repaintsPerSecond > b.repaintsPerSecond; });
    if (static_cast<int>(all.size()) > n)
        all.resize(static_cast<size_t>(n));
    return all;
}

std::vector<ComponentRepaintStats> ComponentProfiler::getTopByTime(int n) const
{
    auto all = getStats();
    std::sort(all.begin(), all.end(),
              [](const auto& a, const auto& b) { return a.totalRepaintTimeMs > b.totalRepaintTimeMs; });
    if (static_cast<int>(all.size()) > n)
        all.resize(static_cast<size_t>(n));
    return all;
}

void ComponentProfiler::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.clear();
    startTime_ = std::chrono::steady_clock::time_point{};
}

// ============================================================================
// MemoryProfiler
// ============================================================================

void MemoryProfiler::recordSnapshot()
{
    MemorySnapshot snapshot;
    snapshot.timestamp = juce::Time::currentTimeMillis();
    
#if JUCE_MAC
    task_basic_info_data_t info;
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS)
    {
        snapshot.residentBytes = static_cast<int64_t>(info.resident_size);
        snapshot.virtualBytes = static_cast<int64_t>(info.virtual_size);
    }
#elif JUCE_WINDOWS
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        snapshot.residentBytes = static_cast<int64_t>(pmc.WorkingSetSize);
        snapshot.virtualBytes = static_cast<int64_t>(pmc.PagefileUsage);
    }
#elif JUCE_LINUX
    std::ifstream statm("/proc/self/statm");
    if (statm.is_open())
    {
        long pages, resident;
        statm >> pages >> resident;
        long pageSize = sysconf(_SC_PAGESIZE);
        snapshot.virtualBytes = pages * pageSize;
        snapshot.residentBytes = resident * pageSize;
    }
#endif
    
    snapshot.residentMB = static_cast<double>(snapshot.residentBytes) / (1024.0 * 1024.0);
    snapshot.virtualMB = static_cast<double>(snapshot.virtualBytes) / (1024.0 * 1024.0);
    
    std::lock_guard<std::mutex> lock(mutex_);
    history_.push_back(snapshot);
    while (history_.size() > MAX_HISTORY)
        history_.pop_front();
}

MemorySnapshot MemoryProfiler::getCurrentSnapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (history_.empty())
    {
        // Record a fresh snapshot
        MemorySnapshot snapshot;
        snapshot.timestamp = juce::Time::currentTimeMillis();
        
#if JUCE_MAC
        task_basic_info_data_t info;
        mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
        
        if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS)
        {
            snapshot.residentBytes = static_cast<int64_t>(info.resident_size);
            snapshot.virtualBytes = static_cast<int64_t>(info.virtual_size);
        }
#endif
        snapshot.residentMB = static_cast<double>(snapshot.residentBytes) / (1024.0 * 1024.0);
        snapshot.virtualMB = static_cast<double>(snapshot.virtualBytes) / (1024.0 * 1024.0);
        return snapshot;
    }
    return history_.back();
}

std::vector<MemorySnapshot> MemoryProfiler::getHistory(int maxEntries) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<MemorySnapshot> result;
    size_t start = 0;
    if (static_cast<int>(history_.size()) > maxEntries)
        start = history_.size() - static_cast<size_t>(maxEntries);
    for (size_t i = start; i < history_.size(); ++i)
        result.push_back(history_[i]);
    return result;
}

double MemoryProfiler::getGrowthRateMBPerSecond() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (history_.size() < 2)
        return 0.0;
    
    const auto& first = history_.front();
    const auto& last = history_.back();
    
    double memDiffMB = last.residentMB - first.residentMB;
    double timeDiffSec = (last.timestamp - first.timestamp) / 1000.0;
    
    if (timeDiffSec <= 0)
        return 0.0;
    
    return memDiffMB / timeDiffSec;
}

double MemoryProfiler::getPeakMemoryMB() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    double peak = 0.0;
    for (const auto& snapshot : history_)
        peak = std::max(peak, snapshot.residentMB);
    return peak;
}

void MemoryProfiler::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    history_.clear();
}

// ============================================================================
// GpuProfiler
// ============================================================================

void GpuProfiler::beginFrame()
{
    std::lock_guard<std::mutex> lock(mutex_);
    frameStart_ = std::chrono::steady_clock::now();
    currentFrame_ = FrameTimingData{};
    currentFrame_.frameNumber = ++frameCounter_;
    currentFrame_.timestamp = juce::Time::currentTimeMillis();
}

void GpuProfiler::endFrame()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    currentFrame_.totalFrameMs = std::chrono::duration<double, std::milli>(now - frameStart_).count();
    
    // Record to histogram (use internal histogram, not the snapshot)
    frameHistogram_.recordFrame(currentFrame_.totalFrameMs);
    totalFrames_++;
    
    // Calculate FPS
    double fps = currentFrame_.totalFrameMs > 0 ? 1000.0 / currentFrame_.totalFrameMs : 0.0;
    currentFps_ = fps;
    avgFps_ = avgFps_ + (fps - avgFps_) / static_cast<double>(totalFrames_);
    minFps_ = std::min(minFps_, fps);
    maxFps_ = std::max(maxFps_, fps);
    
    // Store in history
    frameHistory_.push_back(currentFrame_);
    while (frameHistory_.size() > MAX_FRAME_HISTORY)
        frameHistory_.pop_front();
    
    updateAverages();
}

void GpuProfiler::recordBeginFrame(double ms)
{
    std::lock_guard<std::mutex> lock(mutex_);
    currentFrame_.beginFrameMs = ms;
}

void GpuProfiler::recordWaveformRender(int waveformId, double ms)
{
    std::lock_guard<std::mutex> lock(mutex_);
    currentFrame_.waveformTimings.emplace_back(waveformId, ms);
    currentFrame_.waveformRenderMs += ms;
    currentFrame_.waveformCount++;
}

void GpuProfiler::recordEffectPipeline(double ms)
{
    std::lock_guard<std::mutex> lock(mutex_);
    currentFrame_.effectPipelineMs += ms;
}

void GpuProfiler::recordComposite(double ms)
{
    std::lock_guard<std::mutex> lock(mutex_);
    currentFrame_.compositeMs += ms;
}

void GpuProfiler::recordBlitToScreen(double ms)
{
    std::lock_guard<std::mutex> lock(mutex_);
    currentFrame_.blitToScreenMs = ms;
}

void GpuProfiler::recordEndFrame(double ms)
{
    std::lock_guard<std::mutex> lock(mutex_);
    currentFrame_.endFrameMs = ms;
}

void GpuProfiler::recordFboBind(double ms)
{
    std::lock_guard<std::mutex> lock(mutex_);
    currentFrame_.fboBindUnbindMs += ms;
    currentFrame_.fboBindCount++;
    totalFboBinds_++;
}

void GpuProfiler::recordShaderSwitch(double ms)
{
    std::lock_guard<std::mutex> lock(mutex_);
    currentFrame_.shaderSwitchMs += ms;
    currentFrame_.shaderSwitchCount++;
    totalShaderSwitches_++;
}

void GpuProfiler::recordBudgetStats(double elapsedMs, double budgetMs, 
                                     int skippedWaveforms, int skippedEffects)
{
    std::lock_guard<std::mutex> lock(mutex_);
    double ratio = budgetMs > 0 ? elapsedMs / budgetMs : 0.0;
    
    if (ratio > 1.0)
        budgetExceededCount_++;
    
    totalSkippedWaveforms_ += skippedWaveforms;
    totalSkippedEffects_ += skippedEffects;
    
    // Update averages with exponential moving average
    avgBudgetRatio_ = avgBudgetRatio_ * 0.95 + ratio * 0.05;
    maxBudgetRatio_ = std::max(maxBudgetRatio_, ratio);
}

GpuProfilingData GpuProfiler::getData() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return buildDataSnapshot();
}

GpuProfilingData GpuProfiler::buildDataSnapshot() const
{
    // Build a copyable snapshot of the current state
    GpuProfilingData data;
    data.frameHistogram = frameHistogram_.getSnapshot();
    data.avgBeginFrameMs = avgBeginFrameMs_;
    data.avgWaveformRenderMs = avgWaveformRenderMs_;
    data.avgEffectPipelineMs = avgEffectPipelineMs_;
    data.avgCompositeMs = avgCompositeMs_;
    data.avgBlitMs = avgBlitMs_;
    data.avgEndFrameMs = avgEndFrameMs_;
    data.avgFboBindTimeMs = avgFboBindTimeMs_;
    data.totalFboBinds = totalFboBinds_;
    data.avgShaderSwitchTimeMs = avgShaderSwitchTimeMs_;
    data.totalShaderSwitches = totalShaderSwitches_;
    data.currentFps = currentFps_;
    data.avgFps = avgFps_;
    data.minFps = minFps_;
    data.maxFps = maxFps_;
    data.totalFrames = totalFrames_;
    
    // Budget tracking
    data.budgetExceededCount = budgetExceededCount_;
    data.totalSkippedWaveforms = totalSkippedWaveforms_;
    data.totalSkippedEffects = totalSkippedEffects_;
    data.avgBudgetRatio = avgBudgetRatio_;
    data.maxBudgetRatio = maxBudgetRatio_;
    
    return data;
}

FrameTimingData GpuProfiler::getLastFrameData() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (frameHistory_.empty())
        return {};
    return frameHistory_.back();
}

std::vector<FrameTimingData> GpuProfiler::getRecentFrames(int count) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<FrameTimingData> result;
    int start = static_cast<int>(frameHistory_.size()) - count;
    if (start < 0) start = 0;
    for (size_t i = static_cast<size_t>(start); i < frameHistory_.size(); ++i)
        result.push_back(frameHistory_[i]);
    return result;
}

void GpuProfiler::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    frameHistogram_.reset();
    avgBeginFrameMs_ = 0.0;
    avgWaveformRenderMs_ = 0.0;
    avgEffectPipelineMs_ = 0.0;
    avgCompositeMs_ = 0.0;
    avgBlitMs_ = 0.0;
    avgEndFrameMs_ = 0.0;
    avgFboBindTimeMs_ = 0.0;
    totalFboBinds_ = 0;
    avgShaderSwitchTimeMs_ = 0.0;
    totalShaderSwitches_ = 0;
    currentFps_ = 0.0;
    avgFps_ = 0.0;
    minFps_ = std::numeric_limits<double>::max();
    maxFps_ = 0.0;
    totalFrames_ = 0;
    currentFrame_ = FrameTimingData{};
    frameHistory_.clear();
    frameCounter_ = 0;
    profilingStart_ = std::chrono::steady_clock::now();
}

void GpuProfiler::updateAverages()
{
    if (frameHistory_.empty())
        return;
    
    double sumBegin = 0, sumWaveform = 0, sumEffect = 0, sumComposite = 0, sumBlit = 0, sumEnd = 0;
    double sumFbo = 0, sumShader = 0;
    
    for (const auto& frame : frameHistory_)
    {
        sumBegin += frame.beginFrameMs;
        sumWaveform += frame.waveformRenderMs;
        sumEffect += frame.effectPipelineMs;
        sumComposite += frame.compositeMs;
        sumBlit += frame.blitToScreenMs;
        sumEnd += frame.endFrameMs;
        sumFbo += frame.fboBindUnbindMs;
        sumShader += frame.shaderSwitchMs;
    }
    
    double count = static_cast<double>(frameHistory_.size());
    avgBeginFrameMs_ = sumBegin / count;
    avgWaveformRenderMs_ = sumWaveform / count;
    avgEffectPipelineMs_ = sumEffect / count;
    avgCompositeMs_ = sumComposite / count;
    avgBlitMs_ = sumBlit / count;
    avgEndFrameMs_ = sumEnd / count;
    avgFboBindTimeMs_ = sumFbo / count;
    avgShaderSwitchTimeMs_ = sumShader / count;
}

// ============================================================================
// HotspotDetector
// ============================================================================

std::vector<PerformanceHotspot> HotspotDetector::detectHotspots(
    const GpuProfilingData& gpuData,
    const ThreadProfiler& threadProfiler,
    const ComponentProfiler& componentProfiler,
    const MemoryProfiler& memoryProfiler) const
{
    std::vector<PerformanceHotspot> hotspots;
    
    auto gpuHotspots = detectGpuHotspots(gpuData);
    auto threadHotspots = detectThreadHotspots(threadProfiler);
    auto componentHotspots = detectComponentHotspots(componentProfiler);
    auto memoryHotspots = detectMemoryHotspots(memoryProfiler);
    
    hotspots.insert(hotspots.end(), gpuHotspots.begin(), gpuHotspots.end());
    hotspots.insert(hotspots.end(), threadHotspots.begin(), threadHotspots.end());
    hotspots.insert(hotspots.end(), componentHotspots.begin(), componentHotspots.end());
    hotspots.insert(hotspots.end(), memoryHotspots.begin(), memoryHotspots.end());
    
    // Sort by severity
    std::sort(hotspots.begin(), hotspots.end(),
              [](const auto& a, const auto& b) { return a.severity > b.severity; });
    
    return hotspots;
}

std::vector<PerformanceHotspot> HotspotDetector::detectGpuHotspots(const GpuProfilingData& data) const
{
    std::vector<PerformanceHotspot> hotspots;
    
    // Check frame time (target: 16.67ms for 60fps)
    double avgFrameTime = data.frameHistogram.getAverageMs();
    if (avgFrameTime > 16.67)
    {
        PerformanceHotspot hs;
        hs.category = "GPU";
        hs.location = "Frame Time";
        hs.description = "Average frame time exceeds 60fps target";
        hs.severity = std::min(1.0, (avgFrameTime - 16.67) / 33.33);  // Max severity at 50ms
        hs.impactMs = avgFrameTime - 16.67;
        hs.recommendation = "Reduce waveform count, disable effects, or lower quality settings";
        hotspots.push_back(hs);
    }
    
    // Check P95 frame time for stuttering
    double p95FrameTime = data.frameHistogram.getPercentile(95);
    if (p95FrameTime > 33.33)  // More than 30fps occasionally
    {
        PerformanceHotspot hs;
        hs.category = "GPU";
        hs.location = "Frame Stuttering";
        hs.description = "95th percentile frame time indicates stuttering";
        hs.severity = std::min(1.0, (p95FrameTime - 33.33) / 66.67);
        hs.impactMs = p95FrameTime;
        hs.recommendation = "Check for GC pressure, reduce effect complexity, or optimize shaders";
        hotspots.push_back(hs);
    }
    
    // Check effect pipeline overhead
    if (data.avgEffectPipelineMs > 5.0)
    {
        PerformanceHotspot hs;
        hs.category = "GPU";
        hs.location = "Effect Pipeline";
        hs.description = "Effect pipeline consuming significant frame time";
        hs.severity = std::min(1.0, data.avgEffectPipelineMs / 10.0);
        hs.impactMs = data.avgEffectPipelineMs;
        hs.recommendation = "Reduce effect count or complexity, use lower quality presets";
        hotspots.push_back(hs);
    }
    
    // Check FBO bind overhead
    if (data.avgFboBindTimeMs > 1.0)
    {
        PerformanceHotspot hs;
        hs.category = "GPU";
        hs.location = "FBO Operations";
        hs.description = "Framebuffer bind/unbind overhead is high";
        hs.severity = std::min(1.0, data.avgFboBindTimeMs / 3.0);
        hs.impactMs = data.avgFboBindTimeMs;
        hs.recommendation = "Batch FBO operations, reduce render passes";
        hotspots.push_back(hs);
    }
    
    return hotspots;
}

std::vector<PerformanceHotspot> HotspotDetector::detectThreadHotspots(const ThreadProfiler& profiler) const
{
    std::vector<PerformanceHotspot> hotspots;
    
    auto audioMetrics = profiler.getAudioThreadMetrics();
    auto uiMetrics = profiler.getUIThreadMetrics();
    
    // Check audio thread (must be under ~2ms at 48kHz with 128 sample buffer)
    if (audioMetrics.maxExecutionTimeMs > 2.0)
    {
        PerformanceHotspot hs;
        hs.category = "Thread";
        hs.location = "Audio Thread";
        hs.description = "Audio thread execution time risks buffer underruns";
        hs.severity = std::min(1.0, audioMetrics.maxExecutionTimeMs / 5.0);
        hs.impactMs = audioMetrics.maxExecutionTimeMs;
        hs.recommendation = "Reduce audio processing complexity, check for allocations or locks";
        hotspots.push_back(hs);
    }
    
    // Check audio thread lock contention
    if (audioMetrics.totalLockWaitMs > 0.1 && audioMetrics.lockAcquisitionCount > 0)
    {
        double avgLockWait = audioMetrics.totalLockWaitMs / audioMetrics.lockAcquisitionCount;
        if (avgLockWait > 0.01)  // More than 10 microseconds average
        {
            PerformanceHotspot hs;
            hs.category = "Lock";
            hs.location = "Audio Thread Contention";
            hs.description = "Audio thread experiencing lock contention";
            hs.severity = std::min(1.0, avgLockWait * 10.0);
            hs.impactMs = avgLockWait;
            hs.recommendation = "Use lock-free data structures, reduce critical section size";
            hotspots.push_back(hs);
        }
    }
    
    // Check UI thread responsiveness
    if (uiMetrics.maxExecutionTimeMs > 16.67)
    {
        PerformanceHotspot hs;
        hs.category = "Thread";
        hs.location = "UI Thread";
        hs.description = "UI thread blocked, causing unresponsive interface";
        hs.severity = std::min(1.0, uiMetrics.maxExecutionTimeMs / 50.0);
        hs.impactMs = uiMetrics.maxExecutionTimeMs;
        hs.recommendation = "Move heavy operations to background thread, optimize repaints";
        hotspots.push_back(hs);
    }
    
    return hotspots;
}

std::vector<PerformanceHotspot> HotspotDetector::detectComponentHotspots(const ComponentProfiler& profiler) const
{
    std::vector<PerformanceHotspot> hotspots;
    
    auto topByFreq = profiler.getTopByFrequency(5);
    
    for (const auto& stats : topByFreq)
    {
        // Flag components repainting more than 120 times per second
        if (stats.repaintsPerSecond > 120.0)
        {
            PerformanceHotspot hs;
            hs.category = "UI";
            hs.location = stats.componentName;
            hs.description = "Component repainting excessively (" + 
                            std::to_string(static_cast<int>(stats.repaintsPerSecond)) + "/sec)";
            hs.severity = std::min(1.0, (stats.repaintsPerSecond - 60.0) / 200.0);
            hs.impactMs = stats.avgRepaintTimeMs * stats.repaintsPerSecond / 1000.0;
            hs.recommendation = "Reduce repaint triggers, use setRepaintsOnMouseActivity(false), coalesce updates";
            hotspots.push_back(hs);
        }
    }
    
    auto topByTime = profiler.getTopByTime(5);
    for (const auto& stats : topByTime)
    {
        // Flag expensive repaints
        if (stats.maxRepaintTimeMs > 5.0)
        {
            PerformanceHotspot hs;
            hs.category = "UI";
            hs.location = stats.componentName + " (repaint)";
            hs.description = "Component has expensive paint operations";
            hs.severity = std::min(1.0, stats.maxRepaintTimeMs / 16.67);
            hs.impactMs = stats.maxRepaintTimeMs;
            hs.recommendation = "Optimize paint(), use cached images, reduce path complexity";
            hotspots.push_back(hs);
        }
    }
    
    return hotspots;
}

std::vector<PerformanceHotspot> HotspotDetector::detectMemoryHotspots(const MemoryProfiler& profiler) const
{
    std::vector<PerformanceHotspot> hotspots;
    
    double growthRate = profiler.getGrowthRateMBPerSecond();
    
    // Flag memory leaks (growing more than 1MB/second sustained)
    if (growthRate > 1.0)
    {
        PerformanceHotspot hs;
        hs.category = "Memory";
        hs.location = "Memory Leak";
        hs.description = "Memory growing at " + std::to_string(growthRate) + " MB/sec";
        hs.severity = std::min(1.0, growthRate / 10.0);
        hs.impactMs = 0.0;  // Indirect impact
        hs.recommendation = "Check for accumulating data structures, use memory profiler";
        hotspots.push_back(hs);
    }
    
    // Flag high memory usage
    double peakMB = profiler.getPeakMemoryMB();
    if (peakMB > 500.0)  // More than 500MB
    {
        PerformanceHotspot hs;
        hs.category = "Memory";
        hs.location = "Memory Usage";
        hs.description = "High memory usage: " + std::to_string(static_cast<int>(peakMB)) + " MB";
        hs.severity = std::min(1.0, (peakMB - 500.0) / 1000.0);
        hs.impactMs = 0.0;
        hs.recommendation = "Review buffer sizes, texture memory, reduce history length";
        hotspots.push_back(hs);
    }
    
    return hotspots;
}

// ============================================================================
// PerformanceProfiler
// ============================================================================

PerformanceProfiler& PerformanceProfiler::getInstance()
{
    static PerformanceProfiler instance;
    return instance;
}

void PerformanceProfiler::startProfiling(const ProfilingConfig& config)
{
    if (profiling_.load())
        return;
    
    config_ = config;
    reset();
    startTime_ = std::chrono::steady_clock::now();
    profiling_.store(true);
    
    // Start memory snapshots
    if (config_.enableMemoryProfiling)
    {
        memoryTimerRunning_.store(true);
        memoryProfiler_.recordSnapshot();
    }
}

void PerformanceProfiler::stopProfiling()
{
    profiling_.store(false);
    memoryTimerRunning_.store(false);
}

ProfilingSnapshot PerformanceProfiler::getSnapshot() const
{
    ProfilingSnapshot snapshot;
    snapshot.timestamp = juce::Time::currentTimeMillis();
    
    auto elapsed = std::chrono::steady_clock::now() - startTime_;
    snapshot.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    
    snapshot.gpuData = gpuProfiler_.getData();
    snapshot.audioThreadMetrics = threadProfiler_.getAudioThreadMetrics();
    snapshot.uiThreadMetrics = threadProfiler_.getUIThreadMetrics();
    snapshot.openglThreadMetrics = threadProfiler_.getOpenGLThreadMetrics();
    snapshot.componentStats = componentProfiler_.getStats();
    snapshot.memorySnapshot = memoryProfiler_.getCurrentSnapshot();
    
    // Calculate summary metrics
    snapshot.avgFps = snapshot.gpuData.avgFps;
    snapshot.p50FrameTimeMs = snapshot.gpuData.frameHistogram.getPercentile(50);
    snapshot.p95FrameTimeMs = snapshot.gpuData.frameHistogram.getPercentile(95);
    snapshot.p99FrameTimeMs = snapshot.gpuData.frameHistogram.getPercentile(99);
    snapshot.peakMemoryMB = memoryProfiler_.getPeakMemoryMB();
    
    if (config_.enableHotspotDetection)
    {
        snapshot.hotspots = hotspotDetector_.detectHotspots(
            snapshot.gpuData, threadProfiler_, componentProfiler_, memoryProfiler_);
    }
    
    return snapshot;
}

std::vector<PerformanceHotspot> PerformanceProfiler::getHotspots() const
{
    auto gpuData = gpuProfiler_.getData();
    return hotspotDetector_.detectHotspots(gpuData, threadProfiler_, componentProfiler_, memoryProfiler_);
}

void PerformanceProfiler::reset()
{
    gpuProfiler_.reset();
    threadProfiler_.reset();
    componentProfiler_.reset();
    memoryProfiler_.reset();
    startTime_ = std::chrono::steady_clock::now();
}

juce::String PerformanceProfiler::toJson() const
{
    auto snapshot = getSnapshot();
    
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{";
    ss << "\"timestamp\":" << snapshot.timestamp << ",";
    ss << "\"durationMs\":" << snapshot.durationMs << ",";
    ss << "\"summary\":{";
    ss << "\"avgFps\":" << snapshot.avgFps << ",";
    ss << "\"p50FrameTimeMs\":" << snapshot.p50FrameTimeMs << ",";
    ss << "\"p95FrameTimeMs\":" << snapshot.p95FrameTimeMs << ",";
    ss << "\"p99FrameTimeMs\":" << snapshot.p99FrameTimeMs << ",";
    ss << "\"peakMemoryMB\":" << snapshot.peakMemoryMB;
    ss << "},";
    ss << "\"hotspotCount\":" << snapshot.hotspots.size();
    ss << "}";
    
    return juce::String(ss.str());
}

juce::String PerformanceProfiler::getGpuDataJson() const
{
    auto data = gpuProfiler_.getData();
    
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "{";
    ss << "\"currentFps\":" << data.currentFps << ",";
    ss << "\"avgFps\":" << data.avgFps << ",";
    ss << "\"minFps\":" << (data.minFps == std::numeric_limits<double>::max() ? 0.0 : data.minFps) << ",";
    ss << "\"maxFps\":" << data.maxFps << ",";
    ss << "\"totalFrames\":" << data.totalFrames << ",";
    ss << "\"avgFrameTimeMs\":" << data.frameHistogram.getAverageMs() << ",";
    ss << "\"minFrameTimeMs\":" << (data.frameHistogram.getMinMs() == std::numeric_limits<double>::max() ? 0.0 : data.frameHistogram.getMinMs()) << ",";
    ss << "\"maxFrameTimeMs\":" << data.frameHistogram.getMaxMs() << ",";
    ss << "\"p50FrameTimeMs\":" << data.frameHistogram.getPercentile(50) << ",";
    ss << "\"p95FrameTimeMs\":" << data.frameHistogram.getPercentile(95) << ",";
    ss << "\"p99FrameTimeMs\":" << data.frameHistogram.getPercentile(99) << ",";
    ss << "\"timing\":{";
    ss << "\"avgBeginFrameMs\":" << data.avgBeginFrameMs << ",";
    ss << "\"avgWaveformRenderMs\":" << data.avgWaveformRenderMs << ",";
    ss << "\"avgEffectPipelineMs\":" << data.avgEffectPipelineMs << ",";
    ss << "\"avgCompositeMs\":" << data.avgCompositeMs << ",";
    ss << "\"avgBlitMs\":" << data.avgBlitMs << ",";
    ss << "\"avgEndFrameMs\":" << data.avgEndFrameMs;
    ss << "},";
    ss << "\"operations\":{";
    ss << "\"avgFboBindTimeMs\":" << data.avgFboBindTimeMs << ",";
    ss << "\"totalFboBinds\":" << data.totalFboBinds << ",";
    ss << "\"avgShaderSwitchTimeMs\":" << data.avgShaderSwitchTimeMs << ",";
    ss << "\"totalShaderSwitches\":" << data.totalShaderSwitches;
    ss << "},";
    ss << "\"budget\":{";
    ss << "\"exceededCount\":" << data.budgetExceededCount << ",";
    ss << "\"totalSkippedWaveforms\":" << data.totalSkippedWaveforms << ",";
    ss << "\"totalSkippedEffects\":" << data.totalSkippedEffects << ",";
    ss << "\"avgBudgetRatio\":" << data.avgBudgetRatio << ",";
    ss << "\"maxBudgetRatio\":" << data.maxBudgetRatio;
    ss << "}";
    ss << "}";
    
    return juce::String(ss.str());
}

juce::String PerformanceProfiler::getThreadDataJson() const
{
    auto audio = threadProfiler_.getAudioThreadMetrics();
    auto ui = threadProfiler_.getUIThreadMetrics();
    auto gl = threadProfiler_.getOpenGLThreadMetrics();
    
    auto threadToJson = [](const ThreadMetrics& m) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(3);
        ss << "{";
        ss << "\"name\":\"" << m.threadName << "\",";
        ss << "\"avgExecutionTimeMs\":" << m.avgExecutionTimeMs << ",";
        ss << "\"maxExecutionTimeMs\":" << m.maxExecutionTimeMs << ",";
        ss << "\"minExecutionTimeMs\":" << (m.minExecutionTimeMs == std::numeric_limits<double>::max() ? 0.0 : m.minExecutionTimeMs) << ",";
        ss << "\"invocationCount\":" << m.invocationCount << ",";
        ss << "\"loadPercent\":" << m.loadPercent << ",";
        ss << "\"totalLockWaitMs\":" << m.totalLockWaitMs << ",";
        ss << "\"lockAcquisitionCount\":" << m.lockAcquisitionCount;
        ss << "}";
        return ss.str();
    };
    
    std::ostringstream ss;
    ss << "{";
    ss << "\"audioThread\":" << threadToJson(audio) << ",";
    ss << "\"uiThread\":" << threadToJson(ui) << ",";
    ss << "\"openglThread\":" << threadToJson(gl);
    ss << "}";
    
    return juce::String(ss.str());
}

juce::String PerformanceProfiler::getComponentDataJson() const
{
    auto stats = componentProfiler_.getStats();
    
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "{\"components\":[";
    
    bool first = true;
    for (const auto& s : stats)
    {
        if (!first) ss << ",";
        first = false;
        
        ss << "{";
        ss << "\"name\":\"" << s.componentName << "\",";
        ss << "\"repaintCount\":" << s.repaintCount << ",";
        ss << "\"totalRepaintTimeMs\":" << s.totalRepaintTimeMs << ",";
        ss << "\"avgRepaintTimeMs\":" << s.avgRepaintTimeMs << ",";
        ss << "\"maxRepaintTimeMs\":" << s.maxRepaintTimeMs << ",";
        ss << "\"repaintsPerSecond\":" << s.repaintsPerSecond;
        ss << "}";
    }
    
    ss << "]}";
    return juce::String(ss.str());
}

juce::String PerformanceProfiler::getHotspotsJson() const
{
    auto hotspots = getHotspots();
    
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "{\"hotspots\":[";
    
    bool first = true;
    for (const auto& hs : hotspots)
    {
        if (!first) ss << ",";
        first = false;
        
        ss << "{";
        ss << "\"category\":\"" << hs.category << "\",";
        ss << "\"location\":\"" << hs.location << "\",";
        ss << "\"description\":\"" << hs.description << "\",";
        ss << "\"severity\":" << hs.severity << ",";
        ss << "\"impactMs\":" << hs.impactMs << ",";
        ss << "\"recommendation\":\"" << hs.recommendation << "\"";
        ss << "}";
    }
    
    ss << "]}";
    return juce::String(ss.str());
}

juce::String PerformanceProfiler::getTimelineJson(int frameCount) const
{
    auto frames = gpuProfiler_.getRecentFrames(frameCount);
    
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "{\"frames\":[";
    
    bool first = true;
    for (const auto& f : frames)
    {
        if (!first) ss << ",";
        first = false;
        
        ss << "{";
        ss << "\"frameNumber\":" << f.frameNumber << ",";
        ss << "\"timestamp\":" << f.timestamp << ",";
        ss << "\"totalFrameMs\":" << f.totalFrameMs << ",";
        ss << "\"beginFrameMs\":" << f.beginFrameMs << ",";
        ss << "\"waveformRenderMs\":" << f.waveformRenderMs << ",";
        ss << "\"effectPipelineMs\":" << f.effectPipelineMs << ",";
        ss << "\"compositeMs\":" << f.compositeMs << ",";
        ss << "\"blitToScreenMs\":" << f.blitToScreenMs << ",";
        ss << "\"waveformCount\":" << f.waveformCount;
        ss << "}";
    }
    
    ss << "]}";
    return juce::String(ss.str());
}

// ============================================================================
// RAII Helpers
// ============================================================================

ScopedGpuProfile::ScopedGpuProfile(Phase phase, int waveformId)
    : phase_(phase)
    , waveformId_(waveformId)
    , start_(std::chrono::steady_clock::now())
{
}

ScopedGpuProfile::~ScopedGpuProfile()
{
    if (!PerformanceProfiler::getInstance().isProfiling())
        return;
    
    auto end = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start_).count();
    
    auto& profiler = PerformanceProfiler::getInstance().getGpuProfiler();
    
    switch (phase_)
    {
        case Phase::BeginFrame:
            profiler.recordBeginFrame(ms);
            break;
        case Phase::WaveformRender:
            profiler.recordWaveformRender(waveformId_, ms);
            break;
        case Phase::EffectPipeline:
            profiler.recordEffectPipeline(ms);
            break;
        case Phase::Composite:
            profiler.recordComposite(ms);
            break;
        case Phase::BlitToScreen:
            profiler.recordBlitToScreen(ms);
            break;
        case Phase::EndFrame:
            profiler.recordEndFrame(ms);
            break;
    }
}

ScopedComponentProfile::ScopedComponentProfile(const std::string& componentName)
    : componentName_(componentName)
    , start_(std::chrono::steady_clock::now())
{
}

ScopedComponentProfile::~ScopedComponentProfile()
{
    if (!PerformanceProfiler::getInstance().isProfiling())
        return;
    
    auto end = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start_).count();
    
    PerformanceProfiler::getInstance().getComponentProfiler().recordRepaint(componentName_, ms);
}

ScopedThreadProfile::ScopedThreadProfile(Thread thread)
    : thread_(thread)
    , start_(std::chrono::steady_clock::now())
{
}

ScopedThreadProfile::~ScopedThreadProfile()
{
    if (!PerformanceProfiler::getInstance().isProfiling())
        return;
    
    auto end = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start_).count();
    
    auto& profiler = PerformanceProfiler::getInstance().getThreadProfiler();
    
    switch (thread_)
    {
        case Thread::Audio:
            profiler.recordAudioThreadExecution(ms);
            break;
        case Thread::UI:
            profiler.recordUIThreadExecution(ms);
            break;
        case Thread::OpenGL:
            profiler.recordOpenGLThreadExecution(ms);
            break;
    }
}

} // namespace oscil

