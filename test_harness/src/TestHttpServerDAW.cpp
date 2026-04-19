/*
    MultiScoper Test Harness - HTTP Server: DAW host-lifecycle simulation

    Endpoints defined here close the gap between the fixed-sample-rate /
    fixed-buffer-size test harness and the mutable lifecycle real DAWs
    drive.  See docs/ci_reports/harness_capability_gaps.md (gaps #1-4, #6,
    #8) and coverage_gap_priority.md (P0 rows: SR change, buffer change,
    bus renegotiation, audio-thread prepare, editor detach/reattach,
    scanner cycle) for the production motivation.

    Threading contract
    ------------------
    Every handler that mutates plugin state hops onto the JUCE message
    thread with juce::MessageManager::callAsync + juce::WaitableEvent
    before touching TestDAW / TestTrack.  The HTTP worker thread is an
    arbitrary std::thread; calling prepareToPlay, setBusesLayout, or
    TestTrack window ops from it trips message-thread-affinity assertions
    in MemoryBudgetManager / DecimatingCaptureBuffer / PluginProcessorState
    and can corrupt the editor hierarchy.

    The one exception is the audio-thread prepare simulation: when that
    mode is armed, the TestDAW implementation itself reposts the prepare
    work onto a juce::ThreadPool worker — the HTTP handler still talks to
    the message thread first, which is exactly how a real DAW signals the
    host before letting its own audio thread handle the transition.
*/

#include "TestDAW.h"
#include "TestHttpServer.h"

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <memory>
#include <string>

namespace multiscoper::test
{

void TestHttpServer::setupDawLifecycleRoutes()
{
    server_->Post("/daw/sampleRate",
                  [this](const httplib::Request& req, httplib::Response& res) { handleDawSetSampleRate(req, res); });
    server_->Post("/daw/bufferSize",
                  [this](const httplib::Request& req, httplib::Response& res) { handleDawSetBufferSize(req, res); });
    server_->Post("/daw/audioThreadPrepare", [this](const httplib::Request& req, httplib::Response& res) {
        handleDawSetAudioThreadPrepare(req, res);
    });
    server_->Post("/daw/scanCycle",
                  [this](const httplib::Request& req, httplib::Response& res) { handleDawScanCycle(req, res); });

    server_->Post(R"(/track/(\d+)/channelLayout)",
                  [this](const httplib::Request& req, httplib::Response& res) { handleTrackChannelLayout(req, res); });
    server_->Post(R"(/track/(\d+)/detachEditor)",
                  [this](const httplib::Request& req, httplib::Response& res) { handleTrackDetachEditor(req, res); });
    server_->Post(R"(/track/(\d+)/reattachEditor)",
                  [this](const httplib::Request& req, httplib::Response& res) { handleTrackReattachEditor(req, res); });
    server_->Post("/daw/isolatedRegistries", [this](const httplib::Request& req, httplib::Response& res) {
        handleDawSetIsolatedRegistries(req, res);
    });
    server_->Get(R"(/track/(\d+)/sources)",
                 [this](const httplib::Request& req, httplib::Response& res) { handleTrackSources(req, res); });
}

// ---------------------------------------------------------------------
//  Sample-rate change mid-session
// ---------------------------------------------------------------------
void TestHttpServer::handleDawSetSampleRate(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        if (!body.contains("rate"))
        {
            res.status = 400;
            res.set_content(errorResponse("Missing 'rate'").dump(), "application/json");
            return;
        }
        double rate = body["rate"].get<double>();

        auto ok = std::make_shared<bool>(false);
        auto done = std::make_shared<juce::WaitableEvent>();
        juce::MessageManager::callAsync([this, rate, ok, done]() {
            *ok = daw_.setSampleRate(rate);
            done->signal();
        });
        if (!done->wait(10000))
        {
            res.status = 500;
            res.set_content(errorResponse("Timeout setting sample rate").dump(), "application/json");
            return;
        }
        if (!*ok)
        {
            // setSampleRate returns false for non-finite/non-positive input OR
            // for a pool-dispatched prepare timeout under audioThreadPrepareMode.
            // 400 is the right status for input errors; 504 for timeouts. The
            // two branches are indistinguishable from here, so use 409 Conflict
            // which covers "accepted but could not apply" on either path.
            res.status = 409;
            res.set_content(errorResponse("sample rate rejected or prepare timed out: " + std::to_string(rate)).dump(),
                            "application/json");
            return;
        }

        json data;
        data["rate"] = daw_.getSampleRate();
        data["audioThreadPrepare"] = daw_.isAudioThreadPrepareMode();
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// ---------------------------------------------------------------------
//  Buffer-size change mid-session
// ---------------------------------------------------------------------
void TestHttpServer::handleDawSetBufferSize(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        if (!body.contains("size"))
        {
            res.status = 400;
            res.set_content(errorResponse("Missing 'size'").dump(), "application/json");
            return;
        }
        int size = body["size"].get<int>();

        auto ok = std::make_shared<bool>(false);
        auto done = std::make_shared<juce::WaitableEvent>();
        juce::MessageManager::callAsync([this, size, ok, done]() {
            *ok = daw_.setBufferSize(size);
            done->signal();
        });
        if (!done->wait(10000))
        {
            res.status = 500;
            res.set_content(errorResponse("Timeout setting buffer size").dump(), "application/json");
            return;
        }
        if (!*ok)
        {
            // Same rationale as setSampleRate: false covers both input-range
            // rejection and pool-prepare timeout. 409 Conflict covers both.
            res.status = 409;
            res.set_content(errorResponse("buffer size rejected or prepare timed out: " + std::to_string(size)).dump(),
                            "application/json");
            return;
        }

        json data;
        data["size"] = daw_.getBufferSize();
        data["audioThreadPrepare"] = daw_.isAudioThreadPrepareMode();
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// ---------------------------------------------------------------------
//  Audio-thread prepare mode toggle (Pro Tools / Reaper contract)
// ---------------------------------------------------------------------
void TestHttpServer::handleDawSetAudioThreadPrepare(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        if (!body.contains("enabled"))
        {
            res.status = 400;
            res.set_content(errorResponse("Missing 'enabled'").dump(), "application/json");
            return;
        }
        bool enabled = body["enabled"].get<bool>();

        // The flag is an atomic — safe to set from any thread.  Still hop
        // through the message thread for consistency with the other
        // lifecycle endpoints so tests see a single serialisation point.
        auto done = std::make_shared<juce::WaitableEvent>();
        juce::MessageManager::callAsync([this, enabled, done]() {
            daw_.setAudioThreadPrepareMode(enabled);
            done->signal();
        });
        done->wait(2000);

        json data;
        data["enabled"] = daw_.isAudioThreadPrepareMode();
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// ---------------------------------------------------------------------
//  Plugin-scanner cycle (construct → probe → destroy, repeat)
// ---------------------------------------------------------------------
void TestHttpServer::handleDawScanCycle(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        int cycles = body.value("cycles", 1);
        std::string name = body.value("name", "ScanTrack");
        if (cycles <= 0 || cycles > 10000)
        {
            res.status = 400;
            res.set_content(errorResponse("'cycles' must be in [1, 10000]").dump(), "application/json");
            return;
        }

        auto completed = std::make_shared<int>(0);
        auto done = std::make_shared<juce::WaitableEvent>();
        juce::MessageManager::callAsync([this, cycles, name, completed, done]() {
            *completed = daw_.scanCycle(juce::String(name), cycles);
            done->signal();
        });
        // Cycles can be expensive — scale generously.  Construct/destroy
        // of MultiScoperPluginProcessor, InstanceRegistry register/unregister,
        // and the capture buffer allocation add up to roughly 50-60 ms per
        // cycle on a warm debug build plus thread-scheduling jitter, so
        // 100 ms per cycle + 2 s base is the minimum slack that keeps
        // tight-loop tests reliable.
        const int timeoutMs = std::min(120000, 2000 + cycles * 100);
        if (!done->wait(timeoutMs))
        {
            res.status = 500;
            res.set_content(errorResponse("Timeout during scanCycle").dump(), "application/json");
            return;
        }

        json data;
        data["requested"] = cycles;
        data["completed"] = *completed;
        data["trackCount"] = daw_.getActiveTrackCount();
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// ---------------------------------------------------------------------
//  Per-track channel layout renegotiation (stereo <-> mono)
// ---------------------------------------------------------------------
void TestHttpServer::handleTrackChannelLayout(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        int trackId = std::stoi(req.matches[1]);
        auto body = json::parse(req.body);
        std::string layout = body.value("layout", "");
        if (layout != "mono" && layout != "stereo")
        {
            res.status = 400;
            res.set_content(errorResponse("'layout' must be 'mono' or 'stereo'").dump(), "application/json");
            return;
        }

        auto ok = std::make_shared<bool>(false);
        auto inputChannels = std::make_shared<int>(0);
        auto outputChannels = std::make_shared<int>(0);
        auto done = std::make_shared<juce::WaitableEvent>();
        juce::String layoutStr(layout);
        juce::MessageManager::callAsync([this, trackId, layoutStr, ok, inputChannels, outputChannels, done]() {
            *ok = daw_.setChannelLayout(trackId, layoutStr);
            // Read channel counts inside the same MT lambda so the track
            // cannot be destroyed between the layout change and our read.
            if (auto* t = daw_.getTrack(trackId))
            {
                *inputChannels = t->getProcessor().getTotalNumInputChannels();
                *outputChannels = t->getProcessor().getTotalNumOutputChannels();
            }
            done->signal();
        });
        if (!done->wait(5000))
        {
            res.status = 500;
            res.set_content(errorResponse("Timeout setting channel layout").dump(), "application/json");
            return;
        }

        if (!*ok)
        {
            res.status = 400;
            res.set_content(errorResponse("Layout rejected (processor refused or track missing)").dump(),
                            "application/json");
            return;
        }

        json data;
        data["trackId"] = trackId;
        data["layout"] = layout;
        data["inputChannels"] = *inputChannels;
        data["outputChannels"] = *outputChannels;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// ---------------------------------------------------------------------
//  Editor detach / reattach without destroying the processor
// ---------------------------------------------------------------------
void TestHttpServer::handleTrackDetachEditor(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        int trackId = std::stoi(req.matches[1]);

        auto ok = std::make_shared<bool>(false);
        auto editorVisible = std::make_shared<bool>(false);
        auto done = std::make_shared<juce::WaitableEvent>();
        juce::MessageManager::callAsync([this, trackId, ok, editorVisible, done]() {
            if (auto* t = daw_.getTrack(trackId))
            {
                *ok = t->detachEditor();
                *editorVisible = t->isEditorVisible();
            }
            done->signal();
        });
        if (!done->wait(5000))
        {
            res.status = 500;
            res.set_content(errorResponse("Timeout detaching editor").dump(), "application/json");
            return;
        }

        json data;
        data["trackId"] = trackId;
        data["detached"] = *ok;
        data["editorVisible"] = *editorVisible;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleTrackReattachEditor(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        int trackId = std::stoi(req.matches[1]);

        auto ok = std::make_shared<bool>(false);
        auto editorVisible = std::make_shared<bool>(false);
        auto done = std::make_shared<juce::WaitableEvent>();
        juce::MessageManager::callAsync([this, trackId, ok, editorVisible, done]() {
            if (auto* t = daw_.getTrack(trackId))
            {
                *ok = t->reattachEditor();
                *editorVisible = t->isEditorVisible();
            }
            done->signal();
        });
        if (!done->wait(5000))
        {
            res.status = 500;
            res.set_content(errorResponse("Timeout reattaching editor").dump(), "application/json");
            return;
        }

        json data;
        data["trackId"] = trackId;
        data["reattached"] = *ok;
        data["editorVisible"] = *editorVisible;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// ---------------------------------------------------------------------
//  Per-track InstanceRegistry isolation (AU-sandbox simulation)
//
//  When enabled, subsequent addTrack calls give the new track its own
//  InstanceRegistry instead of the factory singleton's shared one. This
//  reproduces Logic Pro's AU sandbox model where each plugin instance
//  runs in a separate process and does NOT share registry state with
//  other instances.  Without this mode, the in-process harness cannot
//  fail cross-instance-discovery regressions — the shared singleton
//  masks them.
// ---------------------------------------------------------------------
void TestHttpServer::handleDawSetIsolatedRegistries(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        if (!body.contains("enabled"))
        {
            res.status = 400;
            res.set_content(errorResponse("Missing 'enabled'").dump(), "application/json");
            return;
        }
        bool enabled = body["enabled"].get<bool>();
        daw_.setIsolatedRegistriesEnabled(enabled);

        json data;
        data["isolatedRegistries"] = daw_.isIsolatedRegistriesEnabled();
        data["note"] = "Applies to tracks added after this call; existing tracks retain their registry.";
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// GET /track/{id}/sources — returns the sources visible to THIS track's
// registry, distinct from /state/sources which always reports the factory
// singleton registry. Under isolation mode each track sees only its own
// source; without isolation every track sees every registered source.
void TestHttpServer::handleTrackSources(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        int trackId = std::stoi(req.matches[1]);

        auto sourcesJson = std::make_shared<json>(json::array());
        auto found = std::make_shared<bool>(false);
        auto done = std::make_shared<juce::WaitableEvent>();
        juce::MessageManager::callAsync([this, trackId, sourcesJson, found, done]() {
            if (auto* t = daw_.getTrack(trackId))
            {
                *found = true;
                auto all = t->getInstanceRegistry().getAllSources();
                for (auto const& s : all)
                {
                    json entry;
                    entry["id"] = s.sourceId.id.toStdString();
                    entry["name"] = s.name.toStdString();
                    entry["channelCount"] = s.channelCount;
                    entry["sampleRate"] = s.sampleRate;
                    sourcesJson->push_back(entry);
                }
            }
            done->signal();
        });
        if (!done->wait(3000))
        {
            res.status = 500;
            res.set_content(errorResponse("Timeout reading track sources").dump(), "application/json");
            return;
        }
        if (!*found)
        {
            res.status = 404;
            res.set_content(errorResponse("Track not found").dump(), "application/json");
            return;
        }

        res.set_content(successResponse(*sourcesJson).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.status = 500;
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

} // namespace multiscoper::test
