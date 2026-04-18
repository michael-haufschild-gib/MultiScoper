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

#include "TestDAW.h"
#include "TestMetrics.h"
#include "TestScreenshot.h"
#include "TestUIController.h"

#include <atomic>
#include <functional>
#include <future>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <thread>

namespace oscil
{
class OscilPluginProcessor;
class OscilPluginEditor;
} // namespace oscil

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

    // Route handlers - Pane Management
    void handlePaneAdd(const httplib::Request& req, httplib::Response& res);
    void handlePaneRemove(const httplib::Request& req, httplib::Response& res);
    void handleOscillatorMove(const httplib::Request& req, httplib::Response& res);

    // Route handlers - Layout / Per-pane bounds
    void handleLayoutInfo(const httplib::Request& req, httplib::Response& res);
    void handleSetLayout(const httplib::Request& req, httplib::Response& res);
    void handlePaneLayout(const httplib::Request& req, httplib::Response& res);
    void handlePaneMove(const httplib::Request& req, httplib::Response& res);

    // Route handlers - Waveform State
    void handleWaveformState(const httplib::Request& req, httplib::Response& res);

    // Route handlers - Diagnostic
    void handleDiagnosticSnapshot(const httplib::Request& req, httplib::Response& res);

    // Route handlers - Instance/Multi-track
    void setupInstanceRoutes();
    void handleDawTrackAdd(const httplib::Request& req, httplib::Response& res);
    void handleDawTrackRemove(const httplib::Request& req, httplib::Response& res);
    void handleDawTracks(const httplib::Request& req, httplib::Response& res);

    // Route handlers - DAW host-lifecycle simulation (TestHttpServerDAW.cpp)
    //
    // Each endpoint dispatches its state-touching work onto the JUCE
    // message thread via callAsync + WaitableEvent.  The HTTP worker
    // thread must never call prepareToPlay / setBusesLayout / track
    // mutation directly — those paths assume message-thread affinity.
    void setupDawLifecycleRoutes();
    void handleDawSetSampleRate(const httplib::Request& req, httplib::Response& res);
    void handleDawSetBufferSize(const httplib::Request& req, httplib::Response& res);
    void handleDawSetAudioThreadPrepare(const httplib::Request& req, httplib::Response& res);
    void handleDawScanCycle(const httplib::Request& req, httplib::Response& res);
    void handleTrackChannelLayout(const httplib::Request& req, httplib::Response& res);
    void handleTrackDetachEditor(const httplib::Request& req, httplib::Response& res);
    void handleTrackReattachEditor(const httplib::Request& req, httplib::Response& res);
    void handleDawSetIsolatedRegistries(const httplib::Request& req, httplib::Response& res);
    void handleTrackSources(const httplib::Request& req, httplib::Response& res);

    // Reset helpers
    void resetAudioAndTransport();
    void resetOptionsControls();

    // State restore — applies loaded XML, restores TimingEngine, syncs sidebar UI.
    // Returns false if XML parsing fails.
    bool restoreLoadedState(OscilPluginProcessor& processor, OscilPluginEditor* editor, const juce::String& xml);

    // Track resolver — extracts trackId from GET query param or POST body, defaults to 0.
    //
    // The *Id* variants are the preferred API: they only parse the request and
    // never touch `TestDAW::getTrack`, so the HTTP worker thread cannot observe
    // a dangling `TestTrack*` from a concurrent remove on the message thread.
    // Pair them with `runOnTrackSync` so the actual pointer lookup happens
    // inside the message-thread lambda where add/remove are serialized.
    // Thrown by `resolveTrackId` when the request carries an explicit
    // `trackId` that can't be parsed as an integer. Handlers should not
    // swallow these — use `tryResolveTrackId` to convert them into a 400.
    class BadTrackIdError : public std::invalid_argument
    {
    public:
        using std::invalid_argument::invalid_argument;
    };

    int resolveTrackId(const httplib::Request& req);
    // Preferred entry point in handlers: on parse failure writes a 400
    // response and returns `nullopt`. The handler must early-return in that
    // case. Returns the resolved id (defaulting to 0 when absent) otherwise.
    std::optional<int> tryResolveTrackId(const httplib::Request& req, httplib::Response& res);
    int resolveTrackIdFromBody(const json& body);

    // Legacy pointer-returning resolvers — retained for callers that still run
    // entirely on the message thread (e.g., inside a lambda already dispatched
    // via callAsync). Never call from an HTTP worker thread: concurrent
    // addTrack/removeTrack on the MT can free the returned pointer before the
    // caller dereferences it, causing pthread_mutex_lock(EINVAL) when the
    // caller touches TestTrack::sourceIdMutex_.
    TestTrack* resolveTrack(const httplib::Request& req);
    TestTrack* resolveTrackFromBody(const json& body);

    // Result of a `runOnTrackSync` dispatch.
    enum class TrackCallResult
    {
        Ok,       // track existed and fn executed
        NotFound, // message thread ran but no track at that index
        Timeout   // dispatch never completed within the bound
    };

    // Dispatch `fn(track)` onto the message thread and block the caller (HTTP
    // worker thread) until it completes. Looks up the track inside the lambda
    // so add/remove races from other HTTP workers are ordered behind this
    // dispatch. Safe to call from any thread.
    TrackCallResult runOnTrackSync(int trackId, std::function<void(TestTrack&)> fn, int timeoutMs = 3000);

    // If `r` is NotFound/Timeout, write the matching error response and
    // return true; caller should early-return. Returns false on Ok so the
    // caller proceeds with the success payload. Keeps handler bodies short.
    bool respondIfTrackCallFailed(TrackCallResult r, httplib::Response& res, const char* notFoundMsg,
                                  const char* timeoutMsg);

    // Dispatch `fn` onto the message thread and block up to `timeoutMs`.
    //
    // Returns true if the lambda completed, false on timeout. On timeout
    // the lambda may still fire later — that is safe because `fn` and the
    // WaitableEvent are owned by an internal shared_ptr. Any captures
    // *inside* `fn` must also be heap-owned (shared_ptr<T>) so they
    // survive the caller returning.
    bool runOnMessageThreadBlocking(std::function<void()> fn, int timeoutMs, const char* label);

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
