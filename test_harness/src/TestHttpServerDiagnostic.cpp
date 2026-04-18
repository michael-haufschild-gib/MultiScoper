/*
    Oscil Test Harness - HTTP Server: Diagnostic Snapshot Handler

    Single endpoint that dumps the complete plugin state for automated diagnosis.
    Designed to be consumed by AI coding agents and scenario-based test runners.
*/

#include "core/OscilState.h"
#include "core/dsp/TimingEngine.h"
#include "core/dsp/TimingEngineTypes.h"
#include "core/interfaces/IAudioBuffer.h"
#include "ui/layout/PaneComponent.h"
#include "ui/panels/WaveformComponent.h"

#include "TestAudioGenerator.h"
#include "TestElementRegistry.h"
#include "TestHttpServer.h"
#include "TestLogCapture.h"
#include "plugin/PluginEditor.h"
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
            j["captureBuffer"] = {{"available", available},
                                  {"peakL", peakL},
                                  {"peakR", peakR},
                                  {"rmsL", buffer->getRMSLevel(0, 1024)},
                                  {"rmsR", buffer->getRMSLevel(1, 1024)},
                                  {"peak", std::max(peakL, peakR)},
                                  {"hasData", available > 0 && std::max(peakL, peakR) > 0.0001f}};
        }
        else
        {
            j["captureBuffer"] = {{"available", 0}, {"peakL", 0}, {"peakR", 0},      {"rmsL", 0},
                                  {"rmsR", 0},      {"peak", 0},  {"hasData", false}};
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
        arr.push_back({{"id", pane.getId().id.toStdString()},
                       {"name", pane.getName().toStdString()},
                       {"column", pane.getColumnIndex()},
                       {"order", pane.getOrderIndex()},
                       {"collapsed", pane.isCollapsed()}});
    }
    return arr;
}

json snapshotTransport(TestDAW& daw)
{
    auto& transport = daw.getTransport();
    return {{"playing", transport.isPlaying()},
            {"bpm", transport.getBpm()},
            {"positionSamples", transport.getPositionSamples()},
            {"sampleRate", daw.getSampleRate()}};
}

json snapshotTiming(OscilPluginProcessor& processor)
{
    auto& timingEngine = processor.getTimingEngine();
    auto config = timingEngine.getConfig();
    double sr = processor.getSampleRate();
    return {{"mode", config.timingMode == TimingMode::TIME ? "TIME" : "MELODIC"},
            {"timeIntervalMs", config.timeIntervalMs},
            {"actualIntervalMs", config.actualIntervalMs},
            {"noteInterval", static_cast<int>(config.noteInterval)},
            {"internalBPM", config.internalBPM},
            {"hostBPM", config.hostBPM},
            {"hostSyncEnabled", config.hostSyncEnabled},
            {"displaySamples", timingEngine.getDisplaySampleCount(sr)},
            {"triggerMode", static_cast<int>(config.triggerMode)}};
}

json snapshotSources()
{
    json arr = json::array();
    auto allSources = PluginFactory::getInstance().getInstanceRegistry().getAllSources();
    for (const auto& s : allSources)
    {
        arr.push_back({{"id", s.sourceId.id.toStdString()},
                       {"name", s.name.toStdString()},
                       {"channels", s.channelCount},
                       {"sampleRate", s.sampleRate}});
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
    return {{"fps", snapshot.fps},
            {"cpuPercent", snapshot.cpuPercent},
            {"memoryMB", snapshot.memoryMB},
            {"oscillatorCount", snapshot.oscillatorCount},
            {"sourceCount", snapshot.sourceCount}};
}

json snapshotLogs()
{
    auto entries = TestLogCapture::getInstance().getRecentLogs(50);
    json arr = json::array();
    for (const auto& e : entries)
    {
        arr.push_back({{"t", e.timestampMs}, {"msg", e.message}});
    }
    return arr;
}

json snapshotAudioGenerators(TestDAW& daw)
{
    // Iterate on the message thread — add/remove track also runs on the MT,
    // so walking the vector here cannot race a slot reassignment.  Outputs
    // (`arr`, `done`) are heap-owned via shared_ptr so the MT lambda is safe
    // even if we return early on timeout. `&daw` is fine: TestDAW has
    // process lifetime (owned by the application).
    auto arr = std::make_shared<json>(json::array());
    auto done = std::make_shared<juce::WaitableEvent>();
    juce::MessageManager::callAsync([&daw, arr, done]() {
        for (int i = 0; i < daw.getNumTracks(); ++i)
        {
            auto* track = daw.getTrack(i);
            if (!track)
                continue;
            auto& gen = track->getAudioGenerator();
            arr->push_back({{"track", i},
                            {"waveform", TestAudioGenerator::waveformToString(gen.getWaveform()).toStdString()},
                            {"frequency", gen.getFrequency()},
                            {"amplitude", gen.getAmplitude()},
                            {"generating", gen.isGenerating()}});
        }
        done->signal();
    });
    if (!done->wait(3000))
    {
        juce::Logger::writeToLog("[snapshotAudioGenerators] soft timeout 3000ms; giving up");
    }
    return *arr;
}

json serializeWaveformState(WaveformComponent* wf, const Oscillator* osc)
{
    wf->forceUpdateWaveformData();
    json j;
    j["oscillatorId"] = osc->getId().id.toStdString();
    j["name"] = osc->getName().toStdString();
    j["hasWaveformData"] = wf->hasWaveformData();
    j["peakLevel"] = wf->getPeakLevel();
    j["rmsLevel"] = wf->getRMSLevel();
    j["showGrid"] = wf->isGridVisible();
    j["autoScale"] = wf->isAutoScaleEnabled();
    j["holdDisplay"] = wf->isHoldDisplayEnabled();
    j["gainLinear"] = wf->getGainLinear();
    j["opacity"] = wf->getOpacity();
    j["lineWidth"] = wf->getLineWidth();
    j["displaySamples"] = wf->getDisplaySamples();
    j["processingMode"] = static_cast<int>(wf->getProcessingMode());
    auto c = wf->getColour();
    j["colour"] = {{"r", c.getRed()}, {"g", c.getGreen()}, {"b", c.getBlue()}, {"a", c.getAlpha()}};
    auto bounds = wf->getBounds();
    j["bounds"] = {{"x", bounds.getX()}, {"y", bounds.getY()}, {"w", bounds.getWidth()}, {"h", bounds.getHeight()}};
    return j;
}

} // anonymous namespace

json snapshotGUI(TestDAW& daw)
{
    // Must read WaveformComponent state on the message thread.  Outputs
    // (`result`, `done`) are heap-owned via shared_ptr so the MT lambda is
    // safe even if we return early on timeout. `oscilEditor` is captured by
    // value (pointer); its lifetime is managed by the plugin / DAW.
    auto result = std::make_shared<json>();
    (*result)["editorOpen"] = false;

    auto* editor = daw.getPrimaryEditor();
    if (!editor)
        return *result;

    auto* oscilEditor = dynamic_cast<OscilPluginEditor*>(editor);
    if (!oscilEditor)
        return *result;

    auto done = std::make_shared<juce::WaitableEvent>();
    juce::MessageManager::callAsync([oscilEditor, result, done]() {
        (*result)["editorOpen"] = true;
        const auto& paneComponents = oscilEditor->getPaneComponents();
        json panesArr = json::array();

        for (size_t pi = 0; pi < paneComponents.size(); ++pi)
        {
            auto* pane = paneComponents[pi].get();
            if (!pane)
                continue;
            json paneJson;
            paneJson["index"] = pi;
            paneJson["paneId"] = pane->getPaneId().id.toStdString();
            paneJson["oscillatorCount"] = pane->getOscillatorCount();
            auto paneBounds = pane->getBounds();
            paneJson["bounds"] = {{"x", paneBounds.getX()},
                                  {"y", paneBounds.getY()},
                                  {"w", paneBounds.getWidth()},
                                  {"h", paneBounds.getHeight()}};

            json waveformsArr = json::array();
            for (size_t wi = 0; wi < pane->getOscillatorCount(); ++wi)
            {
                auto* wf = pane->getWaveformAt(wi);
                auto* osc = pane->getOscillatorAt(wi);
                if (wf && osc)
                    waveformsArr.push_back(serializeWaveformState(wf, osc));
            }
            paneJson["waveforms"] = waveformsArr;
            panesArr.push_back(paneJson);
        }
        (*result)["panes"] = panesArr;
        done->signal();
    });
    if (!done->wait(3000))
    {
        juce::Logger::writeToLog("[snapshotGUI] soft timeout 3000ms; giving up");
    }

    return *result;
}

void TestHttpServer::handleDiagnosticSnapshot(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        const int trackId = resolveTrackId(req);
        auto data = std::make_shared<json>();

        // Snapshots that read plugin state (oscillators / panes / timing) must
        // run on the message thread: add/remove track and state mutations are
        // all MT-serialized, so a read inside the lambda sees a coherent view.
        // Snapshots that only read process-global state (sources, UI registry,
        // metrics, logs) are internally locked and safe from any thread.
        const auto result = runOnTrackSync(
            trackId,
            [data](TestTrack& track) {
                auto& processor = track.getProcessor();
                (*data)["oscillators"] = snapshotOscillators(processor);
                (*data)["panes"] = snapshotPanes(processor.getState());
                (*data)["timing"] = snapshotTiming(processor);
            },
            5000);

        if (respondIfTrackCallFailed(result, res, "No track available", "Timeout collecting diagnostic snapshot"))
            return;

        (*data)["transport"] = snapshotTransport(daw_);
        (*data)["sources"] = snapshotSources();
        (*data)["audioGenerators"] = snapshotAudioGenerators(daw_);
        (*data)["ui"] = snapshotUI();
        (*data)["gui"] = snapshotGUI(daw_);
        (*data)["metrics"] = snapshotMetrics(metrics_);
        (*data)["logs"] = snapshotLogs();

        res.set_content(successResponse(*data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

} // namespace oscil::test
