/*
    MultiScoper - Waveform Handler Implementation
*/

#include "tools/test_server/WaveformHandler.h"

#include "core/Oscillator.h"
#include "core/SharedCaptureBuffer.h"
#include "ui/layout/PaneComponent.h"
#include "ui/panels/WaveformComponent.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "tools/test_server/TestWaveformGenerator.h"

namespace multiscoper
{

namespace
{

nlohmann::json serializeWaveformInfo(WaveformComponent* waveform, const Oscillator* osc)
{
    waveform->forceUpdateWaveformData();

    nlohmann::json wfInfo;
    wfInfo["oscillatorId"] = osc->getId().id.toStdString();
    wfInfo["oscillatorName"] = osc->getName().toStdString();
    wfInfo["hasWaveformData"] = waveform->hasWaveformData();
    wfInfo["peakLevel"] = waveform->getPeakLevel();
    wfInfo["rmsLevel"] = waveform->getRMSLevel();
    wfInfo["showGrid"] = waveform->isGridVisible();
    wfInfo["autoScale"] = waveform->isAutoScaleEnabled();
    wfInfo["holdDisplay"] = waveform->isHoldDisplayEnabled();
    wfInfo["gainLinear"] = waveform->getGainLinear();
    wfInfo["opacity"] = waveform->getOpacity();
    wfInfo["displaySamples"] = waveform->getDisplaySamples();
    wfInfo["processingMode"] = static_cast<int>(waveform->getProcessingMode());

    auto colour = waveform->getColour();
    wfInfo["colour"] = {
        {"r", colour.getRed()}, {"g", colour.getGreen()}, {"b", colour.getBlue()}, {"a", colour.getAlpha()}};
    return wfInfo;
}

} // namespace

void WaveformHandler::handleInjectTestData(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body.empty() ? "{}" : req.body);
        std::string const waveformType = body.value("type", "sine");
        if (!isValidWaveformType(waveformType))
        {
            // Reject typos explicitly — generateTestWaveform silently maps
            // unknown types to silence, and the success response echoes the
            // caller's string. Without this guard a test sending "squar" or
            // "triangle_" would receive `status: ok` and zero-filled data.
            sendJson(res, {{"error", "unknown waveform type: " + waveformType}}, 400);
            return;
        }
        float const frequency = body.value("frequency", 440.0f);
        float const amplitude = body.value("amplitude", 0.8f);
        int const numSamples = body.value("samples", 4096);
        if (numSamples <= 0 || numSamples > 1048576)
        {
            sendJson(res, {{"error", "samples must be between 1 and 1048576"}}, 400);
            return;
        }
        float sampleRate = body.value("sampleRate", 44100.0f);
        if (sampleRate <= 0.0f)
            sampleRate = 44100.0f;

        auto result = runOnMessageThread([this, waveformType, frequency, amplitude, numSamples,
                                          sampleRate]() -> nlohmann::json { // NOLINT(bugprone-exception-escape)
            nlohmann::json response;

            auto captureBuffer = editor_.getProcessor().getCaptureBuffer();
            if (!captureBuffer)
            {
                response["error"] = "No capture buffer available";
                return response;
            }

            juce::AudioBuffer<float> testBuffer(2, numSamples);
            generateTestWaveform(testBuffer, waveformType, frequency, amplitude, sampleRate);

            auto metadata = makeTestMetadata(sampleRate, numSamples);

            captureBuffer->write(testBuffer, metadata, false);
            editor_.repaint();

            response["status"] = "ok";
            response["waveformType"] = waveformType;
            response["frequency"] = frequency;
            response["amplitude"] = amplitude;
            response["samplesInjected"] = numSamples;
            response["sampleRate"] = sampleRate;
            return response;
        });

        sendJson(res, result);
    }
    catch (const std::exception& e)
    {
        sendJson(res, jsonError(e.what()), 400);
    }
}

void WaveformHandler::handleGetWaveformState(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json panes = nlohmann::json::array();

        const auto& paneComponents = editor_.getPaneComponents();
        for (size_t paneIdx = 0; paneIdx < paneComponents.size(); ++paneIdx)
        {
            auto* pane = paneComponents[paneIdx].get();
            if (!pane)
                continue;

            nlohmann::json paneInfo;
            paneInfo["index"] = paneIdx;
            paneInfo["paneId"] = pane->getPaneId().id.toStdString();
            paneInfo["oscillatorCount"] = pane->getOscillatorCount();

            nlohmann::json waveforms = nlohmann::json::array();
            for (size_t wfIdx = 0; wfIdx < pane->getOscillatorCount(); ++wfIdx)
            {
                auto* waveform = pane->getWaveformAt(wfIdx);
                const auto* osc = pane->getOscillatorAt(wfIdx);
                if (waveform && osc)
                    waveforms.push_back(serializeWaveformInfo(waveform, osc));
            }

            paneInfo["waveforms"] = waveforms;
            panes.push_back(paneInfo);
        }

        response["panes"] = panes;
        response["totalPanes"] = paneComponents.size();
        return response;
    });

    sendJson(res, result);
}

} // namespace multiscoper
