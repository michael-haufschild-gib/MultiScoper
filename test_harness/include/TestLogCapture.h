/*
    Oscil Test Harness - Log Capture
    Thread-safe ring buffer that captures juce::Logger output for diagnostic snapshots.
*/

#pragma once

#include <juce_core/juce_core.h>

#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace oscil::test
{

struct LogEntry
{
    int64_t timestampMs = 0;
    std::string message;
};

/**
 * Logger that captures messages into a thread-safe ring buffer.
 * Install via juce::Logger::setCurrentLogger().
 * Retrieve recent entries for diagnostic snapshots.
 */
class TestLogCapture : public juce::Logger
{
public:
    static constexpr size_t MAX_ENTRIES = 2000;

    TestLogCapture();

    /** Get the last N log entries (most recent last). */
    std::vector<LogEntry> getRecentLogs(size_t maxEntries = MAX_ENTRIES) const;

    /** Clear all captured logs. */
    void clear();

    /** Get singleton (created on first call). */
    static TestLogCapture& getInstance();

protected:
    void logMessage(const juce::String& message) override;

private:
    mutable std::mutex mutex_;
    std::deque<LogEntry> entries_;
    int64_t startTimeMs_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestLogCapture)
};

} // namespace oscil::test
