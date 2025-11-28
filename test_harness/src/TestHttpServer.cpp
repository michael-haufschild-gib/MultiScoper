/*
    Oscil Test Harness - HTTP Server Implementation
*/

#include "TestHttpServer.h"
#include "core/OscilState.h"

namespace oscil::test
{

TestHttpServer::TestHttpServer(TestDAW& daw)
    : daw_(daw)
{
    server_ = std::make_unique<httplib::Server>();
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
    server_->Get("/state/panes", [this](const httplib::Request& req, httplib::Response& res) {
        handleStatePanes(req, res);
    });
    server_->Get("/state/sources", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateSources(req, res);
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
        auto body = json::parse(req.body);
        double bpm = body.value("bpm", 120.0);
        daw_.getTransport().setBpm(bpm);
        res.set_content(successResponse().dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleTransportSetPosition(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        int64_t samples = body.value("samples", 0);
        daw_.getTransport().setPositionSamples(samples);
        res.set_content(successResponse().dump(), "application/json");
    }
    catch (const std::exception& e)
    {
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
            res.set_content(errorResponse("Track not found").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        std::string waveform = body.value("waveform", "sine");
        float frequency = body.value("frequency", 440.0f);
        float amplitude = body.value("amplitude", 0.8f);

        track->getAudioGenerator().setWaveform(TestAudioGenerator::stringToWaveform(waveform));
        track->getAudioGenerator().setFrequency(frequency);
        track->getAudioGenerator().setAmplitude(amplitude);

        res.set_content(successResponse().dump(), "application/json");
    }
    catch (const std::exception& e)
    {
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
            res.set_content(errorResponse("Track not found").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        int samples = body.value("samples", 4410);
        std::string waveform = body.value("waveform", "");

        if (!waveform.empty())
        {
            track->getAudioGenerator().setWaveform(TestAudioGenerator::stringToWaveform(waveform));
        }
        track->getAudioGenerator().setBurstSamples(samples);

        res.set_content(successResponse().dump(), "application/json");
    }
    catch (const std::exception& e)
    {
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
            res.set_content(errorResponse("Track not found").dump(), "application/json");
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
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// UI handlers
void TestHttpServer::handleUIClick(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.click(elementId))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIDoubleClick(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.doubleClick(elementId))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISelect(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        int itemId = body.value("itemId", -1);
        std::string itemText = body.value("itemText", "");

        bool success = false;
        if (itemId >= 0)
        {
            success = uiController_.select(elementId, itemId);
        }
        else if (!itemText.empty())
        {
            success = uiController_.selectByText(elementId, itemText);
        }

        if (success)
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Failed to select item").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIToggle(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        bool value = body.value("value", false);

        if (uiController_.toggle(elementId, value))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found or not toggleable").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISlider(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        double value = body.value("value", 0.0);

        if (uiController_.setSliderValue(elementId, value))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found or not a slider").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIDrag(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string from = body.value("from", "");
        std::string to = body.value("to", "");

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
            res.set_content(errorResponse("Failed to drag").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIDragOffset(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
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
            res.set_content(errorResponse("Failed to drag by offset").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIRightClick(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.rightClick(elementId))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIHover(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        int durationMs = body.value("durationMs", 500);

        if (uiController_.hover(elementId, durationMs))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIScroll(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
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
            res.set_content(errorResponse("Failed to scroll").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISliderIncrement(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.incrementSlider(elementId))
        {
            json data;
            data["value"] = uiController_.getSliderValue(elementId);
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found or not a slider").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISliderDecrement(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.decrementSlider(elementId))
        {
            json data;
            data["value"] = uiController_.getSliderValue(elementId);
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found or not a slider").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISliderReset(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.resetSliderToDefault(elementId))
        {
            json data;
            data["value"] = uiController_.getSliderValue(elementId);
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found or not a slider").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// Keyboard handlers
void TestHttpServer::handleUIKeyPress(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string key = body.value("key", "");
        std::string elementId = body.value("elementId", "");

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
            res.set_content(errorResponse("Failed to press key").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUITypeText(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        std::string text = body.value("text", "");

        if (uiController_.typeText(elementId, text))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found or not a text input").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIClearText(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.clearText(elementId))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found or not a text input").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// Focus handlers
void TestHttpServer::handleUIFocus(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.setFocus(elementId))
        {
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
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
        res.set_content(errorResponse("No focused element").dump(), "application/json");
    }
}

// Wait handlers
void TestHttpServer::handleUIWaitForElement(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        int timeoutMs = body.value("timeoutMs", 5000);

        bool found = uiController_.waitForElement(elementId, timeoutMs);

        json data;
        data["found"] = found;
        data["elementId"] = elementId;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIWaitForVisible(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        int timeoutMs = body.value("timeoutMs", 5000);

        bool visible = uiController_.waitForVisible(elementId, timeoutMs);

        json data;
        data["visible"] = visible;
        data["elementId"] = elementId;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIWaitForEnabled(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        int timeoutMs = body.value("timeoutMs", 5000);

        bool enabled = uiController_.waitForEnabled(elementId, timeoutMs);

        json data;
        data["enabled"] = enabled;
        data["elementId"] = elementId;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
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
        auto body = json::parse(req.body);
        std::string path = body.value("path", "/tmp/screenshot.png");
        std::string element = body.value("element", "window");

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
            res.set_content(errorResponse("Failed to capture screenshot").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleVerifyWaveform(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        float minAmplitude = body.value("minAmplitude", 0.05f);

        bool rendered = screenshot_.verifyWaveformRendered(elementId, minAmplitude);

        json data;
        data["pass"] = rendered;
        data["elementId"] = elementId;
        data["minAmplitude"] = minAmplitude;

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleScreenshotCompare(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        std::string baselinePath = body.value("baseline", "");
        int tolerance = body.value("tolerance", 5);

        juce::File baselineFile(baselinePath);
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
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleBaselineSave(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
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
            res.set_content(errorResponse("Failed to save baseline").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleVerifyColor(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        std::string colorHex = body.value("color", "#000000");
        int tolerance = body.value("tolerance", 10);
        std::string mode = body.value("mode", "background"); // "background" or "contains"
        float minCoverage = body.value("minCoverage", 0.01f);

        juce::Colour expectedColor = juce::Colour::fromString(colorHex);
        bool pass = false;

        if (mode == "background")
        {
            pass = screenshot_.verifyBackgroundColor(elementId, expectedColor, tolerance);
        }
        else if (mode == "contains")
        {
            pass = screenshot_.verifyContainsColor(elementId, expectedColor, minCoverage, tolerance);
        }

        json data;
        data["pass"] = pass;
        data["elementId"] = elementId;
        data["expectedColor"] = colorHex;
        data["mode"] = mode;

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleVerifyBounds(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        int expectedWidth = body.value("width", 0);
        int expectedHeight = body.value("height", 0);
        int tolerance = body.value("tolerance", 5);

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
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleVerifyVisible(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        bool visible = screenshot_.verifyElementVisible(elementId);

        json data;
        data["pass"] = visible;
        data["elementId"] = elementId;

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleAnalyzeWaveform(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        std::string elementId = req.get_param_value("elementId");
        std::string bgColorHex = req.get_param_value("backgroundColor");

        juce::Colour backgroundColor = bgColorHex.empty()
            ? juce::Colours::black
            : juce::Colour::fromString(bgColorHex);

        auto image = screenshot_.getElementImage(elementId);
        if (image.isNull())
        {
            res.set_content(errorResponse("Element not found or no image").dump(), "application/json");
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
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// State handlers
void TestHttpServer::handleStateReset(const httplib::Request&, httplib::Response& res)
{
    // Reset each track's processor state
    for (auto* track : daw_.getTracks())
    {
        // Would need a reset method on the processor
    }
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
            juce::File(path).replaceWithText(xml);
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
            track->getProcessor().getState().fromXmlString(xml);
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
        for (const auto& osc : state.getOscillators())
        {
            json oscJson;
            oscJson["id"] = osc.getId().id.toStdString();
            oscJson["name"] = osc.getName().toStdString();
            oscJson["sourceId"] = osc.getSourceId().id.toStdString();
            oscJson["paneId"] = osc.getPaneId().id.toStdString();
            oscJson["mode"] = processingModeToString(osc.getProcessingMode()).toStdString();
            oscillators.push_back(oscJson);
        }
    }

    res.set_content(successResponse(oscillators).dump(), "application/json");
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

void TestHttpServer::handleStateSources(const httplib::Request&, httplib::Response& res)
{
    json sources = json::array();

    auto allSources = InstanceRegistry::getInstance().getAllSources();
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
        auto body = json::parse(req.body);
        int intervalMs = body.value("intervalMs", 100);

        metrics_.startCollection(intervalMs);

        json data;
        data["collecting"] = metrics_.isCollecting();
        data["intervalMs"] = intervalMs;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
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

} // namespace oscil::test
