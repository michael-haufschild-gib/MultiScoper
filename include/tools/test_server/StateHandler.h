/*
    Oscil - State Handler
    Handles state management test server endpoints
*/

#pragma once

#include "TestServerHandlerBase.h"
#include <memory>
#include <unordered_map>
#include <string>

namespace oscil
{

class SharedCaptureBuffer;

/**
 * Handles state management endpoints:
 * - GET /health - Health check
 * - POST /state/reset - Reset plugin state
 */
class StateHandler : public TestServerHandlerBase
{
public:
    StateHandler(OscilPluginEditor& editor,
                 std::unordered_map<std::string, std::shared_ptr<SharedCaptureBuffer>>& testSourceBuffers,
                 int port)
        : TestServerHandlerBase(editor)
        , testSourceBuffers_(testSourceBuffers)
        , port_(port)
    {
    }

    void handleHealth(const httplib::Request& req, httplib::Response& res);
    void handleStateReset(const httplib::Request& req, httplib::Response& res);

private:
    void clearTestSources();

    std::unordered_map<std::string, std::shared_ptr<SharedCaptureBuffer>>& testSourceBuffers_;
    int port_;
};

} // namespace oscil
