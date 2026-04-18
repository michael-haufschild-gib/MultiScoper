/*
    Oscil Test Harness - HTTP Server: Transport & Track Handlers
*/

#include "TestElementRegistry.h"
#include "TestHttpServer.h"
#include "plugin/PluginEditor.h"

namespace oscil::test
{

void TestHttpServer::setupTransportRoutes()
{
    server_->Post("/transport/play",
                  [this](const httplib::Request& req, httplib::Response& res) { handleTransportPlay(req, res); });
    server_->Post("/transport/stop",
                  [this](const httplib::Request& req, httplib::Response& res) { handleTransportStop(req, res); });
    server_->Post("/transport/setBpm",
                  [this](const httplib::Request& req, httplib::Response& res) { handleTransportSetBpm(req, res); });
    server_->Post("/transport/setPosition", [this](const httplib::Request& req, httplib::Response& res) {
        handleTransportSetPosition(req, res);
    });
    server_->Get("/transport/state",
                 [this](const httplib::Request& req, httplib::Response& res) { handleTransportState(req, res); });
}

void TestHttpServer::setupTrackRoutes()
{
    server_->Post(R"(/track/(\d+)/audio)",
                  [this](const httplib::Request& req, httplib::Response& res) { handleTrackAudio(req, res); });
    server_->Post(R"(/track/(\d+)/burst)",
                  [this](const httplib::Request& req, httplib::Response& res) { handleTrackBurst(req, res); });
    server_->Get(R"(/track/(\d+)/info)",
                 [this](const httplib::Request& req, httplib::Response& res) { handleTrackInfo(req, res); });
    server_->Post(R"(/track/(\d+)/showEditor)",
                  [this](const httplib::Request& req, httplib::Response& res) { handleTrackShowEditor(req, res); });
    server_->Post(R"(/track/(\d+)/hideEditor)",
                  [this](const httplib::Request& req, httplib::Response& res) { handleTrackHideEditor(req, res); });
}

// Transport handlers

void TestHttpServer::handleTransportPlay(const httplib::Request&, httplib::Response& res)
{
    juce::Logger::writeToLog("[Harness] Transport play");
    // Only toggle transport state — do NOT call daw_.start()/stop() which
    // controls the timer-driven audio processing loop.  In a real DAW,
    // stopping transport only pauses position advancement; processBlock
    // keeps running.
    daw_.getTransport().play();
    // Ensure the DAW timer is running (it should be, but guard against
    // earlier bugs that may have stopped it).
    if (!daw_.isRunning())
        daw_.start();
    res.set_content(successResponse().dump(), "application/json");
}

void TestHttpServer::handleTransportStop(const httplib::Request&, httplib::Response& res)
{
    juce::Logger::writeToLog("[Harness] Transport stop");
    // Only pause transport — keep DAW audio processing running.
    daw_.getTransport().stop();
    res.set_content(successResponse().dump(), "application/json");
}

void TestHttpServer::handleTransportSetBpm(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        double bpm = body.value("bpm", 120.0);

        // Fold the transport mutation + per-track refresh into a single
        // MT-dispatched block so a timeout or missing track surfaces as an
        // HTTP error instead of leaving the global transport BPM updated
        // while the timing-engine/editor refresh never happened.
        const auto trackIdOpt = tryResolveTrackId(req, res);
        if (!trackIdOpt)
            return;
        const int trackId = *trackIdOpt;
        const auto result = runOnTrackSync(trackId, [this, bpm](TestTrack& track) {
            daw_.getTransport().setBpm(bpm);
            auto& timingEngine = track.getProcessor().getTimingEngine();
            timingEngine.setInternalBPM(static_cast<float>(bpm));
            daw_.runSingleBlockSynchronously(track);
            if (auto* editor = track.getEditor())
            {
                if (auto* oscilEditor = dynamic_cast<OscilPluginEditor*>(editor))
                    oscilEditor->refreshPanels();
            }
        });

        if (respondIfTrackCallFailed(result, res, "Track not found", "Timeout updating BPM"))
            return;

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
        auto body = json::parse(req.body);
        std::string waveform = body.value("waveform", "sine");
        float frequency = body.value("frequency", 440.0f);
        float amplitude = body.value("amplitude", 0.8f);

        const auto result = runOnTrackSync(trackId, [waveform, frequency, amplitude](TestTrack& track) {
            track.getAudioGenerator().setWaveform(TestAudioGenerator::stringToWaveform(waveform));
            track.getAudioGenerator().setFrequency(frequency);
            track.getAudioGenerator().setAmplitude(amplitude);
        });

        if (respondIfTrackCallFailed(result, res, "Track not found", "Timeout updating track audio"))
            return;
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
        auto body = json::parse(req.body);
        int samples = body.value("samples", 4410);
        std::string waveform = body.value("waveform", "");

        const auto result = runOnTrackSync(trackId, [samples, waveform](TestTrack& track) {
            if (!waveform.empty())
                track.getAudioGenerator().setWaveform(TestAudioGenerator::stringToWaveform(waveform));
            track.getAudioGenerator().setBurstSamples(samples);
        });

        if (respondIfTrackCallFailed(result, res, "Track not found", "Timeout scheduling burst"))
            return;
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

        auto data = std::make_shared<json>();
        const auto result = runOnTrackSync(trackId, [data](TestTrack& track) {
            (*data)["index"] = track.getTrackIndex();
            (*data)["name"] = track.getName().toStdString();
            (*data)["sourceId"] = track.getSourceId().id.toStdString();
            (*data)["waveform"] =
                TestAudioGenerator::waveformToString(track.getAudioGenerator().getWaveform()).toStdString();
            (*data)["frequency"] = track.getAudioGenerator().getFrequency();
            (*data)["amplitude"] = track.getAudioGenerator().getAmplitude();
            (*data)["generating"] = track.getAudioGenerator().isGenerating();
        });

        if (respondIfTrackCallFailed(result, res, "Track not found", "Timeout reading track info"))
            return;
        res.set_content(successResponse(*data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleTrackShowEditor(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        int trackId = std::stoi(req.matches[1]);
        juce::Logger::writeToLog("[Harness] Opening editor for track " + juce::String(trackId));

        auto alreadyOpen = std::make_shared<bool>(false);
        auto editorVisible = std::make_shared<bool>(false);

        const auto result = runOnTrackSync(
            trackId,
            [this, trackId, alreadyOpen, editorVisible](TestTrack& track) {
                if (track.isEditorVisible())
                {
                    *alreadyOpen = true;
                    *editorVisible = true;
                    return;
                }
                // Route through the DAW so showEditor stays serialized with
                // the audio thread pool (see TestDAW::showTrackEditor for
                // the dispatch-mutex rationale). We're already on the MT so
                // another getTrack inside is consistent with our lookup.
                daw_.showTrackEditor(trackId);
                *editorVisible = track.isEditorVisible();
            },
            5000);

        if (respondIfTrackCallFailed(result, res, "Track not found", "Timeout opening editor"))
            return;

        // TestElementRegistry is process-lifetime and internally locked, so
        // reading from the HTTP thread after the MT hop is safe.
        int elemCount = static_cast<int>(TestElementRegistry::getInstance().getAllElements().size());
        json data;
        data["trackId"] = trackId;
        data["editorVisible"] = *editorVisible;
        data["elementsRegistered"] = elemCount;
        data["alreadyOpen"] = *alreadyOpen;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleTrackHideEditor(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        int trackId = std::stoi(req.matches[1]);

        auto editorVisible = std::make_shared<bool>(false);
        const auto result = runOnTrackSync(
            trackId,
            [editorVisible](TestTrack& track) {
                track.hideEditor();
                *editorVisible = track.isEditorVisible();
            },
            5000);

        if (respondIfTrackCallFailed(result, res, "Track not found", "Timeout hiding editor"))
            return;

        json data;
        data["trackId"] = trackId;
        data["editorVisible"] = *editorVisible;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

} // namespace oscil::test
