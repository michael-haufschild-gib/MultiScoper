/*
    Oscil Test Harness - HTTP Server: State Handlers
*/

#include "core/OscilState.h"
#include "core/dsp/TimingEngine.h"

#include "TestAudioGenerator.h"
#include "TestElementRegistry.h"
#include "TestHttpServer.h"
#include "plugin/PluginEditor.h"
#include "plugin/PluginFactory.h"

namespace oscil::test
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
void configureOscillatorFromJson(Oscillator& osc, const json& body, OscilState& state, TestTrack* track)
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
}

void TestHttpServer::handleStateReset(const httplib::Request& req, httplib::Response& res)
{
    juce::Logger::writeToLog("[Harness] State reset requested");
    const int trackId = resolveTrackId(req);

    (void) runOnTrackSync(
        trackId,
        [](TestTrack& track) {
            auto& state = track.getProcessor().getState();
            for (const auto& osc : state.getOscillators())
                state.removeOscillator(osc.getId());
            auto& lm = state.getLayoutManager();
            // Snapshot pane IDs before mutation — getPanes() returns a reference
            // to the underlying vector, which removePane() erases from.
            std::vector<PaneId> paneIds;
            paneIds.reserve(lm.getPanes().size());
            for (const auto& pane : lm.getPanes())
                paneIds.push_back(pane.getId());
            for (const auto& id : paneIds)
                lm.removePane(id);
            track.getProcessor().getTimingEngine().setConfig(EngineTimingConfig{});

            // setConfig() writes the engine SeqLock directly and bypasses the
            // listener chain. Without this sync, the TimingPresenter keeps its
            // stale cached mode/hostSync/BPM, and the next click(MELODIC_SEG)
            // or set_bpm() from a fresh test is suppressed by the presenter's
            // `if (currentMode_ != mode)` guards — leaving the engine in TIME
            // mode so BPM changes never recalculate displaySamples.
            if (auto* editor = track.getEditor())
                if (auto* oscilEditor = dynamic_cast<OscilPluginEditor*>(editor))
                    oscilEditor->refreshTimingSidebarFromEngine();
        },
        5000);

    resetAudioAndTransport();
    resetOptionsControls();

    // Do NOT clear the element registry here — components that are still alive
    // (sidebar, buttons, timing controls) keep their registrations.  Components
    // tied to removed oscillators/panes will self-unregister via their RAII
    // TestRegistration destructors when state listeners destroy them.
    res.set_content(successResponse().dump(), "application/json");
}

void TestHttpServer::resetAudioAndTransport()
{
    // Hop to the message thread so add/remove can't invalidate track slots
    // beneath the generator resets. The transport object itself is stable
    // for the life of the DAW, but the track vector is not.
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

void TestHttpServer::handleStateSave(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string path = body.value("path", "/tmp/state.xml");
        const int trackId = resolveTrackId(req);

        auto xml = std::make_shared<juce::String>();
        const auto result = runOnTrackSync(
            trackId,
            [xml](TestTrack& track) {
                auto& processor = track.getProcessor();
                auto& stateTree = processor.getState().getState();
                auto timingState = processor.getTimingEngine().toValueTree();
                auto existingTiming = stateTree.getChildWithName(StateIds::Timing);
                if (existingTiming.isValid())
                    stateTree.removeChild(existingTiming, nullptr);
                stateTree.appendChild(timingState, nullptr);
                *xml = processor.getState().toXmlString();
            },
            5000);

        if (respondIfTrackCallFailed(result, res, "No tracks available", "Timeout serializing state"))
            return;

        juce::File file(path);
        bool written = file.replaceWithText(*xml);
        if (written && file.existsAsFile())
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Failed to write state to: " + path).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

bool TestHttpServer::restoreLoadedState(OscilPluginProcessor& processor, OscilPluginEditor* editor,
                                        const juce::String& xml)
{
    auto& state = processor.getState();
    if (!state.fromXmlString(xml))
        return false;

    auto timingTree = state.getState().getChildWithName(StateIds::Timing);
    if (timingTree.isValid())
        processor.getTimingEngine().fromValueTree(timingTree);

    uiController_.toggle("sidebar_options_gridToggle", state.isShowGridEnabled());
    uiController_.setSliderValue("sidebar_options_gainSlider", static_cast<double>(state.getGainDb()));
    uiController_.selectById("sidebar_options_layoutDropdown", juce::String(static_cast<int>(state.getColumnLayout())));

    if (editor)
        editor->refreshPanels();

    return true;
}

void TestHttpServer::handleStateLoad(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string path = body.value("path", "");

        juce::File file(path);
        if (!file.existsAsFile())
        {
            res.set_content(errorResponse("File not found").dump(), "application/json");
            return;
        }

        juce::String xml = file.loadFileAsString();
        const int trackId = resolveTrackId(req);

        auto success = std::make_shared<bool>(false);
        const auto result = runOnTrackSync(
            trackId,
            [this, success, xml](TestTrack& track) {
                auto& processor = track.getProcessor();
                auto* editor = dynamic_cast<OscilPluginEditor*>(track.getEditor());
                *success = restoreLoadedState(processor, editor, xml);
            },
            5000);

        if (respondIfTrackCallFailed(result, res, "No tracks available", "Timeout restoring state"))
            return;

        if (*success)
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Failed to restore state from XML").dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStateOscillators(const httplib::Request& req, httplib::Response& res)
{
    const int trackId = resolveTrackId(req);
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
        const int trackId = resolveTrackId(req);
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
        const int trackId = resolveTrackId(req);
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
                if (auto* ed = dynamic_cast<OscilPluginEditor*>(track.getEditor()))
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

void TestHttpServer::handleStateReorderOscillators(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        const int trackId = resolveTrackId(req);
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
                if (auto* ed = dynamic_cast<OscilPluginEditor*>(track.getEditor()))
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

} // namespace oscil::test
