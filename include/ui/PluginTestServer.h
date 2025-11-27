/*
    Oscil - Plugin Test Server
    HTTP server for automated UI testing
*/

#pragma once

// Ensure OpenSSL and Brotli support are NOT enabled for httplib
// (httplib uses #ifdef to check these, so we must ensure they are not defined)
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
#undef CPPHTTPLIB_OPENSSL_SUPPORT
#endif
#ifdef CPPHTTPLIB_BROTLI_SUPPORT
#undef CPPHTTPLIB_BROTLI_SUPPORT
#endif

#include <httplib.h>

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>
#include <functional>

namespace oscil
{

class OscilPluginEditor;

/**
 * HTTP server for automated testing of the plugin UI.
 * Provides endpoints to query and manipulate the UI state.
 */
class PluginTestServer
{
public:
    explicit PluginTestServer(OscilPluginEditor& editor);
    ~PluginTestServer();

    void start(int port = 9876);
    void stop();
    bool isRunning() const { return running_.load(); }
    int getPort() const { return port_; }

private:
    void setupEndpoints();
    void serverThread();

    // Endpoint handlers
    void handleHealth(const httplib::Request& req, httplib::Response& res);
    void handleGetLayout(const httplib::Request& req, httplib::Response& res);
    void handleSetColumnLayout(const httplib::Request& req, httplib::Response& res);
    void handleGetPaneBounds(const httplib::Request& req, httplib::Response& res);
    void handleAddOscillator(const httplib::Request& req, httplib::Response& res);
    void handleDeleteOscillator(const httplib::Request& req, httplib::Response& res);
    void handleMovePane(const httplib::Request& req, httplib::Response& res);
    void handleGetOscillators(const httplib::Request& req, httplib::Response& res);
    void handleTakeScreenshot(const httplib::Request& req, httplib::Response& res);
    void handleRunLayoutTest(const httplib::Request& req, httplib::Response& res);
    void handleRunDragDropTest(const httplib::Request& req, httplib::Response& res);

    // Waveform and rendering test handlers
    void handleInjectTestData(const httplib::Request& req, httplib::Response& res);
    void handleRunWaveformTest(const httplib::Request& req, httplib::Response& res);
    void handleRunSettingsTest(const httplib::Request& req, httplib::Response& res);
    void handleRunRenderingTest(const httplib::Request& req, httplib::Response& res);
    void handleGetWaveformState(const httplib::Request& req, httplib::Response& res);

    // Source management handlers
    void handleGetSources(const httplib::Request& req, httplib::Response& res);
    void handleAddSource(const httplib::Request& req, httplib::Response& res);
    void handleAssignSource(const httplib::Request& req, httplib::Response& res);
    void handleInjectSourceData(const httplib::Request& req, httplib::Response& res);

    // Oscillator property update handler
    void handleUpdateOscillator(const httplib::Request& req, httplib::Response& res);

    // Oscillator reorder handlers
    void handleReorderOscillator(const httplib::Request& req, httplib::Response& res);
    void handleTestOscillatorReorder(const httplib::Request& req, httplib::Response& res);

    // Helper to run on message thread
    template<typename Func>
    auto runOnMessageThread(Func&& func) -> decltype(func());

    OscilPluginEditor& editor_;
    std::unique_ptr<httplib::Server> server_;
    std::unique_ptr<std::thread> serverThread_;
    std::atomic<bool> running_{false};
    int port_ = 9876;

    // Test source management - stores capture buffers for test sources
    std::unordered_map<std::string, std::shared_ptr<class SharedCaptureBuffer>> testSourceBuffers_;
};

} // namespace oscil
