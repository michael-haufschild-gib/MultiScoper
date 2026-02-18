/*
    Oscil - Debug Logger
    File-based logging for diagnosing issues in DAW environments
*/

#pragma once

#include <juce_core/juce_core.h>
#include <fstream>
#include <mutex>
#include <chrono>

namespace oscil
{

class DebugLogger
{
public:
    static DebugLogger& getInstance()
    {
        static DebugLogger instance;
        return instance;
    }

    void log(const juce::String& category, const juce::String& message)
    {
        // THREAD SAFETY: Skip logging if NOT on message thread
        // This prevents blocking on audio thread (styleguide constitutional rule #2)
        // - std::mutex::lock() blocks
        // - file I/O blocks
        // Both are forbidden on audio thread.
        if (!juce::MessageManager::existsAndIsCurrentThread())
        {
            // Not on message thread - likely audio thread, skip to avoid blocking
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if (!file_.is_open())
            return;

        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count() % 1000;
        auto time = std::chrono::system_clock::to_time_t(now);

        char timeStr[32];
        std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", std::localtime(&time));

        file_ << timeStr << "." << std::setfill('0') << std::setw(3) << ms
              << " [" << category.toStdString() << "] "
              << message.toStdString() << "\n";
        file_.flush();
    }

    void logSourceInfo(const juce::String& action, const juce::String& sourceId,
                       const juce::String& name, bool bufferValid, int sourceCount)
    {
        log("SOURCE", action + ": id=" + sourceId + ", name=" + name
            + ", bufferValid=" + (bufferValid ? "YES" : "NO")
            + ", totalSources=" + juce::String(sourceCount));
    }

    void logBufferAccess(const juce::String& location, const juce::String& sourceId,
                         bool found, bool lockSuccess, const juce::String& extra = "")
    {
        juce::String msg = location + ": sourceId=" + sourceId
            + ", found=" + (found ? "YES" : "NO")
            + ", lockSuccess=" + (lockSuccess ? "YES" : "NO");
        if (extra.isNotEmpty())
            msg += ", " + extra;
        log("BUFFER", msg);
    }

    void logRendering(const juce::String& component, const juce::String& message)
    {
        log("RENDER", component + ": " + message);
    }

    void logCapture(const juce::String& message)
    {
        log("CAPTURE", message);
    }

    void logLifecycle(const juce::String& component, const juce::String& action)
    {
        log("LIFECYCLE", component + ": " + action);
    }

    void logError(const juce::String& location, const juce::String& error)
    {
        log("ERROR", location + ": " + error);
    }

    juce::String getLogFilePath() const { return logFilePath_; }

private:
    DebugLogger()
    {
        // Log to user's Documents folder for easy access
        auto docsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
        auto logFile = docsDir.getChildFile("MultiScoper_debug.log");
        logFilePath_ = logFile.getFullPathName();

        file_.open(logFilePath_.toStdString(), std::ios::out | std::ios::trunc);

        if (file_.is_open())
        {
            log("SYSTEM", "=== MultiScoper Debug Log Started ===");
            log("SYSTEM", "Log file: " + logFilePath_);
            log("SYSTEM", "Time: " + juce::Time::getCurrentTime().toString(true, true, true, true));
        }
    }

    ~DebugLogger()
    {
        if (file_.is_open())
        {
            log("SYSTEM", "=== MultiScoper Debug Log Ended ===");
            file_.close();
        }
    }

    std::ofstream file_;
    std::mutex mutex_;
    juce::String logFilePath_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DebugLogger)
};

// Convenience macros
#define OSCIL_LOG(category, msg) oscil::DebugLogger::getInstance().log(category, msg)
#define OSCIL_LOG_SOURCE(action, id, name, valid, count) oscil::DebugLogger::getInstance().logSourceInfo(action, id, name, valid, count)
#define OSCIL_LOG_BUFFER(loc, id, found, lock, extra) oscil::DebugLogger::getInstance().logBufferAccess(loc, id, found, lock, extra)
#define OSCIL_LOG_RENDER(comp, msg) oscil::DebugLogger::getInstance().logRendering(comp, msg)
#define OSCIL_LOG_CAPTURE(msg) oscil::DebugLogger::getInstance().logCapture(msg)
#define OSCIL_LOG_LIFECYCLE(comp, action) oscil::DebugLogger::getInstance().logLifecycle(comp, action)
#define OSCIL_LOG_ERROR(loc, err) oscil::DebugLogger::getInstance().logError(loc, err)

} // namespace oscil
