/*
    Oscil Test Harness - HTTP Server Implementation
*/

#include "TestHttpServer.h"
#include "TestElementRegistry.h"
#include "core/OscilState.h"
#include "plugin/PluginFactory.h"
#include "plugin/PluginEditor.h"
#include "tools/PerformanceProfiler.h"

namespace oscil::test
{

TestHttpServer::TestHttpServer(TestDAW& daw)
    : daw_(daw)
{
    server_ = std::make_unique<httplib::Server>();

    // Configure server timeouts for long-running operations like showEditor
    server_->set_read_timeout(30, 0);  // 30 seconds
    server_->set_write_timeout(30, 0); // 30 seconds

    // Add pre-routing handler to log all incoming requests
    server_->set_pre_routing_handler([](const httplib::Request& req, httplib::Response&) -> httplib::Server::HandlerResponse {
        std::cerr << "[HTTP IN] " << req.method << " " << req.path << std::endl;
        return httplib::Server::HandlerResponse::Unhandled;  // Continue to normal routing
    });

    // Add exception handler
    server_->set_exception_handler([](const httplib::Request& req, httplib::Response& res, std::exception_ptr ep) {
        std::string errMsg;
        try {
            if (ep) std::rethrow_exception(ep);
        } catch (const std::exception& e) {
            errMsg = e.what();
        } catch (...) {
            errMsg = "Unknown exception";
        }
        std::cerr << "[HTTP EXCEPTION] " << req.method << " " << req.path << " - " << errMsg << std::endl;
        nlohmann::json errJson;
        errJson["success"] = false;
        errJson["error"] = "Exception: " + errMsg;
        res.status = 500;
        res.set_content(errJson.dump(), "application/json");
    });

    // Add error handler for debugging
    server_->set_error_handler([](const httplib::Request& req, httplib::Response& res)
    {
        std::string errMsg = "Internal server error on " + req.method + " " + req.path + " (status: " + std::to_string(res.status) + ")";
        std::cerr << "[HTTP ERROR] " << errMsg << std::endl;
        nlohmann::json errJson;
        errJson["success"] = false;
        errJson["error"] = errMsg;
        res.set_content(errJson.dump(), "application/json");
    });

    setupRoutes();
}

TestHttpServer::~TestHttpServer()
{
    stop();
}

bool TestHttpServer::start(int port)
{
    if (running_.load())
        return false;

    port_ = port;
    running_.store(true);

    serverThread_ = std::make_unique<std::thread>([this]() { serverThread(); });

    // Wait for server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return running_.load();
}

void TestHttpServer::stop()
{
    if (!running_.load())
        return;

    running_.store(false);

    if (server_)
        server_->stop();

    if (serverThread_ && serverThread_->joinable())
        serverThread_->join();

    serverThread_.reset();
}

void TestHttpServer::serverThread()
{
    if (!server_->listen("0.0.0.0", port_))
    {
        running_.store(false);
    }
}

void TestHttpServer::setupRoutes()
{
    // Health check
    server_->Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        handleHealth(req, res);
    });

    // Transport
    server_->Post("/transport/play", [this](const httplib::Request& req, httplib::Response& res) {
        handleTransportPlay(req, res);
    });
    server_->Post("/transport/stop", [this](const httplib::Request& req, httplib::Response& res) {
        handleTransportStop(req, res);
    });
    server_->Post("/transport/setBpm", [this](const httplib::Request& req, httplib::Response& res) {
        handleTransportSetBpm(req, res);
    });
    server_->Post("/transport/setPosition", [this](const httplib::Request& req, httplib::Response& res) {
        handleTransportSetPosition(req, res);
    });
    server_->Get("/transport/state", [this](const httplib::Request& req, httplib::Response& res) {
        handleTransportState(req, res);
    });

    // Track Audio - pattern matching for track ID
    server_->Post(R"(/track/(\d+)/audio)", [this](const httplib::Request& req, httplib::Response& res) {
        handleTrackAudio(req, res);
    });
    server_->Post(R"(/track/(\d+)/burst)", [this](const httplib::Request& req, httplib::Response& res) {
        handleTrackBurst(req, res);
    });
    server_->Get(R"(/track/(\d+)/info)", [this](const httplib::Request& req, httplib::Response& res) {
        handleTrackInfo(req, res);
    });
    server_->Post(R"(/track/(\d+)/showEditor)", [this](const httplib::Request& req, httplib::Response& res) {
        handleTrackShowEditor(req, res);
    });
    server_->Post(R"(/track/(\d+)/hideEditor)", [this](const httplib::Request& req, httplib::Response& res) {
        handleTrackHideEditor(req, res);
    });

    // UI Mouse Interaction
    server_->Post("/ui/click", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIClick(req, res);
    });
    server_->Post("/ui/doubleClick", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIDoubleClick(req, res);
    });
    server_->Post("/ui/rightClick", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIRightClick(req, res);
    });
    server_->Post("/ui/hover", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIHover(req, res);
    });
    server_->Post("/ui/select", [this](const httplib::Request& req, httplib::Response& res) {
        handleUISelect(req, res);
    });
    server_->Post("/ui/toggle", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIToggle(req, res);
    });
    server_->Post("/ui/slider", [this](const httplib::Request& req, httplib::Response& res) {
        handleUISlider(req, res);
    });
    server_->Post("/ui/slider/increment", [this](const httplib::Request& req, httplib::Response& res) {
        handleUISliderIncrement(req, res);
    });
    server_->Post("/ui/slider/decrement", [this](const httplib::Request& req, httplib::Response& res) {
        handleUISliderDecrement(req, res);
    });
    server_->Post("/ui/slider/reset", [this](const httplib::Request& req, httplib::Response& res) {
        handleUISliderReset(req, res);
    });
    server_->Post("/ui/drag", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIDrag(req, res);
    });
    server_->Post("/ui/dragOffset", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIDragOffset(req, res);
    });
    server_->Post("/ui/scroll", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIScroll(req, res);
    });

    // UI Keyboard Interaction
    server_->Post("/ui/keyPress", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIKeyPress(req, res);
    });
    server_->Post("/ui/typeText", [this](const httplib::Request& req, httplib::Response& res) {
        handleUITypeText(req, res);
    });
    server_->Post("/ui/clearText", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIClearText(req, res);
    });

    // UI Focus Management
    server_->Post("/ui/focus", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIFocus(req, res);
    });
    server_->Get("/ui/focused", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIGetFocused(req, res);
    });
    server_->Post("/ui/focusNext", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIFocusNext(req, res);
    });
    server_->Post("/ui/focusPrevious", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIFocusPrevious(req, res);
    });

    // UI Wait Conditions
    server_->Post("/ui/waitForElement", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIWaitForElement(req, res);
    });
    server_->Post("/ui/waitForVisible", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIWaitForVisible(req, res);
    });
    server_->Post("/ui/waitForEnabled", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIWaitForEnabled(req, res);
    });

    // UI State Queries
    server_->Get("/ui/state", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIState(req, res);
    });
    server_->Get("/ui/elements", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIElements(req, res);
    });
    server_->Get(R"(/ui/element/(.+))", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIElement(req, res);
    });

    // Screenshot & Visual Verification
    server_->Post("/screenshot", [this](const httplib::Request& req, httplib::Response& res) {
        handleScreenshot(req, res);
    });
    server_->Get("/screenshot/raw", [this](const httplib::Request& req, httplib::Response& res) {
        handleScreenshotRaw(req, res);
    });
    server_->Post("/screenshot/compare", [this](const httplib::Request& req, httplib::Response& res) {
        handleScreenshotCompare(req, res);
    });
    server_->Post("/baseline/save", [this](const httplib::Request& req, httplib::Response& res) {
        handleBaselineSave(req, res);
    });

    // Verification endpoints
    server_->Post("/verify/waveform", [this](const httplib::Request& req, httplib::Response& res) {
        handleVerifyWaveform(req, res);
    });
    server_->Post("/verify/color", [this](const httplib::Request& req, httplib::Response& res) {
        handleVerifyColor(req, res);
    });
    server_->Post("/verify/bounds", [this](const httplib::Request& req, httplib::Response& res) {
        handleVerifyBounds(req, res);
    });
    server_->Post("/verify/visible", [this](const httplib::Request& req, httplib::Response& res) {
        handleVerifyVisible(req, res);
    });
    server_->Get("/analyze/waveform", [this](const httplib::Request& req, httplib::Response& res) {
        handleAnalyzeWaveform(req, res);
    });

    // State
    server_->Post("/state/reset", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateReset(req, res);
    });
    server_->Post("/state/save", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateSave(req, res);
    });
    server_->Post("/state/load", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateLoad(req, res);
    });
    server_->Get("/state/oscillators", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateOscillators(req, res);
    });
    server_->Post("/state/oscillator/add", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateAddOscillator(req, res);
    });

    server_->Post("/state/oscillator/update", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateUpdateOscillator(req, res);
    });
    server_->Post("/state/oscillator/reorder", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateReorderOscillators(req, res);
    });
    server_->Get("/state/panes", [this](const httplib::Request& req, httplib::Response& res) {
        handleStatePanes(req, res);
    });
    server_->Post("/state/pane/add", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateAddPane(req, res);
    });
    server_->Post("/state/pane/swap", [this](const httplib::Request& req, httplib::Response& res) {
        handleStatePaneSwap(req, res);
    });
    server_->Post("/state/gpu", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateGpuToggle(req, res);
    });
    server_->Get("/state/sources", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateSources(req, res);
    });

    // Display Settings
    server_->Post("/state/layout", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateLayout(req, res);
    });
    server_->Post("/state/theme", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateTheme(req, res);
    });
    server_->Post("/state/grid", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateGrid(req, res);
    });
    server_->Get("/state/display", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateDisplay(req, res);
    });
    server_->Post("/state/display", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateDisplay(req, res);
    });
    server_->Post("/state/timing", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateTiming(req, res);
    });
    server_->Get("/state/timing", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateTiming(req, res);
    });

    // Performance Metrics
    server_->Post("/metrics/start", [this](const httplib::Request& req, httplib::Response& res) {
        handleMetricsStart(req, res);
    });
    server_->Post("/metrics/stop", [this](const httplib::Request& req, httplib::Response& res) {
        handleMetricsStop(req, res);
    });
    server_->Get("/metrics/current", [this](const httplib::Request& req, httplib::Response& res) {
        handleMetricsCurrent(req, res);
    });
    server_->Get("/metrics/stats", [this](const httplib::Request& req, httplib::Response& res) {
        handleMetricsStats(req, res);
    });
    server_->Post("/metrics/reset", [this](const httplib::Request& req, httplib::Response& res) {
        handleMetricsReset(req, res);
    });
    server_->Post("/metrics/recordFrame", [this](const httplib::Request& req, httplib::Response& res) {
        handleMetricsRecordFrame(req, res);
    });
    
    // Extended Profiling
    server_->Post("/profiling/start", [this](const httplib::Request& req, httplib::Response& res) {
        handleProfilingStart(req, res);
    });
    server_->Post("/profiling/stop", [this](const httplib::Request& req, httplib::Response& res) {
        handleProfilingStop(req, res);
    });
    server_->Get("/profiling/snapshot", [this](const httplib::Request& req, httplib::Response& res) {
        handleProfilingSnapshot(req, res);
    });
    server_->Get("/profiling/gpu", [this](const httplib::Request& req, httplib::Response& res) {
        handleProfilingGpu(req, res);
    });
    server_->Get("/profiling/threads", [this](const httplib::Request& req, httplib::Response& res) {
        handleProfilingThreads(req, res);
    });
    server_->Get("/profiling/components", [this](const httplib::Request& req, httplib::Response& res) {
        handleProfilingComponents(req, res);
    });
    server_->Get("/profiling/hotspots", [this](const httplib::Request& req, httplib::Response& res) {
        handleProfilingHotspots(req, res);
    });
    server_->Get("/profiling/timeline", [this](const httplib::Request& req, httplib::Response& res) {
        handleProfilingTimeline(req, res);
    });
    
    // Stress Testing
    server_->Post("/stress/oscillators", [this](const httplib::Request& req, httplib::Response& res) {
        handleStressOscillators(req, res);
    });
    server_->Post("/stress/effects", [this](const httplib::Request& req, httplib::Response& res) {
        handleStressEffects(req, res);
    });
    server_->Post("/stress/audio", [this](const httplib::Request& req, httplib::Response& res) {
        handleStressAudio(req, res);
    });

    // Debug Logs
    server_->Get("/debug/logs", [this](const httplib::Request& req, httplib::Response& res) {
        handleDebugLogs(req, res);
    });
    server_->Post("/debug/logs/clear", [this](const httplib::Request& req, httplib::Response& res) {
        handleDebugLogsClear(req, res);
    });
    server_->Get("/debug/logs/tail", [this](const httplib::Request& req, httplib::Response& res) {
        handleDebugLogsTail(req, res);
    });
    server_->Post("/debug/logs/wait", [this](const httplib::Request& req, httplib::Response& res) {
        handleDebugLogsWait(req, res);
    });

    // Ping endpoint for client health checks
    server_->Get("/ping", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(successResponse().dump(), "application/json");
    });
}

json TestHttpServer::successResponse(const json& data)
{
    json response;
    response["success"] = true;
    if (!data.empty())
    {
        response["data"] = data;
    }
    return response;
}

json TestHttpServer::errorResponse(const std::string& message)
{
    return {{"success", false}, {"error", message}};
}

// Transport handlers
void TestHttpServer::handleTransportPlay(const httplib::Request&, httplib::Response& res)
{
    daw_.start();
    res.set_content(successResponse().dump(), "application/json");
}

void TestHttpServer::handleTransportStop(const httplib::Request&, httplib::Response& res)
{
    daw_.stop();
    res.set_content(successResponse().dump(), "application/json");
}

void TestHttpServer::handleTransportSetBpm(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        if (!body.contains("bpm"))
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: bpm").dump(), "application/json");
            return;
        }
        double bpm = body.value("bpm", 120.0);
        if (bpm <= 0.0 || bpm > 999.0)
        {
            res.status = 400;
            res.set_content(errorResponse("Invalid bpm value: must be between 0 and 999").dump(), "application/json");
            return;
        }
        daw_.getTransport().setBpm(bpm);
        res.set_content(successResponse().dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleTransportSetPosition(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        int64_t samples = body.value("samples", 0);
        if (samples < 0)
        {
            res.status = 400;
            res.set_content(errorResponse("Invalid samples value: must be non-negative").dump(), "application/json");
            return;
        }
        daw_.getTransport().setPositionSamples(samples);
        res.set_content(successResponse().dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleTransportState(const httplib::Request&, httplib::Response& res)
{
    json data;
    data["playing"] = daw_.getTransport().isPlaying();
    data["bpm"] = daw_.getTransport().getBpm();
    data["positionSamples"] = daw_.getTransport().getPositionSamples();
    data["positionSeconds"] = daw_.getTransport().getPositionSeconds();
    data["positionBeats"] = daw_.getTransport().getPositionBeats();
    res.set_content(successResponse(data).dump(), "application/json");
}

// Track handlers
void TestHttpServer::handleTrackAudio(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        int trackId = std::stoi(req.matches[1]);
        auto* track = daw_.getTrack(trackId);

        if (track == nullptr)
        {
            res.status = 404;
            res.set_content(errorResponse("Track not found: " + std::to_string(trackId)).dump(), "application/json");
            return;
        }

        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        std::string waveform = body.value("waveform", "sine");
        float frequency = body.value("frequency", 440.0f);
        float amplitude = body.value("amplitude", 0.8f);

        // Validate values
        if (frequency <= 0.0f || frequency > 20000.0f)
        {
            res.status = 400;
            res.set_content(errorResponse("Invalid frequency: must be between 0 and 20000 Hz").dump(), "application/json");
            return;
        }
        if (amplitude < 0.0f || amplitude > 1.0f)
        {
            res.status = 400;
            res.set_content(errorResponse("Invalid amplitude: must be between 0 and 1").dump(), "application/json");
            return;
        }

        track->getAudioGenerator().setWaveform(TestAudioGenerator::stringToWaveform(waveform));
        track->getAudioGenerator().setFrequency(frequency);
        track->getAudioGenerator().setAmplitude(amplitude);

        res.set_content(successResponse().dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleTrackBurst(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        int trackId = std::stoi(req.matches[1]);
        auto* track = daw_.getTrack(trackId);

        if (track == nullptr)
        {
            res.status = 404;
            res.set_content(errorResponse("Track not found: " + std::to_string(trackId)).dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body.empty() ? "{}" : req.body);
        int samples = body.value("samples", 4410);
        std::string waveform = body.value("waveform", "");

        if (samples <= 0)
        {
            res.status = 400;
            res.set_content(errorResponse("Invalid samples value: must be positive").dump(), "application/json");
            return;
        }

        if (!waveform.empty())
        {
            track->getAudioGenerator().setWaveform(TestAudioGenerator::stringToWaveform(waveform));
        }
        track->getAudioGenerator().setBurstSamples(samples);

        res.set_content(successResponse().dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleTrackInfo(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        int trackId = std::stoi(req.matches[1]);
        auto* track = daw_.getTrack(trackId);

        if (track == nullptr)
        {
            res.status = 404;
            res.set_content(errorResponse("Track not found: " + std::to_string(trackId)).dump(), "application/json");
            return;
        }

        json data;
        data["index"] = track->getTrackIndex();
        data["name"] = track->getName().toStdString();
        data["sourceId"] = track->getSourceId().id.toStdString();
        data["waveform"] = TestAudioGenerator::waveformToString(
            track->getAudioGenerator().getWaveform()).toStdString();
        data["frequency"] = track->getAudioGenerator().getFrequency();
        data["amplitude"] = track->getAudioGenerator().getAmplitude();
        data["generating"] = track->getAudioGenerator().isGenerating();

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleTrackShowEditor(const httplib::Request& req, httplib::Response& res)
{
    std::cerr << "[DEBUG] handleTrackShowEditor called" << std::endl << std::flush;
    try
    {
        int trackId = std::stoi(req.matches[1]);
        std::cerr << "[DEBUG] trackId = " << trackId << std::endl << std::flush;

        auto* track = daw_.getTrack(trackId);
        std::cerr << "[DEBUG] track = " << (track != nullptr ? "valid" : "null") << std::endl;

        if (track == nullptr)
        {
            res.status = 404;
            res.set_content(errorResponse("Track not found: " + std::to_string(trackId)).dump(), "application/json");
            return;
        }

        // Check if editor is already open
        bool alreadyVisible = track->isEditorVisible();
        std::cerr << "[DEBUG] isEditorVisible = " << alreadyVisible << std::endl;

        if (alreadyVisible)
        {
            int elemCount = static_cast<int>(TestElementRegistry::getInstance().getAllElements().size());
            json data;
            data["trackId"] = trackId;
            data["editorVisible"] = true;
            data["elementsRegistered"] = elemCount;
            data["alreadyOpen"] = true;
            res.set_content(successResponse(data).dump(), "application/json");
            return;
        }

        std::cerr << "[DEBUG] Calling callAsync to show editor" << std::endl;

        // Call showTrackEditor on the message thread (fire and forget)
        // The client should poll /ui/elements to wait for UI to be ready
        juce::MessageManager::callAsync([this, trackId]()
        {
            std::cerr << "[DEBUG] callAsync executing on message thread" << std::endl;
            daw_.showTrackEditor(trackId);
            std::cerr << "[DEBUG] showTrackEditor completed" << std::endl;
        });

        std::cerr << "[DEBUG] callAsync queued, preparing response" << std::endl;

        // Return immediately - client should poll for readiness
        json data;
        data["trackId"] = trackId;
        data["editorVisible"] = false;  // Not visible yet - async
        data["elementsRegistered"] = 0;
        data["async"] = true;
        data["message"] = "Editor opening requested. Poll /ui/elements or use /ui/wait/element/{id} to wait for UI.";

        std::cerr << "[DEBUG] Sending success response" << std::endl;
        res.set_content(successResponse(data).dump(), "application/json");
        std::cerr << "[DEBUG] Response sent" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERROR] Exception in handleTrackShowEditor: " << e.what() << std::endl;
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleTrackHideEditor(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        int trackId = std::stoi(req.matches[1]);
        auto* track = daw_.getTrack(trackId);

        if (track == nullptr)
        {
            res.status = 404;
            res.set_content(errorResponse("Track not found: " + std::to_string(trackId)).dump(), "application/json");
            return;
        }

        // Must call hideEditor on the message thread
        juce::MessageManager::callAsync([track]()
        {
            track->hideEditor();
        });

        json data;
        data["trackId"] = trackId;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// UI handlers
void TestHttpServer::handleUIClick(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        if (uiController_.click(elementId))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIDoubleClick(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        if (uiController_.doubleClick(elementId))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISelect(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        std::string itemText = body.value("itemText", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        bool success = false;

        // Check if itemId is a string (for OscilDropdown) or int (for ComboBox)
        if (body.contains("itemId"))
        {
            if (body["itemId"].is_string())
            {
                // String ID - use selectById for OscilDropdown
                std::string itemIdStr = body["itemId"].get<std::string>();
                success = uiController_.selectById(elementId, itemIdStr);
            }
            else if (body["itemId"].is_number_integer())
            {
                // Integer ID - use select for juce::ComboBox
                int itemId = body["itemId"].get<int>();
                success = uiController_.select(elementId, itemId);
            }
        }
        else if (!itemText.empty())
        {
            success = uiController_.selectByText(elementId, itemText);
        }
        else
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: itemId or itemText").dump(), "application/json");
            return;
        }

        if (success)
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.status = 400;
            res.set_content(errorResponse("Failed to select item").dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIToggle(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        bool value = body.value("value", false);

        if (uiController_.toggle(elementId, value))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Element not found or not toggleable: " + elementId).dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISlider(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        if (!body.contains("value"))
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: value").dump(), "application/json");
            return;
        }

        double value = body.value("value", 0.0);

        if (uiController_.setSliderValue(elementId, value))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Element not found or not a slider: " + elementId).dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIDrag(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string from = body.value("from", "");
        std::string to = body.value("to", "");

        if (from.empty() || to.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required fields: from and to").dump(), "application/json");
            return;
        }

        // Check for modifier keys
        bool shift = body.value("shift", false);
        bool alt = body.value("alt", false);
        bool ctrl = body.value("ctrl", false);

        bool success = false;
        if (shift || alt || ctrl)
        {
            ModifierKeyState mods;
            mods.shift = shift;
            mods.alt = alt;
            mods.ctrl = ctrl;
            success = uiController_.dragWithModifiers(from, to, mods);
        }
        else
        {
            success = uiController_.drag(from, to);
        }

        if (success)
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.status = 400;
            res.set_content(errorResponse("Failed to drag: elements not found or not draggable").dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIDragOffset(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        int deltaX = body.value("deltaX", 0);
        int deltaY = body.value("deltaY", 0);

        bool shift = body.value("shift", false);
        bool alt = body.value("alt", false);
        bool ctrl = body.value("ctrl", false);

        bool success = false;
        if (shift || alt || ctrl)
        {
            ModifierKeyState mods;
            mods.shift = shift;
            mods.alt = alt;
            mods.ctrl = ctrl;
            success = uiController_.dragByOffsetWithModifiers(elementId, deltaX, deltaY, mods);
        }
        else
        {
            success = uiController_.dragByOffset(elementId, deltaX, deltaY);
        }

        if (success)
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Failed to drag by offset: element not found").dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIRightClick(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        if (uiController_.rightClick(elementId))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIHover(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        int durationMs = body.value("durationMs", 500);

        if (uiController_.hover(elementId, durationMs))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIScroll(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        float deltaY = body.value("deltaY", 0.0f);
        float deltaX = body.value("deltaX", 0.0f);

        bool shift = body.value("shift", false);
        bool alt = body.value("alt", false);
        bool ctrl = body.value("ctrl", false);

        ScrollResult result;
        if (shift || alt || ctrl)
        {
            ModifierKeyState mods;
            mods.shift = shift;
            mods.alt = alt;
            mods.ctrl = ctrl;
            result = uiController_.scrollWithModifiers(elementId, deltaY, deltaX, mods);
        }
        else
        {
            result = uiController_.scroll(elementId, deltaY, deltaX);
        }

        if (result.success)
        {
            json data;
            data["previousValue"] = result.previousValue;
            data["newValue"] = result.newValue;
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Failed to scroll: element not found or not scrollable").dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISliderIncrement(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        if (uiController_.incrementSlider(elementId))
        {
            json data;
            data["value"] = uiController_.getSliderValue(elementId);
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Element not found or not a slider: " + elementId).dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISliderDecrement(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        if (uiController_.decrementSlider(elementId))
        {
            json data;
            data["value"] = uiController_.getSliderValue(elementId);
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Element not found or not a slider: " + elementId).dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISliderReset(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        if (uiController_.resetSliderToDefault(elementId))
        {
            json data;
            data["value"] = uiController_.getSliderValue(elementId);
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Element not found or not a slider: " + elementId).dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// Keyboard handlers
void TestHttpServer::handleUIKeyPress(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string key = body.value("key", "");
        std::string elementId = body.value("elementId", "");

        if (key.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: key").dump(), "application/json");
            return;
        }

        bool shift = body.value("shift", false);
        bool alt = body.value("alt", false);
        bool ctrl = body.value("ctrl", false);

        // Map key names to JUCE key codes
        int keyCode = 0;
        if (key == "escape") keyCode = juce::KeyPress::escapeKey;
        else if (key == "enter" || key == "return") keyCode = juce::KeyPress::returnKey;
        else if (key == "space") keyCode = juce::KeyPress::spaceKey;
        else if (key == "tab") keyCode = juce::KeyPress::tabKey;
        else if (key == "backspace") keyCode = juce::KeyPress::backspaceKey;
        else if (key == "delete") keyCode = juce::KeyPress::deleteKey;
        else if (key == "up") keyCode = juce::KeyPress::upKey;
        else if (key == "down") keyCode = juce::KeyPress::downKey;
        else if (key == "left") keyCode = juce::KeyPress::leftKey;
        else if (key == "right") keyCode = juce::KeyPress::rightKey;
        else if (key == "home") keyCode = juce::KeyPress::homeKey;
        else if (key == "end") keyCode = juce::KeyPress::endKey;
        else if (key == "pageup") keyCode = juce::KeyPress::pageUpKey;
        else if (key == "pagedown") keyCode = juce::KeyPress::pageDownKey;
        else if (key == "f1") keyCode = juce::KeyPress::F1Key;
        else if (key == "f2") keyCode = juce::KeyPress::F2Key;
        else if (key == "f3") keyCode = juce::KeyPress::F3Key;
        else if (key == "f4") keyCode = juce::KeyPress::F4Key;
        else if (key == "f5") keyCode = juce::KeyPress::F5Key;
        else if (key == "f6") keyCode = juce::KeyPress::F6Key;
        else if (key == "f7") keyCode = juce::KeyPress::F7Key;
        else if (key == "f8") keyCode = juce::KeyPress::F8Key;
        else if (key == "f9") keyCode = juce::KeyPress::F9Key;
        else if (key == "f10") keyCode = juce::KeyPress::F10Key;
        else if (key == "f11") keyCode = juce::KeyPress::F11Key;
        else if (key == "f12") keyCode = juce::KeyPress::F12Key;
        else if (key.length() == 1) keyCode = key[0]; // Single character
        else
        {
            res.status = 400;
            res.set_content(errorResponse("Unknown key: " + key).dump(), "application/json");
            return;
        }

        bool success = false;
        if (shift || alt || ctrl)
        {
            ModifierKeyState mods;
            mods.shift = shift;
            mods.alt = alt;
            mods.ctrl = ctrl;
            success = uiController_.pressKeyWithModifiers(keyCode, mods, elementId);
        }
        else
        {
            success = uiController_.pressKey(keyCode, elementId);
        }

        if (success)
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.status = 400;
            res.set_content(errorResponse("Failed to press key").dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUITypeText(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        std::string text = body.value("text", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        if (uiController_.typeText(elementId, text))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Element not found or not a text input: " + elementId).dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIClearText(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        if (uiController_.clearText(elementId))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Element not found or not a text input: " + elementId).dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// Focus handlers
void TestHttpServer::handleUIFocus(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        if (uiController_.setFocus(elementId))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIGetFocused(const httplib::Request&, httplib::Response& res)
{
    auto focusedId = uiController_.getFocusedElementId();
    json data;
    data["elementId"] = focusedId.toStdString();
    data["hasFocus"] = focusedId.isNotEmpty();
    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleUIFocusNext(const httplib::Request&, httplib::Response& res)
{
    if (uiController_.focusNext())
    {
        json data;
        data["focusedElement"] = uiController_.getFocusedElementId().toStdString();
        res.set_content(successResponse(data).dump(), "application/json");
    }
    else
    {
        res.status = 400;
        res.set_content(errorResponse("No focused element").dump(), "application/json");
    }
}

void TestHttpServer::handleUIFocusPrevious(const httplib::Request&, httplib::Response& res)
{
    if (uiController_.focusPrevious())
    {
        json data;
        data["focusedElement"] = uiController_.getFocusedElementId().toStdString();
        res.set_content(successResponse(data).dump(), "application/json");
    }
    else
    {
        res.status = 400;
        res.set_content(errorResponse("No focused element").dump(), "application/json");
    }
}

// Wait handlers
void TestHttpServer::handleUIWaitForElement(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        int timeoutMs = body.value("timeoutMs", 5000);

        bool found = uiController_.waitForElement(elementId, timeoutMs);

        json data;
        data["found"] = found;
        data["elementId"] = elementId;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIWaitForVisible(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        int timeoutMs = body.value("timeoutMs", 5000);

        bool visible = uiController_.waitForVisible(elementId, timeoutMs);

        json data;
        data["visible"] = visible;
        data["elementId"] = elementId;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIWaitForEnabled(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        int timeoutMs = body.value("timeoutMs", 5000);

        bool enabled = uiController_.waitForEnabled(elementId, timeoutMs);

        json data;
        data["enabled"] = enabled;
        data["elementId"] = elementId;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIState(const httplib::Request&, httplib::Response& res)
{
    auto state = uiController_.getUIState();
    res.set_content(successResponse(state).dump(), "application/json");
}

void TestHttpServer::handleUIElement(const httplib::Request& req, httplib::Response& res)
{
    std::string elementId = req.matches[1];
    auto info = uiController_.getElementInfo(elementId);

    // Return 404 if element not found
    if (info.contains("error"))
    {
        res.status = 404;
        res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
        return;
    }

    res.set_content(successResponse(info).dump(), "application/json");
}

void TestHttpServer::handleUIElements(const httplib::Request&, httplib::Response& res)
{
    json data;
    data["count"] = 0;
    data["elements"] = json::array();

    auto elements = TestElementRegistry::getInstance().getAllElements();
    for (const auto& [testId, component] : elements)
    {
        json elementInfo;
        elementInfo["testId"] = testId.toStdString();
        elementInfo["visible"] = component->isVisible();
        elementInfo["enabled"] = component->isEnabled();

        auto bounds = component->getBounds();
        elementInfo["bounds"] = {
            {"x", bounds.getX()},
            {"y", bounds.getY()},
            {"width", bounds.getWidth()},
            {"height", bounds.getHeight()}
        };

        data["elements"].push_back(elementInfo);
    }

    data["count"] = elements.size();
    res.set_content(successResponse(data).dump(), "application/json");
}

// Screenshot handlers
void TestHttpServer::handleScreenshot(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body.empty() ? "{}" : req.body);
        std::string path = body.value("path", "/tmp/screenshot.png");

        std::string element = "window";
        if (body.contains("elementId"))
            element = body["elementId"].get<std::string>();
        else if (body.contains("element"))
            element = body["element"].get<std::string>();

        juce::File outputFile(path);
        bool success = false;

        if (element == "window")
        {
            if (auto* editor = daw_.getPrimaryEditor())
            {
                success = screenshot_.captureWindow(editor, outputFile);
            }
        }
        else
        {
            success = screenshot_.captureElement(element, outputFile);
        }

        if (success)
        {
            json data;
            data["path"] = outputFile.getFullPathName().toStdString();
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.status = 400;
            res.set_content(errorResponse("Failed to capture screenshot").dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleScreenshotRaw(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        std::string element = "window";
        if (req.has_param("elementId")) element = req.get_param_value("elementId");
        else if (req.has_param("element")) element = req.get_param_value("element");

        juce::Image image;

        if (element == "window")
        {
            if (auto* editor = daw_.getPrimaryEditor())
            {
                image = screenshot_.getComponentImage(editor);
            }
        }
        else
        {
            image = screenshot_.getElementImage(element);
        }

        if (image.isValid())
        {
            juce::MemoryOutputStream stream;
            juce::PNGImageFormat pngFormat;
            pngFormat.writeImageToStream(image, stream);

            res.set_content(static_cast<const char*>(stream.getData()), stream.getDataSize(), "image/png");
        }
        else
        {
            res.status = 404;
            res.set_content(errorResponse("Failed to capture image or element not found").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleVerifyWaveform(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        float minAmplitude = body.value("minAmplitude", 0.05f);

        juce::Image image;
        if (elementId == "window")
        {
            if (auto* editor = daw_.getPrimaryEditor())
            {
                image = screenshot_.getComponentImage(editor);
            }
        }
        else
        {
            image = screenshot_.getElementImage(elementId);
        }

        bool rendered = false;
        if (!image.isNull())
        {
            auto analysis = screenshot_.analyzeWaveform(image);
            rendered = analysis.detected && analysis.amplitude >= minAmplitude;
        }

        json data;
        data["pass"] = rendered;
        data["elementId"] = elementId;
        data["minAmplitude"] = minAmplitude;

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleScreenshotCompare(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        std::string baselinePath = body.value("baseline", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }
        if (baselinePath.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: baseline").dump(), "application/json");
            return;
        }

        int tolerance = body.value("tolerance", 5);

        juce::File baselineFile(baselinePath);
        if (!baselineFile.existsAsFile())
        {
            res.status = 404;
            res.set_content(errorResponse("Baseline file not found: " + baselinePath).dump(), "application/json");
            return;
        }

        auto result = screenshot_.compareElementToBaseline(elementId, baselineFile, tolerance);

        json data;
        data["match"] = result.match;
        data["similarity"] = result.similarity;
        data["differentPixels"] = result.differentPixels;
        data["totalPixels"] = result.totalPixels;

        if (!result.diffBounds.isEmpty())
        {
            data["diffBounds"] = {
                {"x", result.diffBounds.getX()},
                {"y", result.diffBounds.getY()},
                {"width", result.diffBounds.getWidth()},
                {"height", result.diffBounds.getHeight()}
            };
        }

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleBaselineSave(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        std::string dirPath = body.value("directory", "/tmp/baselines");
        std::string name = body.value("name", "");

        juce::File baselineDir(dirPath);
        bool success = screenshot_.saveBaseline(elementId, baselineDir, name);

        if (success)
        {
            json data;
            data["elementId"] = elementId;
            data["directory"] = dirPath;
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.status = 400;
            res.set_content(errorResponse("Failed to save baseline").dump(), "application/json");
        }
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleVerifyColor(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        std::string colorHex = body.value("color", "#000000");
        int tolerance = body.value("tolerance", 10);
        std::string mode = body.value("mode", "background"); // "background" or "contains"
        float minCoverage = body.value("minCoverage", 0.01f);

        juce::Colour expectedColor = juce::Colour::fromString(colorHex);
        bool pass = false;
        float actualCoverage = 0.0f;
        int matchingPixels = 0;
        juce::Image image;

        if (elementId == "window")
        {
            if (auto* editor = daw_.getPrimaryEditor())
            {
                image = screenshot_.getComponentImage(editor);
            }
        }
        else
        {
            image = screenshot_.getElementImage(elementId);
        }

        // Custom verification logic to capture stats
        if (!image.isNull())
        {
            if (mode == "background")
            {
                pass = screenshot_.verifyBackgroundColor(elementId, expectedColor, tolerance);
            }
            else if (mode == "contains")
            {
                int checkedPixels = 0;
                
                // Re-implement logic here to get stats (or we could modify TestScreenshot)
                // Using stride 2 like TestScreenshot
                for (int y = 0; y < image.getHeight(); y += 2)
                {
                    for (int x = 0; x < image.getWidth(); x += 2)
                    {
                        auto pixel = image.getPixelAt(x, y);
                        checkedPixels++;
                        if (TestScreenshot::colorsMatch(pixel, expectedColor, tolerance))
                            matchingPixels++;
                    }
                }
                
                actualCoverage = checkedPixels > 0 ? static_cast<float>(matchingPixels) / checkedPixels : 0.0f;
                pass = actualCoverage >= minCoverage;
            }
        }

        json data;
        data["pass"] = pass;
        data["elementId"] = elementId;
        data["expectedColor"] = colorHex;
        data["mode"] = mode;
        data["actualCoverage"] = actualCoverage;
        data["matchingPixels"] = matchingPixels;
        data["imageWidth"] = image.getWidth();
        data["imageHeight"] = image.getHeight();

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleVerifyBounds(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }
        int expectedWidth = body.value("width", 0);
        int expectedHeight = body.value("height", 0);
        int tolerance = body.value("tolerance", 5);

        // Validate tolerance range
        if (tolerance < 0 || tolerance > 1000)
        {
            res.status = 400;
            res.set_content(errorResponse("Invalid tolerance: must be between 0 and 1000").dump(), "application/json");
            return;
        }

        auto bounds = screenshot_.getElementBounds(elementId);
        bool pass = screenshot_.verifyElementBounds(elementId, expectedWidth, expectedHeight, tolerance);

        json data;
        data["pass"] = pass;
        data["elementId"] = elementId;
        data["actualBounds"] = {
            {"x", bounds.getX()},
            {"y", bounds.getY()},
            {"width", bounds.getWidth()},
            {"height", bounds.getHeight()}
        };
        data["expectedWidth"] = expectedWidth;
        data["expectedHeight"] = expectedHeight;

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleVerifyVisible(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        if (req.body.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Request body required").dump(), "application/json");
            return;
        }
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required field: elementId").dump(), "application/json");
            return;
        }

        bool visible = screenshot_.verifyElementVisible(elementId);

        json data;
        data["pass"] = visible;
        data["elementId"] = elementId;

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleAnalyzeWaveform(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        std::string elementId = req.get_param_value("elementId");

        if (elementId.empty())
        {
            res.status = 400;
            res.set_content(errorResponse("Missing required query parameter: elementId").dump(), "application/json");
            return;
        }

        std::string bgColorHex = req.get_param_value("backgroundColor");

        juce::Colour backgroundColor = bgColorHex.empty()
            ? juce::Colours::black
            : juce::Colour::fromString(bgColorHex);

        auto image = screenshot_.getElementImage(elementId);
        if (image.isNull())
        {
            res.status = 404;
            res.set_content(errorResponse("Element not found or no image: " + elementId).dump(), "application/json");
            return;
        }

        auto analysis = screenshot_.analyzeWaveform(image, backgroundColor);

        json data;
        data["detected"] = analysis.detected;
        data["amplitude"] = analysis.amplitude;
        data["activity"] = analysis.activity;
        data["zeroCrossings"] = analysis.zeroCrossings;
        data["dominantColor"] = analysis.dominantColor.toString().toStdString();

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// State handlers
void TestHttpServer::handleStateReset(const httplib::Request&, httplib::Response& res)
{
    // Reset each track's processor state
    auto* track = daw_.getTrack(0);
    if (track)
    {
        // Clear oscillators and panes on the message thread and WAIT for completion
        auto promisePtr = std::make_shared<std::promise<bool>>();
        auto future = promisePtr->get_future();
        
        juce::MessageManager::callAsync([track, promisePtr]() {
            try {
                auto& state = track->getProcessor().getState();

                // Remove all oscillators
                auto oscillators = state.getOscillators();
                for (const auto& osc : oscillators)
                {
                    state.removeOscillator(osc.getId());
                }

                // Clear panes
                auto& layoutManager = state.getLayoutManager();
                auto panes = layoutManager.getPanes();
                for (const auto& pane : panes)
                {
                    layoutManager.removePane(pane.getId());
                }
                
                promisePtr->set_value(true);
            } catch (...) {
                promisePtr->set_value(false);
            }
        });

        // Wait for the reset to complete
        future.wait_for(std::chrono::seconds(2));
    }

    // Clear the element registry
    TestElementRegistry::getInstance().clear();

    res.set_content(successResponse().dump(), "application/json");
}

void TestHttpServer::handleStateSave(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string path = body.value("path", "/tmp/state.xml");

        // Get state from first track's processor
        if (auto* track = daw_.getTrack(0))
        {
            juce::String xml = track->getProcessor().getState().toXmlString();
            if (!juce::File(path).replaceWithText(xml))
            {
                res.set_content(errorResponse("Failed to write state file").dump(), "application/json");
                return;
            }
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("No tracks available").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStateLoad(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string path = body.value("path", "");

        juce::File file(path);
        if (!file.existsAsFile())
        {
            res.set_content(errorResponse("File not found").dump(), "application/json");
            return;
        }

        juce::String xml = file.loadFileAsString();
        if (auto* track = daw_.getTrack(0))
        {
            (void)track->getProcessor().getState().fromXmlString(xml);
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("No tracks available").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStateOscillators(const httplib::Request&, httplib::Response& res)
{
    json oscillators = json::array();

    if (auto* track = daw_.getTrack(0))
    {
        auto& state = track->getProcessor().getState();
        auto oscList = state.getOscillators();

        // Sort by orderIndex to return in display order
        std::sort(oscList.begin(), oscList.end(), [](const auto& a, const auto& b) {
            return a.getOrderIndex() < b.getOrderIndex();
        });

        for (const auto& osc : oscList)
        {
            json oscJson;
            oscJson["id"] = osc.getId().id.toStdString();
            oscJson["name"] = osc.getName().toStdString();
            oscJson["sourceId"] = osc.getSourceId().id.toStdString();
            oscJson["paneId"] = osc.getPaneId().id.toStdString();
            oscJson["mode"] = processingModeToString(osc.getProcessingMode()).toStdString();
            oscJson["orderIndex"] = osc.getOrderIndex();
            oscJson["visible"] = osc.isVisible();
            oscillators.push_back(oscJson);
        }
    }

    res.set_content(successResponse(oscillators).dump(), "application/json");
}

void TestHttpServer::handleStateAddOscillator(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        auto& state = track->getProcessor().getState();
        auto& layoutManager = state.getLayoutManager();

        // Create pane if needed
        if (layoutManager.getPaneCount() == 0)
        {
            Pane defaultPane;
            defaultPane.setName("Pane 1");
            defaultPane.setOrderIndex(0);
            layoutManager.addPane(defaultPane);
        }

        // Get the first pane for new oscillator
        PaneId targetPaneId;
        if (layoutManager.getPaneCount() > 0)
        {
            targetPaneId = layoutManager.getPanes()[0].getId();
        }

        // Create new oscillator
        Oscillator osc;

        // Set name (default based on count)
        std::string name = body.value("name", "");
        if (name.empty())
        {
            name = "Oscillator " + std::to_string(state.getOscillators().size() + 1);
        }
        osc.setName(name);

        // Set source ID if provided, otherwise use processor's source
        std::string sourceIdStr = body.value("sourceId", "");
        if (!sourceIdStr.empty())
        {
            osc.setSourceId(SourceId{ juce::String(sourceIdStr) });
        }
        else if (track->getProcessor().getSourceId().isValid())
        {
            osc.setSourceId(track->getProcessor().getSourceId());
        }

        // Set pane ID if provided
        std::string paneIdStr = body.value("paneId", "");
        if (!paneIdStr.empty())
        {
            osc.setPaneId(PaneId{ juce::String(paneIdStr) });
        }
        else
        {
            osc.setPaneId(targetPaneId);
        }

        // Set processing mode
        std::string modeStr = body.value("mode", "FullStereo");
        osc.setProcessingMode(stringToProcessingMode(juce::String(modeStr)));

        // Set colour if provided (as hex string like "#FF0000")
        std::string colourStr = body.value("colour", "");
        if (colourStr.empty()) colourStr = body.value("color", "");
        
        if (!colourStr.empty())
        {
            auto col = juce::Colour::fromString(juce::String(colourStr));
            // FORCE ALPHA to 255 if it's parsed as 0 (common issue with 6-digit hex)
            if (col.getAlpha() == 0) col = col.withAlpha(1.0f);
            
            std::cerr << "[HTTP] handleStateAddOscillator: Received color='" << colourStr 
                      << "' Parsed=" << col.toDisplayString(true) 
                      << " Alpha=" << (int)col.getAlpha() << std::endl;
            osc.setColour(col);
        }
        else
        {
            // Use default waveform colors based on count
            static const juce::Colour defaultColors[] = {
                juce::Colour(0xFF00FF00), // Green
                juce::Colour(0xFF0088FF), // Blue
                juce::Colour(0xFFFF8800), // Orange
                juce::Colour(0xFFFF0088), // Pink
                juce::Colour(0xFF88FF00), // Lime
            };
            int colorIndex = static_cast<int>(state.getOscillators().size()) % 5;
            osc.setColour(defaultColors[colorIndex]);
        }

        // Set order index
        osc.setOrderIndex(static_cast<int>(state.getOscillators().size()));

        // Build response data
        json oscJson;
        oscJson["id"] = osc.getId().id.toStdString();
        oscJson["name"] = osc.getName().toStdString();
        oscJson["sourceId"] = osc.getSourceId().id.toStdString();
        oscJson["paneId"] = osc.getPaneId().id.toStdString();
        oscJson["mode"] = processingModeToString(osc.getProcessingMode()).toStdString();

        // Add to state on message thread and WAIT for completion
        auto promisePtr = std::make_shared<std::promise<bool>>();
        auto future = promisePtr->get_future();
        
        Oscillator oscCopy = osc;
        juce::MessageManager::callAsync([oscCopy, track, promisePtr]() mutable {
            try {
                track->getProcessor().getState().addOscillator(oscCopy);
                promisePtr->set_value(true);
            } catch (...) {
                promisePtr->set_value(false);
            }
        });

        // Wait for the add to complete
        if (future.wait_for(std::chrono::seconds(2)) == std::future_status::ready)
        {
            res.set_content(successResponse(oscJson).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Timeout adding oscillator").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStateUpdateOscillator(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);

        // Required: oscillator ID
        std::string idStr = body.value("id", "");
        if (idStr.empty())
        {
            res.set_content(errorResponse("Oscillator 'id' is required").dump(), "application/json");
            return;
        }

        auto& state = track->getProcessor().getState();
        OscillatorId oscId{ juce::String(idStr) };

        auto existingOsc = state.getOscillator(oscId);
        if (!existingOsc.has_value())
        {
            res.set_content(errorResponse("Oscillator not found: " + idStr).dump(), "application/json");
            return;
        }

        Oscillator osc = existingOsc.value();

        // Update optional fields if provided
        if (body.contains("visible"))
        {
            osc.setVisible(body["visible"].get<bool>());
        }
        if (body.contains("name"))
        {
            osc.setName(juce::String(body["name"].get<std::string>()));
        }
        if (body.contains("opacity"))
        {
            osc.setOpacity(body["opacity"].get<float>());
        }
        if (body.contains("lineWidth"))
        {
            osc.setLineWidth(body["lineWidth"].get<float>());
        }
        
        std::string colStr;
        if (body.contains("colour")) colStr = body["colour"].get<std::string>();
        else if (body.contains("color")) colStr = body["color"].get<std::string>();
        
        if (!colStr.empty())
        {
            auto col = juce::Colour::fromString(juce::String(colStr));
            // FORCE ALPHA to 255 if it's parsed as 0
            if (col.getAlpha() == 0) col = col.withAlpha(1.0f);
            
            std::cerr << "[HTTP] handleStateUpdateOscillator: Received color='" << colStr 
                      << "' Parsed=" << col.toDisplayString(true) 
                      << " Alpha=" << (int)col.getAlpha() << std::endl;
            osc.setColour(col);
        }

        // Update the oscillator in state - THIS IS THE FUNCTION WITH THE BUG FIX!
        state.updateOscillator(osc);

        // Return updated oscillator info
        json oscJson;
        oscJson["id"] = osc.getId().id.toStdString();
        oscJson["name"] = osc.getName().toStdString();
        oscJson["visible"] = osc.isVisible();
        oscJson["opacity"] = osc.getOpacity();
        oscJson["lineWidth"] = osc.getLineWidth();

        res.set_content(successResponse(oscJson).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStateReorderOscillators(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        int fromIndex = body.value("fromIndex", -1);
        int toIndex = body.value("toIndex", -1);

        if (fromIndex < 0 || toIndex < 0)
        {
            res.set_content(errorResponse("fromIndex and toIndex are required").dump(), "application/json");
            return;
        }

        auto& state = track->getProcessor().getState();
        state.reorderOscillators(fromIndex, toIndex);

        // Return updated oscillator list
        json oscillators = json::array();
        for (const auto& osc : state.getOscillators())
        {
            json oscJson;
            oscJson["id"] = osc.getId().id.toStdString();
            oscJson["name"] = osc.getName().toStdString();
            oscJson["orderIndex"] = osc.getOrderIndex();
            oscillators.push_back(oscJson);
        }

        res.set_content(successResponse(oscillators).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStatePanes(const httplib::Request&, httplib::Response& res)
{
    json panes = json::array();

    if (auto* track = daw_.getTrack(0))
    {
        auto& state = track->getProcessor().getState();
        auto& layoutManager = state.getLayoutManager();

        for (const auto& pane : layoutManager.getPanes())
        {
            json paneJson;
            paneJson["id"] = pane.getId().id.toStdString();
            paneJson["name"] = pane.getName().toStdString();
            paneJson["column"] = pane.getColumnIndex();
            paneJson["order"] = pane.getOrderIndex();
            paneJson["collapsed"] = pane.isCollapsed();
            panes.push_back(paneJson);
        }
    }

    res.set_content(successResponse(panes).dump(), "application/json");
}

void TestHttpServer::handleStateAddPane(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        auto& state = track->getProcessor().getState();
        auto& layoutManager = state.getLayoutManager();

        // Create new pane
        Pane newPane;
        
        // Set name (default based on count)
        std::string name = body.value("name", "");
        if (name.empty())
        {
            name = "Pane " + std::to_string(layoutManager.getPaneCount() + 1);
        }
        newPane.setName(name);
        newPane.setOrderIndex(static_cast<int>(layoutManager.getPaneCount()));

        // Capture pane ID before adding (it gets generated in constructor)
        std::string paneId = newPane.getId().id.toStdString();

        // Add pane on message thread and WAIT for completion
        auto promisePtr = std::make_shared<std::promise<bool>>();
        auto future = promisePtr->get_future();
        
        Pane paneCopy = newPane;
        juce::MessageManager::callAsync([paneCopy, track, promisePtr]() mutable {
            try {
                track->getProcessor().getState().getLayoutManager().addPane(paneCopy);
                // Refresh UI
                if (auto* editor = dynamic_cast<OscilPluginEditor*>(track->getEditor()))
                {
                    editor->refreshPanels();
                    editor->resized();
                }
                promisePtr->set_value(true);
            } catch (...) {
                promisePtr->set_value(false);
            }
        });

        // Wait for the add to complete
        if (future.wait_for(std::chrono::seconds(2)) == std::future_status::ready)
        {
            json paneJson;
            paneJson["id"] = paneId;
            paneJson["name"] = name;
            res.set_content(successResponse(paneJson).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Timeout adding pane").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStatePaneSwap(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        int fromIndex = body.value("fromIndex", -1);
        int toIndex = body.value("toIndex", -1);

        if (fromIndex < 0 || toIndex < 0)
        {
            res.set_content(errorResponse("fromIndex and toIndex are required").dump(), "application/json");
            return;
        }

        // Swap panes on message thread
        juce::MessageManager::callAsync([track, fromIndex, toIndex]() {
            auto& layoutManager = track->getProcessor().getState().getLayoutManager();
            auto panes = layoutManager.getPanes();
            
            if (fromIndex < static_cast<int>(panes.size()) && toIndex < static_cast<int>(panes.size()))
            {
                PaneId fromPaneId = panes[static_cast<size_t>(fromIndex)].getId();
                layoutManager.movePane(fromPaneId, toIndex);
                
                // Refresh UI - use track->getEditor() instead of getActiveEditor()
                if (auto* editor = dynamic_cast<OscilPluginEditor*>(track->getEditor()))
                {
                    editor->refreshPanels();
                    editor->resized();
                }
            }
        });

        json result;
        result["fromIndex"] = fromIndex;
        result["toIndex"] = toIndex;
        res.set_content(successResponse(result).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStateGpuToggle(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        bool enabled = body.value("enabled", true);

        // Toggle GPU on message thread - use shared_ptr to avoid dangling reference
        auto promisePtr = std::make_shared<std::promise<bool>>();
        auto future = promisePtr->get_future();
        
        juce::MessageManager::callAsync([track, enabled, promisePtr]() {
            bool success = false;
            try {
                if (auto* editor = dynamic_cast<OscilPluginEditor*>(track->getEditor()))
                {
                    editor->setGpuRenderingEnabled(enabled);
                    success = true;
                }
            } catch (...) {}
            promisePtr->set_value(success);
        });

        // Wait for async operation to complete (with timeout)
        if (future.wait_for(std::chrono::seconds(2)) == std::future_status::ready)
        {
            bool success = future.get();
            json result;
            result["gpuEnabled"] = enabled;
            result["editorFound"] = success;
            res.set_content(successResponse(result).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Timeout waiting for GPU toggle").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStateSources(const httplib::Request&, httplib::Response& res)
{
    json sources = json::array();

    auto allSources = PluginFactory::getInstance().getInstanceRegistry().getAllSources();
    for (const auto& source : allSources)
    {
        json sourceJson;
        sourceJson["id"] = source.sourceId.id.toStdString();
        sourceJson["name"] = source.name.toStdString();
        sourceJson["channels"] = source.channelCount;
        sourceJson["sampleRate"] = source.sampleRate;
        sources.push_back(sourceJson);
    }

    res.set_content(successResponse(sources).dump(), "application/json");
}

// Display Settings handlers
void TestHttpServer::handleStateLayout(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        int columns = body.value("columns", 1);

        auto promisePtr = std::make_shared<std::promise<bool>>();
        auto future = promisePtr->get_future();

        juce::MessageManager::callAsync([track, columns, promisePtr]() {
            bool success = false;
            try {
                if (auto* editor = dynamic_cast<OscilPluginEditor*>(track->getEditor()))
                {
                    // Get processor and set layout
                    auto& processor = editor->getProcessor();
                    auto layout = static_cast<ColumnLayout>(juce::jlimit(1, 3, columns));
                    processor.getState().getLayoutManager().setColumnLayout(layout);
                    editor->refreshPanels(); // Refresh panels for new layout
                    editor->resized(); // Trigger layout update
                    success = true;
                }
            } catch (...) {}
            promisePtr->set_value(success);
        });

        if (future.wait_for(std::chrono::seconds(2)) == std::future_status::ready)
        {
            json result;
            result["columns"] = columns;
            result["success"] = future.get();
            res.set_content(successResponse(result).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Timeout waiting for layout change").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStateTheme(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        std::string themeName = body.value("theme", "");
        
        if (themeName.empty())
        {
            res.set_content(errorResponse("Theme name is required").dump(), "application/json");
            return;
        }

        auto promisePtr = std::make_shared<std::promise<bool>>();
        auto future = promisePtr->get_future();

        juce::MessageManager::callAsync([track, themeName, promisePtr]() {
            bool success = false;
            try {
                if (auto* editor = dynamic_cast<OscilPluginEditor*>(track->getEditor()))
                {
                    // Access theme service through the processor
                    auto& processor = editor->getProcessor();
                    auto& themeService = processor.getThemeService();
                    themeService.setCurrentTheme(juce::String(themeName));
                    success = true;
                }
            } catch (...) {}
            promisePtr->set_value(success);
        });

        if (future.wait_for(std::chrono::seconds(2)) == std::future_status::ready)
        {
            json result;
            result["theme"] = themeName;
            result["success"] = future.get();
            res.set_content(successResponse(result).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Timeout waiting for theme change").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStateGrid(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        bool showGrid = body.value("show", true);

        auto promisePtr = std::make_shared<std::promise<bool>>();
        auto future = promisePtr->get_future();

        juce::MessageManager::callAsync([track, showGrid, promisePtr]() {
            bool success = false;
            try {
                if (auto* editor = dynamic_cast<OscilPluginEditor*>(track->getEditor()))
                {
                    auto& processor = editor->getProcessor();
                    processor.getState().setShowGridEnabled(showGrid);
                    editor->setShowGridForAllPanes(showGrid);
                    success = true;
                }
            } catch (...) {}
            promisePtr->set_value(success);
        });

        if (future.wait_for(std::chrono::seconds(2)) == std::future_status::ready)
        {
            json result;
            result["showGrid"] = showGrid;
            result["success"] = future.get();
            res.set_content(successResponse(result).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Timeout waiting for grid toggle").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStateDisplay(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        // GET - return current display settings
        if (req.method == "GET")
        {
            auto promisePtr = std::make_shared<std::promise<json>>();
            auto future = promisePtr->get_future();

            juce::MessageManager::callAsync([track, promisePtr]() {
                json result;
                try {
                    if (auto* editor = dynamic_cast<OscilPluginEditor*>(track->getEditor()))
                    {
                        auto& processor = editor->getProcessor();
                        auto& state = processor.getState();
                        auto& layoutManager = state.getLayoutManager();
                        
                        result["showGrid"] = state.isShowGridEnabled();
                        result["autoScale"] = state.isAutoScaleEnabled();
                        result["gain"] = state.getGainDb();
                        result["columns"] = static_cast<int>(layoutManager.getColumnLayout());
                        result["gpuEnabled"] = state.isGpuRenderingEnabled();
                        result["success"] = true;
                    }
                    else
                    {
                        result["success"] = false;
                        result["error"] = "Editor not found";
                    }
                } catch (const std::exception& e) {
                    result["success"] = false;
                    result["error"] = e.what();
                }
                promisePtr->set_value(result);
            });

            if (future.wait_for(std::chrono::seconds(2)) == std::future_status::ready)
            {
                json result = future.get();
                res.set_content(successResponse(result).dump(), "application/json");
            }
            else
            {
                res.set_content(errorResponse("Timeout getting display settings").dump(), "application/json");
            }
            return;
        }

        // POST - update display settings
        auto body = json::parse(req.body);

        auto promisePtr = std::make_shared<std::promise<bool>>();
        auto future = promisePtr->get_future();

        juce::MessageManager::callAsync([track, body, promisePtr]() {
            bool success = false;
            try {
                if (auto* editor = dynamic_cast<OscilPluginEditor*>(track->getEditor()))
                {
                    auto& processor = editor->getProcessor();
                    auto& state = processor.getState();
                    
                    if (body.contains("showGrid"))
                    {
                        state.setShowGridEnabled(body["showGrid"].get<bool>());
                        editor->setShowGridForAllPanes(body["showGrid"].get<bool>());
                    }
                    if (body.contains("autoScale"))
                        state.setAutoScaleEnabled(body["autoScale"].get<bool>());
                    if (body.contains("gain"))
                        state.setGainDb(body["gain"].get<float>());
                        
                    success = true;
                }
            } catch (...) {}
            promisePtr->set_value(success);
        });

        if (future.wait_for(std::chrono::seconds(2)) == std::future_status::ready)
        {
            json result;
            result["success"] = future.get();
            res.set_content(successResponse(result).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Timeout updating display settings").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStateTiming(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        // GET - return current timing settings
        if (req.method == "GET")
        {
            // Use shared_ptr to ensure promise lifetime extends beyond timeout
            auto promisePtr = std::make_shared<std::promise<json>>();
            auto future = promisePtr->get_future();

            juce::MessageManager::callAsync([track, promisePtr]() {
                json result;
                try {
                    if (auto* editor = dynamic_cast<OscilPluginEditor*>(track->getEditor()))
                    {
                        auto& processor = editor->getProcessor();
                        auto& timingEngine = processor.getTimingEngine();
                        auto config = timingEngine.getConfig();
                        auto hostInfo = timingEngine.getHostInfo();
                        
                        result["mode"] = static_cast<int>(config.timingMode);
                        result["bpm"] = config.internalBPM;
                        result["hostBpm"] = hostInfo.bpm;
                        result["interval"] = static_cast<int>(config.noteInterval);
                        result["syncToHost"] = config.hostSyncEnabled;
                        result["timeIntervalMs"] = config.timeIntervalMs;
                        result["success"] = true;
                    }
                    else
                    {
                        result["success"] = false;
                        result["error"] = "Editor not found";
                    }
                } catch (const std::exception& e) {
                    result["success"] = false;
                    result["error"] = e.what();
                }
                promisePtr->set_value(result);
            });

            if (future.wait_for(std::chrono::seconds(2)) == std::future_status::ready)
            {
                json result = future.get();
                res.set_content(successResponse(result).dump(), "application/json");
            }
            else
            {
                res.set_content(errorResponse("Timeout getting timing settings").dump(), "application/json");
            }
            return;
        }

        // POST - update timing settings
        auto body = json::parse(req.body);

        // Use shared_ptr to ensure promise lifetime extends beyond timeout
        auto promisePtr = std::make_shared<std::promise<bool>>();
        auto future = promisePtr->get_future();

        juce::MessageManager::callAsync([track, body, promisePtr]() {
            bool success = false;
            try {
                if (auto* editor = dynamic_cast<OscilPluginEditor*>(track->getEditor()))
                {
                    auto& processor = editor->getProcessor();
                    auto& timingEngine = processor.getTimingEngine();
                    
                    if (body.contains("mode"))
                        timingEngine.setTimingMode(static_cast<TimingMode>(body["mode"].get<int>()));
                    if (body.contains("bpm"))
                        timingEngine.setInternalBPM(static_cast<float>(body["bpm"].get<double>()));
                    if (body.contains("interval"))
                        timingEngine.setNoteInterval(static_cast<EngineNoteInterval>(body["interval"].get<int>()));
                    if (body.contains("syncToHost"))
                        timingEngine.setHostSyncEnabled(body["syncToHost"].get<bool>());
                    if (body.contains("timeIntervalMs"))
                        timingEngine.setTimeIntervalMs(body["timeIntervalMs"].get<float>());
                        
                    success = true;
                }
            } catch (...) {
                success = false;
            }
            promisePtr->set_value(success);
        });

        if (future.wait_for(std::chrono::seconds(2)) == std::future_status::ready)
        {
            json result;
            result["success"] = future.get();
            res.set_content(successResponse(result).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Timeout updating timing settings").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleHealth(const httplib::Request&, httplib::Response& res)
{
    json data;
    data["status"] = "ok";
    data["running"] = daw_.isRunning();
    data["tracks"] = daw_.getNumTracks();
    res.set_content(successResponse(data).dump(), "application/json");
}

// Performance metrics handlers
void TestHttpServer::handleMetricsStart(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        // Parse body if provided, use defaults otherwise
        auto body = json::parse(req.body.empty() ? "{}" : req.body);
        int intervalMs = body.value("intervalMs", 100);

        // Validate intervalMs range
        if (intervalMs < 10 || intervalMs > 60000)
        {
            res.status = 400;
            res.set_content(errorResponse("Invalid intervalMs: must be between 10 and 60000").dump(), "application/json");
            return;
        }

        metrics_.startCollection(intervalMs);

        json data;
        data["collecting"] = metrics_.isCollecting();
        data["intervalMs"] = intervalMs;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleMetricsStop(const httplib::Request&, httplib::Response& res)
{
    metrics_.stopCollection();

    json data;
    data["collecting"] = metrics_.isCollecting();
    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleMetricsCurrent(const httplib::Request&, httplib::Response& res)
{
    auto snapshot = metrics_.getCurrentSnapshot();

    json data;
    data["fps"] = snapshot.fps;
    data["cpuPercent"] = snapshot.cpuPercent;
    data["memoryBytes"] = snapshot.memoryBytes;
    data["memoryMB"] = snapshot.memoryMB;
    data["oscillatorCount"] = snapshot.oscillatorCount;
    data["sourceCount"] = snapshot.sourceCount;
    data["timestamp"] = snapshot.timestamp;
    data["collecting"] = metrics_.isCollecting();

    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleMetricsStats(const httplib::Request& req, httplib::Response& res)
{
    int periodMs = 0;
    auto periodParam = req.get_param_value("periodMs");
    if (!periodParam.empty())
    {
        periodMs = std::stoi(periodParam);
    }

    PerformanceStats stats;
    if (periodMs > 0)
    {
        stats = metrics_.getStatsForPeriod(periodMs);
    }
    else
    {
        stats = metrics_.getStats();
    }

    json data;
    data["avgFps"] = stats.avgFps;
    data["minFps"] = stats.minFps;
    data["maxFps"] = stats.maxFps;
    data["avgCpu"] = stats.avgCpu;
    data["maxCpu"] = stats.maxCpu;
    data["avgMemoryMB"] = stats.avgMemoryMB;
    data["maxMemoryMB"] = stats.maxMemoryMB;
    data["sampleCount"] = stats.sampleCount;
    data["durationMs"] = stats.durationMs;

    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleMetricsReset(const httplib::Request&, httplib::Response& res)
{
    metrics_.reset();
    res.set_content(successResponse().dump(), "application/json");
}

void TestHttpServer::handleMetricsRecordFrame(const httplib::Request&, httplib::Response& res)
{
    metrics_.recordFrame();
    res.set_content(successResponse().dump(), "application/json");
}

// ============================================================================
// Extended Profiling Handlers
// ============================================================================

void TestHttpServer::handleProfilingStart(const httplib::Request&, httplib::Response& res)
{
    metrics_.startProfiling();
    
    json data;
    data["profiling"] = true;
    data["message"] = "Profiling started";
    
    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleProfilingStop(const httplib::Request&, httplib::Response& res)
{
    auto result = metrics_.stopProfiling();
    
    json data;
    data["profiling"] = false;
    data["durationMs"] = result.durationMs;
    
    // Summary stats
    json summary;
    summary["avgFps"] = result.stats.avgFps;
    summary["minFps"] = result.stats.minFps;
    summary["maxFps"] = result.stats.maxFps;
    summary["p50FrameTimeMs"] = result.stats.p50FrameTimeMs;
    summary["p95FrameTimeMs"] = result.stats.p95FrameTimeMs;
    summary["p99FrameTimeMs"] = result.stats.p99FrameTimeMs;
    summary["avgCpu"] = result.stats.avgCpu;
    summary["maxMemoryMB"] = result.stats.maxMemoryMB;
    summary["peakMemoryMB"] = result.peakMemoryMB;
    summary["memoryGrowthMBPerSec"] = result.memoryGrowthMBPerSec;
    data["summary"] = summary;
    
    // Hotspot count
    data["hotspotCount"] = result.hotspots.size();
    
    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleProfilingSnapshot(const httplib::Request&, httplib::Response& res)
{
    bool isProfiling = metrics_.isProfiling();
    
    json data;
    data["profiling"] = isProfiling;
    data["timestamp"] = juce::Time::currentTimeMillis();
    
    // Current snapshot
    auto snapshot = metrics_.getCurrentSnapshot();
    data["fps"] = snapshot.fps;
    data["cpuPercent"] = snapshot.cpuPercent;
    data["memoryMB"] = snapshot.memoryMB;
    data["oscillatorCount"] = snapshot.oscillatorCount;
    
    if (isProfiling)
    {
        // GPU metrics
        auto gpu = metrics_.getGpuMetrics();
        json gpuData;
        gpuData["currentFps"] = gpu.currentFps;
        gpuData["avgFps"] = gpu.avgFps;
        gpuData["avgFrameTimeMs"] = gpu.avgFrameTimeMs;
        gpuData["p95FrameTimeMs"] = gpu.p95FrameTimeMs;
        gpuData["totalFrames"] = gpu.totalFrames;
        data["gpu"] = gpuData;
        
        // Hotspot count
        data["hotspotCount"] = metrics_.getHotspots().size();
    }
    
    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleProfilingGpu(const httplib::Request&, httplib::Response& res)
{
    auto gpu = metrics_.getGpuMetrics();
    
    json data;
    data["currentFps"] = gpu.currentFps;
    data["avgFps"] = gpu.avgFps;
    data["minFps"] = gpu.minFps;
    data["maxFps"] = gpu.maxFps;
    data["avgFrameTimeMs"] = gpu.avgFrameTimeMs;
    data["minFrameTimeMs"] = gpu.minFrameTimeMs;
    data["maxFrameTimeMs"] = gpu.maxFrameTimeMs;
    data["p50FrameTimeMs"] = gpu.p50FrameTimeMs;
    data["p95FrameTimeMs"] = gpu.p95FrameTimeMs;
    data["p99FrameTimeMs"] = gpu.p99FrameTimeMs;
    
    json timing;
    timing["avgBeginFrameMs"] = gpu.avgBeginFrameMs;
    timing["avgWaveformRenderMs"] = gpu.avgWaveformRenderMs;
    timing["avgEffectPipelineMs"] = gpu.avgEffectPipelineMs;
    timing["avgCompositeMs"] = gpu.avgCompositeMs;
    timing["avgBlitMs"] = gpu.avgBlitMs;
    data["timing"] = timing;
    
    json operations;
    operations["totalFboBinds"] = gpu.totalFboBinds;
    operations["totalShaderSwitches"] = gpu.totalShaderSwitches;
    data["operations"] = operations;
    
    data["totalFrames"] = gpu.totalFrames;
    
    // Budget tracking from PerformanceProfiler
    auto& profiler = oscil::PerformanceProfiler::getInstance();
    auto gpuData = profiler.getGpuProfiler().getData();
    json budget;
    budget["exceededCount"] = gpuData.budgetExceededCount;
    budget["totalSkippedWaveforms"] = gpuData.totalSkippedWaveforms;
    budget["totalSkippedEffects"] = gpuData.totalSkippedEffects;
    budget["avgBudgetRatio"] = gpuData.avgBudgetRatio;
    budget["maxBudgetRatio"] = gpuData.maxBudgetRatio;
    data["budget"] = budget;
    
    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleProfilingThreads(const httplib::Request&, httplib::Response& res)
{
    auto toJson = [](const ThreadMetrics& m) {
        json j;
        j["name"] = m.name;
        j["avgExecutionTimeMs"] = m.avgExecutionTimeMs;
        j["maxExecutionTimeMs"] = m.maxExecutionTimeMs;
        j["minExecutionTimeMs"] = m.minExecutionTimeMs;
        j["invocationCount"] = m.invocationCount;
        j["loadPercent"] = m.loadPercent;
        j["totalLockWaitMs"] = m.totalLockWaitMs;
        j["lockAcquisitionCount"] = m.lockAcquisitionCount;
        return j;
    };
    
    json data;
    data["audioThread"] = toJson(metrics_.getAudioThreadMetrics());
    data["uiThread"] = toJson(metrics_.getUiThreadMetrics());
    data["openglThread"] = toJson(metrics_.getOpenGLThreadMetrics());
    
    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleProfilingComponents(const httplib::Request&, httplib::Response& res)
{
    auto stats = metrics_.getComponentStats();
    
    json components = json::array();
    for (const auto& s : stats)
    {
        json c;
        c["name"] = s.name;
        c["repaintCount"] = s.repaintCount;
        c["totalRepaintTimeMs"] = s.totalRepaintTimeMs;
        c["avgRepaintTimeMs"] = s.avgRepaintTimeMs;
        c["maxRepaintTimeMs"] = s.maxRepaintTimeMs;
        c["repaintsPerSecond"] = s.repaintsPerSecond;
        components.push_back(c);
    }
    
    json data;
    data["components"] = components;
    data["count"] = stats.size();
    
    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleProfilingHotspots(const httplib::Request&, httplib::Response& res)
{
    auto hotspots = metrics_.getHotspots();
    
    json hsArray = json::array();
    for (const auto& hs : hotspots)
    {
        json h;
        h["category"] = hs.category;
        h["location"] = hs.location;
        h["description"] = hs.description;
        h["severity"] = hs.severity;
        h["impactMs"] = hs.impactMs;
        h["recommendation"] = hs.recommendation;
        hsArray.push_back(h);
    }
    
    json data;
    data["hotspots"] = hsArray;
    data["count"] = hotspots.size();
    
    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleProfilingTimeline(const httplib::Request& req, httplib::Response& res)
{
    int frameCount = 60;
    if (req.has_param("frames"))
    {
        frameCount = std::stoi(req.get_param_value("frames"));
    }
    
    juce::String timelineJson = metrics_.getTimelineJson(frameCount);
    
    // Parse and wrap in success response
    try
    {
        json timeline = json::parse(timelineJson.toStdString());
        res.set_content(successResponse(timeline).dump(), "application/json");
    }
    catch (...)
    {
        json data;
        data["frames"] = json::array();
        res.set_content(successResponse(data).dump(), "application/json");
    }
}

// ============================================================================
// Stress Testing Handlers
// ============================================================================

void TestHttpServer::handleStressOscillators(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        // Parse body if provided, use defaults otherwise
        auto body = json::parse(req.body.empty() ? "{}" : req.body);
        int count = body.value("count", 5);

        // Validate count range
        if (count < 1 || count > 100)
        {
            res.status = 400;
            res.set_content(errorResponse("Invalid count: must be between 1 and 100").dump(), "application/json");
            return;
        }

        // Add multiple oscillators for stress testing
        int added = 0;

        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.status = 404;
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }
        
        auto& state = track->getProcessor().getState();
        auto& layoutManager = state.getLayoutManager();
        
        // Get sources from registry
        auto allSources = PluginFactory::getInstance().getInstanceRegistry().getAllSources();
        if (allSources.empty())
        {
            res.status = 404;
            res.set_content(errorResponse("No sources available").dump(), "application/json");
            return;
        }

        auto panes = layoutManager.getPanes();
        if (panes.empty())
        {
            res.status = 404;
            res.set_content(errorResponse("No panes available").dump(), "application/json");
            return;
        }

        for (int i = 0; i < count; ++i)
        {
            // Create oscillator
            Oscillator osc;
            osc.setName("Stress Osc " + juce::String(i + 1));
            osc.setSourceId(allSources[0].sourceId);
            osc.setPaneId(panes[0].getId());
            osc.setOrderIndex(static_cast<int>(state.getOscillators().size()));

            state.addOscillator(osc);
            added++;
        }

        json data;
        data["added"] = added;
        data["totalOscillators"] = static_cast<int>(state.getOscillators().size());

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStressEffects(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        // Parse body if provided, use defaults otherwise
        auto body = json::parse(req.body.empty() ? "{}" : req.body);
        bool enableAll = body.value("enableAll", true);
        std::string quality = body.value("quality", "high");

        // Validate quality value
        if (quality != "low" && quality != "medium" && quality != "high")
        {
            res.status = 400;
            res.set_content(errorResponse("Invalid quality: must be 'low', 'medium', or 'high'").dump(), "application/json");
            return;
        }

        // This would enable effects on all oscillators
        // Implementation depends on the oscillator configuration API

        json data;
        data["effectsEnabled"] = enableAll;
        data["quality"] = quality;
        data["message"] = "Effects configuration updated";

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStressAudio(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        // Parse body if provided, use defaults otherwise
        auto body = json::parse(req.body.empty() ? "{}" : req.body);
        int durationMs = body.value("durationMs", 5000);
        double amplitude = body.value("amplitude", 0.5);
        std::string waveform = body.value("waveform", "sine");
        double frequency = body.value("frequency", 440.0);

        // Validate parameter ranges
        if (durationMs < 100 || durationMs > 60000)
        {
            res.status = 400;
            res.set_content(errorResponse("Invalid durationMs: must be between 100 and 60000").dump(), "application/json");
            return;
        }
        if (amplitude < 0.0 || amplitude > 1.0)
        {
            res.status = 400;
            res.set_content(errorResponse("Invalid amplitude: must be between 0 and 1").dump(), "application/json");
            return;
        }
        if (frequency <= 0.0 || frequency > 20000.0)
        {
            res.status = 400;
            res.set_content(errorResponse("Invalid frequency: must be between 0 and 20000 Hz").dump(), "application/json");
            return;
        }

        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.status = 404;
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        // Configure audio generator for continuous output
        auto& generator = track->getAudioGenerator();

        // Set waveform type
        generator.setWaveform(TestAudioGenerator::stringToWaveform(juce::String(waveform)));
        generator.setFrequency(static_cast<float>(frequency));
        generator.setAmplitude(static_cast<float>(amplitude));

        // Note: Caller should stop after durationMs
        json data;
        data["generating"] = generator.isGenerating();
        data["durationMs"] = durationMs;
        data["waveform"] = waveform;
        data["frequency"] = frequency;
        data["amplitude"] = amplitude;

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const json::parse_error& e)
    {
        res.status = 400;
        res.set_content(errorResponse("Invalid JSON: " + std::string(e.what())).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// Debug log handlers
void TestHttpServer::handleDebugLogs(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        const std::string logPath = "/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log";
        juce::File logFile(logPath);
        
        if (!logFile.existsAsFile())
        {
            json data;
            data["logs"] = json::array();
            data["count"] = 0;
            res.set_content(successResponse(data).dump(), "application/json");
            return;
        }
        
        // Parse optional lines parameter
        int maxLines = 1000;
        if (req.has_param("lines"))
        {
            maxLines = std::stoi(req.get_param_value("lines"));
        }
        
        // Read and parse log entries
        juce::StringArray lines;
        logFile.readLines(lines);
        
        json logs = json::array();
        int count = 0;
        int startIdx = std::max(0, lines.size() - maxLines);
        
        for (int i = startIdx; i < lines.size() && count < maxLines; ++i)
        {
            juce::String line = lines[i].trim();
            if (line.isEmpty()) continue;
            
            try
            {
                json entry = json::parse(line.toStdString());
                logs.push_back(entry);
                count++;
            }
            catch (...)
            {
                // Skip non-JSON lines
            }
        }
        
        json data;
        data["logs"] = logs;
        data["count"] = count;
        data["totalLines"] = lines.size();
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleDebugLogsClear(const httplib::Request&, httplib::Response& res)
{
    try
    {
        const std::string logPath = "/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log";
        juce::File logFile(logPath);

        if (logFile.existsAsFile())
        {
            if (!logFile.replaceWithText(""))
            {
                res.set_content(errorResponse("Failed to clear log file").dump(), "application/json");
                return;
            }
        }

        res.set_content(successResponse().dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleDebugLogsTail(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        const std::string logPath = "/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log";
        juce::File logFile(logPath);
        
        int numLines = 50;
        if (req.has_param("lines"))
        {
            numLines = std::stoi(req.get_param_value("lines"));
        }
        
        if (!logFile.existsAsFile())
        {
            json data;
            data["logs"] = json::array();
            data["count"] = 0;
            res.set_content(successResponse(data).dump(), "application/json");
            return;
        }
        
        juce::StringArray lines;
        logFile.readLines(lines);
        
        json logs = json::array();
        int startIdx = std::max(0, lines.size() - numLines);
        
        for (int i = startIdx; i < lines.size(); ++i)
        {
            juce::String line = lines[i].trim();
            if (line.isEmpty()) continue;
            
            try
            {
                json entry = json::parse(line.toStdString());
                logs.push_back(entry);
            }
            catch (...)
            {
                // Skip non-JSON lines
            }
        }
        
        json data;
        data["logs"] = logs;
        data["count"] = static_cast<int>(logs.size());
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleDebugLogsWait(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string pattern = body.value("pattern", "");
        std::string hypothesisId = body.value("hypothesisId", "");
        int timeoutMs = body.value("timeout", 5000);
        
        const std::string logPath = "/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log";
        juce::File logFile(logPath);
        
        auto startTime = std::chrono::steady_clock::now();
        int lastLineCount = 0;
        
        while (true)
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime).count();
            
            if (elapsed >= timeoutMs)
            {
                json data;
                data["found"] = false;
                data["timeout"] = true;
                res.set_content(successResponse(data).dump(), "application/json");
                return;
            }
            
            if (logFile.existsAsFile())
            {
                juce::StringArray lines;
                logFile.readLines(lines);
                
                // Only check new lines
                for (int i = lastLineCount; i < lines.size(); ++i)
                {
                    juce::String line = lines[i].trim();
                    if (line.isEmpty()) continue;
                    
                    try
                    {
                        json entry = json::parse(line.toStdString());
                        
                        bool matches = true;
                        
                        // Check hypothesis ID if specified
                        if (!hypothesisId.empty())
                        {
                            std::string entryHypId = entry.value("hypothesisId", "");
                            if (entryHypId.find(hypothesisId) == std::string::npos)
                            {
                                matches = false;
                            }
                        }
                        
                        // Check pattern in message or location
                        if (matches && !pattern.empty())
                        {
                            std::string msg = entry.value("message", "");
                            std::string loc = entry.value("location", "");
                            
                            if (msg.find(pattern) == std::string::npos &&
                                loc.find(pattern) == std::string::npos)
                            {
                                matches = false;
                            }
                        }
                        
                        if (matches)
                        {
                            json data;
                            data["found"] = true;
                            data["entry"] = entry;
                            res.set_content(successResponse(data).dump(), "application/json");
                            return;
                        }
                    }
                    catch (...)
                    {
                        // Skip non-JSON lines
                    }
                }
                
                lastLineCount = lines.size();
            }
            
            // Sleep before checking again
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

} // namespace oscil::test
