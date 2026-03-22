/*
    Oscil Test Harness - HTTP Server: Diagnostic Snapshot Handler

    Single endpoint that dumps the complete plugin state for automated diagnosis.
    Designed to be consumed by AI coding agents and scenario-based test runners.
*/

#include "TestHttpServer.h"
#include "TestLogCapture.h"
#include "TestElementRegistry.h"
#include "TestAudioGenerator.h"
#include "core/OscilState.h"
#include "core/dsp/TimingEngine.h"
#include "core/dsp/TimingEngineTypes.h"
#include "core/interfaces/IAudioBuffer.h"
#include "plugin/PluginFactory.h"
#include <algorithm>

namespace oscil::test
{

namespace
{

json snapshotOscillators(OscilPluginProcessor& processor)
{
    auto& state = processor.getState();
    auto oscillators = state.getOscillators();
    json arr = json::array();

    for (const auto& osc : oscillators)
    {
        json j;
        j["id"] = osc.getId().id.toStdString();
        j["name"] = osc.getName().toStdString();
        j["sourceId"] = osc.getSourceId().id.toStdString();
        j["paneId"] = osc.getPaneId().id.toStdString();
        j["mode"] = processingModeToString(osc.getProcessingMode()).toStdString();
        j["orderIndex"] = osc.getOrderIndex();
        j["visible"] = osc.isVisible();
        j["opacity"] = osc.getOpacity();
        j["lineWidth"] = osc.getLineWidth();

        // Capture buffer stats for this oscillator's source
        auto buffer = processor.getBuffer(osc.getSourceId());
        if (buffer)
        {
            auto available = static_cast<int64_t>(buffer->getAvailableSamples());
            float peakL = buffer->getPeakLevel(0, 1024);
            float peakR = buffer->getPeakLevel(1, 1024);
            j["captureBuffer"] = {
                {"available", available},
                {"peakL", peakL}, {"peakR", peakR},
                {"rmsL", buffer->getRMSLevel(0, 1024)},
                {"rmsR", buffer->getRMSLevel(1, 1024)},
                {"peak", std::max(peakL, peakR)},
                {"hasData", available > 0 && std::max(peakL, peakR) > 0.0001f}
            };
        }
        else
        {
            j["captureBuffer"] = {
                {"available", 0}, {"peakL", 0}, {"peakR", 0},
                {"rmsL", 0}, {"rmsR", 0}, {"peak", 0}, {"hasData", false}
            };
        }

        arr.push_back(j);
    }
    return arr;
}

json snapshotPanes(const OscilState& state)
{
    auto& layoutManager = state.getLayoutManager();
    json arr = json::array();
    for (const auto& pane : layoutManager.getPanes())
    {
        arr.push_back({
            {"id", pane.getId().id.toStdString()},
            {"name", pane.getName().toStdString()},
            {"column", pane.getColumnIndex()},
            {"order", pane.getOrderIndex()},
            {"collapsed", pane.isCollapsed()}
        });
    }
    return arr;
}

json snapshotTransport(TestDAW& daw)
{
    auto& transport = daw.getTransport();
    return {
        {"playing", transport.isPlaying()},
        {"bpm", transport.getBpm()},
        {"positionSamples", transport.getPositionSamples()},
        {"sampleRate", daw.getSampleRate()}
    };
}

json snapshotTiming(OscilPluginProcessor& processor)
{
    auto& timingEngine = processor.getTimingEngine();
    auto config = timingEngine.getConfig();
    double sr = processor.getSampleRate();
    return {
        {"mode", config.timingMode == TimingMode::TIME ? "TIME" : "MELODIC"},
        {"timeIntervalMs", config.timeIntervalMs},
        {"actualIntervalMs", config.actualIntervalMs},
        {"noteInterval", static_cast<int>(config.noteInterval)},
        {"internalBPM", config.internalBPM},
        {"hostBPM", config.hostBPM},
        {"hostSyncEnabled", config.hostSyncEnabled},
        {"displaySamples", timingEngine.getDisplaySampleCount(sr)},
        {"triggerMode", static_cast<int>(config.triggerMode)}
    };
}

json snapshotSources()
{
    json arr = json::array();
    auto allSources = PluginFactory::getInstance().getInstanceRegistry().getAllSources();
    for (const auto& s : allSources)
    {
        arr.push_back({
            {"id", s.sourceId.id.toStdString()},
            {"name", s.name.toStdString()},
            {"channels", s.channelCount},
            {"sampleRate", s.sampleRate}
        });
    }
    return arr;
}

json snapshotUI()
{
    auto& registry = TestElementRegistry::getInstance();
    auto elements = registry.getAllElements();
    json ids = json::array();
    for (const auto& [id, _] : elements)
        ids.push_back(id.toStdString());
    return {{"elementCount", static_cast<int>(elements.size())}, {"elementIds", ids}};
}

json snapshotMetrics(TestMetrics& metrics)
{
    auto snapshot = metrics.getCurrentSnapshot();
    return {
        {"fps", snapshot.fps},
        {"cpuPercent", snapshot.cpuPercent},
        {"memoryMB", snapshot.memoryMB},
        {"oscillatorCount", snapshot.oscillatorCount},
        {"sourceCount", snapshot.sourceCount}
    };
}

json snapshotLogs()
{
    auto entries = TestLogCapture::getInstance().getRecentLogs(50);
    json arr = json::array();
    for (const auto& e : entries)
    {
        arr.push_back({
            {"t", e.timestampMs},
            {"msg", e.message}
        });
    }
    return arr;
}

json snapshotAudioGenerators(TestDAW& daw)
{
    json arr = json::array();
    for (int i = 0; i < daw.getNumTracks(); ++i)
    {
        auto* track = daw.getTrack(i);
        if (!track) continue;
        auto& gen = track->getAudioGenerator();
        arr.push_back({
            {"track", i},
            {"waveform", TestAudioGenerator::waveformToString(gen.getWaveform()).toStdString()},
            {"frequency", gen.getFrequency()},
            {"amplitude", gen.getAmplitude()},
            {"generating", gen.isGenerating()}
        });
    }
    return arr;
}

} // anonymous namespace

void TestHttpServer::handleDiagnosticSnapshot(const httplib::Request&, httplib::Response& res)
{
    try
    {
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto& processor = track->getProcessor();
        json data;
        data["oscillators"] = snapshotOscillators(processor);
        data["panes"] = snapshotPanes(processor.getState());
        data["transport"] = snapshotTransport(daw_);
        data["timing"] = snapshotTiming(processor);
        data["sources"] = snapshotSources();
        data["audioGenerators"] = snapshotAudioGenerators(daw_);
        data["ui"] = snapshotUI();
        data["metrics"] = snapshotMetrics(metrics_);
        data["logs"] = snapshotLogs();

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

} // namespace oscil::test
