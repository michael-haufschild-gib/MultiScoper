/*
    Oscil - Waveform Handler Implementation
*/

#include "tools/test_server/WaveformHandler.h"

#include "core/Oscillator.h"
#include "core/SharedCaptureBuffer.h"
#include "ui/layout/PaneComponent.h"
#include "ui/panels/WaveformComponent.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"

#include <cmath>

namespace oscil
{

namespace
{

void generateTestWaveform(juce::AudioBuffer<float>& buffer, const std::string& waveformType, float frequency,
                          float amplitude, float sampleRate)
{
    int numSamples = buffer.getNumSamples();
    float phase = 0.0f;
    float phaseIncrement = (2.0f * juce::MathConstants<float>::pi * frequency) / sampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;
        if (waveformType == "sine")
            sample = std::sin(phase) * amplitude;
        else if (waveformType == "square")
            sample = (std::sin(phase) > 0.0f ? 1.0f : -1.0f) * amplitude;
        else if (waveformType == "triangle")
        {
            float t = phase / (2.0f * juce::MathConstants<float>::pi);
            sample = (2.0f * std::abs(2.0f * (t - std::floor(t + 0.5f))) - 1.0f) * amplitude;
        }
        else if (waveformType == "sawtooth")
        {
            float t = phase / (2.0f * juce::MathConstants<float>::pi);
            sample = (2.0f * (t - std::floor(t + 0.5f))) * amplitude;
        }
        else if (waveformType == "noise")
            sample = ((std::rand() / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f) * amplitude;
        else if (waveformType == "dc")
            sample = amplitude;
        // "silence" or unknown → sample remains 0.0f

        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample * 0.8f);
        phase += phaseIncrement;
        if (phase > 2.0f * juce::MathConstants<float>::pi)
            phase -= 2.0f * juce::MathConstants<float>::pi;
    }
}

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
        std::string waveformType = body.value("type", "sine");
        float frequency = body.value("frequency", 440.0f);
        float amplitude = body.value("amplitude", 0.8f);
        int numSamples = body.value("samples", 4096);
        float sampleRate = body.value("sampleRate", 44100.0f);
        if (sampleRate <= 0.0f)
            sampleRate = 44100.0f;

        auto result =
            runOnMessageThread([this, waveformType, frequency, amplitude, numSamples, sampleRate]() -> nlohmann::json {
                nlohmann::json response;

                auto captureBuffer = editor_.getProcessor().getCaptureBuffer();
                if (!captureBuffer)
                {
                    response["error"] = "No capture buffer available";
                    return response;
                }

                juce::AudioBuffer<float> testBuffer(2, numSamples);
                generateTestWaveform(testBuffer, waveformType, frequency, amplitude, sampleRate);

                CaptureFrameMetadata metadata;
                metadata.sampleRate = sampleRate;
                metadata.numChannels = 2;
                metadata.numSamples = numSamples;
                metadata.isPlaying = true;
                metadata.bpm = 120.0f;
                metadata.timestamp = 0;

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

        res.set_content(result.dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        nlohmann::json error;
        error["error"] = e.what();
        res.status = 400;
        res.set_content(error.dump(), "application/json");
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
                auto* osc = pane->getOscillatorAt(wfIdx);
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

    res.set_content(result.dump(), "application/json");
}

} // namespace oscil
