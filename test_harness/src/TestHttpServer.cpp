/*
    Oscil Test Harness - HTTP Server Core
*/

#include "TestHttpServer.h"

#include "core/OscilState.h"

#include "TestElementRegistry.h"
#include "plugin/PluginFactory.h"

namespace oscil::test
{

TestHttpServer::TestHttpServer(TestDAW& daw) : daw_(daw)
{
    server_ = std::make_unique<httplib::Server>();

    server_->set_read_timeout(30, 0);
    server_->set_write_timeout(30, 0);

    server_->set_pre_routing_handler(
        [](const httplib::Request& req, httplib::Response&) -> httplib::Server::HandlerResponse {
            std::cerr << "[HTTP IN] " << req.method << " " << req.path << std::endl;
            return httplib::Server::HandlerResponse::Unhandled;
        });

    server_->set_exception_handler([](const httplib::Request& req, httplib::Response& res, std::exception_ptr ep) {
        std::string errMsg;
        try
        {
            if (ep)
                std::rethrow_exception(ep);
        }
        catch (const std::exception& e)
        {
            errMsg = e.what();
        }
        catch (...)
        {
            errMsg = "Unknown exception";
        }
        std::cerr << "[HTTP EXCEPTION] " << req.method << " " << req.path << " - " << errMsg << std::endl;
        nlohmann::json errJson;
        errJson["success"] = false;
        errJson["error"] = "Exception: " + errMsg;
        res.status = 500;
        res.set_content(errJson.dump(), "application/json");
    });

    server_->set_error_handler([](const httplib::Request& req, httplib::Response& res) {
        std::string errMsg =
            "Internal server error on " + req.method + " " + req.path + " (status: " + std::to_string(res.status) + ")";
        std::cerr << "[HTTP ERROR] " << errMsg << std::endl;
        nlohmann::json errJson;
        errJson["success"] = false;
        errJson["error"] = errMsg;
        res.set_content(errJson.dump(), "application/json");
    });

    setupRoutes();
}

TestHttpServer::~TestHttpServer() { stop(); }

bool TestHttpServer::start(int port)
{
    if (running_.load())
        return false;

    port_ = port;
    running_.store(true);

    serverThread_ = std::make_unique<std::thread>([this]() { serverThread(); });

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
    if (!server_->listen("127.0.0.1", port_))
    {
        running_.store(false);
    }
}

void TestHttpServer::setupRoutes()
{
    // Health check
    server_->Get("/health", [this](const httplib::Request& req, httplib::Response& res) { handleHealth(req, res); });

    setupTransportRoutes();
    setupTrackRoutes();
    setupInstanceRoutes();
    setupUIMouseRoutes();
    setupUIKeyboardRoutes();
    setupVerificationRoutes();
    setupStateRoutes();
    setupMetricsRoutes();

    // Layout / per-pane bounds
    server_->Get("/layout",
                 [this](const httplib::Request& req, httplib::Response& res) { handleLayoutInfo(req, res); });
    server_->Post("/layout",
                  [this](const httplib::Request& req, httplib::Response& res) { handleSetLayout(req, res); });
    server_->Get("/panes", [this](const httplib::Request& req, httplib::Response& res) { handlePaneLayout(req, res); });
    server_->Post("/pane/move",
                  [this](const httplib::Request& req, httplib::Response& res) { handlePaneMove(req, res); });

    // Waveform state (data-level verification, no screenshots)
    server_->Get("/waveform/state",
                 [this](const httplib::Request& req, httplib::Response& res) { handleWaveformState(req, res); });

    // Diagnostic snapshot (complete state dump for AI agent consumption)
    server_->Get("/diagnostic/snapshot",
                 [this](const httplib::Request& req, httplib::Response& res) { handleDiagnosticSnapshot(req, res); });
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

json TestHttpServer::errorResponse(const std::string& message) { return {{"success", false}, {"error", message}}; }

void TestHttpServer::handleHealth(const httplib::Request&, httplib::Response& res)
{
    json data;
    data["status"] = "ok";
    data["running"] = daw_.isRunning();
    data["tracks"] = daw_.getNumTracks();
    data["sources"] = static_cast<int>(PluginFactory::getInstance().getInstanceRegistry().getSourceCount());
    res.set_content(successResponse(data).dump(), "application/json");
}

// ================== Track Resolver ==================

TestTrack* TestHttpServer::resolveTrack(const httplib::Request& req)
{
    // 1. Check URL query parameter (?trackId=N)
    auto it = req.params.find("trackId");
    if (it != req.params.end())
    {
        try
        {
            int id = std::stoi(it->second);
            if (auto* t = daw_.getTrack(id))
                return t;
        }
        catch (...)
        {
        }
    }

    // 2. For POST requests, also check the JSON body for "trackId"
    if (req.method == "POST" && !req.body.empty())
    {
        try
        {
            auto body = json::parse(req.body);
            if (body.contains("trackId"))
            {
                int id = body["trackId"].get<int>();
                if (auto* t = daw_.getTrack(id))
                    return t;
            }
        }
        catch (...)
        {
        }
    }

    return daw_.getTrack(0);
}

TestTrack* TestHttpServer::resolveTrackFromBody(const json& body)
{
    int id = body.value("trackId", 0);
    if (auto* t = daw_.getTrack(id))
        return t;
    return daw_.getTrack(0);
}

// ================== Instance Routes ==================

void TestHttpServer::setupInstanceRoutes()
{
    server_->Post("/daw/track/add",
                  [this](const httplib::Request& req, httplib::Response& res) { handleDawTrackAdd(req, res); });
    server_->Post("/daw/track/remove",
                  [this](const httplib::Request& req, httplib::Response& res) { handleDawTrackRemove(req, res); });
    server_->Get("/daw/tracks",
                 [this](const httplib::Request& req, httplib::Response& res) { handleDawTracks(req, res); });
}

void TestHttpServer::handleDawTrackAdd(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string name = body.value("name", "");

        int index = daw_.addTrack(juce::String(name));
        auto* track = daw_.getTrack(index);

        // prepareToPlay defers source registration to the message thread.
        // Wait for the registration to complete so sourceId is available
        // in the response (callers need it immediately).
        auto resolvedSourceId = std::make_shared<juce::String>();
        if (track)
        {
            auto done = std::make_shared<juce::WaitableEvent>();
            auto* processor = &track->getProcessor();
            juce::MessageManager::callAsync([processor, resolvedSourceId, done]() {
                *resolvedSourceId = processor->getSourceId().id;
                done->signal();
            });
            done->wait(3000);
        }

        json data;
        data["trackIndex"] = index;
        data["name"] = track ? track->getName().toStdString() : "";
        data["sourceId"] = resolvedSourceId->toStdString();
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleDawTrackRemove(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        int trackIndex = body.value("trackIndex", -1);

        if (trackIndex < 0)
        {
            res.set_content(errorResponse("trackIndex is required").dump(), "application/json");
            return;
        }

        if (daw_.removeTrack(trackIndex))
        {
            json data;
            data["trackIndex"] = trackIndex;
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Track not found: " + std::to_string(trackIndex)).dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleDawTracks(const httplib::Request&, httplib::Response& res)
{
    json tracks = json::array();
    for (int i = 0; i < daw_.getNumTracks(); ++i)
    {
        auto* track = daw_.getTrack(i);
        if (track == nullptr)
            continue;

        json t;
        t["index"] = track->getTrackIndex();
        t["name"] = track->getName().toStdString();
        t["sourceId"] = track->getSourceId().id.toStdString();
        t["editorVisible"] = track->isEditorVisible();
        tracks.push_back(t);
    }
    res.set_content(successResponse(tracks).dump(), "application/json");
}

} // namespace oscil::test
