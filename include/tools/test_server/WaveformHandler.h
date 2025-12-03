/*
    Oscil - Waveform Handler
    Handles waveform-related test server endpoints
*/

#pragma once

#include "TestServerHandlerBase.h"

namespace oscil
{

/**
 * Handles waveform endpoints:
 * - POST /test/inject - Inject test waveform data
 * - GET /waveform/state - Get current waveform state
 */
class WaveformHandler : public TestServerHandlerBase
{
public:
    using TestServerHandlerBase::TestServerHandlerBase;

    void handleInjectTestData(const httplib::Request& req, httplib::Response& res);
    void handleGetWaveformState(const httplib::Request& req, httplib::Response& res);
};

} // namespace oscil
