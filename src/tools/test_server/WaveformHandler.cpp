/*
    Oscil - Waveform Handler Implementation
*/

#include "tools/test_server/WaveformHandler.h"
#include "plugin/PluginEditor.h"
#include "ui/layout/PaneComponent.h"
#include "ui/panels/WaveformComponent.h"
#include "plugin/PluginProcessor.h"
#include "core/SharedCaptureBuffer.h"
#include <cmath>
#include <random>

namespace oscil
{

void WaveformHandler::handleInjectTestData(const httplib::Request& req, httplib::Response& res)
{
    // Parse JSON body with error handling
    nlohmann::json body;
    try
    {
        body = nlohmann::json::parse(req.body.empty() ? "{}" : req.body);
    }
    catch (const nlohmann::json::parse_error& e)
    {
        sendJson(res, jsonError("Invalid JSON: " + std::string(e.what())), 400);
        return;
    }

    try
    {
        std::string waveformType = body.value("type", "sine");
        float frequency = body.value("frequency", 440.0f);
        float amplitude = body.value("amplitude", 0.8f);
        int numSamples = body.value("samples", 4096);
        float sampleRate = body.value("sampleRate", 44100.0f);
        // Guard against invalid sample rate from request body
        if (sampleRate <= 0.0f)
            sampleRate = 44100.0f;

        auto result = runOnMessageThread([this, waveformType, frequency, amplitude, numSamples, sampleRate]() -> std::pair<nlohmann::json, int> {
            nlohmann::json response;

            // Get DecimatingCaptureBuffer to test the actual lock-free SIMD path
            auto captureBuffer = editor_.getProcessor().getDecimatingCaptureBuffer();
            if (!captureBuffer)
            {
                response["error"] = "No capture buffer available";
                return {response, 500};
            }

            // Generate test waveform
            juce::AudioBuffer<float> testBuffer(2, numSamples);
            float phase = 0.0f;
            float phaseIncrement = (2.0f * juce::MathConstants<float>::pi * frequency) / sampleRate;

            for (int i = 0; i < numSamples; ++i)
            {
                float sample = 0.0f;

                if (waveformType == "sine")
                {
                    sample = std::sin(phase) * amplitude;
                }
                else if (waveformType == "square")
                {
                    sample = (std::sin(phase) > 0.0f ? 1.0f : -1.0f) * amplitude;
                }
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
                {
                    // Use thread-local RNG for thread safety
                    static thread_local std::mt19937 rng(std::random_device{}());
                    static thread_local std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
                    sample = dist(rng) * amplitude;
                }
                else if (waveformType == "dc")
                {
                    sample = amplitude;
                }
                else if (waveformType == "silence")
                {
                    sample = 0.0f;
                }

                testBuffer.setSample(0, i, sample);  // Left channel
                testBuffer.setSample(1, i, sample * 0.8f);  // Right channel (slightly lower for testing)

                phase += phaseIncrement;
                if (phase > 2.0f * juce::MathConstants<float>::pi)
                    phase -= 2.0f * juce::MathConstants<float>::pi;
            }

            // Write to capture buffer
            CaptureFrameMetadata metadata;
            metadata.sampleRate = sampleRate;
            metadata.numChannels = 2;
            metadata.numSamples = numSamples;
            metadata.isPlaying = true;
            metadata.bpm = 120.0f;
            metadata.timestamp = 0;

            // Write through DecimatingCaptureBuffer - tests the actual lock-free SIMD path
            captureBuffer->write(testBuffer, metadata);

            // Force UI update
            editor_.repaint();

            response["status"] = "ok";
            response["waveformType"] = waveformType;
            response["frequency"] = frequency;
            response["amplitude"] = amplitude;
            response["samplesInjected"] = numSamples;
            response["sampleRate"] = sampleRate;

            return {response, 200};
        });

        sendJson(res, result.first, result.second);
    }
    catch (const std::exception& e)
    {
        sendJson(res, jsonError(e.what()), 500);
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
            if (!pane) continue;

            nlohmann::json paneInfo;
            paneInfo["index"] = paneIdx;
            paneInfo["paneId"] = pane->getPaneId().id.toStdString();
            paneInfo["oscillatorCount"] = pane->getOscillatorCount();

            nlohmann::json waveforms = nlohmann::json::array();
            for (size_t wfIdx = 0; wfIdx < pane->getOscillatorCount(); ++wfIdx)
            {
                auto* waveform = pane->getWaveformAt(wfIdx);
                auto* osc = pane->getOscillatorAt(wfIdx);
                if (!waveform || !osc) continue;

                // Force update waveform data to ensure peak/RMS levels are calculated
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

                // LOD (Level-of-Detail) information
                wfInfo["lodTier"] = getLODTierName(waveform->getCurrentLODTier());
                wfInfo["targetSampleCount"] = waveform->getTargetSampleCount();
                wfInfo["width"] = waveform->getWidth();
                wfInfo["decimatorWidth"] = waveform->getDecimatorWidth();  // For debugging LOD

                auto colour = waveform->getColour();
                wfInfo["colour"] = {
                    {"r", colour.getRed()},
                    {"g", colour.getGreen()},
                    {"b", colour.getBlue()},
                    {"a", colour.getAlpha()}
                };

                waveforms.push_back(wfInfo);
            }

            paneInfo["waveforms"] = waveforms;
            panes.push_back(paneInfo);
        }

        response["panes"] = panes;
        response["totalPanes"] = paneComponents.size();

        return response;
    });

    sendJson(res, result, 200);
}

} // namespace oscil
