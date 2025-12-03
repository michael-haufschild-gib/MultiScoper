/*
    Oscil - Source Handler
    Handles source management test server endpoints
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
 * Handles source management endpoints:
 * - GET /sources - Get all available sources
 * - POST /source/add - Add a new test source
 * - POST /source/remove - Remove a test source
 * - POST /oscillator/assign-source - Assign source to oscillator
 * - POST /source/inject - Inject test data into a source
 */
class SourceHandler : public TestServerHandlerBase
{
public:
    SourceHandler(OscilPluginEditor& editor,
                  std::unordered_map<std::string, std::shared_ptr<SharedCaptureBuffer>>& testSourceBuffers)
        : TestServerHandlerBase(editor)
        , testSourceBuffers_(testSourceBuffers)
    {
    }

    void handleGetSources(const httplib::Request& req, httplib::Response& res);
    void handleAddSource(const httplib::Request& req, httplib::Response& res);
    void handleRemoveSource(const httplib::Request& req, httplib::Response& res);
    void handleAssignSource(const httplib::Request& req, httplib::Response& res);
    void handleInjectSourceData(const httplib::Request& req, httplib::Response& res);

private:
    std::unordered_map<std::string, std::shared_ptr<SharedCaptureBuffer>>& testSourceBuffers_;
};

} // namespace oscil
