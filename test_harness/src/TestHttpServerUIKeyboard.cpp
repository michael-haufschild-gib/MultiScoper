/*
    Oscil Test Harness - HTTP Server: UI Keyboard, Focus, Wait & Element Query Handlers
*/

#include "TestHttpServer.h"
#include "TestElementRegistry.h"

namespace oscil::test
{

namespace
{

// Maps a key name string to a JUCE key code. Returns 0 on unknown key.
int mapKeyNameToCode(const std::string& key)
{
    if (key == "escape") return juce::KeyPress::escapeKey;
    if (key == "enter" || key == "return") return juce::KeyPress::returnKey;
    if (key == "space") return juce::KeyPress::spaceKey;
    if (key == "tab") return juce::KeyPress::tabKey;
    if (key == "backspace") return juce::KeyPress::backspaceKey;
    if (key == "delete") return juce::KeyPress::deleteKey;
    if (key == "up") return juce::KeyPress::upKey;
    if (key == "down") return juce::KeyPress::downKey;
    if (key == "left") return juce::KeyPress::leftKey;
    if (key == "right") return juce::KeyPress::rightKey;
    if (key == "home") return juce::KeyPress::homeKey;
    if (key == "end") return juce::KeyPress::endKey;
    if (key == "pageup") return juce::KeyPress::pageUpKey;
    if (key == "pagedown") return juce::KeyPress::pageDownKey;
    if (key == "f1") return juce::KeyPress::F1Key;
    if (key == "f2") return juce::KeyPress::F2Key;
    if (key == "f3") return juce::KeyPress::F3Key;
    if (key == "f4") return juce::KeyPress::F4Key;
    if (key == "f5") return juce::KeyPress::F5Key;
    if (key == "f6") return juce::KeyPress::F6Key;
    if (key == "f7") return juce::KeyPress::F7Key;
    if (key == "f8") return juce::KeyPress::F8Key;
    if (key == "f9") return juce::KeyPress::F9Key;
    if (key == "f10") return juce::KeyPress::F10Key;
    if (key == "f11") return juce::KeyPress::F11Key;
    if (key == "f12") return juce::KeyPress::F12Key;
    if (key.length() == 1) return key[0];
    return 0;
}

} // anonymous namespace

void TestHttpServer::setupUIKeyboardRoutes()
{
    // Keyboard
    server_->Post("/ui/keyPress", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIKeyPress(req, res);
    });
    server_->Post("/ui/typeText", [this](const httplib::Request& req, httplib::Response& res) {
        handleUITypeText(req, res);
    });
    server_->Post("/ui/clearText", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIClearText(req, res);
    });

    // Focus
    server_->Post("/ui/focus", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIFocus(req, res);
    });
    server_->Get("/ui/focused", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIGetFocused(req, res);
    });
    server_->Post("/ui/focusNext", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIFocusNext(req, res);
    });
    server_->Post("/ui/focusPrevious", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIFocusPrevious(req, res);
    });

    // Wait
    server_->Post("/ui/waitForElement", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIWaitForElement(req, res);
    });
    server_->Post("/ui/waitForVisible", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIWaitForVisible(req, res);
    });
    server_->Post("/ui/waitForEnabled", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIWaitForEnabled(req, res);
    });

    // State queries
    server_->Get("/ui/state", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIState(req, res);
    });
    server_->Get("/ui/elements", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIElements(req, res);
    });
    server_->Get(R"(/ui/element/(.+))", [this](const httplib::Request& req, httplib::Response& res) {
        handleUIElement(req, res);
    });
}

void TestHttpServer::handleUIKeyPress(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string key = body.value("key", "");
        std::string elementId = body.value("elementId", "");
        bool shift = body.value("shift", false);
        bool alt = body.value("alt", false);
        bool ctrl = body.value("ctrl", false);

        int keyCode = mapKeyNameToCode(key);
        if (keyCode == 0)
        {
            res.set_content(errorResponse("Unknown key: " + key).dump(), "application/json");
            return;
        }

        bool success = false;
        if (shift || alt || ctrl)
        {
            ModifierKeyState mods;
            mods.shift = shift;
            mods.alt = alt;
            mods.ctrl = ctrl;
            success = uiController_.pressKeyWithModifiers(keyCode, mods, elementId);
        }
        else
        {
            success = uiController_.pressKey(keyCode, elementId);
        }

        if (success)
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Failed to press key").dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUITypeText(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        std::string text = body.value("text", "");

        if (uiController_.typeText(elementId, text))
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Element not found or not a text input").dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIClearText(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.clearText(elementId))
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Element not found or not a text input").dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIFocus(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        if (uiController_.setFocus(elementId))
            res.set_content(successResponse().dump(), "application/json");
        else
            res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIGetFocused(const httplib::Request&, httplib::Response& res)
{
    auto focusedId = uiController_.getFocusedElementId();
    json data;
    data["elementId"] = focusedId.toStdString();
    data["hasFocus"] = focusedId.isNotEmpty();
    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleUIFocusNext(const httplib::Request&, httplib::Response& res)
{
    if (uiController_.focusNext())
    {
        json data;
        data["focusedElement"] = uiController_.getFocusedElementId().toStdString();
        res.set_content(successResponse(data).dump(), "application/json");
    }
    else
    {
        res.set_content(errorResponse("No focused element").dump(), "application/json");
    }
}

void TestHttpServer::handleUIFocusPrevious(const httplib::Request&, httplib::Response& res)
{
    if (uiController_.focusPrevious())
    {
        json data;
        data["focusedElement"] = uiController_.getFocusedElementId().toStdString();
        res.set_content(successResponse(data).dump(), "application/json");
    }
    else
    {
        res.set_content(errorResponse("No focused element").dump(), "application/json");
    }
}

void TestHttpServer::handleUIWaitForElement(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        int timeoutMs = body.value("timeoutMs", 5000);

        json data;
        data["found"] = uiController_.waitForElement(elementId, timeoutMs);
        data["elementId"] = elementId;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIWaitForVisible(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        int timeoutMs = body.value("timeoutMs", 5000);

        json data;
        data["visible"] = uiController_.waitForVisible(elementId, timeoutMs);
        data["elementId"] = elementId;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIWaitForEnabled(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        int timeoutMs = body.value("timeoutMs", 5000);

        json data;
        data["enabled"] = uiController_.waitForEnabled(elementId, timeoutMs);
        data["elementId"] = elementId;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleUIState(const httplib::Request&, httplib::Response& res)
{
    auto state = uiController_.getUIState();
    res.set_content(successResponse(state).dump(), "application/json");
}

void TestHttpServer::handleUIElement(const httplib::Request& req, httplib::Response& res)
{
    std::string elementId = req.matches[1];

    // Run on message thread — component queries (isVisible, isShowing,
    // getBounds) must only be called there to avoid data races.
    json info;
    juce::WaitableEvent done;
    auto& ctrl = uiController_;
    juce::MessageManager::callAsync([&info, &done, &ctrl, elementId]()
    {
        info = ctrl.getElementInfo(juce::String(elementId));
        done.signal();
    });
    bool waited = done.wait(3000);

    if (!waited)
    {
        fprintf(stderr, "[UIElement] TIMEOUT for %s\n", elementId.c_str());
        res.status = 504;
        res.set_content(errorResponse("Timeout waiting for message thread: " + elementId).dump(), "application/json");
        return;
    }
    fprintf(stderr, "[UIElement] callAsync completed for %s, info.contains(error)=%d\n",
            elementId.c_str(), info.contains("error") ? 1 : 0);

    if (info.contains("error"))
    {
        res.status = 404;
        res.set_content(errorResponse("Element not found: " + elementId).dump(), "application/json");
        return;
    }

    res.set_content(successResponse(info).dump(), "application/json");
}

void TestHttpServer::handleUIElements(const httplib::Request&, httplib::Response& res)
{
    // Build the response on the message thread because component methods
    // (isVisible, isEnabled, getBounds) must only be called there.
    json data;
    juce::WaitableEvent done;

    juce::MessageManager::callAsync([&data, &done]()
    {
        data["count"] = 0;
        data["elements"] = json::array();

        // getAllElements() uses SafePointer internally — dead components
        // are automatically filtered out and cleaned from the registry.
        auto elements = TestElementRegistry::getInstance().getAllElements();
        for (const auto& [testId, component] : elements)
        {
            json elementInfo;
            elementInfo["testId"] = testId.toStdString();
            elementInfo["visible"] = component->isVisible();
            elementInfo["enabled"] = component->isEnabled();

            auto bounds = component->getBounds();
            elementInfo["bounds"] = {
                {"x", bounds.getX()}, {"y", bounds.getY()},
                {"width", bounds.getWidth()}, {"height", bounds.getHeight()}
            };

            data["elements"].push_back(elementInfo);
        }

        data["count"] = static_cast<int>(data["elements"].size());
        done.signal();
    });

    done.wait(5000);
    res.set_content(successResponse(data).dump(), "application/json");
}

} // namespace oscil::test
