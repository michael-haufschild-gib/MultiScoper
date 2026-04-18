/*
    Oscil Test Harness - HTTP Server: Pane Management Handlers
*/

#include "core/OscilState.h"
#include "ui/layout/PaneComponent.h"

#include "TestHttpServer.h"
#include "plugin/PluginEditor.h"
#include "plugin/PluginFactory.h"

namespace oscil::test
{

namespace
{

void reassignOrphansAndRemovePane(OscilState& state, PaneLayoutManager& layoutManager, PaneId paneId)
{
    PaneId fallback = PaneId::invalid();
    for (const auto& pane : layoutManager.getPanes())
        if (pane.getId() != paneId)
        {
            fallback = pane.getId();
            break;
        }

    for (auto& osc : state.getOscillators())
    {
        if (osc.getPaneId() == paneId)
        {
            osc.setPaneId(fallback);
            state.updateOscillator(osc);
        }
    }
    layoutManager.removePane(paneId);
}

} // namespace

void TestHttpServer::handlePaneAdd(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        const int trackId = resolveTrackId(req);
        auto body = json::parse(req.body);
        std::string name = body.value("name", "");

        auto pane = std::make_shared<Pane>();
        const auto result = runOnTrackSync(
            trackId,
            [pane, name](TestTrack& track) {
                auto& layoutManager = track.getProcessor().getState().getLayoutManager();
                if (!name.empty())
                    pane->setName(juce::String(name));
                else
                    pane->setName("Pane " + juce::String(layoutManager.getPaneCount() + 1));
                pane->setOrderIndex(static_cast<int>(layoutManager.getPaneCount()));
                layoutManager.addPane(*pane);
                if (auto* ed = dynamic_cast<OscilPluginEditor*>(track.getEditor()))
                    ed->refreshPanels();
            },
            5000);

        if (respondIfTrackCallFailed(result, res, "No track available", "Timeout adding pane"))
            return;

        json data;
        data["id"] = pane->getId().id.toStdString();
        data["name"] = pane->getName().toStdString();
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handlePaneRemove(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        const int trackId = resolveTrackId(req);
        auto body = json::parse(req.body);
        std::string idStr = body.value("id", "");
        if (idStr.empty())
        {
            res.set_content(errorResponse("Pane 'id' is required").dump(), "application/json");
            return;
        }

        PaneId paneId{juce::String(idStr)};
        auto lastPane = std::make_shared<bool>(false);

        const auto result = runOnTrackSync(
            trackId,
            [paneId, lastPane](TestTrack& track) {
                auto& state = track.getProcessor().getState();
                auto& layoutManager = state.getLayoutManager();
                if (layoutManager.getPaneCount() <= 1)
                {
                    *lastPane = true;
                    return;
                }
                reassignOrphansAndRemovePane(state, layoutManager, paneId);
                if (auto* ed = dynamic_cast<OscilPluginEditor*>(track.getEditor()))
                    ed->refreshPanels();
            },
            5000);

        if (respondIfTrackCallFailed(result, res, "No track available", "Timeout removing pane"))
            return;
        if (*lastPane)
        {
            res.set_content(errorResponse("Cannot remove the last pane").dump(), "application/json");
            return;
        }

        json data;
        data["id"] = idStr;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleOscillatorMove(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        const int trackId = resolveTrackId(req);
        auto body = json::parse(req.body);
        std::string idStr = body.value("id", "");
        std::string paneIdStr = body.value("paneId", "");

        if (idStr.empty() || paneIdStr.empty())
        {
            res.set_content(errorResponse("'id' and 'paneId' are required").dump(), "application/json");
            return;
        }

        OscillatorId oscId{juce::String(idStr)};
        PaneId targetPaneId{juce::String(paneIdStr)};

        auto oscMissing = std::make_shared<bool>(false);
        auto targetPaneMissing = std::make_shared<bool>(false);

        const auto result = runOnTrackSync(
            trackId,
            [oscId, targetPaneId, oscMissing, targetPaneMissing](TestTrack& track) {
                auto& state = track.getProcessor().getState();
                auto& layoutManager = state.getLayoutManager();
                auto existingOsc = state.getOscillator(oscId);
                if (!existingOsc.has_value())
                {
                    *oscMissing = true;
                    return;
                }
                if (layoutManager.getPane(targetPaneId) == nullptr)
                {
                    *targetPaneMissing = true;
                    return;
                }
                Oscillator osc = existingOsc.value();
                osc.setPaneId(targetPaneId);
                state.updateOscillator(osc);
                if (auto* ed = dynamic_cast<OscilPluginEditor*>(track.getEditor()))
                    ed->refreshPanels();
            },
            5000);

        if (respondIfTrackCallFailed(result, res, "No track available", "Timeout moving oscillator"))
            return;
        if (*oscMissing)
        {
            res.set_content(errorResponse("Oscillator not found: " + idStr).dump(), "application/json");
            return;
        }
        if (*targetPaneMissing)
        {
            res.set_content(errorResponse("Target pane not found: " + paneIdStr).dump(), "application/json");
            return;
        }

        json data;
        data["id"] = idStr;
        data["paneId"] = paneIdStr;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleLayoutInfo(const httplib::Request& req, httplib::Response& res)
{
    const int trackId = resolveTrackId(req);
    auto data = std::make_shared<json>();
    const auto result = runOnTrackSync(trackId, [data](TestTrack& track) {
        auto& layoutManager = track.getProcessor().getState().getLayoutManager();
        (*data)["columns"] = layoutManager.getColumnCount();
        (*data)["paneCount"] = static_cast<int>(layoutManager.getPaneCount());
    });

    if (respondIfTrackCallFailed(result, res, "No track available", "Timeout reading layout info"))
        return;
    res.set_content(successResponse(*data).dump(), "application/json");
}

void TestHttpServer::handleSetLayout(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        const int trackId = resolveTrackId(req);
        auto body = json::parse(req.body);
        int columns = body.value("columns", 1);
        if (columns < 1 || columns > 3)
        {
            res.set_content(errorResponse("columns must be 1, 2, or 3").dump(), "application/json");
            return;
        }

        auto layout = static_cast<ColumnLayout>(columns);
        auto resolvedColumns = std::make_shared<int>(0);

        const auto result = runOnTrackSync(trackId, [layout, resolvedColumns](TestTrack& track) {
            track.getProcessor().getState().setColumnLayout(layout);
            if (auto* ed = dynamic_cast<OscilPluginEditor*>(track.getEditor()))
                ed->refreshPanels();
            *resolvedColumns = track.getProcessor().getState().getLayoutManager().getColumnCount();
        });

        if (respondIfTrackCallFailed(result, res, "No track available", "Timeout setting layout"))
            return;

        json data;
        data["status"] = "ok";
        data["columns"] = *resolvedColumns;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

namespace
{

json buildAvailableArea(OscilPluginEditor* editor)
{
    int x = 0, y = 0, w = 0, h = 0;
    if (editor)
    {
        w = editor->getWidth();
        h = editor->getHeight();

        auto& paneComps = editor->getPaneComponents();
        if (!paneComps.empty() && paneComps[0] != nullptr)
        {
            if (auto* parent = paneComps[0]->getParentComponent())
            {
                x = parent->getX();
                y = parent->getY();
                w = parent->getWidth();
                h = parent->getHeight();
            }
        }
    }
    return {{"x", x}, {"y", y}, {"width", w}, {"height", h}};
}

json buildPaneBounds(const PaneId& paneId, OscilPluginEditor* editor)
{
    if (editor)
    {
        for (auto& comp : editor->getPaneComponents())
        {
            if (comp && comp->getPaneId() == paneId)
                return {{"x", comp->getX()},
                        {"y", comp->getY()},
                        {"width", comp->getWidth()},
                        {"height", comp->getHeight()}};
        }
    }
    return {{"x", 0}, {"y", 0}, {"width", 0}, {"height", 0}};
}

} // namespace

void TestHttpServer::handlePaneLayout(const httplib::Request& req, httplib::Response& res)
{
    const int trackId = resolveTrackId(req);

    auto data = std::make_shared<json>();
    const auto result = runOnTrackSync(
        trackId,
        [data](TestTrack& track) {
            auto& layoutManager = track.getProcessor().getState().getLayoutManager();
            auto* editor = dynamic_cast<OscilPluginEditor*>(track.getEditor());
            (*data)["columns"] = layoutManager.getColumnCount();
            (*data)["availableArea"] = buildAvailableArea(editor);

            json panesJson = json::array();
            for (int i = 0; i < static_cast<int>(layoutManager.getPanes().size()); ++i)
            {
                const auto& pane = layoutManager.getPanes()[static_cast<size_t>(i)];
                panesJson.push_back({{"index", i},
                                     {"id", pane.getId().id.toStdString()},
                                     {"name", pane.getName().toStdString()},
                                     {"columnIndex", pane.getColumnIndex()},
                                     {"bounds", buildPaneBounds(pane.getId(), editor)}});
            }
            (*data)["panes"] = panesJson;
        },
        5000);

    if (respondIfTrackCallFailed(result, res, "No track available", "Timeout building pane layout"))
        return;

    res.set_content(successResponse(*data).dump(), "application/json");
}

void TestHttpServer::handlePaneMove(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        const int trackId = resolveTrackId(req);
        auto body = json::parse(req.body);
        int fromIndex = body.value("fromIndex", -1);
        int toIndex = body.value("toIndex", -1);

        if (fromIndex < 0 || toIndex < 0)
        {
            res.set_content(errorResponse("fromIndex and toIndex required").dump(), "application/json");
            return;
        }

        enum class RangeErr
        {
            None,
            FromOutOfRange,
            ToOutOfRange
        };
        auto rangeErr = std::make_shared<RangeErr>(RangeErr::None);

        const auto result = runOnTrackSync(
            trackId,
            [fromIndex, toIndex, rangeErr](TestTrack& track) {
                auto& layoutManager = track.getProcessor().getState().getLayoutManager();
                auto& panes = layoutManager.getPanes();
                const int paneCount = static_cast<int>(panes.size());
                if (fromIndex >= paneCount)
                {
                    *rangeErr = RangeErr::FromOutOfRange;
                    return;
                }
                if (toIndex >= paneCount)
                {
                    *rangeErr = RangeErr::ToOutOfRange;
                    return;
                }
                const auto paneId = panes[static_cast<size_t>(fromIndex)].getId();
                layoutManager.movePane(paneId, toIndex);
            },
            5000);

        if (respondIfTrackCallFailed(result, res, "No track available", "Timeout moving pane"))
            return;
        if (*rangeErr == RangeErr::FromOutOfRange)
        {
            res.set_content(errorResponse("fromIndex out of range").dump(), "application/json");
            return;
        }
        if (*rangeErr == RangeErr::ToOutOfRange)
        {
            res.set_content(errorResponse("toIndex out of range").dump(), "application/json");
            return;
        }

        json data;
        data["status"] = "ok";
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleStatePanes(const httplib::Request& req, httplib::Response& res)
{
    const int trackId = resolveTrackId(req);
    auto panes = std::make_shared<json>(json::array());

    (void) runOnTrackSync(trackId, [panes](TestTrack& track) {
        auto& layoutManager = track.getProcessor().getState().getLayoutManager();
        for (const auto& pane : layoutManager.getPanes())
        {
            json paneJson;
            paneJson["id"] = pane.getId().id.toStdString();
            paneJson["name"] = pane.getName().toStdString();
            paneJson["column"] = pane.getColumnIndex();
            paneJson["order"] = pane.getOrderIndex();
            paneJson["collapsed"] = pane.isCollapsed();
            panes->push_back(paneJson);
        }
    });

    // NotFound or Timeout fall through to an empty array, preserving the
    // historical "best-effort" semantic this endpoint offered.
    res.set_content(successResponse(*panes).dump(), "application/json");
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

} // namespace oscil::test
