/*
    Oscil - Oscillator Handler
    Handles oscillator-related test server endpoints
*/

#pragma once

#include "TestServerHandlerBase.h"

namespace oscil
{

/**
 * Handles oscillator-related endpoints:
 * - POST /oscillator/add - Add new oscillator
 * - POST /oscillator/delete - Delete oscillator
 * - POST /oscillator/update - Update oscillator properties
 * - GET /oscillators - Get all oscillators
 * - POST /oscillator/reorder - Reorder oscillators
 * - POST /test/oscillator-reorder - Test oscillator reordering
 */
class OscillatorHandler : public TestServerHandlerBase
{
public:
    using TestServerHandlerBase::TestServerHandlerBase;

    void handleAddOscillator(const httplib::Request& req, httplib::Response& res);
    void handleDeleteOscillator(const httplib::Request& req, httplib::Response& res);
    void handleUpdateOscillator(const httplib::Request& req, httplib::Response& res);
    void handleGetOscillators(const httplib::Request& req, httplib::Response& res);
    void handleReorderOscillator(const httplib::Request& req, httplib::Response& res);
    void handleTestOscillatorReorder(const httplib::Request& req, httplib::Response& res);

private:
    nlohmann::json updateOscillatorOnMessageThread(const std::string& idStr, int index,
                                                    int processingMode, int visible);
};

} // namespace oscil
