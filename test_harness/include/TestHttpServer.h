/*
    Oscil Test Harness - HTTP Server
    REST API for remote control of the test harness
*/

#pragma once

// Disable SSL and Brotli support - we only need HTTP for local testing.
// httplib v0.38+ uses #ifdef (existence check), not #if (value check),
// so we must ensure these macros are NOT defined at all.
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
#undef CPPHTTPLIB_OPENSSL_SUPPORT
#endif
#ifdef CPPHTTPLIB_BROTLI_SUPPORT
#undef CPPHTTPLIB_BROTLI_SUPPORT
#endif

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "TestDAW.h"
#include "TestUIController.h"
#include "TestScreenshot.h"
#include "TestMetrics.h"
#include <thread>
#include <atomic>
#include <future>

namespace oscil::test
{

using json = nlohmann::json;

/**
 * HTTP server providing REST API for test harness control.
 * Runs on a separate thread.
 */
class TestHttpServer
{
public:
    static constexpr int DEFAULT_PORT = 8765;

    TestHttpServer(TestDAW& daw);
    ~TestHttpServer();

    /**
     * Start the server on the given port
     */
    bool start(int port = DEFAULT_PORT);

    /**
     * Stop the server
     */
    void stop();

    /**
     * Check if server is running
     */
    bool isRunning() const { return running_.load(); }

    /**
     * Get the port number
     */
    int getPort() const { return port_; }

private:
    void setupRoutes();
    void setupTransportRoutes();
    void setupTrackRoutes();
    void setupUIMouseRoutes();
    void setupUIKeyboardRoutes();
    void setupVerificationRoutes();
    void setupStateRoutes();
    void setupMetricsRoutes();
    void serverThread();

    // Utility functions
    json successResponse(const json& data = {});
    json errorResponse(const std::string& message);

    // Route handlers - Transport
    void handleTransportPlay(const httplib::Request& req, httplib::Response& res);
    void handleTransportStop(const httplib::Request& req, httplib::Response& res);
    void handleTransportSetBpm(const httplib::Request& req, httplib::Response& res);
    void handleTransportSetPosition(const httplib::Request& req, httplib::Response& res);
    void handleTransportState(const httplib::Request& req, httplib::Response& res);

    // Route handlers - Track Audio
    void handleTrackAudio(const httplib::Request& req, httplib::Response& res);
    void handleTrackBurst(const httplib::Request& req, httplib::Response& res);
    void handleTrackInfo(const httplib::Request& req, httplib::Response& res);
    void handleTrackShowEditor(const httplib::Request& req, httplib::Response& res);
    void handleTrackHideEditor(const httplib::Request& req, httplib::Response& res);

    // Route handlers - UI Mouse
    void handleUIClick(const httplib::Request& req, httplib::Response& res);
    void handleUIDoubleClick(const httplib::Request& req, httplib::Response& res);
    void handleUIRightClick(const httplib::Request& req, httplib::Response& res);
    void handleUIHover(const httplib::Request& req, httplib::Response& res);
    void handleUISelect(const httplib::Request& req, httplib::Response& res);
    void handleUIToggle(const httplib::Request& req, httplib::Response& res);
    void handleUISlider(const httplib::Request& req, httplib::Response& res);
    void handleUISliderIncrement(const httplib::Request& req, httplib::Response& res);
    void handleUISliderDecrement(const httplib::Request& req, httplib::Response& res);
    void handleUISliderReset(const httplib::Request& req, httplib::Response& res);
    void handleUIDrag(const httplib::Request& req, httplib::Response& res);
    void handleUIDragOffset(const httplib::Request& req, httplib::Response& res);
    void handleUIScroll(const httplib::Request& req, httplib::Response& res);
    void handleUIState(const httplib::Request& req, httplib::Response& res);
    void handleUIElement(const httplib::Request& req, httplib::Response& res);
    void handleUIElements(const httplib::Request& req, httplib::Response& res);

    // Route handlers - UI Keyboard
    void handleUIKeyPress(const httplib::Request& req, httplib::Response& res);
    void handleUITypeText(const httplib::Request& req, httplib::Response& res);
    void handleUIClearText(const httplib::Request& req, httplib::Response& res);

    // Route handlers - UI Focus
    void handleUIFocus(const httplib::Request& req, httplib::Response& res);
    void handleUIGetFocused(const httplib::Request& req, httplib::Response& res);
    void handleUIFocusNext(const httplib::Request& req, httplib::Response& res);
    void handleUIFocusPrevious(const httplib::Request& req, httplib::Response& res);

    // Route handlers - UI Wait
    void handleUIWaitForElement(const httplib::Request& req, httplib::Response& res);
    void handleUIWaitForVisible(const httplib::Request& req, httplib::Response& res);
    void handleUIWaitForEnabled(const httplib::Request& req, httplib::Response& res);

    // Route handlers - Screenshot & Visual Verification
    void handleScreenshot(const httplib::Request& req, httplib::Response& res);
    void handleScreenshotCompare(const httplib::Request& req, httplib::Response& res);
    void handleBaselineSave(const httplib::Request& req, httplib::Response& res);

    // Route handlers - Verification
    void handleVerifyWaveform(const httplib::Request& req, httplib::Response& res);
    void handleVerifyColor(const httplib::Request& req, httplib::Response& res);
    void handleVerifyBounds(const httplib::Request& req, httplib::Response& res);
    void handleVerifyVisible(const httplib::Request& req, httplib::Response& res);
    void handleAnalyzeWaveform(const httplib::Request& req, httplib::Response& res);

    // Route handlers - State
    void handleStateReset(const httplib::Request& req, httplib::Response& res);
    void handleStateSave(const httplib::Request& req, httplib::Response& res);
    void handleStateLoad(const httplib::Request& req, httplib::Response& res);
    void handleStateOscillators(const httplib::Request& req, httplib::Response& res);
    void handleStateAddOscillator(const httplib::Request& req, httplib::Response& res);
    void handleStateUpdateOscillator(const httplib::Request& req, httplib::Response& res);
    void handleStateReorderOscillators(const httplib::Request& req, httplib::Response& res);
    void handleStatePanes(const httplib::Request& req, httplib::Response& res);
    void handleStateSources(const httplib::Request& req, httplib::Response& res);
    void handleStateDeleteOscillator(const httplib::Request& req, httplib::Response& res);

    // Route handlers - Waveform State
    void handleWaveformState(const httplib::Request& req, httplib::Response& res);

    // Route handlers - Diagnostic
    void handleDiagnosticSnapshot(const httplib::Request& req, httplib::Response& res);

    // Health check
    void handleHealth(const httplib::Request& req, httplib::Response& res);

    // Route handlers - Performance Metrics
    void handleMetricsStart(const httplib::Request& req, httplib::Response& res);
    void handleMetricsStop(const httplib::Request& req, httplib::Response& res);
    void handleMetricsCurrent(const httplib::Request& req, httplib::Response& res);
    void handleMetricsStats(const httplib::Request& req, httplib::Response& res);
    void handleMetricsReset(const httplib::Request& req, httplib::Response& res);
    void handleMetricsRecordFrame(const httplib::Request& req, httplib::Response& res);

    TestDAW& daw_;
    TestUIController uiController_;
    TestScreenshot screenshot_;
    TestMetrics metrics_;

    std::unique_ptr<httplib::Server> server_;
    std::unique_ptr<std::thread> serverThread_;
    std::atomic<bool> running_{false};
    int port_ = DEFAULT_PORT;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestHttpServer)
};

} // namespace oscil::test
