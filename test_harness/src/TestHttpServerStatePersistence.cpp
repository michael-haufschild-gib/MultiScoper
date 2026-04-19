/*
    MultiScoper Test Harness - HTTP Server: Single-Track State Persistence

    The /state/save and /state/load endpoints persist a single track's plugin
    state to/from an XML file. This is the raw XML path (no multi-track
    bundling) — see TestHttpServerProject.cpp for the multi-track equivalent.

    restoreLoadedState is defined here as well because it is the shared
    sidebar-refresh logic used by both /state/load and /project/reopen;
    keeping it next to handleStateLoad keeps the "apply loaded XML" story
    in one translation unit.
*/

#include "core/MultiScoperState.h"
#include "core/dsp/TimingEngine.h"

#include "TestHttpServer.h"
#include "plugin/PluginEditor.h"

namespace multiscoper::test
{

void TestHttpServer::handleStateSave(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string path = body.value("path", "/tmp/state.xml");
        const auto trackIdOpt = tryResolveTrackId(req, res);
        if (!trackIdOpt)
            return;
        const int trackId = *trackIdOpt;

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

bool TestHttpServer::restoreLoadedState(MultiScoperPluginProcessor& processor, MultiScoperPluginEditor* editor,
                                        const juce::String& xml)
{
    // Prefer the production setStateInformation path so every restore hits
    // the same code the DAW drives — including reRegisterUnderPersistedTrackIdentifier,
    // the branch that rebinds this instance to its persisted SourceId. The
    // legacy direct-fromXmlString path bypassed that and silently broke the
    // cross-session binding invariant exercised by test_project_save_reload.
    const auto utf8 = xml.toRawUTF8();
    const int sizeBytes = static_cast<int>(xml.getNumBytesAsUTF8());
    processor.setStateInformation(utf8, sizeBytes);

    // setStateInformation is synchronous when invoked on the message thread
    // (which this path always is — see runOnMessageThreadBlocking above).
    auto& state = processor.getState();

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
        const auto trackIdOpt = tryResolveTrackId(req, res);
        if (!trackIdOpt)
            return;
        const int trackId = *trackIdOpt;

        auto success = std::make_shared<bool>(false);
        const auto result = runOnTrackSync(
            trackId,
            [this, success, xml](TestTrack& track) {
                auto& processor = track.getProcessor();
                auto* editor = dynamic_cast<MultiScoperPluginEditor*>(track.getEditor());
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

} // namespace multiscoper::test
