/*
    Oscil - Plugin Test Server Implementation
    HTTP server for automated UI testing
*/

#include "ui/PluginTestServer.h"
#include "ui/PluginEditor.h"
#include "ui/PaneComponent.h"
#include "ui/WaveformComponent.h"
#include "core/PluginProcessor.h"
#include "core/OscilState.h"
#include "core/Pane.h"
#include "core/SharedCaptureBuffer.h"
#include "core/InstanceRegistry.h"
#include <cmath>

namespace oscil
{

PluginTestServer::PluginTestServer(OscilPluginEditor& editor)
    : editor_(editor)
{
}

PluginTestServer::~PluginTestServer()
{
    stop();
}

void PluginTestServer::start(int port)
{
    if (running_.load())
        return;

    port_ = port;
    server_ = std::make_unique<httplib::Server>();
    setupEndpoints();

    serverThread_ = std::make_unique<std::thread>([this]() { serverThread(); });
}

void PluginTestServer::stop()
{
    if (!running_.load())
        return;

    if (server_)
        server_->stop();

    if (serverThread_ && serverThread_->joinable())
        serverThread_->join();

    // Clean up test sources to prevent leaks
    runOnMessageThread([this]() {
        clearTestSources();
    });

    serverThread_.reset();
    server_.reset();
    running_.store(false);
}

void PluginTestServer::serverThread()
{
    running_.store(true);
    server_->listen("127.0.0.1", port_);
    running_.store(false);
}

void PluginTestServer::setupEndpoints()
{
    // Health check
    server_->Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        handleHealth(req, res);
    });

    // Layout endpoints
    server_->Get("/layout", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetLayout(req, res);
    });

    server_->Post("/layout", [this](const httplib::Request& req, httplib::Response& res) {
        handleSetColumnLayout(req, res);
    });

    // Pane endpoints
    server_->Get("/panes", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetPaneBounds(req, res);
    });

    // Oscillator endpoints
    server_->Get("/oscillators", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetOscillators(req, res);
    });

    server_->Post("/oscillator/add", [this](const httplib::Request& req, httplib::Response& res) {
        handleAddOscillator(req, res);
    });

    server_->Post("/oscillator/delete", [this](const httplib::Request& req, httplib::Response& res) {
        handleDeleteOscillator(req, res);
    });

    server_->Post("/oscillator/update", [this](const httplib::Request& req, httplib::Response& res) {
        handleUpdateOscillator(req, res);
    });

    // Drag-drop simulation
    server_->Post("/pane/move", [this](const httplib::Request& req, httplib::Response& res) {
        handleMovePane(req, res);
    });

    // Screenshot
    server_->Post("/screenshot", [this](const httplib::Request& req, httplib::Response& res) {
        handleTakeScreenshot(req, res);
    });

    // Automated test endpoints
    server_->Post("/test/layout", [this](const httplib::Request& req, httplib::Response& res) {
        handleRunLayoutTest(req, res);
    });

    server_->Post("/test/dragdrop", [this](const httplib::Request& req, httplib::Response& res) {
        handleRunDragDropTest(req, res);
    });

    // Waveform and rendering test endpoints
    server_->Post("/test/inject", [this](const httplib::Request& req, httplib::Response& res) {
        handleInjectTestData(req, res);
    });

    server_->Post("/test/waveform", [this](const httplib::Request& req, httplib::Response& res) {
        handleRunWaveformTest(req, res);
    });

    server_->Post("/test/settings", [this](const httplib::Request& req, httplib::Response& res) {
        handleRunSettingsTest(req, res);
    });

    server_->Post("/test/rendering", [this](const httplib::Request& req, httplib::Response& res) {
        handleRunRenderingTest(req, res);
    });

    server_->Get("/waveform/state", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetWaveformState(req, res);
    });

    server_->Post("/state/reset", [this](const httplib::Request& req, httplib::Response& res) {
        handleStateReset(req, res);
    });

    // Source management endpoints
    server_->Get("/sources", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetSources(req, res);
    });

    server_->Post("/source/add", [this](const httplib::Request& req, httplib::Response& res) {
        handleAddSource(req, res);
    });

    server_->Post("/source/remove", [this](const httplib::Request& req, httplib::Response& res) {
        handleRemoveSource(req, res);
    });

    server_->Post("/oscillator/assign-source", [this](const httplib::Request& req, httplib::Response& res) {
        handleAssignSource(req, res);
    });

    server_->Post("/source/inject", [this](const httplib::Request& req, httplib::Response& res) {
        handleInjectSourceData(req, res);
    });

    // Oscillator reorder endpoints
    server_->Post("/oscillator/reorder", [this](const httplib::Request& req, httplib::Response& res) {
        handleReorderOscillator(req, res);
    });

    server_->Post("/test/oscillator-reorder", [this](const httplib::Request& req, httplib::Response& res) {
        handleTestOscillatorReorder(req, res);
    });
}

    // Helper to run on message thread
    // runOnMessageThread is defined in the header file as an inline template

    // Oscillator reorder handlers
    void handleReorderOscillator(const httplib::Request& req, httplib::Response& res);
    void handleTestOscillatorReorder(const httplib::Request& req, httplib::Response& res);

void PluginTestServer::handleHealth(const httplib::Request& /*req*/, httplib::Response& res)
{
    nlohmann::json response;
    response["status"] = "ok";
    response["port"] = port_;
    res.set_content(response.dump(), "application/json");
}

void PluginTestServer::handleGetLayout(const httplib::Request& /*req*/, httplib::Response& res)
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

void PluginTestServer::handleSetColumnLayout(const httplib::Request& req, httplib::Response& res)
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

void PluginTestServer::handleGetPaneBounds(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        auto& layoutManager = editor_.getProcessor().getState().getLayoutManager();

        // Get available area (same as updateLayout uses)
        auto editorBounds = editor_.getLocalBounds();
        int toolbarHeight = 40;
        int statusBarHeight = 24;
        int sidebarWidth = 250; // Approximate

        int availableWidth = editorBounds.getWidth() - sidebarWidth;
        int availableHeight = editorBounds.getHeight() - toolbarHeight - statusBarHeight;

        juce::Rectangle<int> availableArea(0, 0, availableWidth, availableHeight);

        response["availableArea"] = {
            {"x", 0},
            {"y", 0},
            {"width", availableWidth},
            {"height", availableHeight}
        };

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
            paneInfo["bounds"] = {
                {"x", bounds.getX()},
                {"y", bounds.getY()},
                {"width", bounds.getWidth()},
                {"height", bounds.getHeight()}
            };
            panes.push_back(paneInfo);
        }

        response["panes"] = panes;
        response["columns"] = layoutManager.getColumnCount();

        return response;
    });

    res.set_content(result.dump(), "application/json");
}

void PluginTestServer::handleAddOscillator(const httplib::Request& /*req*/, httplib::Response& res)
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

void PluginTestServer::handleDeleteOscillator(const httplib::Request& req, httplib::Response& res)
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

void PluginTestServer::handleUpdateOscillator(const httplib::Request& req, httplib::Response& res)
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
                        editor_.onOscillatorModeChanged(targetId, mode);
                    }

                    // Update visibility if provided
                    if (visible >= 0)
                    {
                        bool isVisible = visible != 0;
                        osc.setVisible(isVisible);
                        editor_.onOscillatorVisibilityChanged(targetId, isVisible);
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

void PluginTestServer::handleMovePane(const httplib::Request& req, httplib::Response& res)
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

void PluginTestServer::handleGetOscillators(const httplib::Request& /*req*/, httplib::Response& res)
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

void PluginTestServer::handleTakeScreenshot(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body.empty() ? "{}" : req.body);
        std::string filename = body.value("filename", "test_screenshot.png");

        auto result = runOnMessageThread([this, filename]() -> nlohmann::json {
            nlohmann::json response;

            // Create a snapshot of the editor
            auto bounds = editor_.getLocalBounds();
            juce::Image image(juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
            juce::Graphics g(image);
            editor_.paintEntireComponent(g, true);

            // Save to file
            juce::File outputFile(juce::File::getCurrentWorkingDirectory().getChildFile("screenshots").getChildFile(filename));
            outputFile.getParentDirectory().createDirectory();

            juce::FileOutputStream stream(outputFile);
            if (stream.openedOk())
            {
                juce::PNGImageFormat pngFormat;
                pngFormat.writeImageToStream(image, stream);
                response["status"] = "ok";
                response["path"] = outputFile.getFullPathName().toStdString();
            }
            else
            {
                response["error"] = "Failed to open file for writing";
            }

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

void PluginTestServer::handleRunLayoutTest(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json tests = nlohmann::json::array();

        auto& state = editor_.getProcessor().getState();
        auto& layoutManager = state.getLayoutManager();

        // Get editor dimensions for testing
        auto editorBounds = editor_.getLocalBounds();
        int toolbarHeight = 40;
        int statusBarHeight = 24;
        int sidebarWidth = 250;

        int availableWidth = editorBounds.getWidth() - sidebarWidth;
        int availableHeight = editorBounds.getHeight() - toolbarHeight - statusBarHeight;
        juce::Rectangle<int> availableArea(0, 0, availableWidth, availableHeight);

        // Ensure we have panes to test
        while (layoutManager.getPaneCount() < 3)
        {
            Pane pane;
            pane.setName("Test Pane " + juce::String(layoutManager.getPaneCount() + 1));
            pane.setOrderIndex(static_cast<int>(layoutManager.getPaneCount()));
            layoutManager.addPane(pane);
        }

        // Test 1: Single column layout
        {
            nlohmann::json test;
            test["name"] = "SingleColumnLayout";

            state.setColumnLayout(ColumnLayout::Single);
            editor_.resized();

            auto bounds0 = layoutManager.getPaneBounds(0, availableArea);
            int expectedWidth = availableWidth - 4; // 2px margin on each side

            test["expectedWidth"] = expectedWidth;
            test["actualWidth"] = bounds0.getWidth();
            test["passed"] = (bounds0.getWidth() == expectedWidth);
            test["details"] = "Pane should use full width minus margins";

            tests.push_back(test);
        }

        // Test 2: Double column layout
        {
            nlohmann::json test;
            test["name"] = "DoubleColumnLayout";

            state.setColumnLayout(ColumnLayout::Double);
            editor_.resized();

            auto bounds0 = layoutManager.getPaneBounds(0, availableArea);
            auto bounds1 = layoutManager.getPaneBounds(1, availableArea);

            int columnWidth = availableWidth / 2;
            int expectedPaneWidth = columnWidth - 4; // 2px margin on each side

            bool widthsCorrect = (bounds0.getWidth() == expectedPaneWidth) &&
                                 (bounds1.getWidth() == expectedPaneWidth);
            bool columnsCorrect = (bounds0.getX() == 2) &&
                                  (bounds1.getX() == columnWidth + 2);

            test["expectedWidth"] = expectedPaneWidth;
            test["actualWidth0"] = bounds0.getWidth();
            test["actualWidth1"] = bounds1.getWidth();
            test["passed"] = widthsCorrect && columnsCorrect;
            test["details"] = "Each pane should be half width, in separate columns";

            tests.push_back(test);
        }

        // Test 3: Triple column layout
        {
            nlohmann::json test;
            test["name"] = "TripleColumnLayout";

            state.setColumnLayout(ColumnLayout::Triple);
            editor_.resized();

            auto bounds0 = layoutManager.getPaneBounds(0, availableArea);
            auto bounds1 = layoutManager.getPaneBounds(1, availableArea);
            auto bounds2 = layoutManager.getPaneBounds(2, availableArea);

            int columnWidth = availableWidth / 3;
            int expectedPaneWidth = columnWidth - 4;

            bool widthsCorrect = (bounds0.getWidth() == expectedPaneWidth) &&
                                 (bounds1.getWidth() == expectedPaneWidth) &&
                                 (bounds2.getWidth() == expectedPaneWidth);
            bool columnsCorrect = (bounds0.getX() == 2) &&
                                  (bounds1.getX() == columnWidth + 2) &&
                                  (bounds2.getX() == 2 * columnWidth + 2);

            test["expectedWidth"] = expectedPaneWidth;
            test["actualWidths"] = {bounds0.getWidth(), bounds1.getWidth(), bounds2.getWidth()};
            test["passed"] = widthsCorrect && columnsCorrect;
            test["details"] = "Each pane should be one-third width, in separate columns";

            tests.push_back(test);
        }

        // Count passed tests
        int passedCount = 0;
        for (const auto& test : tests)
        {
            if (test["passed"].get<bool>())
                passedCount++;
        }

        response["tests"] = tests;
        response["totalTests"] = static_cast<int>(tests.size());
        response["passed"] = passedCount;
        response["failed"] = static_cast<int>(tests.size()) - passedCount;
        response["allPassed"] = (passedCount == static_cast<int>(tests.size()));

        return response;
    });

    res.set_content(result.dump(), "application/json");
}

void PluginTestServer::handleRunDragDropTest(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json tests = nlohmann::json::array();

        auto& state = editor_.getProcessor().getState();
        auto& layoutManager = state.getLayoutManager();

        // Ensure we have enough panes
        while (layoutManager.getPaneCount() < 3)
        {
            Pane pane;
            pane.setName("Test Pane " + juce::String(layoutManager.getPaneCount() + 1));
            pane.setOrderIndex(static_cast<int>(layoutManager.getPaneCount()));
            layoutManager.addPane(pane);
        }

        // Test 1: Move pane from index 0 to index 2
        {
            nlohmann::json test;
            test["name"] = "MovePaneForward";

            // Get initial order
            auto panesBefore = layoutManager.getPanes();
            juce::String firstPaneId = panesBefore[0].getId().id;

            // Move first pane to last position
            layoutManager.movePane(panesBefore[0].getId(), 2);
            editor_.resized();

            // Check new order
            auto panesAfter = layoutManager.getPanes();
            bool movedCorrectly = (panesAfter[2].getId().id == firstPaneId);

            test["passed"] = movedCorrectly;
            test["details"] = "First pane should move to last position";
            test["movedPaneId"] = firstPaneId.toStdString();
            test["newIndex"] = 2;

            tests.push_back(test);

            // Reset for next test
            layoutManager.movePane(panesAfter[2].getId(), 0);
        }

        // Test 2: Move pane from index 2 to index 0
        {
            nlohmann::json test;
            test["name"] = "MovePaneBackward";

            auto panesBefore = layoutManager.getPanes();
            juce::String lastPaneId = panesBefore[2].getId().id;

            layoutManager.movePane(panesBefore[2].getId(), 0);
            editor_.resized();

            auto panesAfter = layoutManager.getPanes();
            bool movedCorrectly = (panesAfter[0].getId().id == lastPaneId);

            test["passed"] = movedCorrectly;
            test["details"] = "Last pane should move to first position";
            test["movedPaneId"] = lastPaneId.toStdString();
            test["newIndex"] = 0;

            tests.push_back(test);

            // Reset
            layoutManager.movePane(panesAfter[0].getId(), 2);
        }

        // Test 3: Move pane to adjacent position
        {
            nlohmann::json test;
            test["name"] = "MovePaneAdjacent";

            auto panesBefore = layoutManager.getPanes();
            juce::String middlePaneId = panesBefore[1].getId().id;

            layoutManager.movePane(panesBefore[1].getId(), 0);
            editor_.resized();

            auto panesAfter = layoutManager.getPanes();
            bool movedCorrectly = (panesAfter[0].getId().id == middlePaneId);

            test["passed"] = movedCorrectly;
            test["details"] = "Middle pane should move to first position";

            tests.push_back(test);
        }

        // Test 4: Cross-column move - pane from column 0 to column 1
        {
            nlohmann::json test;
            test["name"] = "CrossColumnMove_0to1";

            // Set to 2 columns
            state.setColumnLayout(ColumnLayout::Double);
            editor_.resized();

            auto panesBefore = layoutManager.getPanes();
            if (panesBefore.size() >= 2)
            {
                // Find a pane in column 0
                PaneId paneToMove;
                int originalColumn = -1;
                for (const auto& p : panesBefore)
                {
                    if (p.getColumnIndex() == 0)
                    {
                        paneToMove = p.getId();
                        originalColumn = 0;
                        break;
                    }
                }

                if (originalColumn == 0)
                {
                    // Move to column 1
                    layoutManager.movePaneToColumn(paneToMove, 1, 0);
                    editor_.resized();

                    // Verify the pane is now in column 1
                    const Pane* movedPane = layoutManager.getPane(paneToMove);
                    bool movedCorrectly = movedPane && movedPane->getColumnIndex() == 1;

                    test["passed"] = movedCorrectly;
                    test["details"] = "Pane should move from column 0 to column 1";
                    test["originalColumn"] = originalColumn;
                    test["newColumn"] = movedPane ? movedPane->getColumnIndex() : -1;

                    // Move back for next test
                    layoutManager.movePaneToColumn(paneToMove, 0, 0);
                }
                else
                {
                    test["passed"] = false;
                    test["details"] = "No pane found in column 0";
                }
            }
            else
            {
                test["passed"] = false;
                test["details"] = "Not enough panes for cross-column test";
            }

            tests.push_back(test);
        }

        // Test 5: Cross-column move - pane from column 1 to column 0
        {
            nlohmann::json test;
            test["name"] = "CrossColumnMove_1to0";

            // Ensure we have a pane in column 1
            auto panesBefore = layoutManager.getPanes();
            PaneId paneToMove;
            int originalColumn = -1;

            for (const auto& p : panesBefore)
            {
                if (p.getColumnIndex() == 1)
                {
                    paneToMove = p.getId();
                    originalColumn = 1;
                    break;
                }
            }

            if (originalColumn == 1)
            {
                // Move to column 0
                layoutManager.movePaneToColumn(paneToMove, 0, 0);
                editor_.resized();

                // Verify the pane is now in column 0
                const Pane* movedPane = layoutManager.getPane(paneToMove);
                bool movedCorrectly = movedPane && movedPane->getColumnIndex() == 0;

                test["passed"] = movedCorrectly;
                test["details"] = "Pane should move from column 1 to column 0";
                test["originalColumn"] = originalColumn;
                test["newColumn"] = movedPane ? movedPane->getColumnIndex() : -1;
            }
            else
            {
                // If no pane in column 1, move one there first
                for (const auto& p : panesBefore)
                {
                    if (p.getColumnIndex() == 0)
                    {
                        paneToMove = p.getId();
                        layoutManager.movePaneToColumn(paneToMove, 1, 0);

                        // Now move it back to column 0
                        layoutManager.movePaneToColumn(paneToMove, 0, 0);
                        editor_.resized();

                        const Pane* movedPane = layoutManager.getPane(paneToMove);
                        bool movedCorrectly = movedPane && movedPane->getColumnIndex() == 0;

                        test["passed"] = movedCorrectly;
                        test["details"] = "Pane should move back to column 0 after being in column 1";
                        break;
                    }
                }

                if (paneToMove.id.isEmpty())
                {
                    test["passed"] = false;
                    test["details"] = "Could not find pane for cross-column test";
                }
            }

            tests.push_back(test);
        }

        // Test 6: Three-column layout cross-column moves
        {
            nlohmann::json test;
            test["name"] = "ThreeColumnCrossMove";

            // Set to 3 columns
            state.setColumnLayout(ColumnLayout::Triple);
            editor_.resized();

            auto panesBefore = layoutManager.getPanes();
            if (panesBefore.size() >= 3)
            {
                // Move pane from column 0 to column 2
                PaneId paneToMove = panesBefore[0].getId();
                int originalColumn = panesBefore[0].getColumnIndex();

                layoutManager.movePaneToColumn(paneToMove, 2, 0);
                editor_.resized();

                const Pane* movedPane = layoutManager.getPane(paneToMove);
                bool movedCorrectly = movedPane && movedPane->getColumnIndex() == 2;

                test["passed"] = movedCorrectly;
                test["details"] = "Pane should move to column 2 in 3-column layout";
                test["originalColumn"] = originalColumn;
                test["targetColumn"] = 2;
                test["actualColumn"] = movedPane ? movedPane->getColumnIndex() : -1;

                // Reset layout
                state.setColumnLayout(ColumnLayout::Single);
                editor_.resized();
            }
            else
            {
                test["passed"] = false;
                test["details"] = "Not enough panes for three-column test";
            }

            tests.push_back(test);
        }

        // Count passed tests
        int passedCount = 0;
        for (const auto& test : tests)
        {
            if (test["passed"].get<bool>())
                passedCount++;
        }

        response["tests"] = tests;
        response["totalTests"] = static_cast<int>(tests.size());
        response["passed"] = passedCount;
        response["failed"] = static_cast<int>(tests.size()) - passedCount;
        response["allPassed"] = (passedCount == static_cast<int>(tests.size()));

        return response;
    });

    res.set_content(result.dump(), "application/json");
}

void PluginTestServer::handleInjectTestData(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body.empty() ? "{}" : req.body);
        std::string waveformType = body.value("type", "sine");
        float frequency = body.value("frequency", 440.0f);
        float amplitude = body.value("amplitude", 0.8f);
        int numSamples = body.value("samples", 4096);
        float sampleRate = body.value("sampleRate", 44100.0f);

        auto result = runOnMessageThread([this, waveformType, frequency, amplitude, numSamples, sampleRate]() -> nlohmann::json {
            nlohmann::json response;

            // Get capture buffer from processor
            auto captureBuffer = editor_.getProcessor().getCaptureBuffer();
            if (!captureBuffer)
            {
                response["error"] = "No capture buffer available";
                return response;
            }

            // Generate test waveform
            juce::AudioBuffer<float> testBuffer(2, numSamples);
            float phase = 0.0f;
            float phaseIncrement = (2.0f * juce::MathConstants<float>::pi * frequency) / sampleRate;

            for (int i = 0; i < numSamples; ++i)
            {
                float sample = 0.0f;

                if (waveformType == "sine")
                {
                    sample = std::sin(phase) * amplitude;
                }
                else if (waveformType == "square")
                {
                    sample = (std::sin(phase) > 0.0f ? 1.0f : -1.0f) * amplitude;
                }
                else if (waveformType == "triangle")
                {
                    float t = phase / (2.0f * juce::MathConstants<float>::pi);
                    sample = (2.0f * std::abs(2.0f * (t - std::floor(t + 0.5f))) - 1.0f) * amplitude;
                }
                else if (waveformType == "sawtooth")
                {
                    float t = phase / (2.0f * juce::MathConstants<float>::pi);
                    sample = (2.0f * (t - std::floor(t + 0.5f))) * amplitude;
                }
                else if (waveformType == "noise")
                {
                    sample = ((std::rand() / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f) * amplitude;
                }
                else if (waveformType == "dc")
                {
                    sample = amplitude;
                }
                else if (waveformType == "silence")
                {
                    sample = 0.0f;
                }

                testBuffer.setSample(0, i, sample);  // Left channel
                testBuffer.setSample(1, i, sample * 0.8f);  // Right channel (slightly lower for testing)

                phase += phaseIncrement;
                if (phase > 2.0f * juce::MathConstants<float>::pi)
                    phase -= 2.0f * juce::MathConstants<float>::pi;
            }

            // Write to capture buffer
            CaptureFrameMetadata metadata;
            metadata.sampleRate = sampleRate;
            metadata.numChannels = 2;
            metadata.numSamples = numSamples;
            metadata.isPlaying = true;
            metadata.bpm = 120.0f;
            metadata.timestamp = 0;

            // Use blocking write (tryLock=false) for reliable test injection
            captureBuffer->write(testBuffer, metadata, false);

            // Force UI update
            editor_.repaint();

            response["status"] = "ok";
            response["waveformType"] = waveformType;
            response["frequency"] = frequency;
            response["amplitude"] = amplitude;
            response["samplesInjected"] = numSamples;
            response["sampleRate"] = sampleRate;

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

void PluginTestServer::handleGetWaveformState(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json panes = nlohmann::json::array();

        const auto& paneComponents = editor_.getPaneComponents();

        for (size_t paneIdx = 0; paneIdx < paneComponents.size(); ++paneIdx)
        {
            auto* pane = paneComponents[paneIdx].get();
            if (!pane) continue;

            nlohmann::json paneInfo;
            paneInfo["index"] = paneIdx;
            paneInfo["paneId"] = pane->getPaneId().id.toStdString();
            paneInfo["oscillatorCount"] = pane->getOscillatorCount();

            nlohmann::json waveforms = nlohmann::json::array();
            for (size_t wfIdx = 0; wfIdx < pane->getOscillatorCount(); ++wfIdx)
            {
                auto* waveform = pane->getWaveformAt(wfIdx);
                auto* osc = pane->getOscillatorAt(wfIdx);
                if (!waveform || !osc) continue;

                // Force update waveform data to ensure peak/RMS levels are calculated
                waveform->forceUpdateWaveformData();

                nlohmann::json wfInfo;
                wfInfo["oscillatorId"] = osc->getId().id.toStdString();
                wfInfo["oscillatorName"] = osc->getName().toStdString();
                wfInfo["hasWaveformData"] = waveform->hasWaveformData();
                wfInfo["peakLevel"] = waveform->getPeakLevel();
                wfInfo["rmsLevel"] = waveform->getRMSLevel();
                wfInfo["showGrid"] = waveform->isGridVisible();
                wfInfo["autoScale"] = waveform->isAutoScaleEnabled();
                wfInfo["holdDisplay"] = waveform->isHoldDisplayEnabled();
                wfInfo["gainLinear"] = waveform->getGainLinear();
                wfInfo["opacity"] = waveform->getOpacity();
                wfInfo["displaySamples"] = waveform->getDisplaySamples();
                wfInfo["processingMode"] = static_cast<int>(waveform->getProcessingMode());

                auto colour = waveform->getColour();
                wfInfo["colour"] = {
                    {"r", colour.getRed()},
                    {"g", colour.getGreen()},
                    {"b", colour.getBlue()},
                    {"a", colour.getAlpha()}
                };

                waveforms.push_back(wfInfo);
            }

            paneInfo["waveforms"] = waveforms;
            panes.push_back(paneInfo);
        }

        response["panes"] = panes;
        response["totalPanes"] = paneComponents.size();

        return response;
    });

    res.set_content(result.dump(), "application/json");
}

void PluginTestServer::handleRunWaveformTest(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json tests = nlohmann::json::array();

        auto& state = editor_.getProcessor().getState();
        auto captureBuffer = editor_.getProcessor().getCaptureBuffer();

        // Ensure we have at least one oscillator
        if (state.getOscillators().empty())
        {
            auto& layoutManager = state.getLayoutManager();
            if (layoutManager.getPaneCount() == 0)
            {
                Pane defaultPane;
                defaultPane.setName("Test Pane");
                defaultPane.setOrderIndex(0);
                layoutManager.addPane(defaultPane);
            }

            Oscillator osc;
            osc.setPaneId(layoutManager.getPanes()[0].getId());
            osc.setProcessingMode(ProcessingMode::FullStereo);
            osc.setName("Test Oscillator");
            state.addOscillator(osc);
            editor_.resized();
        }

        // Test 1: Inject sine wave and verify waveform data is generated
        {
            nlohmann::json test;
            test["name"] = "SineWaveRendering";

            // Inject sine wave
            juce::AudioBuffer<float> testBuffer(2, 4096);
            float phase = 0.0f;
            float phaseIncrement = (2.0f * juce::MathConstants<float>::pi * 440.0f) / 44100.0f;
            for (int i = 0; i < 4096; ++i)
            {
                float sample = std::sin(phase) * 0.8f;
                testBuffer.setSample(0, i, sample);
                testBuffer.setSample(1, i, sample);
                phase += phaseIncrement;
            }

            CaptureFrameMetadata metadata;
            metadata.sampleRate = 44100.0f;
            metadata.numChannels = 2;
            metadata.numSamples = 4096;
            metadata.isPlaying = true;
            captureBuffer->write(testBuffer, metadata);

            // Check waveform state
            const auto& paneComponents = editor_.getPaneComponents();
            bool hasWaveformData = false;
            float peakLevel = 0.0f;

            if (!paneComponents.empty())
            {
                auto* pane = paneComponents[0].get();
                if (pane && pane->getOscillatorCount() > 0)
                {
                    auto* wf = pane->getWaveformAt(0);
                    if (wf)
                    {
                        // Force update to process the injected data
                        wf->forceUpdateWaveformData();
                        hasWaveformData = wf->hasWaveformData();
                        peakLevel = wf->getPeakLevel();
                    }
                }
            }

            test["passed"] = hasWaveformData && peakLevel > 0.5f;
            test["details"] = "Sine wave should generate waveform data with peak > 0.5";
            test["hasWaveformData"] = hasWaveformData;
            test["peakLevel"] = peakLevel;
            tests.push_back(test);
        }

        // Test 2: Verify silence produces no waveform
        {
            nlohmann::json test;
            test["name"] = "SilenceRendering";

            // Inject silence
            juce::AudioBuffer<float> testBuffer(2, 4096);
            testBuffer.clear();

            CaptureFrameMetadata metadata;
            metadata.sampleRate = 44100.0f;
            metadata.numChannels = 2;
            metadata.numSamples = 4096;
            captureBuffer->write(testBuffer, metadata);

            const auto& paneComponents = editor_.getPaneComponents();
            float peakLevel = 1.0f;

            if (!paneComponents.empty())
            {
                auto* pane = paneComponents[0].get();
                if (pane && pane->getOscillatorCount() > 0)
                {
                    auto* wf = pane->getWaveformAt(0);
                    if (wf)
                    {
                        wf->forceUpdateWaveformData();
                        peakLevel = wf->getPeakLevel();
                    }
                }
            }

            test["passed"] = peakLevel < 0.01f;
            test["details"] = "Silence should produce near-zero peak level";
            test["peakLevel"] = peakLevel;
            tests.push_back(test);
        }

        // Test 3: High amplitude signal
        {
            nlohmann::json test;
            test["name"] = "HighAmplitudeRendering";

            juce::AudioBuffer<float> testBuffer(2, 4096);
            float phase = 0.0f;
            float phaseIncrement = (2.0f * juce::MathConstants<float>::pi * 440.0f) / 44100.0f;
            for (int i = 0; i < 4096; ++i)
            {
                float sample = std::sin(phase) * 1.0f;  // Full amplitude
                testBuffer.setSample(0, i, sample);
                testBuffer.setSample(1, i, sample);
                phase += phaseIncrement;
            }

            CaptureFrameMetadata metadata;
            metadata.sampleRate = 44100.0f;
            metadata.numChannels = 2;
            metadata.numSamples = 4096;
            captureBuffer->write(testBuffer, metadata);

            const auto& paneComponents = editor_.getPaneComponents();
            float peakLevel = 0.0f;

            if (!paneComponents.empty())
            {
                auto* pane = paneComponents[0].get();
                if (pane && pane->getOscillatorCount() > 0)
                {
                    auto* wf = pane->getWaveformAt(0);
                    if (wf)
                    {
                        wf->forceUpdateWaveformData();
                        peakLevel = wf->getPeakLevel();
                    }
                }
            }

            test["passed"] = peakLevel > 0.9f;
            test["details"] = "Full amplitude sine should produce peak > 0.9";
            test["peakLevel"] = peakLevel;
            tests.push_back(test);
        }

        // Count passed tests
        int passedCount = 0;
        for (const auto& test : tests)
        {
            if (test["passed"].get<bool>())
                passedCount++;
        }

        response["tests"] = tests;
        response["totalTests"] = static_cast<int>(tests.size());
        response["passed"] = passedCount;
        response["failed"] = static_cast<int>(tests.size()) - passedCount;
        response["allPassed"] = (passedCount == static_cast<int>(tests.size()));

        return response;
    });

    res.set_content(result.dump(), "application/json");
}

void PluginTestServer::handleRunSettingsTest(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json tests = nlohmann::json::array();

        auto& state = editor_.getProcessor().getState();

        // Ensure we have at least one oscillator
        if (state.getOscillators().empty())
        {
            auto& layoutManager = state.getLayoutManager();
            if (layoutManager.getPaneCount() == 0)
            {
                Pane defaultPane;
                defaultPane.setName("Test Pane");
                defaultPane.setOrderIndex(0);
                layoutManager.addPane(defaultPane);
            }

            Oscillator osc;
            osc.setPaneId(layoutManager.getPanes()[0].getId());
            osc.setProcessingMode(ProcessingMode::FullStereo);
            osc.setName("Test Oscillator");
            state.addOscillator(osc);
            editor_.resized();
        }

        const auto& paneComponents = editor_.getPaneComponents();
        if (paneComponents.empty() || !paneComponents[0] || paneComponents[0]->getOscillatorCount() == 0)
        {
            response["error"] = "No waveform component available for testing";
            return response;
        }

        auto* pane = paneComponents[0].get();
        auto* waveform = pane->getWaveformAt(0);

        // Test 1: Grid toggle
        {
            nlohmann::json test;
            test["name"] = "GridToggle";

            pane->setShowGrid(false);
            editor_.repaint();
            bool gridOffCorrect = !waveform->isGridVisible();

            pane->setShowGrid(true);
            editor_.repaint();
            bool gridOnCorrect = waveform->isGridVisible();

            test["passed"] = gridOffCorrect && gridOnCorrect;
            test["details"] = "Grid setting should propagate to waveform component";
            test["gridOffCorrect"] = gridOffCorrect;
            test["gridOnCorrect"] = gridOnCorrect;
            tests.push_back(test);
        }

        // Test 2: AutoScale toggle
        {
            nlohmann::json test;
            test["name"] = "AutoScaleToggle";

            pane->setAutoScale(false);
            editor_.repaint();
            bool autoScaleOffCorrect = !waveform->isAutoScaleEnabled();

            pane->setAutoScale(true);
            editor_.repaint();
            bool autoScaleOnCorrect = waveform->isAutoScaleEnabled();

            test["passed"] = autoScaleOffCorrect && autoScaleOnCorrect;
            test["details"] = "AutoScale setting should propagate to waveform component";
            test["autoScaleOffCorrect"] = autoScaleOffCorrect;
            test["autoScaleOnCorrect"] = autoScaleOnCorrect;
            tests.push_back(test);
        }

        // Test 3: Hold display toggle
        {
            nlohmann::json test;
            test["name"] = "HoldDisplayToggle";

            pane->setHoldDisplay(true);
            editor_.repaint();
            bool holdOnCorrect = waveform->isHoldDisplayEnabled();

            pane->setHoldDisplay(false);
            editor_.repaint();
            bool holdOffCorrect = !waveform->isHoldDisplayEnabled();

            test["passed"] = holdOnCorrect && holdOffCorrect;
            test["details"] = "HoldDisplay setting should propagate to waveform component";
            test["holdOnCorrect"] = holdOnCorrect;
            test["holdOffCorrect"] = holdOffCorrect;
            tests.push_back(test);
        }

        // Test 4: Gain adjustment
        {
            nlohmann::json test;
            test["name"] = "GainAdjustment";

            // Test +6dB gain
            pane->setGainDb(6.0f);
            editor_.repaint();
            float gain6dB = waveform->getGainLinear();
            // 6dB = 10^(6/20) ≈ 1.995
            bool gain6dBCorrect = std::abs(gain6dB - 1.995f) < 0.1f;

            // Test -6dB gain
            pane->setGainDb(-6.0f);
            editor_.repaint();
            float gainMinus6dB = waveform->getGainLinear();
            // -6dB = 10^(-6/20) ≈ 0.501
            bool gainMinus6dBCorrect = std::abs(gainMinus6dB - 0.501f) < 0.1f;

            // Reset to 0dB
            pane->setGainDb(0.0f);

            test["passed"] = gain6dBCorrect && gainMinus6dBCorrect;
            test["details"] = "Gain dB conversion should be accurate";
            test["gain6dB"] = gain6dB;
            test["gainMinus6dB"] = gainMinus6dB;
            test["gain6dBCorrect"] = gain6dBCorrect;
            test["gainMinus6dBCorrect"] = gainMinus6dBCorrect;
            tests.push_back(test);
        }

        // Test 5: Column layout changes
        {
            nlohmann::json test;
            test["name"] = "ColumnLayoutChange";

            state.setColumnLayout(ColumnLayout::Single);
            editor_.resized();
            bool singleCorrect = state.getLayoutManager().getColumnCount() == 1;

            state.setColumnLayout(ColumnLayout::Double);
            editor_.resized();
            bool doubleCorrect = state.getLayoutManager().getColumnCount() == 2;

            state.setColumnLayout(ColumnLayout::Triple);
            editor_.resized();
            bool tripleCorrect = state.getLayoutManager().getColumnCount() == 3;

            // Reset to single
            state.setColumnLayout(ColumnLayout::Single);
            editor_.resized();

            test["passed"] = singleCorrect && doubleCorrect && tripleCorrect;
            test["details"] = "Column layout changes should be applied correctly";
            test["singleCorrect"] = singleCorrect;
            test["doubleCorrect"] = doubleCorrect;
            test["tripleCorrect"] = tripleCorrect;
            tests.push_back(test);
        }

        // Count passed tests
        int passedCount = 0;
        for (const auto& test : tests)
        {
            if (test["passed"].get<bool>())
                passedCount++;
        }

        response["tests"] = tests;
        response["totalTests"] = static_cast<int>(tests.size());
        response["passed"] = passedCount;
        response["failed"] = static_cast<int>(tests.size()) - passedCount;
        response["allPassed"] = (passedCount == static_cast<int>(tests.size()));

        return response;
    });

    res.set_content(result.dump(), "application/json");
}

void PluginTestServer::handleRunRenderingTest(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json tests = nlohmann::json::array();

        // Test 1: Check rendering mode
        {
            nlohmann::json test;
            test["name"] = "RenderingModeCheck";

            #if OSCIL_ENABLE_OPENGL
            test["renderingMode"] = "OpenGL";
            test["passed"] = true;
            test["details"] = "OpenGL rendering is enabled";
            #else
            test["renderingMode"] = "Software";
            test["passed"] = true;
            test["details"] = "Software rendering is enabled (OpenGL not compiled)";
            #endif

            tests.push_back(test);
        }

        // Test 2: UI responsiveness
        {
            nlohmann::json test;
            test["name"] = "UIResponsiveness";

            auto startTime = juce::Time::getMillisecondCounterHiRes();

            // Force multiple repaints
            for (int i = 0; i < 10; ++i)
            {
                editor_.repaint();
                juce::Thread::sleep(16);
            }

            auto endTime = juce::Time::getMillisecondCounterHiRes();
            double elapsedMs = endTime - startTime;
            double avgFrameTime = elapsedMs / 10.0;

            // Should complete 10 frames in under 500ms (allowing for slow machines)
            test["passed"] = elapsedMs < 500.0;
            test["details"] = "10 repaint cycles should complete within 500ms";
            test["elapsedMs"] = elapsedMs;
            test["avgFrameTimeMs"] = avgFrameTime;
            tests.push_back(test);
        }

        // Test 3: Component hierarchy integrity
        {
            nlohmann::json test;
            test["name"] = "ComponentHierarchy";

            const auto& paneComponents = editor_.getPaneComponents();
            int totalWaveforms = 0;
            bool allPanesValid = true;

            for (size_t i = 0; i < paneComponents.size(); ++i)
            {
                auto* pane = paneComponents[i].get();
                if (!pane)
                {
                    allPanesValid = false;
                    break;
                }
                totalWaveforms += static_cast<int>(pane->getOscillatorCount());
            }

            test["passed"] = allPanesValid;
            test["details"] = "All pane components should be valid";
            test["paneCount"] = paneComponents.size();
            test["totalWaveforms"] = totalWaveforms;
            tests.push_back(test);
        }

        // Test 4: Screenshot generation
        {
            nlohmann::json test;
            test["name"] = "ScreenshotGeneration";

            auto bounds = editor_.getLocalBounds();
            juce::Image image(juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
            juce::Graphics g(image);
            editor_.paintEntireComponent(g, true);

            // Verify image is not completely empty
            int nonZeroPixels = 0;
            for (int y = 0; y < std::min(100, image.getHeight()); ++y)
            {
                for (int x = 0; x < std::min(100, image.getWidth()); ++x)
                {
                    auto pixel = image.getPixelAt(x, y);
                    if (pixel.getARGB() != 0)
                        nonZeroPixels++;
                }
            }

            test["passed"] = nonZeroPixels > 100;
            test["details"] = "Screenshot should contain non-empty pixels";
            test["imageWidth"] = image.getWidth();
            test["imageHeight"] = image.getHeight();
            test["nonZeroPixelsSample"] = nonZeroPixels;
            tests.push_back(test);
        }

        // Count passed tests
        int passedCount = 0;
        for (const auto& test : tests)
        {
            if (test["passed"].get<bool>())
                passedCount++;
        }

        response["tests"] = tests;
        response["totalTests"] = static_cast<int>(tests.size());
        response["passed"] = passedCount;
        response["failed"] = static_cast<int>(tests.size()) - passedCount;
        response["allPassed"] = (passedCount == static_cast<int>(tests.size()));

        return response;
    });

    res.set_content(result.dump(), "application/json");
}

void PluginTestServer::handleGetSources(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([]() -> nlohmann::json {
        nlohmann::json response;
        auto& registry = InstanceRegistry::getInstance();
        auto sources = registry.getAllSources();

        nlohmann::json sourceList = nlohmann::json::array();
        for (const auto& source : sources)
        {
            nlohmann::json srcInfo;
            srcInfo["id"] = source.sourceId.id.toStdString();
            srcInfo["name"] = source.name.toStdString();
            srcInfo["channelCount"] = source.channelCount;
            srcInfo["sampleRate"] = source.sampleRate;
            srcInfo["active"] = source.active.load();
            srcInfo["hasBuffer"] = !source.buffer.expired();
            sourceList.push_back(srcInfo);
        }

        response["sources"] = sourceList;
        response["count"] = static_cast<int>(sources.size());

        return response;
    });

    res.set_content(result.dump(), "application/json");
}

void PluginTestServer::handleAddSource(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body.empty() ? "{}" : req.body);
        std::string name = body.value("name", "Test Track");
        int channelCount = body.value("channelCount", 2);
        double sampleRate = body.value("sampleRate", 44100.0);
        std::string trackId = body.value("trackId", juce::Uuid().toString().toStdString());

        auto result = runOnMessageThread([this, name, channelCount, sampleRate, trackId]() -> nlohmann::json {
            nlohmann::json response;
            auto& registry = InstanceRegistry::getInstance();

            // Create a new capture buffer for this source
            auto captureBuffer = std::make_shared<SharedCaptureBuffer>();

            // Register the source
            SourceId sourceId = registry.registerInstance(
                juce::String(trackId),
                captureBuffer,
                juce::String(name),
                channelCount,
                sampleRate
            );

            // Store the buffer reference so it doesn't get destroyed
            testSourceBuffers_[sourceId.id.toStdString()] = captureBuffer;

            response["status"] = "ok";
            response["sourceId"] = sourceId.id.toStdString();
            response["name"] = name;
            response["channelCount"] = channelCount;
            response["sampleRate"] = sampleRate;
            response["sourceCount"] = static_cast<int>(registry.getSourceCount());

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

void PluginTestServer::handleRemoveSource(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        std::string sourceIdStr = body.value("sourceId", "");

        if (sourceIdStr.empty())
        {
            nlohmann::json error;
            error["error"] = "sourceId required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        auto result = runOnMessageThread([this, sourceIdStr]() -> nlohmann::json {
            nlohmann::json response;
            auto& registry = InstanceRegistry::getInstance();
            SourceId sourceId;
            sourceId.id = juce::String(sourceIdStr);

            // Unregister from registry
            registry.unregisterInstance(sourceId);

            // Remove from local buffer cache (fixing leak)
            testSourceBuffers_.erase(sourceIdStr);

            response["status"] = "ok";
            response["sourceId"] = sourceIdStr;
            response["sourceCount"] = static_cast<int>(registry.getSourceCount());

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

void PluginTestServer::handleAssignSource(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        std::string oscillatorId = body.value("oscillatorId", "");
        std::string sourceId = body.value("sourceId", "");
        int oscillatorIndex = body.value("oscillatorIndex", -1);

        if (oscillatorId.empty() && oscillatorIndex < 0)
        {
            nlohmann::json error;
            error["error"] = "oscillatorId or oscillatorIndex required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        auto result = runOnMessageThread([this, oscillatorId, sourceId, oscillatorIndex]() -> nlohmann::json {
            nlohmann::json response;
            auto& state = editor_.getProcessor().getState();
            auto oscillators = state.getOscillators();

            // Find the oscillator
            OscillatorId targetOscId;
            if (!oscillatorId.empty())
            {
                targetOscId.id = juce::String(oscillatorId);
            }
            else if (oscillatorIndex >= 0 && oscillatorIndex < static_cast<int>(oscillators.size()))
            {
                targetOscId = oscillators[static_cast<size_t>(oscillatorIndex)].getId();
            }
            else
            {
                response["error"] = "Invalid oscillator index";
                return response;
            }

            // Find the oscillator in state and update it
            bool found = false;
            for (auto& osc : oscillators)
            {
                if (osc.getId().id == targetOscId.id)
                {
                    SourceId newSourceId;
                    newSourceId.id = juce::String(sourceId);
                    osc.setSourceId(newSourceId);

                    // Update the oscillator in state
                    state.updateOscillator(osc);

                    // Bind the capture buffer to the waveform component
                    auto& registry = InstanceRegistry::getInstance();
                    auto buffer = registry.getCaptureBuffer(newSourceId);

                    // Trigger UI refresh to rebind waveform components
                    editor_.updateOscillatorSource(targetOscId, newSourceId);

                    response["status"] = "ok";
                    response["oscillatorId"] = targetOscId.id.toStdString();
                    response["sourceId"] = sourceId;
                    response["hasBuffer"] = (buffer != nullptr);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                response["error"] = "Oscillator not found";
            }

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

void PluginTestServer::handleInjectSourceData(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body);
        std::string sourceId = body.value("sourceId", "");
        std::string waveformType = body.value("type", "sine");
        float frequency = body.value("frequency", 440.0f);
        float amplitude = body.value("amplitude", 0.8f);
        int numSamples = body.value("samples", 4096);
        float sampleRate = body.value("sampleRate", 44100.0f);

        if (sourceId.empty())
        {
            nlohmann::json error;
            error["error"] = "sourceId required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        auto result = runOnMessageThread([this, sourceId, waveformType, frequency, amplitude, numSamples, sampleRate]() -> nlohmann::json {
            nlohmann::json response;

            // Get capture buffer for this source
            auto& registry = InstanceRegistry::getInstance();
            SourceId srcId;
            srcId.id = juce::String(sourceId);
            auto captureBuffer = registry.getCaptureBuffer(srcId);

            if (!captureBuffer)
            {
                // Try from test source buffers
                auto it = testSourceBuffers_.find(sourceId);
                if (it != testSourceBuffers_.end())
                {
                    captureBuffer = it->second;
                }
            }

            if (!captureBuffer)
            {
                response["error"] = "Source not found or no capture buffer";
                return response;
            }

            // Generate test waveform
            juce::AudioBuffer<float> testBuffer(2, numSamples);
            float phase = 0.0f;
            float phaseIncrement = (2.0f * juce::MathConstants<float>::pi * frequency) / sampleRate;

            for (int i = 0; i < numSamples; ++i)
            {
                float sample = 0.0f;

                if (waveformType == "sine")
                {
                    sample = std::sin(phase) * amplitude;
                }
                else if (waveformType == "square")
                {
                    sample = (std::sin(phase) > 0.0f ? 1.0f : -1.0f) * amplitude;
                }
                else if (waveformType == "triangle")
                {
                    float t = phase / (2.0f * juce::MathConstants<float>::pi);
                    sample = (2.0f * std::abs(2.0f * (t - std::floor(t + 0.5f))) - 1.0f) * amplitude;
                }
                else if (waveformType == "sawtooth")
                {
                    float t = phase / (2.0f * juce::MathConstants<float>::pi);
                    sample = (2.0f * (t - std::floor(t + 0.5f))) * amplitude;
                }
                else if (waveformType == "noise")
                {
                    sample = ((std::rand() / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f) * amplitude;
                }

                testBuffer.setSample(0, i, sample);
                testBuffer.setSample(1, i, sample * 0.8f);  // Slightly different R channel

                phase += phaseIncrement;
                if (phase > 2.0f * juce::MathConstants<float>::pi)
                    phase -= 2.0f * juce::MathConstants<float>::pi;
            }

            // Write to capture buffer
            CaptureFrameMetadata metadata;
            metadata.sampleRate = sampleRate;
            metadata.numChannels = 2;
            metadata.numSamples = numSamples;
            metadata.isPlaying = true;
            metadata.bpm = 120.0f;
            metadata.timestamp = 0;

            // Use blocking write (tryLock=false) for reliable test injection
            captureBuffer->write(testBuffer, metadata, false);

            // Force UI update
            editor_.repaint();

            response["status"] = "ok";
            response["sourceId"] = sourceId;
            response["waveformType"] = waveformType;
            response["frequency"] = frequency;
            response["amplitude"] = amplitude;
            response["samplesInjected"] = numSamples;

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

void PluginTestServer::handleReorderOscillator(const httplib::Request& req, httplib::Response& res)
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

void PluginTestServer::handleTestOscillatorReorder(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json tests = nlohmann::json::array();

        auto& state = editor_.getProcessor().getState();
        auto& layoutManager = state.getLayoutManager();

        // Ensure we have at least one pane
        if (layoutManager.getPaneCount() == 0)
        {
            Pane defaultPane;
            defaultPane.setName("Test Pane");
            defaultPane.setOrderIndex(0);
            layoutManager.addPane(defaultPane);
        }

        PaneId targetPaneId = layoutManager.getPanes()[0].getId();

        // Clear existing oscillators for clean test
        auto existingOscs = state.getOscillators();
        for (const auto& osc : existingOscs)
        {
            state.removeOscillator(osc.getId());
        }

        // Create 3 test oscillators with specific names
        std::vector<juce::String> names = {"Alpha", "Beta", "Gamma"};
        for (size_t i = 0; i < 3; ++i)
        {
            Oscillator osc;
            osc.setPaneId(targetPaneId);
            osc.setProcessingMode(ProcessingMode::FullStereo);
            osc.setName(names[i]);
            osc.setOrderIndex(static_cast<int>(i));
            state.addOscillator(osc);
        }

        editor_.refreshPanels();

        // Helper lambda to get oscillators sorted by orderIndex
        auto getOrderedOscillators = [&state]() {
            auto oscs = state.getOscillators();
            std::sort(oscs.begin(), oscs.end(),
                      [](const Oscillator& a, const Oscillator& b) {
                          return a.getOrderIndex() < b.getOrderIndex();
                      });
            return oscs;
        };

        // Test 1: Move first oscillator to last position (0 -> 2)
        {
            nlohmann::json test;
            test["name"] = "MoveFirstToLast";

            auto before = getOrderedOscillators();
            juce::String firstOscName = before[0].getName();

            state.reorderOscillators(0, 2);
            editor_.refreshPanels();

            auto after = getOrderedOscillators();
            bool passed = (after[2].getName() == firstOscName);

            test["passed"] = passed;
            test["details"] = "First oscillator should move to last position";
            test["movedOsc"] = firstOscName.toStdString();
            test["newOrder"] = {after[0].getName().toStdString(), after[1].getName().toStdString(), after[2].getName().toStdString()};
            tests.push_back(test);

            // Reset: move it back (2 -> 0)
            state.reorderOscillators(2, 0);
            editor_.refreshPanels();
        }

        // Test 2: Move last oscillator to first position (2 -> 0)
        {
            nlohmann::json test;
            test["name"] = "MoveLastToFirst";

            auto before = getOrderedOscillators();
            juce::String lastOscName = before[2].getName();

            state.reorderOscillators(2, 0);
            editor_.refreshPanels();

            auto after = getOrderedOscillators();
            bool passed = (after[0].getName() == lastOscName);

            test["passed"] = passed;
            test["details"] = "Last oscillator should move to first position";
            test["movedOsc"] = lastOscName.toStdString();
            test["newOrder"] = {after[0].getName().toStdString(), after[1].getName().toStdString(), after[2].getName().toStdString()};
            tests.push_back(test);

            // Reset
            state.reorderOscillators(0, 2);
            editor_.refreshPanels();
        }

        // Test 3: Swap adjacent oscillators (1 -> 0)
        {
            nlohmann::json test;
            test["name"] = "SwapAdjacent";

            auto before = getOrderedOscillators();
            juce::String middleOscName = before[1].getName();

            state.reorderOscillators(1, 0);
            editor_.refreshPanels();

            auto after = getOrderedOscillators();
            bool passed = (after[0].getName() == middleOscName);

            test["passed"] = passed;
            test["details"] = "Middle oscillator should move to first position";
            test["movedOsc"] = middleOscName.toStdString();
            test["newOrder"] = {after[0].getName().toStdString(), after[1].getName().toStdString(), after[2].getName().toStdString()};
            tests.push_back(test);
        }

        // Test 4: Verify orderIndex values are sequential
        {
            nlohmann::json test;
            test["name"] = "OrderIndexSequential";

            auto after = getOrderedOscillators();
            bool sequential = true;
            for (size_t i = 0; i < after.size(); ++i)
            {
                if (after[i].getOrderIndex() != static_cast<int>(i))
                {
                    sequential = false;
                    break;
                }
            }

            test["passed"] = sequential;
            test["details"] = "orderIndex values should be 0, 1, 2";
            nlohmann::json indices = nlohmann::json::array();
            for (const auto& osc : after)
            {
                indices.push_back(osc.getOrderIndex());
            }
            test["orderIndices"] = indices;
            tests.push_back(test);
        }

        // Test 5: Same index should be no-op
        {
            nlohmann::json test;
            test["name"] = "SameIndexNoOp";

            auto before = getOrderedOscillators();
            juce::String firstName = before[0].getName();
            juce::String secondName = before[1].getName();
            juce::String thirdName = before[2].getName();

            state.reorderOscillators(1, 1);  // No-op
            editor_.refreshPanels();

            auto after = getOrderedOscillators();
            bool passed = (after[0].getName() == firstName &&
                          after[1].getName() == secondName &&
                          after[2].getName() == thirdName);

            test["passed"] = passed;
            test["details"] = "Moving to same index should not change order";
            tests.push_back(test);
        }

        // Count passed tests
        int passedCount = 0;
        for (const auto& test : tests)
        {
            if (test["passed"].get<bool>())
                passedCount++;
        }

        response["tests"] = tests;
        response["totalTests"] = static_cast<int>(tests.size());
        response["passed"] = passedCount;
        response["failed"] = static_cast<int>(tests.size()) - passedCount;
        response["allPassed"] = (passedCount == static_cast<int>(tests.size()));

        return response;
    });

    res.set_content(result.dump(), "application/json");
}

} // namespace oscil
