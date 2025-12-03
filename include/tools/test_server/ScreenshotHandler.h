/*
    Oscil - Screenshot Handler
    Handles screenshot test server endpoints
*/

#pragma once

#include "TestServerHandlerBase.h"

namespace oscil
{

/**
 * Handles screenshot endpoint:
 * - POST /screenshot - Capture screenshot of editor
 */
class ScreenshotHandler : public TestServerHandlerBase
{
public:
    using TestServerHandlerBase::TestServerHandlerBase;

    void handleTakeScreenshot(const httplib::Request& req, httplib::Response& res);
};

} // namespace oscil
