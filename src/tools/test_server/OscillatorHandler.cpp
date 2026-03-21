/*
    Oscil - Oscillator Handler Implementation
*/

#include "tools/test_server/OscillatorHandler.h"
#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "core/OscilState.h"
#include "core/Oscillator.h"
#include "core/Pane.h"
#include <algorithm>

namespace oscil
{

void OscillatorHandler::handleAddOscillator(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;

        auto& state = editor_.getProcessor().getState();
        int countBefore = static_cast<int>(state.getOscillators().size());

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
        PaneId targetPaneId = layoutManager.getPanes()[0].getId();

        // Create oscillator
        Oscillator osc;
        osc.setPaneId(targetPaneId);
        osc.setProcessingMode(ProcessingMode::FullStereo);
        osc.setName("Oscillator " + juce::String(state.getOscillators().size() + 1));

        state.addOscillator(osc);

        int countAfter = static_cast<int>(state.getOscillators().size());

        response["status"] = "ok";
        response["oscillatorCount"] = countAfter;
        response["added"] = countAfter > countBefore;
        response["oscillatorId"] = osc.getId().id.toStdString();

        // Trigger UI refresh - must use refreshPanels to recreate pane components
        editor_.refreshPanels();

        return response;
    });

    res.set_content(result.dump(), "application/json");
}

void OscillatorHandler::handleDeleteOscillator(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        std::string oscillatorId = body.value("id", "");
        int index = body.value("index", -1);

        auto result = runOnMessageThread([this, oscillatorId, index]() -> nlohmann::json {
            nlohmann::json response;
            auto& state = editor_.getProcessor().getState();
            auto oscillators = state.getOscillators();

            OscillatorId targetId;

            if (!oscillatorId.empty())
            {
                targetId.id = juce::String(oscillatorId);
            }
            else if (index >= 0 && index < static_cast<int>(oscillators.size()))
            {
                targetId = oscillators[static_cast<size_t>(index)].getId();
            }
            else
            {
                response["error"] = "No valid oscillator specified";
                return response;
            }

            int countBefore = static_cast<int>(oscillators.size());
            state.removeOscillator(targetId);
            int countAfter = static_cast<int>(state.getOscillators().size());

            response["status"] = "ok";
            response["oscillatorCount"] = countAfter;
            response["deleted"] = countAfter < countBefore;

            // Trigger UI refresh - must use refreshPanels to recreate pane components
            editor_.refreshPanels();

            return response;
        });

        res.set_content(result.dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        nlohmann::json error;
        error["error"] = e.what();
        res.status = 400;
        res.set_content(error.dump(), "application/json");
    }
}

void OscillatorHandler::handleUpdateOscillator(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        int oscillatorIndex = body.value("index", -1);
        std::string oscillatorIdStr = body.value("id", "");
        int processingMode = body.value("processingMode", -1);
        int visible = body.value("visible", -1);

        if (oscillatorIndex < 0 && oscillatorIdStr.empty())
        {
            nlohmann::json error;
            error["error"] = "Either index or id required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        auto result = runOnMessageThread([this, oscillatorIndex, oscillatorIdStr, processingMode, visible]() -> nlohmann::json {
            nlohmann::json response;
            auto& state = editor_.getProcessor().getState();
            auto oscillators = state.getOscillators();

            // Find the oscillator
            OscillatorId targetId;
            if (!oscillatorIdStr.empty())
            {
                targetId.id = juce::String(oscillatorIdStr);
            }
            else if (oscillatorIndex >= 0 && oscillatorIndex < static_cast<int>(oscillators.size()))
            {
                targetId = oscillators[static_cast<size_t>(oscillatorIndex)].getId();
            }

            // Find and update the oscillator
            bool found = false;
            for (auto& osc : oscillators)
            {
                if (osc.getId() == targetId)
                {
                    // Update processing mode if provided
                    if (processingMode >= 0 && processingMode <= 3)
                    {
                        auto mode = static_cast<ProcessingMode>(processingMode);
                        osc.setProcessingMode(mode);
                        if (auto* controller = editor_.getOscillatorPanelController())
                            controller->oscillatorModeChanged(targetId, mode);
                    }

                    // Update visibility if provided
                    if (visible >= 0)
                    {
                        bool isVisible = visible != 0;
                        osc.setVisible(isVisible);
                        if (auto* controller = editor_.getOscillatorPanelController())
                            controller->oscillatorVisibilityChanged(targetId, isVisible);
                    }

                    found = true;
                    break;
                }
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
        });

        res.set_content(result.dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        nlohmann::json error;
        error["error"] = e.what();
        res.status = 400;
        res.set_content(error.dump(), "application/json");
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

    res.set_content(result.dump(), "application/json");
}

void OscillatorHandler::handleReorderOscillator(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        int fromIndex = body.value("fromIndex", -1);
        int toIndex = body.value("toIndex", -1);

        if (fromIndex < 0 || toIndex < 0)
        {
            nlohmann::json error;
            error["error"] = "fromIndex and toIndex are required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        auto result = runOnMessageThread([this, fromIndex, toIndex]() -> nlohmann::json {
            nlohmann::json response;
            auto& state = editor_.getProcessor().getState();
            auto oscillatorsBefore = state.getOscillators();

            if (fromIndex >= static_cast<int>(oscillatorsBefore.size()) ||
                toIndex >= static_cast<int>(oscillatorsBefore.size()))
            {
                response["error"] = "Index out of range";
                return response;
            }

            // Perform the reorder
            state.reorderOscillators(fromIndex, toIndex);

            // Refresh the UI
            editor_.refreshPanels();

            // Get updated oscillators
            auto oscillatorsAfter = state.getOscillators();

            // Sort by orderIndex for response
            std::sort(oscillatorsAfter.begin(), oscillatorsAfter.end(),
                      [](const Oscillator& a, const Oscillator& b) {
                          return a.getOrderIndex() < b.getOrderIndex();
                      });

            nlohmann::json oscList = nlohmann::json::array();
            for (const auto& osc : oscillatorsAfter)
            {
                nlohmann::json oscInfo;
                oscInfo["id"] = osc.getId().id.toStdString();
                oscInfo["name"] = osc.getName().toStdString();
                oscInfo["orderIndex"] = osc.getOrderIndex();
                oscList.push_back(oscInfo);
            }

            response["status"] = "ok";
            response["fromIndex"] = fromIndex;
            response["toIndex"] = toIndex;
            response["oscillators"] = oscList;

            return response;
        });

        res.set_content(result.dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        nlohmann::json error;
        error["error"] = e.what();
        res.status = 400;
        res.set_content(error.dump(), "application/json");
    }
}

// handleTestOscillatorReorder is in OscillatorHandlerTests.cpp

} // namespace oscil
