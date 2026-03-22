/*
    Oscil - Plugin Test Server Implementation (Refactored)
    HTTP server for automated UI testing - thin router delegating to handlers
*/

#include "tools/test_server/PluginTestServer.h"
#include "plugin/PluginEditor.h"
#include "tools/test_server/LayoutHandler.h"
#include "tools/test_server/OscillatorHandler.h"
#include "tools/test_server/SourceHandler.h"
#include "tools/test_server/ScreenshotHandler.h"
#include "tools/test_server/TestRunnerHandler.h"
#include "tools/test_server/WaveformHandler.h"
#include "tools/test_server/StateHandler.h"

namespace oscil
{

PluginTestServer::PluginTestServer(OscilPluginEditor& editor)
    : editor_(editor)
{
}

PluginTestServer::~PluginTestServer()
{
    stop();
}

void PluginTestServer::start(int port)
{
    if (running_.load())
        return;

    port_ = port;
    server_ = std::make_unique<httplib::Server>();

    // Create handler instances
    layoutHandler_ = std::make_unique<LayoutHandler>(editor_);
    oscillatorHandler_ = std::make_unique<OscillatorHandler>(editor_);
    sourceHandler_ = std::make_unique<SourceHandler>(editor_, testSourceBuffers_);
    screenshotHandler_ = std::make_unique<ScreenshotHandler>(editor_);
    testRunnerHandler_ = std::make_unique<TestRunnerHandler>(editor_);
    waveformHandler_ = std::make_unique<WaveformHandler>(editor_);
    stateHandler_ = std::make_unique<StateHandler>(editor_, testSourceBuffers_, port_);

    setupEndpoints();

    // Set running before launching thread to prevent stop()/destructor race
    // where stop() sees running_==false and skips join on a joinable thread
    running_.store(true);
    serverThread_ = std::make_unique<std::thread>([this]() { serverThread(); });
}

void PluginTestServer::stop()
{
    if (!running_.load())
        return;

    running_.store(false);

    if (server_)
        server_->stop();

    if (serverThread_ && serverThread_->joinable())
        serverThread_->join();

    serverThread_.reset();
    server_.reset();

    // Destroy handlers (server is stopped, no concurrent request processing)
    stateHandler_.reset();
    waveformHandler_.reset();
    testRunnerHandler_.reset();
    screenshotHandler_.reset();
    sourceHandler_.reset();
    oscillatorHandler_.reset();
    layoutHandler_.reset();
}

void PluginTestServer::serverThread()
{
    server_->listen("127.0.0.1", port_);
}

void PluginTestServer::setupLayoutEndpoints()
{
    server_->Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        stateHandler_->handleHealth(req, res);
    });
    server_->Get("/layout", [this](const httplib::Request& req, httplib::Response& res) {
        layoutHandler_->handleGetLayout(req, res);
    });
    server_->Post("/layout", [this](const httplib::Request& req, httplib::Response& res) {
        layoutHandler_->handleSetColumnLayout(req, res);
    });
    server_->Get("/panes", [this](const httplib::Request& req, httplib::Response& res) {
        layoutHandler_->handleGetPaneBounds(req, res);
    });
    server_->Post("/pane/move", [this](const httplib::Request& req, httplib::Response& res) {
        layoutHandler_->handleMovePane(req, res);
    });
}

void PluginTestServer::setupOscillatorEndpoints()
{
    server_->Get("/oscillators", [this](const httplib::Request& req, httplib::Response& res) {
        oscillatorHandler_->handleGetOscillators(req, res);
    });
    server_->Post("/oscillator/add", [this](const httplib::Request& req, httplib::Response& res) {
        oscillatorHandler_->handleAddOscillator(req, res);
    });
    server_->Post("/oscillator/delete", [this](const httplib::Request& req, httplib::Response& res) {
        oscillatorHandler_->handleDeleteOscillator(req, res);
    });
    server_->Post("/oscillator/update", [this](const httplib::Request& req, httplib::Response& res) {
        oscillatorHandler_->handleUpdateOscillator(req, res);
    });
    server_->Post("/oscillator/reorder", [this](const httplib::Request& req, httplib::Response& res) {
        oscillatorHandler_->handleReorderOscillator(req, res);
    });
    server_->Post("/test/oscillator-reorder", [this](const httplib::Request& req, httplib::Response& res) {
        oscillatorHandler_->handleTestOscillatorReorder(req, res);
    });
}

void PluginTestServer::setupTestEndpoints()
{
    server_->Post("/screenshot", [this](const httplib::Request& req, httplib::Response& res) {
        screenshotHandler_->handleTakeScreenshot(req, res);
    });
    server_->Post("/test/layout", [this](const httplib::Request& req, httplib::Response& res) {
        testRunnerHandler_->handleRunLayoutTest(req, res);
    });
    server_->Post("/test/dragdrop", [this](const httplib::Request& req, httplib::Response& res) {
        testRunnerHandler_->handleRunDragDropTest(req, res);
    });
    server_->Post("/test/waveform", [this](const httplib::Request& req, httplib::Response& res) {
        testRunnerHandler_->handleRunWaveformTest(req, res);
    });
    server_->Post("/test/settings", [this](const httplib::Request& req, httplib::Response& res) {
        testRunnerHandler_->handleRunSettingsTest(req, res);
    });
    server_->Post("/test/rendering", [this](const httplib::Request& req, httplib::Response& res) {
        testRunnerHandler_->handleRunRenderingTest(req, res);
    });
}

void PluginTestServer::setupWaveformEndpoints()
{
    server_->Post("/test/inject", [this](const httplib::Request& req, httplib::Response& res) {
        waveformHandler_->handleInjectTestData(req, res);
    });
    server_->Get("/waveform/state", [this](const httplib::Request& req, httplib::Response& res) {
        waveformHandler_->handleGetWaveformState(req, res);
    });
    server_->Post("/state/reset", [this](const httplib::Request& req, httplib::Response& res) {
        stateHandler_->handleStateReset(req, res);
    });
}

void PluginTestServer::setupSourceEndpoints()
{
    server_->Get("/sources", [this](const httplib::Request& req, httplib::Response& res) {
        sourceHandler_->handleGetSources(req, res);
    });
    server_->Post("/source/add", [this](const httplib::Request& req, httplib::Response& res) {
        sourceHandler_->handleAddSource(req, res);
    });
    server_->Post("/source/remove", [this](const httplib::Request& req, httplib::Response& res) {
        sourceHandler_->handleRemoveSource(req, res);
    });
    server_->Post("/oscillator/assign-source", [this](const httplib::Request& req, httplib::Response& res) {
        sourceHandler_->handleAssignSource(req, res);
    });
    server_->Post("/source/inject", [this](const httplib::Request& req, httplib::Response& res) {
        sourceHandler_->handleInjectSourceData(req, res);
    });
}

void PluginTestServer::setupEndpoints()
{
    setupLayoutEndpoints();
    setupOscillatorEndpoints();
    setupTestEndpoints();
    setupWaveformEndpoints();
    setupSourceEndpoints();
}

} // namespace oscil
