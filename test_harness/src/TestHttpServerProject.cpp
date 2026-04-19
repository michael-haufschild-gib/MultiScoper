/*
    MultiScoper Test Harness - HTTP Server: Project-Bundle Handlers

    The "project" endpoints bundle every live track into a single JSON file so
    tests can round-trip a multi-track session the same way a DAW does when
    saving a project. Each TestTrack is serialized independently via the
    normal plugin state XML path, then unpacked on reopen.

    Thread model: all DAW/track mutation happens on the message thread via
    runOnMessageThreadBlocking. See ADR-016 for the capture rules.
*/

#include "core/MultiScoperState.h"
#include "core/dsp/TimingEngine.h"

#include "TestHttpServer.h"
#include "plugin/PluginEditor.h"

namespace multiscoper::test
{

namespace
{

// Serialize a single track to the project-bundle entry shape. Must run on
// the message thread: reads the state tree and touches the timing engine's
// ValueTree mirror, neither of which is safe from worker threads.
json serializeTrackForBundle(TestTrack& track, int ordinal)
{
    auto& processor = track.getProcessor();

    // Sync timing into the state tree before serializing, matching the
    // production getStateInformation path via updateCachedState.
    auto& stateTree = processor.getState().getState();
    auto timingState = processor.getTimingEngine().toValueTree();
    auto existingTiming = stateTree.getChildWithName(StateIds::Timing);
    if (existingTiming.isValid())
        stateTree.removeChild(existingTiming, nullptr);
    stateTree.appendChild(timingState, nullptr);

    json entry;
    entry["ordinal"] = ordinal;
    entry["name"] = track.getName().toStdString();
    entry["xml"] = processor.getState().toXmlString().toStdString();
    return entry;
}

} // anonymous namespace

// Bundles every live track's serialized state into a single JSON file:
//   { "tracks": [ { "ordinal": 0, "xml": "<MultiScoperState...>" }, ... ] }
//
// ``ordinal`` is the write-order position so /project/reopen can recreate
// tracks in the same order the saved session had. The track index in the
// live DAW may include null slots (from /daw/track/remove) — we skip those
// and compact, since a reopened project always starts from a clean slate.
void TestHttpServer::handleProjectSave(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = req.body.empty() ? json::object() : json::parse(req.body);
        std::string path = body.value("path", "/tmp/multiscoper_project.json");

        auto bundle = std::make_shared<json>(json::object());
        (*bundle)["tracks"] = json::array();

        const bool ok = runOnMessageThreadBlocking(
            [this, bundle]() {
                int ordinal = 0;
                for (int i = 0; i < daw_.getNumTracks(); ++i)
                {
                    auto* track = daw_.getTrack(i);
                    if (track == nullptr)
                        continue;
                    (*bundle)["tracks"].push_back(serializeTrackForBundle(*track, ordinal++));
                }
            },
            5000, "handleProjectSave");
        if (!ok)
        {
            res.status = 504;
            res.set_content(errorResponse("Timeout serializing project").dump(), "application/json");
            return;
        }

        juce::File file(path);
        const bool written = file.replaceWithText(juce::String(bundle->dump()));
        if (written && file.existsAsFile())
        {
            json data;
            data["path"] = path;
            data["trackCount"] = (*bundle)["tracks"].size();
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Failed to write project bundle to: " + path).dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

// Destroys every live track. Each TestTrack dtor runs the plugin
// processor's unregisterInstance path, so the shared InstanceRegistry ends
// empty — matching what happens when a DAW closes a project.
void TestHttpServer::handleProjectClose(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto removedCount = std::make_shared<int>(0);
    const bool ok = runOnMessageThreadBlocking(
        [this, removedCount]() {
            // Iterate from the end so we don't have to worry about the
            // compaction semantics of removeTrack vs. slot reuse.
            for (int i = daw_.getNumTracks() - 1; i >= 0; --i)
            {
                if (daw_.getTrack(i) == nullptr)
                    continue;
                if (daw_.removeTrack(i))
                    ++(*removedCount);
            }
        },
        10000, "handleProjectClose");
    if (!ok)
    {
        res.status = 504;
        res.set_content(errorResponse("Timeout closing project").dump(), "application/json");
        return;
    }
    json data;
    data["removed"] = *removedCount;
    data["remaining"] = daw_.getActiveTrackCount();
    res.set_content(successResponse(data).dump(), "application/json");
}

int TestHttpServer::reopenTrackFromBundleEntry(const json& entry, std::string& failureMessage)
{
    const std::string name = entry.value("name", "");
    const std::string xml = entry.value("xml", "");
    const int ordinal = entry.value("ordinal", -1);

    const int index = daw_.addTrack(juce::String(name));
    auto* track = daw_.getTrack(index);
    if (track == nullptr)
    {
        failureMessage = "addTrack failed for ordinal " + std::to_string(ordinal);
        return -1;
    }

    auto* editor = dynamic_cast<MultiScoperPluginEditor*>(track->getEditor());
    if (!restoreLoadedState(track->getProcessor(), editor, juce::String(xml)))
    {
        failureMessage = "restoreLoadedState failed for ordinal " + std::to_string(ordinal);
        return -1;
    }

    // setStateInformation may have swapped the processor's SourceId (when
    // the persisted trackIdentifier differs from the fresh UUID
    // prepareToPlay registered under). Refresh the cache so /daw/tracks
    // returns the post-restore SourceId.
    track->refreshSourceIdFromProcessor();
    return index;
}

// Recreates tracks from a saved bundle and restores each one's state. The
// bundle is the file produced by /project/save. The caller is responsible
// for invoking /project/close first if any live tracks remain — reopen
// does NOT implicitly tear down existing tracks so callers can script
// partial restores in future tests.
void TestHttpServer::handleProjectReopen(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string path = body.value("path", "/tmp/multiscoper_project.json");

        juce::File file(path);
        if (!file.existsAsFile())
        {
            res.set_content(errorResponse("Bundle not found: " + path).dump(), "application/json");
            return;
        }

        auto bundle = std::make_shared<json>(json::parse(file.loadFileAsString().toStdString()));
        if (!bundle->contains("tracks") || !(*bundle)["tracks"].is_array())
        {
            res.set_content(errorResponse("Bundle missing 'tracks' array").dump(), "application/json");
            return;
        }

        // Preserve ordinal order so the reopened session mirrors the saved
        // session's track layout.
        auto entries = std::make_shared<std::vector<json>>();
        for (const auto& entry : (*bundle)["tracks"])
            entries->push_back(entry);
        std::sort(entries->begin(), entries->end(),
                  [](const json& a, const json& b) { return a.value("ordinal", 0) < b.value("ordinal", 0); });

        auto createdIndices = std::make_shared<std::vector<int>>();
        auto failures = std::make_shared<std::vector<std::string>>();

        const bool ok = runOnMessageThreadBlocking(
            [this, entries, createdIndices, failures]() {
                for (const auto& entry : *entries)
                {
                    std::string failureMessage;
                    const int index = reopenTrackFromBundleEntry(entry, failureMessage);
                    if (index < 0)
                        failures->push_back(std::move(failureMessage));
                    else
                        createdIndices->push_back(index);
                }
            },
            15000, "handleProjectReopen");

        if (!ok)
        {
            res.status = 504;
            res.set_content(errorResponse("Timeout reopening project").dump(), "application/json");
            return;
        }

        json data;
        data["trackIndices"] = *createdIndices;
        data["failures"] = *failures;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

} // namespace multiscoper::test
