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
    // Name
    std::string name = body.value("name", "");
    if (name.empty())
        name = "Oscillator " + std::to_string(state.getOscillators().size() + 1);
    osc.setName(name);

    // Source ID
    std::string sourceIdStr = body.value("sourceId", "");
    if (!sourceIdStr.empty())
        osc.setSourceId(SourceId{juce::String(sourceIdStr)});
    else if (track->getProcessor().getSourceId().isValid())
        osc.setSourceId(track->getProcessor().getSourceId());

    // Pane ID
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

    // Colour
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

void TestHttpServer::handleStateReset(const httplib::Request&, httplib::Response& res)
{
    juce::Logger::writeToLog("[Harness] State reset requested");
    auto* track = daw_.getTrack(0);
    if (track)
    {
        // Block until message thread completes the removal to prevent race
        // conditions where the next operation starts before reset finishes.
        juce::WaitableEvent done;
        juce::MessageManager::callAsync([track, &done]() {
            auto& state = track->getProcessor().getState();
            auto oscillators = state.getOscillators();
            for (const auto& osc : oscillators)
                state.removeOscillator(osc.getId());

            auto& layoutManager = state.getLayoutManager();
            auto panes = layoutManager.getPanes();
            for (const auto& pane : panes)
                layoutManager.removePane(pane.getId());

            // Reset timing engine to defaults (TIME mode, 500ms, 120 BPM)
            auto& timingEngine = track->getProcessor().getTimingEngine();
            EngineTimingConfig defaultTimingConfig;
            timingEngine.setConfig(defaultTimingConfig);

            // Wait for queued refreshPanels to complete before signalling
            juce::MessageManager::callAsync([&done]() { done.signal(); });
        });
        done.wait(5000);
    }

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

        if (auto* track = daw_.getTrack(0))
        {
            juce::String xml = track->getProcessor().getState().toXmlString();
            juce::File(path).replaceWithText(xml);
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("No tracks available").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
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
        if (auto* track = daw_.getTrack(0))
        {
            (void) track->getProcessor().getState().fromXmlString(xml);
            res.set_content(successResponse().dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("No tracks available").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStateOscillators(const httplib::Request&, httplib::Response& res)
{
    json oscillators = json::array();

    if (auto* track = daw_.getTrack(0))
    {
        auto& state = track->getProcessor().getState();
        auto oscList = state.getOscillators();

        std::sort(oscList.begin(), oscList.end(),
                  [](const auto& a, const auto& b) { return a.getOrderIndex() < b.getOrderIndex(); });

        for (const auto& osc : oscList)
            oscillators.push_back(oscillatorToJson(osc));
    }

    res.set_content(successResponse(oscillators).dump(), "application/json");
}

void TestHttpServer::handleStateAddOscillator(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        auto& state = track->getProcessor().getState();
        auto& layoutManager = state.getLayoutManager();

        if (layoutManager.getPaneCount() == 0)
        {
            Pane defaultPane;
            defaultPane.setName("Pane 1");
            defaultPane.setOrderIndex(0);
            layoutManager.addPane(defaultPane);
        }

        Oscillator osc;
        configureOscillatorFromJson(osc, body, state, track);

        json oscJson = oscillatorToJson(osc);

        juce::Logger::writeToLog("[Harness] Adding oscillator: " + osc.getName() + " source=" + osc.getSourceId().id +
                                 " pane=" + osc.getPaneId().id);
        Oscillator oscCopy = osc;
        juce::WaitableEvent done;
        juce::MessageManager::callAsync([oscCopy, track, &done]() mutable {
            track->getProcessor().getState().addOscillator(oscCopy);
            // The addOscillator triggers valueTreeChildAdded which queues
            // refreshPanels via callAsync.  Queue another callback AFTER
            // refreshPanels so we only signal done once the UI is updated.
            juce::MessageManager::callAsync([&done]() { done.signal(); });
        });
        done.wait(5000);

        res.set_content(successResponse(oscJson).dump(), "application/json");
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
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        std::string idStr = body.value("id", "");
        if (idStr.empty())
        {
            res.set_content(errorResponse("Oscillator 'id' is required").dump(), "application/json");
            return;
        }

        auto& state = track->getProcessor().getState();
        OscillatorId oscId{juce::String(idStr)};

        auto existingOsc = state.getOscillator(oscId);
        if (!existingOsc.has_value())
        {
            res.set_content(errorResponse("Oscillator not found: " + idStr).dump(), "application/json");
            return;
        }

        Oscillator osc = existingOsc.value();
        applyOscillatorJsonUpdates(osc, body);
        state.updateOscillator(osc);

        // Refresh UI so sidebar list reflects the updated oscillator state
        auto* editor = track->getEditor();
        if (editor)
        {
            juce::WaitableEvent refreshDone;
            juce::MessageManager::callAsync([editor, &refreshDone]() {
                if (auto* oscilEditor = dynamic_cast<OscilPluginEditor*>(editor))
                    oscilEditor->refreshPanels();
                refreshDone.signal();
            });
            refreshDone.wait(3000);
        }

        json oscJson;
        oscJson["id"] = osc.getId().id.toStdString();
        oscJson["name"] = osc.getName().toStdString();
        oscJson["visible"] = osc.isVisible();
        oscJson["opacity"] = osc.getOpacity();
        oscJson["lineWidth"] = osc.getLineWidth();
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
        auto* track = daw_.getTrack(0);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        int fromIndex = body.value("fromIndex", -1);
        int toIndex = body.value("toIndex", -1);

        if (fromIndex < 0 || toIndex < 0)
        {
            res.set_content(errorResponse("fromIndex and toIndex are required").dump(), "application/json");
            return;
        }

        auto& state = track->getProcessor().getState();
        state.reorderOscillators(fromIndex, toIndex);

        json oscillators = json::array();
        for (const auto& osc : state.getOscillators())
        {
            json oscJson;
            oscJson["id"] = osc.getId().id.toStdString();
            oscJson["name"] = osc.getName().toStdString();
            oscJson["orderIndex"] = osc.getOrderIndex();
            oscillators.push_back(oscJson);
        }

        res.set_content(successResponse(oscillators).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStatePanes(const httplib::Request&, httplib::Response& res)
{
    json panes = json::array();

    if (auto* track = daw_.getTrack(0))
    {
        auto& layoutManager = track->getProcessor().getState().getLayoutManager();

        for (const auto& pane : layoutManager.getPanes())
        {
            json paneJson;
            paneJson["id"] = pane.getId().id.toStdString();
            paneJson["name"] = pane.getName().toStdString();
            paneJson["column"] = pane.getColumnIndex();
            paneJson["order"] = pane.getOrderIndex();
            paneJson["collapsed"] = pane.isCollapsed();
            panes.push_back(paneJson);
        }
    }

    res.set_content(successResponse(panes).dump(), "application/json");
}

void TestHttpServer::handleStateSources(const httplib::Request&, httplib::Response& res)
{
    json sources = json::array();

    auto allSources = PluginFactory::getInstance().getInstanceRegistry().getAllSources();
    for (const auto& source : allSources)
    {
        json sourceJson;
        sourceJson["id"] = source.sourceId.id.toStdString();
        sourceJson["name"] = source.name.toStdString();
        sourceJson["channels"] = source.channelCount;
        sourceJson["sampleRate"] = source.sampleRate;
        sources.push_back(sourceJson);
    }

    res.set_content(successResponse(sources).dump(), "application/json");
}

// handleStateDeleteOscillator and handleWaveformState are in TestHttpServerWaveform.cpp

} // namespace oscil::test
