/*
    MultiScoper Test Harness - HTTP Server: Waveform State & Oscillator Delete Handlers

    Provides data-level waveform verification endpoints that read capture buffer
    statistics (peak, RMS, available samples) without pixel analysis. This enables
    deterministic, numeric assertions about audio data flow and rendering state.
*/

#include "core/MultiScoperState.h"
#include "core/dsp/TimingEngine.h"
#include "core/interfaces/IAudioBuffer.h"

#include "TestHttpServer.h"

#include <algorithm>

namespace multiscoper::test
{

void TestHttpServer::handleStateDeleteOscillator(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        const auto trackIdOpt = tryResolveTrackId(req, res);
        if (!trackIdOpt)
            return;
        const int trackId = *trackIdOpt;
        auto body = json::parse(req.body);
        const std::string idStr = body.value("oscillatorId", "");
        if (idStr.empty())
        {
            res.set_content(errorResponse("Oscillator 'oscillatorId' is required").dump(), "application/json");
            return;
        }

        auto oscId = std::make_shared<OscillatorId>(OscillatorId{juce::String(idStr)});
        auto oscMissing = std::make_shared<bool>(false);

        juce::Logger::writeToLog("[Harness] Deleting oscillator: " + juce::String(idStr));
        const auto result = runOnTrackSync(
            trackId,
            [oscId, oscMissing](TestTrack& track) {
                auto& state = track.getProcessor().getState();
                if (!state.getOscillator(*oscId).has_value())
                {
                    *oscMissing = true;
                    return;
                }
                state.removeOscillator(*oscId);
            },
            5000);

        if (respondIfTrackCallFailed(result, res, "No track available", "Timeout deleting oscillator"))
            return;
        if (*oscMissing)
        {
            res.set_content(errorResponse("Oscillator not found: " + idStr).dump(), "application/json");
            return;
        }

        json data;
        data["id"] = idStr;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

namespace
{

json buildWaveformJson(const Oscillator& osc, MultiScoperPluginProcessor& processor, int displaySamples)
{
    json wfJson;
    wfJson["oscillatorId"] = osc.getId().id.toStdString();
    wfJson["name"] = osc.getName().toStdString();
    wfJson["visible"] = osc.isVisible();
    wfJson["displaySamples"] = displaySamples;

    auto buffer = processor.getBuffer(osc.getSourceId());
    if (buffer)
    {
        size_t available = buffer->getAvailableSamples();
        float peakL = buffer->getPeakLevel(0, 1024);
        float peakR = buffer->getPeakLevel(1, 1024);
        float rmsL = buffer->getRMSLevel(0, 1024);
        float rmsR = buffer->getRMSLevel(1, 1024);
        float peak = std::max(peakL, peakR);
        float rms = std::max(rmsL, rmsR);

        wfJson["hasWaveformData"] = available > 0 && peak > 0.0001f;
        wfJson["availableSamples"] = static_cast<int64_t>(available);
        wfJson["peakLevel"] = peak;
        wfJson["rmsLevel"] = rms;
        wfJson["peakLevelLeft"] = peakL;
        wfJson["peakLevelRight"] = peakR;
        wfJson["rmsLevelLeft"] = rmsL;
        wfJson["rmsLevelRight"] = rmsR;
    }
    else
    {
        wfJson["hasWaveformData"] = false;
        wfJson["availableSamples"] = 0;
        wfJson["peakLevel"] = 0.0f;
        wfJson["rmsLevel"] = 0.0f;
    }

    return wfJson;
}

} // anonymous namespace

void TestHttpServer::handleWaveformState(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        const auto trackIdOpt = tryResolveTrackId(req, res);
        if (!trackIdOpt)
            return;
        const int trackId = *trackIdOpt;
        auto data = std::make_shared<json>();

        const auto result = runOnTrackSync(
            trackId,
            [data](TestTrack& track) {
                auto& processor = track.getProcessor();
                auto& state = processor.getState();
                auto& timingEngine = processor.getTimingEngine();
                double sampleRate = processor.getSampleRate();
                int displaySamples = timingEngine.getDisplaySampleCount(sampleRate);

                auto oscillators = state.getOscillators();
                auto panes = state.getLayoutManager().getPanes();

                json panesJson = json::array();
                for (const auto& pane : panes)
                {
                    json paneJson;
                    paneJson["id"] = pane.getId().id.toStdString();
                    paneJson["name"] = pane.getName().toStdString();

                    json waveformsJson = json::array();
                    int oscCount = 0;

                    for (const auto& osc : oscillators)
                    {
                        if (osc.getPaneId() != pane.getId())
                            continue;
                        oscCount++;
                        waveformsJson.push_back(buildWaveformJson(osc, processor, displaySamples));
                    }

                    paneJson["oscillatorCount"] = oscCount;
                    paneJson["waveforms"] = waveformsJson;
                    panesJson.push_back(paneJson);
                }

                (*data)["panes"] = panesJson;
                (*data)["displaySamples"] = displaySamples;
                (*data)["sampleRate"] = sampleRate;
                (*data)["oscillatorCount"] = static_cast<int>(oscillators.size());
                (*data)["paneCount"] = static_cast<int>(panes.size());
            },
            5000);

        if (respondIfTrackCallFailed(result, res, "No track available", "Timeout reading waveform state"))
            return;

        res.set_content(successResponse(*data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

} // namespace multiscoper::test
