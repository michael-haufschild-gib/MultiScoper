/*
    Oscil - Test Runner Handler
    Handles automated test runner endpoints
*/

#pragma once

#include "TestServerHandlerBase.h"

namespace oscil
{

/**
 * Handles test runner endpoints:
 * - POST /test/layout - Run layout tests
 * - POST /test/dragdrop - Run drag/drop tests
 * - POST /test/waveform - Run waveform rendering tests
 * - POST /test/settings - Run settings tests
 * - POST /test/rendering - Run rendering tests
 */
class TestRunnerHandler : public TestServerHandlerBase
{
public:
    using TestServerHandlerBase::TestServerHandlerBase;

    void handleRunLayoutTest(const httplib::Request& req, httplib::Response& res);
    void handleRunDragDropTest(const httplib::Request& req, httplib::Response& res);
    void handleRunWaveformTest(const httplib::Request& req, httplib::Response& res);
    void handleRunSettingsTest(const httplib::Request& req, httplib::Response& res);
    void handleRunRenderingTest(const httplib::Request& req, httplib::Response& res);
};

} // namespace oscil
