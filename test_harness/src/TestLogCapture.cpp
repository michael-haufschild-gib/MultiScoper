/*
    Oscil Test Harness - Log Capture Implementation
*/

#include "TestLogCapture.h"

namespace oscil::test
{

TestLogCapture::TestLogCapture()
    : startTimeMs_(juce::Time::getMillisecondCounter())
{
}

TestLogCapture& TestLogCapture::getInstance()
{
    static TestLogCapture instance;
    return instance;
}

void TestLogCapture::logMessage(const juce::String& message)
{
    LogEntry entry;
    entry.timestampMs = static_cast<int64_t>(juce::Time::getMillisecondCounter()) - startTimeMs_;
    entry.message = message.toStdString();

    // Also print to stderr for debugging
    fprintf(stderr, "[%lld] %s\n", entry.timestampMs, entry.message.c_str());

    std::scoped_lock lock(mutex_);
    entries_.push_back(std::move(entry));
    while (entries_.size() > MAX_ENTRIES)
        entries_.pop_front();
}

std::vector<LogEntry> TestLogCapture::getRecentLogs(size_t maxEntries) const
{
    std::scoped_lock lock(mutex_);
    size_t count = std::min(maxEntries, entries_.size());
    auto start = entries_.end() - static_cast<std::ptrdiff_t>(count);
    return { start, entries_.end() };
}

void TestLogCapture::clear()
{
    std::scoped_lock lock(mutex_);
    entries_.clear();
}

} // namespace oscil::test
