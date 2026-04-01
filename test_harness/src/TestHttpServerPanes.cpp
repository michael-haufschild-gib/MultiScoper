/*
    Oscil Test Harness - HTTP Server: Pane Management Handlers
*/

#include "core/OscilState.h"

#include "TestHttpServer.h"

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
        auto* track = resolveTrack(req);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        std::string name = body.value("name", "");

        auto& state = track->getProcessor().getState();
        auto& layoutManager = state.getLayoutManager();

        Pane pane;
        if (!name.empty())
            pane.setName(juce::String(name));
        else
            pane.setName("Pane " + juce::String(layoutManager.getPaneCount() + 1));
        pane.setOrderIndex(static_cast<int>(layoutManager.getPaneCount()));

        juce::WaitableEvent done;
        juce::MessageManager::callAsync([pane, track, &done]() mutable {
            track->getProcessor().getState().getLayoutManager().addPane(pane);
            done.signal();
        });
        done.wait(3000);

        json data;
        data["id"] = pane.getId().id.toStdString();
        data["name"] = pane.getName().toStdString();
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
        auto* track = resolveTrack(req);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        std::string idStr = body.value("id", "");
        if (idStr.empty())
        {
            res.set_content(errorResponse("Pane 'id' is required").dump(), "application/json");
            return;
        }

        PaneId paneId{juce::String(idStr)};
        auto& state = track->getProcessor().getState();
        auto& layoutManager = state.getLayoutManager();

        if (layoutManager.getPaneCount() <= 1)
        {
            res.set_content(errorResponse("Cannot remove the last pane").dump(), "application/json");
            return;
        }

        juce::WaitableEvent done;
        juce::MessageManager::callAsync([paneId, &state, &layoutManager, &done]() {
            reassignOrphansAndRemovePane(state, layoutManager, paneId);
            done.signal();
        });
        done.wait(3000);

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
        auto* track = resolveTrack(req);
        if (!track)
        {
            res.set_content(errorResponse("No track available").dump(), "application/json");
            return;
        }

        auto body = json::parse(req.body);
        std::string idStr = body.value("id", "");
        std::string paneIdStr = body.value("paneId", "");

        if (idStr.empty() || paneIdStr.empty())
        {
            res.set_content(errorResponse("'id' and 'paneId' are required").dump(), "application/json");
            return;
        }

        auto& state = track->getProcessor().getState();
        auto& layoutManager = state.getLayoutManager();
        OscillatorId oscId{juce::String(idStr)};
        PaneId targetPaneId{juce::String(paneIdStr)};

        auto existingOsc = state.getOscillator(oscId);
        if (!existingOsc.has_value())
        {
            res.set_content(errorResponse("Oscillator not found: " + idStr).dump(), "application/json");
            return;
        }

        // Validate target pane exists
        if (layoutManager.getPane(targetPaneId) == nullptr)
        {
            res.set_content(errorResponse("Target pane not found: " + paneIdStr).dump(), "application/json");
            return;
        }

        Oscillator osc = existingOsc.value();
        osc.setPaneId(targetPaneId);
        state.updateOscillator(osc);

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

} // namespace oscil::test
