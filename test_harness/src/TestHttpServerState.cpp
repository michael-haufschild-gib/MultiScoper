/*
    MultiScoper Test Harness - HTTP Server: State Handlers
*/

#include "core/MultiScoperState.h"
#include "core/dsp/TimingEngine.h"

#include "TestAudioGenerator.h"
#include "TestElementRegistry.h"
#include "TestHttpServer.h"
#include "plugin/PluginEditor.h"
#include "plugin/PluginFactory.h"

namespace multiscoper::test
{

namespace
{

// Build JSON representation of an Oscillator
json oscillatorToJson(const Oscillator& osc)
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
    auto colour = osc.getColour();
    j["colour"] = juce::String::toHexString(static_cast<int>(colour.getARGB())).toStdString();
    return j;
}

// Configure an Oscillator from JSON request body fields
void configureOscillatorFromJson(Oscillator& osc, const json& body, MultiScoperState& state, TestTrack* track)
{
    std::string name = body.value("name", "");
    if (name.empty())
        name = "Oscillator " + std::to_string(state.getOscillators().size() + 1);
    osc.setName(name);

    std::string sourceIdStr = body.value("sourceId", "");
    if (!sourceIdStr.empty())
        osc.setSourceId(SourceId{juce::String(sourceIdStr)});
    else if (track->getProcessor().getSourceId().isValid())
        osc.setSourceId(track->getProcessor().getSourceId());

    std::string paneIdStr = body.value("paneId", "");
    if (!paneIdStr.empty())
    {
        osc.setPaneId(PaneId{juce::String(paneIdStr)});
    }
    else
    {
        auto& layoutManager = state.getLayoutManager();
        if (layoutManager.getPaneCount() > 0)
            osc.setPaneId(layoutManager.getPanes()[0].getId());
    }

    // Processing mode
    std::string modeStr = body.value("mode", "FullStereo");
    osc.setProcessingMode(stringToProcessingMode(juce::String(modeStr)));

    std::string colourStr = body.value("colour", "");
    if (!colourStr.empty())
    {
        osc.setColour(juce::Colour::fromString(juce::String(colourStr)));
    }
    else
    {
        static const juce::Colour defaultColors[] = {
            juce::Colour(0xFF00FF00), juce::Colour(0xFF0088FF), juce::Colour(0xFFFF8800),
            juce::Colour(0xFFFF0088), juce::Colour(0xFF88FF00),
        };
        int colorIndex = static_cast<int>(state.getOscillators().size()) % 5;
        osc.setColour(defaultColors[colorIndex]);
    }

    osc.setOrderIndex(static_cast<int>(state.getOscillators().size()));
}

// Apply partial updates from JSON body to an existing Oscillator
void applyOscillatorJsonUpdates(Oscillator& osc, const json& body)
{
    if (body.contains("visible"))
        osc.setVisible(body["visible"].get<bool>());
    if (body.contains("name"))
        osc.setName(juce::String(body["name"].get<std::string>()));
    if (body.contains("mode"))
        osc.setProcessingMode(stringToProcessingMode(juce::String(body["mode"].get<std::string>())));
    if (body.contains("opacity"))
        osc.setOpacity(body["opacity"].get<float>());
    if (body.contains("lineWidth"))
        osc.setLineWidth(body["lineWidth"].get<float>());
}

} // anonymous namespace

void TestHttpServer::setupStateRoutes()
{
    server_->Post("/state/reset",
                  [this](const httplib::Request& req, httplib::Response& res) { handleStateReset(req, res); });
    server_->Post("/state/save",
                  [this](const httplib::Request& req, httplib::Response& res) { handleStateSave(req, res); });
    server_->Post("/state/load",
                  [this](const httplib::Request& req, httplib::Response& res) { handleStateLoad(req, res); });
    server_->Get("/state/oscillators",
                 [this](const httplib::Request& req, httplib::Response& res) { handleStateOscillators(req, res); });
    server_->Post("/state/oscillator/add",
                  [this](const httplib::Request& req, httplib::Response& res) { handleStateAddOscillator(req, res); });
    server_->Post("/state/oscillator/update", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateUpdateOscillator(req, res);
    });
    server_->Post("/state/oscillator/reorder", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateReorderOscillators(req, res);
    });
    server_->Get("/state/panes",
                 [this](const httplib::Request& req, httplib::Response& res) { handleStatePanes(req, res); });
    server_->Get("/state/sources",
                 [this](const httplib::Request& req, httplib::Response& res) { handleStateSources(req, res); });
    server_->Post("/state/oscillator/delete", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateDeleteOscillator(req, res);
    });
    server_->Post("/state/pane/add",
                  [this](const httplib::Request& req, httplib::Response& res) { handlePaneAdd(req, res); });
    server_->Post("/state/pane/remove",
                  [this](const httplib::Request& req, httplib::Response& res) { handlePaneRemove(req, res); });
    server_->Post("/state/oscillator/move",
                  [this](const httplib::Request& req, httplib::Response& res) { handleOscillatorMove(req, res); });
    server_->Post("/project/save",
                  [this](const httplib::Request& req, httplib::Response& res) { handleProjectSave(req, res); });
    server_->Post("/project/close",
                  [this](const httplib::Request& req, httplib::Response& res) { handleProjectClose(req, res); });
    server_->Post("/project/reopen",
                  [this](const httplib::Request& req, httplib::Response& res) { handleProjectReopen(req, res); });
}

void TestHttpServer::handleStateReset(const httplib::Request& req, httplib::Response& res)
{
    juce::Logger::writeToLog("[Harness] State reset requested");
    const auto trackIdOpt = tryResolveTrackId(req, res);
    if (!trackIdOpt)
        return;
    const int trackId = *trackIdOpt;

    // Intentionally lenient: /state/reset succeeds even when trackId is
    // missing; transport/options reset below runs unconditionally.
    (void) runOnTrackSync(
        trackId,
        [](TestTrack& track) {
            auto& state = track.getProcessor().getState();
            for (const auto& osc : state.getOscillators())
                state.removeOscillator(osc.getId());
            auto& lm = state.getLayoutManager();
            // Snapshot pane IDs before mutation (getPanes() is a reference).
            std::vector<PaneId> paneIds;
            paneIds.reserve(lm.getPanes().size());
            for (const auto& pane : lm.getPanes())
                paneIds.push_back(pane.getId());
            for (const auto& id : paneIds)
                lm.removePane(id);
            track.getProcessor().getTimingEngine().setConfig(EngineTimingConfig{});

            // setConfig() bypasses the listener chain (writes the SeqLock
            // directly); refresh the sidebar so the presenter's cached
            // mode/hostSync/BPM don't suppress the next test's changes via
            // its `if (currentMode_ != mode)` early-outs.
            if (auto* editor = track.getEditor())
                if (auto* multiscoperEditor = dynamic_cast<MultiScoperPluginEditor*>(editor))
                    multiscoperEditor->refreshTimingSidebarFromEngine();
        },
        5000);

    resetAudioAndTransport();
    resetOptionsControls();

    // Element registry is NOT cleared here: live components (sidebar, buttons,
    // timing controls) keep their registrations; components tied to removed
    // oscillators/panes self-unregister via RAII TestRegistration.
    res.set_content(successResponse().dump(), "application/json");
}

void TestHttpServer::resetAudioAndTransport()
{
    // Hop to the message thread so add/remove can't invalidate track slots.
    // Transport itself is stable for the DAW's lifetime; the track vector is not.
    runOnMessageThreadBlocking(
        [this]() {
            for (int i = 0; i < daw_.getNumTracks(); ++i)
            {
                if (auto* t = daw_.getTrack(i))
                {
                    t->getAudioGenerator().setWaveform(Waveform::Sine);
                    t->getAudioGenerator().setFrequency(440.0f);
                    t->getAudioGenerator().setAmplitude(0.5f);
                }
            }
            daw_.getTransport().play();
            daw_.getTransport().setBpm(120.0);
            daw_.getTransport().setPositionSamples(0);
        },
        3000, "resetAudioAndTransport");
}

void TestHttpServer::resetOptionsControls()
{
    uiController_.setSliderValue("sidebar_options_gainSlider", 0.0);
    uiController_.toggle("sidebar_options_gridToggle", false);
    uiController_.toggle("sidebar_options_autoScaleToggle", false);
    uiController_.toggle("sidebar_options_gpuRenderingToggle", false);
    uiController_.toggle("sidebar_options_autoAdjustToggle", true);
    uiController_.selectById("sidebar_options_layoutDropdown", "1");
}

// handleStateSave, handleStateLoad, and restoreLoadedState are defined in
// TestHttpServerStatePersistence.cpp — they share the "apply XML" sidebar
// refresh logic.

void TestHttpServer::handleStateOscillators(const httplib::Request& req, httplib::Response& res)
{
    const auto trackIdOpt = tryResolveTrackId(req, res);
    if (!trackIdOpt)
        return;
    const int trackId = *trackIdOpt;
    auto oscillators = std::make_shared<json>(json::array());

    const auto result = runOnTrackSync(trackId, [oscillators](TestTrack& track) {
        auto& state = track.getProcessor().getState();
        auto oscList = state.getOscillators();

        std::sort(oscList.begin(), oscList.end(),
                  [](const auto& a, const auto& b) { return a.getOrderIndex() < b.getOrderIndex(); });

        for (const auto& osc : oscList)
            oscillators->push_back(oscillatorToJson(osc));
    });

    if (respondIfTrackCallFailed(result, res, "No track available", "Timeout listing oscillators"))
        return;

    res.set_content(successResponse(*oscillators).dump(), "application/json");
}

void TestHttpServer::handleStateAddOscillator(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        const auto trackIdOpt = tryResolveTrackId(req, res);
        if (!trackIdOpt)
            return;
        const int trackId = *trackIdOpt;
        auto body = std::make_shared<json>(json::parse(req.body));

        auto oscJson = std::make_shared<json>();

        const auto result = runOnTrackSync(
            trackId,
            [body, oscJson](TestTrack& track) {
                Oscillator osc;
                auto& state = track.getProcessor().getState();
                auto& layoutManager = state.getLayoutManager();

                if (layoutManager.getPaneCount() == 0)
                {
                    Pane defaultPane;
                    defaultPane.setName("Pane 1");
                    defaultPane.setOrderIndex(0);
                    layoutManager.addPane(defaultPane);
                }

                configureOscillatorFromJson(osc, *body, state, &track);
                *oscJson = oscillatorToJson(osc);

                juce::Logger::writeToLog("[Harness] Adding oscillator: " + osc.getName() +
                                         " source=" + osc.getSourceId().id + " pane=" + osc.getPaneId().id);
                state.addOscillator(osc);
            },
            5000);

        if (respondIfTrackCallFailed(result, res, "No track available", "Timeout adding oscillator"))
            return;

        res.set_content(successResponse(*oscJson).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStateUpdateOscillator(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        const auto trackIdOpt = tryResolveTrackId(req, res);
        if (!trackIdOpt)
            return;
        const int trackId = *trackIdOpt;
        auto body = std::make_shared<json>(json::parse(req.body));
        std::string idStr = body->value("id", "");
        if (idStr.empty())
        {
            res.set_content(errorResponse("Oscillator 'id' is required").dump(), "application/json");
            return;
        }

        OscillatorId oscId{juce::String(idStr)};
        auto osc = std::make_shared<Oscillator>();
        auto oscMissing = std::make_shared<bool>(false);

        const auto result = runOnTrackSync(
            trackId,
            [oscId, body, osc, oscMissing](TestTrack& track) {
                auto& state = track.getProcessor().getState();
                auto existingOsc = state.getOscillator(oscId);
                if (!existingOsc.has_value())
                {
                    *oscMissing = true;
                    return;
                }
                *osc = existingOsc.value();
                applyOscillatorJsonUpdates(*osc, *body);
                state.updateOscillator(*osc);
                if (auto* ed = dynamic_cast<MultiScoperPluginEditor*>(track.getEditor()))
                    ed->refreshPanels();
            },
            5000);

        if (respondIfTrackCallFailed(result, res, "No track available", "Timeout updating oscillator"))
            return;
        if (*oscMissing)
        {
            res.set_content(errorResponse("Oscillator not found: " + idStr).dump(), "application/json");
            return;
        }

        json oscJson;
        oscJson["id"] = osc->getId().id.toStdString();
        oscJson["name"] = osc->getName().toStdString();
        oscJson["visible"] = osc->isVisible();
        oscJson["opacity"] = osc->getOpacity();
        oscJson["lineWidth"] = osc->getLineWidth();
        res.set_content(successResponse(oscJson).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// handleStateReorderOscillators is the last handler in this translation unit.
// Project-bundle handlers (handleProjectSave/Close/Reopen) are defined in
// TestHttpServerProject.cpp — they are wired into setupStateRoutes above.
void TestHttpServer::handleStateReorderOscillators(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        const auto trackIdOpt = tryResolveTrackId(req, res);
        if (!trackIdOpt)
            return;
        const int trackId = *trackIdOpt;
        auto body = json::parse(req.body);
        int fromIndex = body.value("fromIndex", -1);
        int toIndex = body.value("toIndex", -1);

        if (fromIndex < 0 || toIndex < 0)
        {
            res.set_content(errorResponse("fromIndex and toIndex are required").dump(), "application/json");
            return;
        }

        auto oscillators = std::make_shared<json>(json::array());
        const auto result = runOnTrackSync(
            trackId,
            [fromIndex, toIndex, oscillators](TestTrack& track) {
                auto& state = track.getProcessor().getState();
                state.reorderOscillators(fromIndex, toIndex);
                if (auto* ed = dynamic_cast<MultiScoperPluginEditor*>(track.getEditor()))
                    ed->refreshPanels();
                for (const auto& osc : state.getOscillators())
                {
                    json oscJson;
                    oscJson["id"] = osc.getId().id.toStdString();
                    oscJson["name"] = osc.getName().toStdString();
                    oscJson["orderIndex"] = osc.getOrderIndex();
                    oscillators->push_back(oscJson);
                }
            },
            5000);

        if (respondIfTrackCallFailed(result, res, "No track available", "Timeout reordering oscillators"))
            return;

        res.set_content(successResponse(*oscillators).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

} // namespace multiscoper::test
