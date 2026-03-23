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
    setupUIMouseRoutes();
    setupUIKeyboardRoutes();
    setupVerificationRoutes();
    setupStateRoutes();
    setupMetricsRoutes();

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
    res.set_content(successResponse(data).dump(), "application/json");
}

} // namespace oscil::test
