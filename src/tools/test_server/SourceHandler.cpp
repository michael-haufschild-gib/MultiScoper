/*
    Oscil - Source Handler Implementation
*/

#include "tools/test_server/SourceHandler.h"

#include "core/DecimatingCaptureBuffer.h"
#include "core/InstanceRegistry.h"
#include "core/OscilState.h"
#include "core/Oscillator.h"
#include "core/SharedCaptureBuffer.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginFactory.h"
#include "plugin/PluginProcessor.h"

#include <cmath>
#include <utility>

namespace oscil
{

namespace
{

OscillatorId resolveOscId(const std::string& idStr, int index, const std::vector<Oscillator>& oscillators)
{
    OscillatorId targetId;
    if (!idStr.empty())
        targetId.id = juce::String(idStr);
    else if (index >= 0 && std::cmp_less(index, oscillators.size()))
        targetId = oscillators[static_cast<size_t>(index)].getId();
    return targetId;
}

void generateTestWaveform(juce::AudioBuffer<float>& buffer, const std::string& waveformType, float frequency,
                          float amplitude, float sampleRate)
{
    int const numSamples = buffer.getNumSamples();
    float phase = 0.0f;
    float const safeSampleRate = sampleRate > 0.0f ? sampleRate : 44100.0f;
    float const phaseIncrement = (2.0f * juce::MathConstants<float>::pi * frequency) / safeSampleRate;

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
            float const t = phase / (2.0f * juce::MathConstants<float>::pi);
            sample = ((2.0f * std::abs(2.0f * (t - std::floor(t + 0.5f)))) - 1.0f) * amplitude;
        }
        else if (waveformType == "sawtooth")
        {
            float const t = phase / (2.0f * juce::MathConstants<float>::pi);
            sample = (2.0f * (t - std::floor(t + 0.5f))) * amplitude;
        }
        else if (waveformType == "noise")
        {
            sample = (((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.0f) - 1.0f) * amplitude;
        }

        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample * 0.8f);

        phase += phaseIncrement;
        if (phase > 2.0f * juce::MathConstants<float>::pi)
            phase -= 2.0f * juce::MathConstants<float>::pi;
    }
}

CaptureFrameMetadata makeTestMetadata(float sampleRate, int numSamples)
{
    CaptureFrameMetadata metadata;
    metadata.sampleRate = sampleRate;
    metadata.numChannels = 2;
    metadata.numSamples = numSamples;
    metadata.isPlaying = true;
    metadata.bpm = 120.0f;
    metadata.timestamp = 0;
    return metadata;
}

} // namespace

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
            srcInfo["active"] = source.active;
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
        std::string const name = body.value("name", "Test Track");
        int const channelCount = body.value("channelCount", 2);
        double const sampleRate = body.value("sampleRate", 44100.0);
        std::string const trackId = body.value("trackId", juce::Uuid().toString().toStdString());

        auto result = runOnMessageThread([this, name, channelCount, sampleRate, trackId]() -> nlohmann::json {
            nlohmann::json response;
            auto& registry = PluginFactory::getInstance().getInstanceRegistry();

            // Create a new capture buffer for this source
            auto captureBuffer = std::make_shared<SharedCaptureBuffer>();

            // Register the source
            SourceId const sourceId = registry.registerInstance(juce::String(trackId), captureBuffer,
                                                                juce::String(name), channelCount, sampleRate);

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
        std::string const sourceIdStr = body.value("sourceId", "");

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

nlohmann::json SourceHandler::assignSourceOnMessageThread(const std::string& oscillatorId, const std::string& sourceId,
                                                          int oscillatorIndex)
{
    nlohmann::json response;
    auto& state = editor_.getProcessor().getState();
    auto oscillators = state.getOscillators();

    auto targetOscId = resolveOscId(oscillatorId, oscillatorIndex, oscillators);
    if (!targetOscId.isValid())
    {
        response["error"] = "Invalid oscillator index";
        return response;
    }

    for (auto& osc : oscillators)
    {
        if (osc.getId().id != targetOscId.id)
            continue;

        SourceId newSourceId;
        newSourceId.id = juce::String(sourceId);
        osc.setSourceId(newSourceId);
        state.updateOscillator(osc);

        auto& registry = PluginFactory::getInstance().getInstanceRegistry();
        auto buffer = registry.getCaptureBuffer(newSourceId);

        if (auto* controller = editor_.getOscillatorPanelController())
            controller->updateOscillatorSource(targetOscId, newSourceId);

        response["status"] = "ok";
        response["oscillatorId"] = targetOscId.id.toStdString();
        response["sourceId"] = sourceId;
        response["hasBuffer"] = (buffer != nullptr);
        return response;
    }
    response["error"] = "Oscillator not found";
    return response;
}

void SourceHandler::handleAssignSource(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        std::string const oscillatorId = body.value("oscillatorId", "");
        std::string const sourceId = body.value("sourceId", "");
        int const oscillatorIndex = body.value("oscillatorIndex", -1);

        if (oscillatorId.empty() && oscillatorIndex < 0)
        {
            nlohmann::json error;
            error["error"] = "oscillatorId or oscillatorIndex required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        auto result = runOnMessageThread([this, oscillatorId, sourceId, oscillatorIndex]() {
            return assignSourceOnMessageThread(oscillatorId, sourceId, oscillatorIndex);
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

nlohmann::json SourceHandler::injectSourceDataOnMessageThread(const std::string& sourceId,
                                                              const std::string& waveformType, float frequency,
                                                              float amplitude, int numSamples, float sampleRate)
{
    nlohmann::json response;

    auto& registry = PluginFactory::getInstance().getInstanceRegistry();
    SourceId srcId;
    srcId.id = juce::String(sourceId);
    auto captureBuffer = registry.getCaptureBuffer(srcId);

    if (!captureBuffer)
    {
        auto it = testSourceBuffers_.find(sourceId);
        if (it != testSourceBuffers_.end())
            captureBuffer = it->second;
    }

    if (!captureBuffer)
    {
        response["error"] = "Source not found or no capture buffer";
        return response;
    }

    juce::AudioBuffer<float> testBuffer(2, numSamples);
    generateTestWaveform(testBuffer, waveformType, frequency, amplitude, sampleRate);

    auto metadata = makeTestMetadata(sampleRate, numSamples);

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
        return response;
    }

    editor_.repaint();

    response["status"] = "ok";
    response["sourceId"] = sourceId;
    response["waveformType"] = waveformType;
    response["frequency"] = frequency;
    response["amplitude"] = amplitude;
    response["samplesInjected"] = numSamples;
    return response;
}

void SourceHandler::handleInjectSourceData(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        std::string const sourceId = body.value("sourceId", "");
        std::string const waveformType = body.value("type", "sine");
        float const frequency = body.value("frequency", 440.0f);
        float const amplitude = body.value("amplitude", 0.8f);
        int const numSamples = body.value("samples", 4096);
        float const sampleRate = body.value("sampleRate", 44100.0f);

        if (sourceId.empty())
        {
            nlohmann::json error;
            error["error"] = "sourceId required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        auto result =
            runOnMessageThread([this, sourceId, waveformType, frequency, amplitude, numSamples, sampleRate]() {
                return injectSourceDataOnMessageThread(sourceId, waveformType, frequency, amplitude, numSamples,
                                                       sampleRate);
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
