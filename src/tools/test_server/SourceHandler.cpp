/*
    Oscil - Source Handler Implementation
*/

#include "tools/test_server/SourceHandler.h"
#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "plugin/PluginFactory.h"
#include "core/OscilState.h"
#include "core/Oscillator.h"
#include "core/SharedCaptureBuffer.h"
#include "core/DecimatingCaptureBuffer.h"
#include "core/InstanceRegistry.h"
#include <cmath>

namespace oscil
{

void SourceHandler::handleGetSources(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([]() -> nlohmann::json {
        nlohmann::json response;
        auto& registry = PluginFactory::getInstance().getInstanceRegistry();
        auto sources = registry.getAllSources();

        nlohmann::json sourceList = nlohmann::json::array();
        for (const auto& source : sources)
        {
            nlohmann::json srcInfo;
            srcInfo["id"] = source.sourceId.id.toStdString();
            srcInfo["name"] = source.name.toStdString();
            srcInfo["channelCount"] = source.channelCount;
            srcInfo["sampleRate"] = source.sampleRate;
            srcInfo["active"] = source.active.load();
            srcInfo["hasBuffer"] = !source.buffer.expired();
            sourceList.push_back(srcInfo);
        }

        response["sources"] = sourceList;
        response["count"] = static_cast<int>(sources.size());

        return response;
    });

    sendJson(res, result, 200);
}

void SourceHandler::handleAddSource(const httplib::Request& req, httplib::Response& res)
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
        std::string name = body.value("name", "Test Track");
        int channelCount = body.value("channelCount", 2);
        double sampleRate = body.value("sampleRate", 44100.0);
        std::string trackId = body.value("trackId", juce::Uuid().toString().toStdString());

        auto result = runOnMessageThread([this, name, channelCount, sampleRate, trackId]() -> nlohmann::json {
            nlohmann::json response;
            auto& registry = PluginFactory::getInstance().getInstanceRegistry();

            // Create a new capture buffer for this source
            auto captureBuffer = std::make_shared<SharedCaptureBuffer>();

            // Register the source
            SourceId sourceId = registry.registerInstance(
                juce::String(trackId),
                captureBuffer,
                juce::String(name),
                channelCount,
                sampleRate
            );

            // Store the buffer reference so it doesn't get destroyed
            testSourceBuffers_[sourceId.id.toStdString()] = captureBuffer;

            response["status"] = "ok";
            response["sourceId"] = sourceId.id.toStdString();
            response["name"] = name;
            response["channelCount"] = channelCount;
            response["sampleRate"] = sampleRate;
            response["sourceCount"] = static_cast<int>(registry.getSourceCount());

            return response;
        });

        sendJson(res, result, 200);
    }
    catch (const std::exception& e)
    {
        sendJson(res, jsonError(e.what()), 500);
    }
}

void SourceHandler::handleRemoveSource(const httplib::Request& req, httplib::Response& res)
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

    // Validate required field
    if (!body.contains("sourceId") || !body["sourceId"].is_string() || body["sourceId"].get<std::string>().empty())
    {
        sendJson(res, jsonError("Missing required field: sourceId"), 400);
        return;
    }

    try
    {
        std::string sourceIdStr = body["sourceId"].get<std::string>();

        auto result = runOnMessageThread([this, sourceIdStr]() -> nlohmann::json {
            nlohmann::json response;
            auto& registry = PluginFactory::getInstance().getInstanceRegistry();
            SourceId sourceId;
            sourceId.id = juce::String(sourceIdStr);

            // Unregister from registry
            registry.unregisterInstance(sourceId);

            // Remove from local buffer cache (fixing leak)
            testSourceBuffers_.erase(sourceIdStr);

            response["status"] = "ok";
            response["sourceId"] = sourceIdStr;
            response["sourceCount"] = static_cast<int>(registry.getSourceCount());

            return response;
        });

        sendJson(res, result, 200);
    }
    catch (const std::exception& e)
    {
        sendJson(res, jsonError(e.what()), 500);
    }
}

void SourceHandler::handleAssignSource(const httplib::Request& req, httplib::Response& res)
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

    std::string oscillatorId = body.value("oscillatorId", "");
    std::string sourceId = body.value("sourceId", "");
    int oscillatorIndex = body.value("oscillatorIndex", -1);

    // Validate required fields
    if (oscillatorId.empty() && oscillatorIndex < 0)
    {
        sendJson(res, jsonError("Missing required field: oscillatorId or oscillatorIndex"), 400);
        return;
    }

    try
    {
        auto result = runOnMessageThread([this, oscillatorId, sourceId, oscillatorIndex]() -> std::pair<nlohmann::json, int> {
            nlohmann::json response;
            auto& state = editor_.getProcessor().getState();
            auto oscillators = state.getOscillators();

            // Find the oscillator
            OscillatorId targetOscId;
            if (!oscillatorId.empty())
            {
                targetOscId.id = juce::String(oscillatorId);
            }
            else if (oscillatorIndex >= 0 && oscillatorIndex < static_cast<int>(oscillators.size()))
            {
                targetOscId = oscillators[static_cast<size_t>(oscillatorIndex)].getId();
            }
            else
            {
                response["error"] = "Invalid oscillator index";
                return {response, 400};
            }

            // Find the oscillator in state and update it
            bool found = false;
            auto& registry = PluginFactory::getInstance().getInstanceRegistry();

            for (auto& osc : oscillators)
            {
                if (osc.getId().id == targetOscId.id)
                {
                    SourceId newSourceId;
                    newSourceId.id = juce::String(sourceId);

                    // Look up source info to get name and UUID for persistence
                    juce::String sourceName = "Unknown Source";
                    juce::String sourceUUID;
                    auto sourceInfo = registry.getSource(newSourceId);
                    if (sourceInfo.has_value())
                    {
                        sourceName = sourceInfo->name;
                        sourceUUID = sourceInfo->instanceUUID;
                    }
                    osc.setSourceIdNameAndUUID(newSourceId, sourceName, sourceUUID);

                    // Update the oscillator in state
                    state.updateOscillator(osc);

                    // Bind the capture buffer to the waveform component
                    auto buffer = registry.getCaptureBuffer(newSourceId);

                    // Trigger UI refresh to rebind waveform components
                    if (auto* controller = editor_.getOscillatorPanelController())
                    {
                        controller->updateOscillatorSource(targetOscId, newSourceId);
                    }

                    response["status"] = "ok";
                    response["oscillatorId"] = targetOscId.id.toStdString();
                    response["sourceId"] = sourceId;
                    response["hasBuffer"] = (buffer != nullptr);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                response["error"] = "Oscillator not found";
                return {response, 404};
            }

            return {response, 200};
        });

        sendJson(res, result.first, result.second);
    }
    catch (const std::exception& e)
    {
        sendJson(res, jsonError(e.what()), 500);
    }
}

void SourceHandler::handleInjectSourceData(const httplib::Request& req, httplib::Response& res)
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

    // Validate required field
    if (!body.contains("sourceId") || !body["sourceId"].is_string() || body["sourceId"].get<std::string>().empty())
    {
        sendJson(res, jsonError("Missing required field: sourceId"), 400);
        return;
    }

    try
    {
        std::string sourceId = body["sourceId"].get<std::string>();
        std::string waveformType = body.value("type", "sine");
        float frequency = body.value("frequency", 440.0f);
        float amplitude = body.value("amplitude", 0.8f);
        int numSamples = body.value("samples", 4096);
        float sampleRate = body.value("sampleRate", 44100.0f);

        auto result = runOnMessageThread([this, sourceId, waveformType, frequency, amplitude, numSamples, sampleRate]() -> std::pair<nlohmann::json, int> {
            nlohmann::json response;

            // Get capture buffer for this source
            auto& registry = PluginFactory::getInstance().getInstanceRegistry();
            SourceId srcId;
            srcId.id = juce::String(sourceId);
            auto captureBuffer = registry.getCaptureBuffer(srcId);

            if (!captureBuffer)
            {
                // Try from test source buffers
                auto it = testSourceBuffers_.find(sourceId);
                if (it != testSourceBuffers_.end())
                {
                    captureBuffer = it->second;
                }
            }

            if (!captureBuffer)
            {
                response["error"] = "Source not found or no capture buffer";
                return {response, 404};
            }

            // Generate test waveform
            juce::AudioBuffer<float> testBuffer(2, numSamples);
            float phase = 0.0f;
            // Guard against division by zero
            float safeSampleRate = sampleRate > 0.0f ? sampleRate : 44100.0f;
            float phaseIncrement = (2.0f * juce::MathConstants<float>::pi * frequency) / safeSampleRate;

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
                    sample = ((std::rand() / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f) * amplitude;
                }

                testBuffer.setSample(0, i, sample);
                testBuffer.setSample(1, i, sample * 0.8f);  // Slightly different R channel

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

            // Use blocking write (tryLock=false) for reliable test injection
            if (auto decBuffer = std::dynamic_pointer_cast<DecimatingCaptureBuffer>(captureBuffer))
            {
                decBuffer->write(testBuffer, metadata);
            }
            else if (auto sharedBuffer = std::dynamic_pointer_cast<SharedCaptureBuffer>(captureBuffer))
            {
                sharedBuffer->write(testBuffer, metadata, false);
            }
            else
            {
                response["error"] = "Buffer is not writable (unknown type)";
                return {response, 500};
            }

            // Force UI update
            editor_.repaint();

            response["status"] = "ok";
            response["sourceId"] = sourceId;
            response["waveformType"] = waveformType;
            response["frequency"] = frequency;
            response["amplitude"] = amplitude;
            response["samplesInjected"] = numSamples;

            return {response, 200};
        });

        sendJson(res, result.first, result.second);
    }
    catch (const std::exception& e)
    {
        sendJson(res, jsonError(e.what()), 500);
    }
}

} // namespace oscil
