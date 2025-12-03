/*
    Oscil - Source Handler Implementation
*/

#include "tools/test_server/SourceHandler.h"
#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "core/OscilState.h"
#include "core/Oscillator.h"
#include "core/SharedCaptureBuffer.h"
#include "core/InstanceRegistry.h"
#include <cmath>

namespace oscil
{

void SourceHandler::handleGetSources(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([]() -> nlohmann::json {
        nlohmann::json response;
        auto& registry = InstanceRegistry::getInstance();
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

    res.set_content(result.dump(), "application/json");
}

void SourceHandler::handleAddSource(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body.empty() ? "{}" : req.body);
        std::string name = body.value("name", "Test Track");
        int channelCount = body.value("channelCount", 2);
        double sampleRate = body.value("sampleRate", 44100.0);
        std::string trackId = body.value("trackId", juce::Uuid().toString().toStdString());

        auto result = runOnMessageThread([this, name, channelCount, sampleRate, trackId]() -> nlohmann::json {
            nlohmann::json response;
            auto& registry = InstanceRegistry::getInstance();

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

void SourceHandler::handleRemoveSource(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        std::string sourceIdStr = body.value("sourceId", "");

        if (sourceIdStr.empty())
        {
            nlohmann::json error;
            error["error"] = "sourceId required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        auto result = runOnMessageThread([this, sourceIdStr]() -> nlohmann::json {
            nlohmann::json response;
            auto& registry = InstanceRegistry::getInstance();
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

void SourceHandler::handleAssignSource(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        std::string oscillatorId = body.value("oscillatorId", "");
        std::string sourceId = body.value("sourceId", "");
        int oscillatorIndex = body.value("oscillatorIndex", -1);

        if (oscillatorId.empty() && oscillatorIndex < 0)
        {
            nlohmann::json error;
            error["error"] = "oscillatorId or oscillatorIndex required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        auto result = runOnMessageThread([this, oscillatorId, sourceId, oscillatorIndex]() -> nlohmann::json {
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
                return response;
            }

            // Find the oscillator in state and update it
            bool found = false;
            for (auto& osc : oscillators)
            {
                if (osc.getId().id == targetOscId.id)
                {
                    SourceId newSourceId;
                    newSourceId.id = juce::String(sourceId);
                    osc.setSourceId(newSourceId);

                    // Update the oscillator in state
                    state.updateOscillator(osc);

                    // Bind the capture buffer to the waveform component
                    auto& registry = InstanceRegistry::getInstance();
                    auto buffer = registry.getCaptureBuffer(newSourceId);

                    // Trigger UI refresh to rebind waveform components
                    editor_.updateOscillatorSource(targetOscId, newSourceId);

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
            }

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

void SourceHandler::handleInjectSourceData(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        std::string sourceId = body.value("sourceId", "");
        std::string waveformType = body.value("type", "sine");
        float frequency = body.value("frequency", 440.0f);
        float amplitude = body.value("amplitude", 0.8f);
        int numSamples = body.value("samples", 4096);
        float sampleRate = body.value("sampleRate", 44100.0f);

        if (sourceId.empty())
        {
            nlohmann::json error;
            error["error"] = "sourceId required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        auto result = runOnMessageThread([this, sourceId, waveformType, frequency, amplitude, numSamples, sampleRate]() -> nlohmann::json {
            nlohmann::json response;

            // Get capture buffer for this source
            auto& registry = InstanceRegistry::getInstance();
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
                return response;
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
            captureBuffer->write(testBuffer, metadata, false);

            // Force UI update
            editor_.repaint();

            response["status"] = "ok";
            response["sourceId"] = sourceId;
            response["waveformType"] = waveformType;
            response["frequency"] = frequency;
            response["amplitude"] = amplitude;
            response["samplesInjected"] = numSamples;

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

} // namespace oscil
