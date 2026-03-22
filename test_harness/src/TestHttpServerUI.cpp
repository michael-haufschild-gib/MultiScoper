/*
    Oscil Test Harness - HTTP Server: UI Mouse Interaction Handlers
*/

#include "TestHttpServer.h"
#include "TestElementRegistry.h"

namespace oscil::test
{

void TestHttpServer::setupUIMouseRoutes()
{
    server_->Post("/ui/click", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIClick(req, res);
    });
    server_->Post("/ui/doubleClick", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIDoubleClick(req, res);
    });
    server_->Post("/ui/rightClick", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIRightClick(req, res);
    });
    server_->Post("/ui/hover", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIHover(req, res);
    });
    server_->Post("/ui/select", [this](const httplib::Request& req, httplib::Response& res) {
        handleUISelect(req, res);
    });
    server_->Post("/ui/toggle", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIToggle(req, res);
    });
    server_->Post("/ui/slider", [this](const httplib::Request& req, httplib::Response& res) {
        handleUISlider(req, res);
    });
    server_->Post("/ui/slider/increment", [this](const httplib::Request& req, httplib::Response& res) {
        handleUISliderIncrement(req, res);
    });
    server_->Post("/ui/slider/decrement", [this](const httplib::Request& req, httplib::Response& res) {
        handleUISliderDecrement(req, res);
    });
    server_->Post("/ui/slider/reset", [this](const httplib::Request& req, httplib::Response& res) {
        handleUISliderReset(req, res);
    });
    server_->Post("/ui/drag", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIDrag(req, res);
    });
    server_->Post("/ui/dragOffset", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIDragOffset(req, res);
    });
    server_->Post("/ui/scroll", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIScroll(req, res);
    });
}

void TestHttpServer::handleUIClick(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.click(elementId))
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIDoubleClick(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.doubleClick(elementId))
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIRightClick(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.rightClick(elementId))
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIHover(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        int durationMs = body.value("durationMs", 500);

        if (uiController_.hover(elementId, durationMs))
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISelect(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        std::string itemText = body.value("itemText", "");

        bool success = false;

        if (body.contains("itemId"))
        {
            if (body["itemId"].is_string())
            {
                std::string itemIdStr = body["itemId"].get<std::string>();
                success = uiController_.selectById(elementId, itemIdStr);
            }
            else if (body["itemId"].is_number_integer())
            {
                int itemId = body["itemId"].get<int>();
                success = uiController_.select(elementId, itemId);
            }
        }
        else if (!itemText.empty())
        {
            success = uiController_.selectByText(elementId, itemText);
        }

        if (success)
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Failed to select item").dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIToggle(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        bool value = body.value("value", false);

        if (uiController_.toggle(elementId, value))
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Element not found or not toggleable").dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISlider(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        double value = body.value("value", 0.0);

        if (uiController_.setSliderValue(elementId, value))
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Element not found or not a slider").dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISliderIncrement(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.incrementSlider(elementId))
        {
            json data;
            data["value"] = uiController_.getSliderValue(elementId);
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found or not a slider").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISliderDecrement(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.decrementSlider(elementId))
        {
            json data;
            data["value"] = uiController_.getSliderValue(elementId);
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found or not a slider").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUISliderReset(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.resetSliderToDefault(elementId))
        {
            json data;
            data["value"] = uiController_.getSliderValue(elementId);
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Element not found or not a slider").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIDrag(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string from = body.value("from", "");
        std::string to = body.value("to", "");

        bool shift = body.value("shift", false);
        bool alt = body.value("alt", false);
        bool ctrl = body.value("ctrl", false);

        bool success = false;
        if (shift || alt || ctrl)
        {
            ModifierKeyState mods;
            mods.shift = shift;
            mods.alt = alt;
            mods.ctrl = ctrl;
            success = uiController_.dragWithModifiers(from, to, mods);
        }
        else
        {
            success = uiController_.drag(from, to);
        }

        if (success)
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Failed to drag").dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIDragOffset(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        int deltaX = body.value("deltaX", 0);
        int deltaY = body.value("deltaY", 0);

        bool shift = body.value("shift", false);
        bool alt = body.value("alt", false);
        bool ctrl = body.value("ctrl", false);

        bool success = false;
        if (shift || alt || ctrl)
        {
            ModifierKeyState mods;
            mods.shift = shift;
            mods.alt = alt;
            mods.ctrl = ctrl;
            success = uiController_.dragByOffsetWithModifiers(elementId, deltaX, deltaY, mods);
        }
        else
        {
            success = uiController_.dragByOffset(elementId, deltaX, deltaY);
        }

        if (success)
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Failed to drag by offset").dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIScroll(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        float deltaY = body.value("deltaY", 0.0f);
        float deltaX = body.value("deltaX", 0.0f);

        bool shift = body.value("shift", false);
        bool alt = body.value("alt", false);
        bool ctrl = body.value("ctrl", false);

        ScrollResult result;
        if (shift || alt || ctrl)
        {
            ModifierKeyState mods;
            mods.shift = shift;
            mods.alt = alt;
            mods.ctrl = ctrl;
            result = uiController_.scrollWithModifiers(elementId, deltaY, deltaX, mods);
        }
        else
        {
            result = uiController_.scroll(elementId, deltaY, deltaX);
        }

        if (result.success)
        {
            json data;
            data["previousValue"] = result.previousValue;
            data["newValue"] = result.newValue;
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Failed to scroll").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

} // namespace oscil::test
