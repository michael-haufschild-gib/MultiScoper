/*
    Oscil - Test Server Handler Base Class
    Base class for all test server endpoint handlers
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <httplib.h>
#include <memory>
#include <nlohmann/json.hpp>

namespace oscil
{

class OscilPluginEditor;

/**
 * Base class for test server handlers.
 * Provides common functionality for running operations on the message thread
 * and building JSON responses.
 */
class TestServerHandlerBase
{
public:
    explicit TestServerHandlerBase(OscilPluginEditor& editor) : editor_(editor) {}

    virtual ~TestServerHandlerBase() = default;

protected:
    /**
     * Execute a function on the JUCE message thread and wait for completion.
     * Handles both void and non-void return types.
     * Uses shared_ptr for signal/result so a timeout doesn't cause use-after-free.
     */
    template <typename Func>
    auto runOnMessageThread(Func&& func) -> decltype(func())
    {
        if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        {
            return func();
        }

        using ReturnType = decltype(func());

        if constexpr (std::is_void_v<ReturnType>)
        {
            auto done = std::make_shared<juce::WaitableEvent>();
            juce::MessageManager::callAsync([f = std::forward<Func>(func), done]() mutable {
                f();
                done->signal();
            });
            done->wait(MESSAGE_THREAD_TIMEOUT_MS);
        }
        else
        {
            auto result = std::make_shared<ReturnType>();
            auto done = std::make_shared<juce::WaitableEvent>();
            juce::MessageManager::callAsync([f = std::forward<Func>(func), result, done]() mutable {
                *result = f();
                done->signal();
            });
            if (!done->wait(MESSAGE_THREAD_TIMEOUT_MS))
            {
                ReturnType error;
                error["error"] = "Message thread timeout";
                return error;
            }
            return *result;
        }
    }

    /**
     * Create a success JSON response.
     */
    nlohmann::json jsonSuccess() const
    {
        nlohmann::json response;
        response["status"] = "ok";
        return response;
    }

    /**
     * Create an error JSON response.
     */
    nlohmann::json jsonError(const std::string& message) const
    {
        nlohmann::json response;
        response["error"] = message;
        return response;
    }

    /**
     * Send a JSON response to the client.
     */
    void sendJson(httplib::Response& res, const nlohmann::json& json, int statusCode = 200) const
    {
        res.status = statusCode;
        res.set_content(json.dump(), "application/json");
    }

    /**
     * Summarize test results into a response: totalTests, passed, failed, allPassed.
     */
    static void countTestResults(const nlohmann::json& tests, nlohmann::json& response)
    {
        int passedCount = 0;
        for (const auto& test : tests)
        {
            if (test["passed"].get<bool>())
                passedCount++;
        }
        response["tests"] = tests;
        response["totalTests"] = static_cast<int>(tests.size());
        response["passed"] = passedCount;
        response["failed"] = static_cast<int>(tests.size()) - passedCount;
        response["allPassed"] = (passedCount == static_cast<int>(tests.size()));
    }

    OscilPluginEditor& editor_;

    static constexpr int MESSAGE_THREAD_TIMEOUT_MS = 10000;
};

} // namespace oscil
