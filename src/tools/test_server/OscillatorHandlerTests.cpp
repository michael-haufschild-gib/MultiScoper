/*
    Oscil - Oscillator Handler Integration Tests
    handleTestOscillatorReorder for E2E test server
*/

#include "core/Oscillator.h"
#include "core/Pane.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "tools/test_server/OscillatorHandler.h"

#include <nlohmann/json.hpp>

namespace oscil
{

namespace
{

void setupReorderTestOscillators(OscilState& state, PaneLayoutManager& layoutManager, OscilPluginEditor& editor)
{
    if (layoutManager.getPaneCount() == 0)
    {
        Pane defaultPane;
        defaultPane.setName("Test Pane");
        defaultPane.setOrderIndex(0);
        layoutManager.addPane(defaultPane);
    }

    PaneId targetPaneId = layoutManager.getPanes()[0].getId();

    auto existingOscs = state.getOscillators();
    for (const auto& osc : existingOscs)
        state.removeOscillator(osc.getId());

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
    editor.refreshPanels();
}

std::vector<Oscillator> getOrderedOscillators(OscilState& state)
{
    auto oscs = state.getOscillators();
    std::sort(oscs.begin(), oscs.end(),
              [](const Oscillator& a, const Oscillator& b) { return a.getOrderIndex() < b.getOrderIndex(); });
    return oscs;
}

nlohmann::json runReorderTest(OscilState& state, OscilPluginEditor& editor, const char* testName, int from, int to,
                              int expectedIdx, const char* details)
{
    nlohmann::json test;
    test["name"] = testName;
    auto before = getOrderedOscillators(state);
    juce::String movedName = before[static_cast<size_t>(from)].getName();
    state.reorderOscillators(from, to);
    editor.refreshPanels();
    auto after = getOrderedOscillators(state);
    bool passed = (after[static_cast<size_t>(expectedIdx)].getName() == movedName);
    test["passed"] = passed;
    test["details"] = details;
    test["movedOsc"] = movedName.toStdString();
    test["newOrder"] = {after[0].getName().toStdString(), after[1].getName().toStdString(),
                        after[2].getName().toStdString()};
    return test;
}

void countPassedTests(const nlohmann::json& tests, nlohmann::json& response)
{
    int passedCount = 0;
    for (const auto& test : tests)
        if (test["passed"].get<bool>())
            passedCount++;

    response["tests"] = tests;
    response["totalTests"] = static_cast<int>(tests.size());
    response["passed"] = passedCount;
    response["failed"] = static_cast<int>(tests.size()) - passedCount;
    response["allPassed"] = (passedCount == static_cast<int>(tests.size()));
}

nlohmann::json runSequentialIndexTest(OscilState& state)
{
    nlohmann::json test;
    test["name"] = "OrderIndexSequential";
    auto after = getOrderedOscillators(state);
    bool sequential = true;
    nlohmann::json indices = nlohmann::json::array();
    for (size_t i = 0; i < after.size(); ++i)
    {
        indices.push_back(after[i].getOrderIndex());
        if (after[i].getOrderIndex() != static_cast<int>(i))
            sequential = false;
    }
    test["passed"] = sequential;
    test["details"] = "orderIndex values should be 0, 1, 2";
    test["orderIndices"] = indices;
    return test;
}

nlohmann::json runSameIndexNoOpTest(OscilState& state, OscilPluginEditor& editor)
{
    nlohmann::json test;
    test["name"] = "SameIndexNoOp";
    auto before = getOrderedOscillators(state);
    state.reorderOscillators(1, 1);
    editor.refreshPanels();
    auto after = getOrderedOscillators(state);
    test["passed"] = (after[0].getName() == before[0].getName() && after[1].getName() == before[1].getName() &&
                      after[2].getName() == before[2].getName());
    test["details"] = "Moving to same index should not change order";
    return test;
}

} // namespace

void OscillatorHandler::handleTestOscillatorReorder(const httplib::Request&, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json tests = nlohmann::json::array();

        auto& state = editor_.getProcessor().getState();
        auto& layoutManager = state.getLayoutManager();
        setupReorderTestOscillators(state, layoutManager, editor_);

        tests.push_back(runReorderTest(state, editor_, "MoveFirstToLast", 0, 2, 2,
                                       "First oscillator should move to last position"));
        state.reorderOscillators(2, 0);
        editor_.refreshPanels();

        tests.push_back(runReorderTest(state, editor_, "MoveLastToFirst", 2, 0, 0,
                                       "Last oscillator should move to first position"));
        state.reorderOscillators(0, 2);
        editor_.refreshPanels();

        tests.push_back(
            runReorderTest(state, editor_, "SwapAdjacent", 1, 0, 0, "Middle oscillator should move to first position"));

        tests.push_back(runSequentialIndexTest(state));
        tests.push_back(runSameIndexNoOpTest(state, editor_));

        countPassedTests(tests, response);
        return response;
    });

    res.set_content(result.dump(), "application/json");
}

} // namespace oscil
