/*
    Oscil - Layout Handler
    Handles layout-related test server endpoints
*/

#pragma once

#include "TestServerHandlerBase.h"

namespace oscil
{

/**
 * Handles layout-related endpoints:
 * - GET /layout - Get current column layout
 * - POST /layout - Set column layout
 * - GET /panes - Get pane bounds
 * - POST /pane/move - Move pane to new position
 */
class LayoutHandler : public TestServerHandlerBase
{
public:
    using TestServerHandlerBase::TestServerHandlerBase;

    void handleGetLayout(const httplib::Request& req, httplib::Response& res);
    void handleSetColumnLayout(const httplib::Request& req, httplib::Response& res);
    void handleGetPaneBounds(const httplib::Request& req, httplib::Response& res);
    void handleMovePane(const httplib::Request& req, httplib::Response& res);
};

} // namespace oscil
