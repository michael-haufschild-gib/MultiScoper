/*
    Oscil - Layout Handler Implementation
*/

#include "tools/test_server/LayoutHandler.h"

#include "core/OscilState.h"
#include "core/Pane.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"

namespace oscil
{

void LayoutHandler::handleGetLayout(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        auto& layoutManager = editor_.getProcessor().getState().getLayoutManager();

        int columns = layoutManager.getColumnCount();
        response["columns"] = columns;
        response["paneCount"] = layoutManager.getPaneCount();

        // Get editor bounds
        auto editorBounds = editor_.getLocalBounds();
        response["editorWidth"] = editorBounds.getWidth();
        response["editorHeight"] = editorBounds.getHeight();

        return response;
    });

    res.set_content(result.dump(), "application/json");
}

void LayoutHandler::handleSetColumnLayout(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        int columns = body.value("columns", 1);

        if (columns < 1 || columns > 3)
        {
            nlohmann::json error;
            error["error"] = "columns must be 1, 2, or 3";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        runOnMessageThread([this, columns]() {
            auto& state = editor_.getProcessor().getState();
            state.setColumnLayout(static_cast<ColumnLayout>(columns));
            // Trigger UI refresh
            editor_.resized();
        });

        nlohmann::json response;
        response["status"] = "ok";
        response["columns"] = columns;
        res.set_content(response.dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        nlohmann::json error;
        error["error"] = e.what();
        res.status = 400;
        res.set_content(error.dump(), "application/json");
    }
}

void LayoutHandler::handleGetPaneBounds(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        auto& layoutManager = editor_.getProcessor().getState().getLayoutManager();

        // Approximate available area for pane layout.
        // These constants must stay in sync with PluginEditorLayout.
        // A future improvement would expose getContentArea() from the editor.
        auto editorBounds = editor_.getLocalBounds();
        static constexpr int STATUS_BAR_HEIGHT = 24; // PluginEditorLayout::STATUS_BAR_HEIGHT
        int sidebarWidth = 250;                      // Approximate (user-adjustable)

        int availableWidth = editorBounds.getWidth() - sidebarWidth;
        int availableHeight = editorBounds.getHeight() - STATUS_BAR_HEIGHT;

        juce::Rectangle<int> availableArea(0, 0, availableWidth, availableHeight);

        response["availableArea"] = {{"x", 0}, {"y", 0}, {"width", availableWidth}, {"height", availableHeight}};

        nlohmann::json panes = nlohmann::json::array();
        const auto& paneList = layoutManager.getPanes();

        for (size_t i = 0; i < paneList.size(); ++i)
        {
            auto bounds = layoutManager.getPaneBounds(static_cast<int>(i), availableArea);
            nlohmann::json paneInfo;
            paneInfo["index"] = i;
            paneInfo["id"] = paneList[i].getId().id.toStdString();
            paneInfo["name"] = paneList[i].getName().toStdString();
            paneInfo["columnIndex"] = paneList[i].getColumnIndex();
            paneInfo["bounds"] = {{"x", bounds.getX()},
                                  {"y", bounds.getY()},
                                  {"width", bounds.getWidth()},
                                  {"height", bounds.getHeight()}};
            panes.push_back(paneInfo);
        }

        response["panes"] = panes;
        response["columns"] = layoutManager.getColumnCount();

        return response;
    });

    res.set_content(result.dump(), "application/json");
}

void LayoutHandler::handleMovePane(const httplib::Request& req, httplib::Response& res)
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
            auto& layoutManager = editor_.getProcessor().getState().getLayoutManager();

            const auto& panes = layoutManager.getPanes();
            if (fromIndex >= static_cast<int>(panes.size()) || toIndex >= static_cast<int>(panes.size()))
            {
                response["error"] = "Index out of range";
                return response;
            }

            PaneId movedPaneId = panes[static_cast<size_t>(fromIndex)].getId();
            layoutManager.movePane(movedPaneId, toIndex);

            response["status"] = "ok";
            response["fromIndex"] = fromIndex;
            response["toIndex"] = toIndex;

            // Trigger UI refresh
            editor_.resized();

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

} // namespace oscil
