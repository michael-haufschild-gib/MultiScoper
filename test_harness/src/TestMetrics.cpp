/*
    Oscil Test Harness - Performance Metrics Implementation
*/

#include "TestMetrics.h"
#include <algorithm>
#include <numeric>

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

namespace oscil::test
{

TestMetrics::TestMetrics()
{
}

TestMetrics::~TestMetrics()
{
    stopCollection();
}

void TestMetrics::startCollection(int intervalMs)
{
    if (collecting_.load())
        return;

    reset();
    collecting_.store(true);
    startTimer(intervalMs);
}

void TestMetrics::stopCollection()
{
    collecting_.store(false);
    stopTimer();
}

void TestMetrics::timerCallback()
{
    collectSample();
}

void TestMetrics::collectSample()
{
    PerformanceSnapshot snapshot;
    snapshot.fps = calculateFps();
    snapshot.cpuPercent = measureCpuUsage();
    snapshot.memoryBytes = measureMemoryUsage();
    snapshot.memoryMB = static_cast<double>(snapshot.memoryBytes) / (1024.0 * 1024.0);
    snapshot.oscillatorCount = oscillatorCount_.load();
    snapshot.sourceCount = sourceCount_.load();
    snapshot.timestamp = juce::Time::currentTimeMillis();

    std::lock_guard<std::mutex> lock(snapshotMutex_);
    snapshots_.push_back(snapshot);
    while (snapshots_.size() > MAX_SNAPSHOTS)
        snapshots_.pop_front();
}

PerformanceSnapshot TestMetrics::getCurrentSnapshot()
{
    std::lock_guard<std::mutex> lock(snapshotMutex_);
    if (snapshots_.empty())
    {
        // Return a live snapshot if no collected data
        PerformanceSnapshot snapshot;
        snapshot.fps = calculateFps();
        snapshot.cpuPercent = measureCpuUsage();
        snapshot.memoryBytes = measureMemoryUsage();
        snapshot.memoryMB = static_cast<double>(snapshot.memoryBytes) / (1024.0 * 1024.0);
        snapshot.oscillatorCount = oscillatorCount_.load();
        snapshot.sourceCount = sourceCount_.load();
        snapshot.timestamp = juce::Time::currentTimeMillis();
        return snapshot;
    }
    return snapshots_.back();
}

PerformanceStats TestMetrics::getStats()
{
    std::lock_guard<std::mutex> lock(snapshotMutex_);

    PerformanceStats stats;
    if (snapshots_.empty())
        return stats;

    stats.sampleCount = static_cast<int>(snapshots_.size());

    double fpsSum = 0, cpuSum = 0, memSum = 0;
    stats.minFps = snapshots_.front().fps;
    stats.maxFps = snapshots_.front().fps;
    stats.maxCpu = 0;
    stats.maxMemoryMB = 0;

    for (const auto& s : snapshots_)
    {
        fpsSum += s.fps;
        cpuSum += s.cpuPercent;
        memSum += s.memoryMB;

        stats.minFps = std::min(stats.minFps, s.fps);
        stats.maxFps = std::max(stats.maxFps, s.fps);
        stats.maxCpu = std::max(stats.maxCpu, s.cpuPercent);
        stats.maxMemoryMB = std::max(stats.maxMemoryMB, s.memoryMB);
    }

    stats.avgFps = fpsSum / snapshots_.size();
    stats.avgCpu = cpuSum / snapshots_.size();
    stats.avgMemoryMB = memSum / snapshots_.size();

    if (snapshots_.size() >= 2)
    {
        stats.durationMs = snapshots_.back().timestamp - snapshots_.front().timestamp;
    }

    return stats;
}

PerformanceStats TestMetrics::getStatsForPeriod(int periodMs)
{
    std::lock_guard<std::mutex> lock(snapshotMutex_);

    PerformanceStats stats;
    if (snapshots_.empty())
        return stats;

    int64_t cutoff = juce::Time::currentTimeMillis() - periodMs;

    // Find snapshots within the period
    std::vector<PerformanceSnapshot> periodSnapshots;
    for (const auto& s : snapshots_)
    {
        if (s.timestamp >= cutoff)
            periodSnapshots.push_back(s);
    }

    if (periodSnapshots.empty())
        return stats;

    stats.sampleCount = static_cast<int>(periodSnapshots.size());

    double fpsSum = 0, cpuSum = 0, memSum = 0;
    stats.minFps = periodSnapshots.front().fps;
    stats.maxFps = periodSnapshots.front().fps;
    stats.maxCpu = 0;
    stats.maxMemoryMB = 0;

    for (const auto& s : periodSnapshots)
    {
        fpsSum += s.fps;
        cpuSum += s.cpuPercent;
        memSum += s.memoryMB;

        stats.minFps = std::min(stats.minFps, s.fps);
        stats.maxFps = std::max(stats.maxFps, s.fps);
        stats.maxCpu = std::max(stats.maxCpu, s.cpuPercent);
        stats.maxMemoryMB = std::max(stats.maxMemoryMB, s.memoryMB);
    }

    stats.avgFps = fpsSum / periodSnapshots.size();
    stats.avgCpu = cpuSum / periodSnapshots.size();
    stats.avgMemoryMB = memSum / periodSnapshots.size();
    stats.durationMs = periodMs;

    return stats;
}

void TestMetrics::reset()
{
    {
        std::lock_guard<std::mutex> lock(snapshotMutex_);
        snapshots_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(frameTimeMutex_);
        frameTimes_.clear();
    }
    lastCpuTime_ = 0;
    lastSystemTime_ = 0;
}

void TestMetrics::recordFrame()
{
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    std::lock_guard<std::mutex> lock(frameTimeMutex_);
    frameTimes_.push_back(nowMs);
    while (frameTimes_.size() > MAX_FRAME_TIMES)
        frameTimes_.pop_front();
}

double TestMetrics::getCurrentFps()
{
    return calculateFps();
}

double TestMetrics::calculateFps()
{
    std::lock_guard<std::mutex> lock(frameTimeMutex_);

    if (frameTimes_.size() < 2)
        return 0.0;

    // Calculate FPS based on frames in the last second
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    int64_t oneSecondAgo = nowMs - 1000;
    int frameCount = 0;

    for (auto it = frameTimes_.rbegin(); it != frameTimes_.rend(); ++it)
    {
        if (*it >= oneSecondAgo)
            ++frameCount;
        else
            break;
    }

    // If we have frames in the last second, return that count as FPS
    if (frameCount > 0)
        return static_cast<double>(frameCount);

    // Otherwise, calculate from the time span of all frames
    int64_t elapsed = frameTimes_.back() - frameTimes_.front();
    if (elapsed > 0)
    {
        return static_cast<double>(frameTimes_.size() - 1) * 1000.0 / elapsed;
    }

    return 0.0;
}

int64_t TestMetrics::getCurrentMemoryBytes()
{
    return measureMemoryUsage();
}

double TestMetrics::getCurrentMemoryMB()
{
    return static_cast<double>(measureMemoryUsage()) / (1024.0 * 1024.0);
}

double TestMetrics::getCurrentCpuPercent()
{
    return measureCpuUsage();
}

void TestMetrics::setOscillatorCount(int count)
{
    oscillatorCount_.store(count);
}

void TestMetrics::setSourceCount(int count)
{
    sourceCount_.store(count);
}

int64_t TestMetrics::measureMemoryUsage()
{
#if JUCE_MAC
    task_basic_info_data_t info;
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;

    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS)
    {
        return static_cast<int64_t>(info.resident_size);
    }
    return 0;

#elif JUCE_WINDOWS
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        return static_cast<int64_t>(pmc.WorkingSetSize);
    }
    return 0;

#elif JUCE_LINUX
    std::ifstream statm("/proc/self/statm");
    if (statm.is_open())
    {
        long pages;
        statm >> pages;
        return pages * sysconf(_SC_PAGESIZE);
    }
    return 0;

#else
    return 0;
#endif
}

double TestMetrics::measureCpuUsage()
{
#if JUCE_MAC
    task_thread_times_info_data_t info;
    mach_msg_type_number_t count = TASK_THREAD_TIMES_INFO_COUNT;

    if (task_info(mach_task_self(), TASK_THREAD_TIMES_INFO, (task_info_t)&info, &count) == KERN_SUCCESS)
    {
        int64_t cpuTime = (info.user_time.seconds + info.system_time.seconds) * 1000000LL
                         + info.user_time.microseconds + info.system_time.microseconds;

        int64_t now = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        if (lastCpuTime_ > 0 && lastSystemTime_ > 0)
        {
            int64_t cpuDelta = cpuTime - lastCpuTime_;
            int64_t timeDelta = now - lastSystemTime_;

            if (timeDelta > 0)
            {
                double percent = (static_cast<double>(cpuDelta) / timeDelta) * 100.0;
                lastCpuTime_ = cpuTime;
                lastSystemTime_ = now;
                return std::clamp(percent, 0.0, 100.0);
            }
        }

        lastCpuTime_ = cpuTime;
        lastSystemTime_ = now;
        return 0.0;
    }
    return 0.0;

#elif JUCE_WINDOWS
    // Windows implementation uses GetProcessTimes
    FILETIME createTime, exitTime, kernelTime, userTime;
    if (GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime))
    {
        ULARGE_INTEGER kernel, user;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;

        int64_t cpuTime = (kernel.QuadPart + user.QuadPart) / 10; // Convert to microseconds
        int64_t now = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        if (lastCpuTime_ > 0 && lastSystemTime_ > 0)
        {
            int64_t cpuDelta = cpuTime - lastCpuTime_;
            int64_t timeDelta = now - lastSystemTime_;

            if (timeDelta > 0)
            {
                double percent = (static_cast<double>(cpuDelta) / timeDelta) * 100.0;
                lastCpuTime_ = cpuTime;
                lastSystemTime_ = now;
                return std::clamp(percent, 0.0, 100.0);
            }
        }

        lastCpuTime_ = cpuTime;
        lastSystemTime_ = now;
    }
    return 0.0;

#elif JUCE_LINUX
    std::ifstream stat("/proc/self/stat");
    if (stat.is_open())
    {
        std::string ignore;
        long utime, stime;
        for (int i = 0; i < 13; ++i) stat >> ignore;
        stat >> utime >> stime;

        int64_t cpuTime = (utime + stime) * (1000000 / sysconf(_SC_CLK_TCK));
        int64_t now = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        if (lastCpuTime_ > 0 && lastSystemTime_ > 0)
        {
            int64_t cpuDelta = cpuTime - lastCpuTime_;
            int64_t timeDelta = now - lastSystemTime_;

            if (timeDelta > 0)
            {
                double percent = (static_cast<double>(cpuDelta) / timeDelta) * 100.0;
                lastCpuTime_ = cpuTime;
                lastSystemTime_ = now;
                return std::clamp(percent, 0.0, 100.0);
            }
        }

        lastCpuTime_ = cpuTime;
        lastSystemTime_ = now;
    }
    return 0.0;

#else
    return 0.0;
#endif
}

} // namespace oscil::test
