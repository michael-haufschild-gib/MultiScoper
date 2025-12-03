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
#include <memory>
#include <unordered_map>

namespace oscil
{

class OscilPluginEditor;
class SharedCaptureBuffer;

// Forward declare handlers
class LayoutHandler;
class OscillatorHandler;
class SourceHandler;
class ScreenshotHandler;
class TestRunnerHandler;
class WaveformHandler;
class StateHandler;

/**
 * HTTP server for automated testing of the plugin UI.
 * Provides endpoints to query and manipulate the UI state.
 *
 * Refactored into focused handler classes for better maintainability.
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

    OscilPluginEditor& editor_;
    std::unique_ptr<httplib::Server> server_;
    std::unique_ptr<std::thread> serverThread_;
    std::atomic<bool> running_{false};
    int port_ = 9876;

    // Test source management - stores capture buffers for test sources
    std::unordered_map<std::string, std::shared_ptr<SharedCaptureBuffer>> testSourceBuffers_;

    // Handler instances
    std::unique_ptr<LayoutHandler> layoutHandler_;
    std::unique_ptr<OscillatorHandler> oscillatorHandler_;
    std::unique_ptr<SourceHandler> sourceHandler_;
    std::unique_ptr<ScreenshotHandler> screenshotHandler_;
    std::unique_ptr<TestRunnerHandler> testRunnerHandler_;
    std::unique_ptr<WaveformHandler> waveformHandler_;
    std::unique_ptr<StateHandler> stateHandler_;
};

} // namespace oscil
