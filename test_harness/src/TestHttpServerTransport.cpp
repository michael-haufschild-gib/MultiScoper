/*
    Oscil Test Harness - HTTP Server: Transport & Track Handlers
*/

#include "TestHttpServer.h"
#include "TestElementRegistry.h"

namespace oscil::test
{

void TestHttpServer::setupTransportRoutes()
{
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
}

void TestHttpServer::setupTrackRoutes()
{
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

void TestHttpServer::handleTrackShowEditor(const httplib::Request& req, httplib::Response& res)
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

        if (track->isEditorVisible())
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

        juce::MessageManager::callAsync([this, trackId]() {
            daw_.showTrackEditor(trackId);
        });

        json data;
        data["trackId"] = trackId;
        data["editorVisible"] = false;
        data["elementsRegistered"] = 0;
        data["async"] = true;
        data["message"] = "Editor opening requested. Poll /ui/elements or use /ui/wait/element/{id} to wait for UI.";
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
        auto* track = daw_.getTrack(trackId);

        if (track == nullptr)
        {
            res.set_content(errorResponse("Track not found").dump(), "application/json");
            return;
        }

        juce::MessageManager::callAsync([track]() {
            track->hideEditor();
        });

        json data;
        data["trackId"] = trackId;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

} // namespace oscil::test
