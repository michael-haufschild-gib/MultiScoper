/*
    Oscil - Test Runner Handler - Layout & Rendering Tests
*/

#include "tools/test_server/TestRunnerHandler.h"
#include "plugin/PluginEditor.h"
#include "ui/layout/PaneComponent.h"
#include "ui/panels/WaveformComponent.h"
#include "plugin/PluginProcessor.h"
#include "core/OscilState.h"
#include "core/Pane.h"

namespace oscil
{

namespace
{
void countPassed(const nlohmann::json& tests, nlohmann::json& response)
{
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
}
nlohmann::json testSingleColumnLayout(OscilState& state, OscilPluginEditor& editor,
                                      PaneLayoutManager& layoutManager,
                                      const juce::Rectangle<int>& availableArea)
{
    nlohmann::json test;
    test["name"] = "SingleColumnLayout";
    state.setColumnLayout(ColumnLayout::Single);
    editor.resized();
    auto bounds0 = layoutManager.getPaneBounds(0, availableArea);
    test["passed"] = (bounds0.getWidth() == availableArea.getWidth() - 4);
    test["details"] = "Pane should use full width minus margins";
    return test;
}

nlohmann::json testDoubleColumnLayout(OscilState& state, OscilPluginEditor& editor,
                                      PaneLayoutManager& layoutManager,
                                      const juce::Rectangle<int>& availableArea)
{
    nlohmann::json test;
    test["name"] = "DoubleColumnLayout";
    state.setColumnLayout(ColumnLayout::Double);
    editor.resized();
    auto bounds0 = layoutManager.getPaneBounds(0, availableArea);
    auto bounds1 = layoutManager.getPaneBounds(1, availableArea);
    int columnWidth = availableArea.getWidth() / 2;
    int expectedPaneWidth = columnWidth - 4;
    bool widthsCorrect = (bounds0.getWidth() == expectedPaneWidth) && (bounds1.getWidth() == expectedPaneWidth);
    bool columnsCorrect = (bounds0.getX() == 2) && (bounds1.getX() == columnWidth + 2);
    test["passed"] = widthsCorrect && columnsCorrect;
    test["details"] = "Each pane should be half width, in separate columns";
    return test;
}

nlohmann::json testTripleColumnLayout(OscilState& state, OscilPluginEditor& editor,
                                      PaneLayoutManager& layoutManager,
                                      const juce::Rectangle<int>& availableArea)
{
    nlohmann::json test;
    test["name"] = "TripleColumnLayout";
    state.setColumnLayout(ColumnLayout::Triple);
    editor.resized();
    auto bounds0 = layoutManager.getPaneBounds(0, availableArea);
    auto bounds1 = layoutManager.getPaneBounds(1, availableArea);
    auto bounds2 = layoutManager.getPaneBounds(2, availableArea);
    int columnWidth = availableArea.getWidth() / 3;
    int expectedPaneWidth = columnWidth - 4;
    bool widthsCorrect = (bounds0.getWidth() == expectedPaneWidth) &&
                         (bounds1.getWidth() == expectedPaneWidth) &&
                         (bounds2.getWidth() == expectedPaneWidth);
    bool columnsCorrect = (bounds0.getX() == 2) &&
                          (bounds1.getX() == columnWidth + 2) &&
                          (bounds2.getX() == 2 * columnWidth + 2);
    test["passed"] = widthsCorrect && columnsCorrect;
    test["details"] = "Each pane should be one-third width, in separate columns";
    return test;
}

nlohmann::json testUIResponsiveness(OscilPluginEditor& editor)
{
    nlohmann::json test;
    test["name"] = "UIResponsiveness";
    auto startTime = juce::Time::getMillisecondCounterHiRes();
    for (int i = 0; i < 10; ++i)
    {
        editor.repaint();
        juce::Thread::sleep(16);
    }
    double elapsedMs = juce::Time::getMillisecondCounterHiRes() - startTime;
    test["passed"] = elapsedMs < 500.0;
    test["details"] = "10 repaint cycles should complete within 500ms";
    test["elapsedMs"] = elapsedMs;
    return test;
}

nlohmann::json testScreenshotGeneration(OscilPluginEditor& editor)
{
    nlohmann::json test;
    test["name"] = "ScreenshotGeneration";
    auto bounds = editor.getLocalBounds();
    juce::Image image(juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
    juce::Graphics g(image);
    editor.paintEntireComponent(g, true);

    int nonZeroPixels = 0;
    for (int y = 0; y < std::min(100, image.getHeight()); ++y)
        for (int x = 0; x < std::min(100, image.getWidth()); ++x)
            if (image.getPixelAt(x, y).getARGB() != 0)
                nonZeroPixels++;

    test["passed"] = nonZeroPixels > 100;
    test["details"] = "Screenshot should contain non-empty pixels";
    return test;
}

} // namespace

void TestRunnerHandler::handleRunLayoutTest(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json tests = nlohmann::json::array();

        auto& state = editor_.getProcessor().getState();
        auto& layoutManager = state.getLayoutManager();

        auto editorBounds = editor_.getLocalBounds();
        int availableWidth = std::max(100, editorBounds.getWidth() - 250);
        int availableHeight = std::max(100, editorBounds.getHeight() - 40 - 24);
        juce::Rectangle<int> availableArea(0, 0, availableWidth, availableHeight);

        while (layoutManager.getPaneCount() < 3)
        {
            Pane pane;
            pane.setName("Test Pane " + juce::String(layoutManager.getPaneCount() + 1));
            pane.setOrderIndex(static_cast<int>(layoutManager.getPaneCount()));
            layoutManager.addPane(pane);
        }

        tests.push_back(testSingleColumnLayout(state, editor_, layoutManager, availableArea));
        tests.push_back(testDoubleColumnLayout(state, editor_, layoutManager, availableArea));
        tests.push_back(testTripleColumnLayout(state, editor_, layoutManager, availableArea));

        countPassed(tests, response);
        return response;
    });

    res.set_content(result.dump(), "application/json");
}

void TestRunnerHandler::handleRunRenderingTest(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json tests = nlohmann::json::array();

        // Test 1: Rendering mode check
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

        tests.push_back(testUIResponsiveness(editor_));

        // Test 3: Component hierarchy integrity
        {
            nlohmann::json test;
            test["name"] = "ComponentHierarchy";
            const auto& paneComponents = editor_.getPaneComponents();
            bool allPanesValid = true;
            for (size_t i = 0; i < paneComponents.size(); ++i)
                if (!paneComponents[i].get()) { allPanesValid = false; break; }
            test["passed"] = allPanesValid;
            test["details"] = "All pane components should be valid";
            test["paneCount"] = paneComponents.size();
            tests.push_back(test);
        }

        tests.push_back(testScreenshotGeneration(editor_));

        countPassed(tests, response);
        return response;
    });

    res.set_content(result.dump(), "application/json");
}

} // namespace oscil
