/*
    Oscil Test Harness - HTTP Server Core
*/

#include "TestHttpServer.h"

#include "core/OscilState.h"

#include "TestElementRegistry.h"
#include "plugin/PluginFactory.h"

#include <juce_core/juce_core.h>

#if JUCE_WINDOWS
    #include <process.h>
#else
    #include <unistd.h>
#endif

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
    setupDawLifecycleRoutes();
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
    data["tracks"] = daw_.getActiveTrackCount();
    data["sources"] = static_cast<int>(PluginFactory::getInstance().getInstanceRegistry().getSourceCount());
#if JUCE_WINDOWS
    data["pid"] = static_cast<int>(_getpid());
#else
    data["pid"] = static_cast<int>(::getpid());
#endif
    // Multi-core hosting diagnostics: proves the audio dispatcher is alive
    // and reveals how much parallelism the harness is modeling.
    data["audioTicks"] = static_cast<uint64_t>(daw_.getProcessedTickCount());
    data["audioPoolThreads"] = daw_.getPoolThreadCount();
    res.set_content(successResponse(data).dump(), "application/json");
}

// ================== Track Resolver ==================

int TestHttpServer::resolveTrackId(const httplib::Request& req)
{
    // A malformed `trackId` (e.g. "abc") must not silently alias to track 0 —
    // that would let a bad request mutate the first track. Parse errors on an
    // explicitly-provided `trackId` throw `BadTrackIdError`; handlers convert
    // that into a 400 via `tryResolveTrackId`.
    auto it = req.params.find("trackId");
    if (it != req.params.end())
    {
        try
        {
            size_t pos = 0;
            const int parsed = std::stoi(it->second, &pos);
            if (pos != it->second.size())
                throw BadTrackIdError("Invalid trackId query param: " + it->second);
            return parsed;
        }
        catch (const BadTrackIdError&)
        {
            throw;
        }
        catch (...)
        {
            throw BadTrackIdError("Invalid trackId query param: " + it->second);
        }
    }

    if (req.method == "POST" && !req.body.empty())
    {
        try
        {
            auto body = json::parse(req.body);
            if (body.contains("trackId"))
            {
                if (!body["trackId"].is_number_integer())
                    throw BadTrackIdError("trackId must be an integer");
                return body["trackId"].get<int>();
            }
        }
        catch (const BadTrackIdError&)
        {
            throw;
        }
        catch (const json::exception&)
        {
            // Body didn't parse as JSON — handler's own body parse will surface
            // that error. Fall through to the default-track path.
        }
    }

    return 0;
}

std::optional<int> TestHttpServer::tryResolveTrackId(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        return resolveTrackId(req);
    }
    catch (const BadTrackIdError& e)
    {
        res.status = 400;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
        return std::nullopt;
    }
}

int TestHttpServer::resolveTrackIdFromBody(const json& body) { return body.value("trackId", 0); }

TestTrack* TestHttpServer::resolveTrack(const httplib::Request& req)
{
    // Legacy — see header warning. Only safe on the message thread.
    const int id = resolveTrackId(req);
    if (auto* t = daw_.getTrack(id))
        return t;
    return daw_.getTrack(0);
}

TestTrack* TestHttpServer::resolveTrackFromBody(const json& body)
{
    // Legacy — see header warning. Only safe on the message thread.
    const int id = resolveTrackIdFromBody(body);
    if (auto* t = daw_.getTrack(id))
        return t;
    return daw_.getTrack(0);
}

bool TestHttpServer::runOnMessageThreadBlocking(std::function<void()> fn, int timeoutMs, const char* label)
{
    // Same heap-own-state contract as runOnTrackSync: `fn` and `done` both
    // live in a shared_ptr so a timeout here is safe even when the lambda
    // eventually runs. Caller's `fn` captures must still be heap-owned
    // (shared_ptr<T>) — this helper cannot make stack captures safe.
    struct State
    {
        std::function<void()> fn;
        juce::WaitableEvent done;
    };
    auto state = std::make_shared<State>();
    state->fn = std::move(fn);

    juce::MessageManager::callAsync([state]() {
        if (state->fn)
            state->fn();
        state->done.signal();
    });

    if (!state->done.wait(timeoutMs))
    {
        juce::Logger::writeToLog(juce::String("[runOnMessageThreadBlocking] timeout after ") + juce::String(timeoutMs) +
                                 juce::String("ms: ") + juce::String(label != nullptr ? label : "<unlabeled>"));
        return false;
    }
    return true;
}

bool TestHttpServer::respondIfTrackCallFailed(TrackCallResult r, httplib::Response& res, const char* notFoundMsg,
                                              const char* timeoutMsg)
{
    if (r == TrackCallResult::NotFound)
    {
        // Match HTTP semantics with the JSON body: a missing track is a 404,
        // mirroring the 504 we already set for Timeout. Status-only clients
        // (CI smoke probes, browser tooling) would otherwise read the 200
        // default as success.
        res.status = 404;
        res.set_content(errorResponse(notFoundMsg).dump(), "application/json");
        return true;
    }
    if (r == TrackCallResult::Timeout)
    {
        res.status = 504;
        res.set_content(errorResponse(timeoutMsg).dump(), "application/json");
        return true;
    }
    return false;
}

TestHttpServer::TrackCallResult TestHttpServer::runOnTrackSync(int trackId, std::function<void(TestTrack&)> fn,
                                                               int timeoutMs)
{
    // All helper-owned state lives on the heap via `state`. The MT lambda
    // captures `state` by value (shared_ptr), so even if this helper returns
    // on timeout, the lambda still finds `fn`, `result`, and `done` alive
    // when it finally runs. No use-after-free on helper internals.
    //
    // The ONLY remaining unsafe pattern is a handler that captures its own
    // stack into `fn` via `[&]`. Handlers in this codebase are required to
    // capture all lambda outputs via `std::shared_ptr<T>` (by value) so that
    // a timeout here — which does return Timeout and lets the handler
    // return — cannot leave the lambda writing into dead memory when it
    // eventually fires.
    struct State
    {
        std::function<void(TestTrack&)> fn;
        TrackCallResult result = TrackCallResult::NotFound;
        juce::WaitableEvent done;
    };
    auto state = std::make_shared<State>();
    state->fn = std::move(fn);

    juce::MessageManager::callAsync([this, trackId, state]() {
        if (auto* t = daw_.getTrack(trackId))
        {
            state->result = TrackCallResult::Ok;
            if (state->fn)
                state->fn(*t);
        }
        state->done.signal();
    });

    if (!state->done.wait(timeoutMs))
    {
        juce::Logger::writeToLog(juce::String("[runOnTrackSync] timeout after ") + juce::String(timeoutMs) +
                                 juce::String("ms waiting on MT for trackId=") + juce::String(trackId));
        return TrackCallResult::Timeout;
    }
    return state->result;
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

        // Construct + prepare the track on the message thread.  TestTrack
        // construction invokes prepareToPlay, which in turn runs plugin
        // init paths that assert message-thread affinity (MemoryBudget,
        // DecimatingCaptureBuffer, PluginProcessorState).
        auto index = std::make_shared<int>(-1);
        auto resolvedSourceId = std::make_shared<juce::String>();
        auto resolvedName = std::make_shared<juce::String>();
        auto done = std::make_shared<juce::WaitableEvent>();
        juce::MessageManager::callAsync([this, name, index, resolvedSourceId, resolvedName, done]() {
            *index = daw_.addTrack(juce::String(name));
            if (auto* t = daw_.getTrack(*index))
            {
                *resolvedSourceId = t->getProcessor().getSourceId().id;
                *resolvedName = t->getName();
            }
            done->signal();
        });
        if (!done->wait(5000))
        {
            res.status = 504;
            res.set_content(errorResponse("Timeout adding track").dump(), "application/json");
            return;
        }

        json data;
        data["trackIndex"] = *index;
        data["name"] = resolvedName->toStdString();
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

        // Destroy the track (and its plugin processor) on the message thread.
        // MemoryBudgetManager::unregisterBuffer asserts message-thread affinity;
        // destroying a TestTrack from the HTTP thread trips that assertion.
        auto removed = std::make_shared<bool>(false);
        auto done = std::make_shared<juce::WaitableEvent>();
        juce::MessageManager::callAsync([this, trackIndex, removed, done]() {
            *removed = daw_.removeTrack(trackIndex);
            done->signal();
        });
        if (!done->wait(5000))
        {
            res.status = 504;
            res.set_content(errorResponse("Timeout removing track").dump(), "application/json");
            return;
        }
        if (*removed)
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
    // Iterate tracks on the message thread so we never observe a slot being
    // reassigned by addTrack/removeTrack while we read it. Touching
    // TestTrack::sourceIdMutex_ from the HTTP worker thread without this
    // barrier produces pthread_mutex_lock(EINVAL) when the track is freed
    // mid-iteration.
    auto tracks = std::make_shared<json>(json::array());
    const bool ok = runOnMessageThreadBlocking(
        [this, tracks]() {
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
                tracks->push_back(t);
            }
        },
        5000, "handleDawTracks");
    // Without the ok check a timeout would return an empty or partial
    // track list under the "success": true shape — the callers cannot
    // distinguish that from "no tracks", and assertions downstream go
    // flaky. Mirror handleDawTrackAdd / handleDawTrackRemove and return
    // a clean timeout error instead.
    if (!ok)
    {
        res.status = 504;
        res.set_content(errorResponse("Timeout listing tracks").dump(), "application/json");
        return;
    }
    res.set_content(successResponse(*tracks).dump(), "application/json");
}

} // namespace oscil::test
