/*
    Oscil - Oscillator Handler Implementation
*/

#include "tools/test_server/OscillatorHandler.h"

#include "core/OscilState.h"
#include "core/Oscillator.h"
#include "core/Pane.h"
#include "ui/controllers/OscillatorPanelController.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "tools/test_server/TestServerUtils.h"

#include <algorithm>
#include <utility>

namespace oscil
{

namespace
{

nlohmann::json serializeOscillatorList(std::vector<Oscillator> oscillators)
{
    std::ranges::sort(oscillators,
                      [](const Oscillator& a, const Oscillator& b) { return a.getOrderIndex() < b.getOrderIndex(); });

    nlohmann::json oscList = nlohmann::json::array();
    for (const auto& osc : oscillators)
    {
        nlohmann::json oscInfo;
        oscInfo["id"] = osc.getId().id.toStdString();
        oscInfo["name"] = osc.getName().toStdString();
        oscInfo["orderIndex"] = osc.getOrderIndex();
        oscList.push_back(oscInfo);
    }
    return oscList;
}

} // namespace

nlohmann::json OscillatorHandler::updateOscillatorOnMessageThread(const std::string& idStr, int index,
                                                                  int processingMode, int visible)
{
    nlohmann::json response;
    auto& state = editor_.getProcessor().getState();
    auto oscillators = state.getOscillators();
    auto targetId = resolveOscillatorId(idStr, index, oscillators);

    bool found = false;
    for (auto& osc : oscillators)
    {
        if (osc.getId() != targetId)
            continue;

        if (processingMode >= 0 && processingMode <= static_cast<int>(ProcessingMode::Right))
        {
            auto mode = static_cast<ProcessingMode>(processingMode);
            osc.setProcessingMode(mode);
            if (auto* ctrl = editor_.getOscillatorPanelController())
                ctrl->oscillatorModeChanged(targetId, mode);
        }
        if (visible >= 0)
        {
            bool const isVisible = visible != 0;
            osc.setVisible(isVisible);
            if (auto* ctrl = editor_.getOscillatorPanelController())
                ctrl->oscillatorVisibilityChanged(targetId, isVisible);
        }
        found = true;
        break;
    }

    if (!found)
    {
        response["error"] = "Oscillator not found";
        return response;
    }

    response["status"] = "ok";
    response["oscillatorId"] = targetId.id.toStdString();
    if (processingMode >= 0)
        response["processingMode"] = processingMode;
    if (visible >= 0)
        response["visible"] = visible != 0;
    return response;
}

void OscillatorHandler::handleAddOscillator(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;

        auto& state = editor_.getProcessor().getState();
        int const countBefore = state.getOscillatorCount();

        // Simulate clicking the Add button
        // We need to call the same method the button would call
        // This is done by triggering the UI action

        // Create a new oscillator via the editor's mechanism
        auto& layoutManager = state.getLayoutManager();

        // Ensure we have at least one pane
        if (layoutManager.getPaneCount() == 0)
        {
            Pane defaultPane;
            defaultPane.setName("Pane 1");
            defaultPane.setOrderIndex(0);
            layoutManager.addPane(defaultPane);
        }

        // Get the first pane
        PaneId const targetPaneId = layoutManager.getPanes()[0].getId();

        // Create oscillator
        Oscillator osc;
        osc.setPaneId(targetPaneId);
        osc.setProcessingMode(ProcessingMode::FullStereo);
        osc.setName("Oscillator " + juce::String(state.getOscillatorCount() + 1));

        state.addOscillator(osc);

        int const countAfter = state.getOscillatorCount();

        response["status"] = "ok";
        response["oscillatorCount"] = countAfter;
        response["added"] = countAfter > countBefore;
        response["oscillatorId"] = osc.getId().id.toStdString();

        // Trigger UI refresh - must use refreshPanels to recreate pane components
        editor_.refreshPanels();

        return response;
    });

    sendJson(res, result);
}

void OscillatorHandler::handleDeleteOscillator(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        std::string const oscillatorId = body.value("id", "");
        int const index = body.value("index", -1);

        // NOLINTNEXTLINE(bugprone-exception-escape)
        auto result = runOnMessageThread([this, oscillatorId, index]() -> nlohmann::json {
            nlohmann::json response;
            auto& state = editor_.getProcessor().getState();
            auto oscillators = state.getOscillators();

            OscillatorId targetId;

            if (!oscillatorId.empty())
            {
                targetId.id = juce::String(oscillatorId);
            }
            else if (index >= 0 && std::cmp_less(index, oscillators.size()))
            {
                targetId = oscillators[static_cast<size_t>(index)].getId();
            }
            else
            {
                return jsonError("No valid oscillator specified");
            }

            int const countBefore = static_cast<int>(oscillators.size());
            state.removeOscillator(targetId);
            int const countAfter = state.getOscillatorCount();

            response["status"] = "ok";
            response["oscillatorCount"] = countAfter;
            response["deleted"] = countAfter < countBefore;

            // Trigger UI refresh - must use refreshPanels to recreate pane components
            editor_.refreshPanels();

            return response;
        });

        sendJson(res, result);
    }
    catch (const std::exception& e)
    {
        sendJson(res, jsonError(e.what()), 400);
    }
}

void OscillatorHandler::handleUpdateOscillator(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        int const oscillatorIndex = body.value("index", -1);
        std::string const oscillatorIdStr = body.value("id", "");
        int const processingMode = body.value("processingMode", -1);
        int const visible = body.value("visible", -1);

        if (oscillatorIndex < 0 && oscillatorIdStr.empty())
        {
            sendJson(res, jsonError("Either index or id required"), 400);
            return;
        }

        // NOLINTNEXTLINE(bugprone-exception-escape)
        auto result = runOnMessageThread([this, oscillatorIndex, oscillatorIdStr, processingMode, visible]() {
            return updateOscillatorOnMessageThread(oscillatorIdStr, oscillatorIndex, processingMode, visible);
        });
        sendJson(res, result);
    }
    catch (const std::exception& e)
    {
        sendJson(res, jsonError(e.what()), 400);
    }
}

void OscillatorHandler::handleGetOscillators(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        auto& state = editor_.getProcessor().getState();
        auto oscillators = state.getOscillators();

        nlohmann::json oscList = nlohmann::json::array();
        for (const auto& osc : oscillators)
        {
            nlohmann::json oscInfo;
            oscInfo["id"] = osc.getId().id.toStdString();
            oscInfo["name"] = osc.getName().toStdString();
            oscInfo["paneId"] = osc.getPaneId().id.toStdString();
            oscInfo["sourceId"] = osc.getSourceId().id.toStdString();
            oscList.push_back(oscInfo);
        }

        response["oscillators"] = oscList;
        response["count"] = static_cast<int>(oscillators.size());

        return response;
    });

    sendJson(res, result);
}

void OscillatorHandler::handleReorderOscillator(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        int const fromIndex = body.value("fromIndex", -1);
        int const toIndex = body.value("toIndex", -1);

        if (fromIndex < 0 || toIndex < 0)
        {
            sendJson(res, jsonError("fromIndex and toIndex are required"), 400);
            return;
        }

        auto result = runOnMessageThread([this, fromIndex, toIndex]() -> nlohmann::json {
            nlohmann::json response;
            auto& state = editor_.getProcessor().getState();
            auto oscillatorsBefore = state.getOscillators();

            if (std::cmp_greater_equal(fromIndex, oscillatorsBefore.size()) ||
                std::cmp_greater_equal(toIndex, oscillatorsBefore.size()))
            {
                response["error"] = "Index out of range";
                return response;
            }

            state.reorderOscillators(fromIndex, toIndex);
            editor_.refreshPanels();

            auto oscillatorsAfter = state.getOscillators();

            response["status"] = "ok";
            response["fromIndex"] = fromIndex;
            response["toIndex"] = toIndex;
            response["oscillators"] = serializeOscillatorList(oscillatorsAfter);
            return response;
        });

        sendJson(res, result);
    }
    catch (const std::exception& e)
    {
        sendJson(res, jsonError(e.what()), 400);
    }
}

// handleTestOscillatorReorder is in OscillatorHandlerTests.cpp

} // namespace oscil
