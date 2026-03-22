/*
    Oscil - Test Runner Handler - Drag/Drop Tests
*/

#include "tools/test_server/TestRunnerHandler.h"
#include "plugin/PluginEditor.h"
#include "core/OscilState.h"
#include "core/Pane.h"

namespace oscil
{

namespace
{
void ensureMinPanes(PaneLayoutManager& layoutManager, int minCount)
{
    while (layoutManager.getPaneCount() < static_cast<size_t>(minCount))
    {
        Pane pane;
        pane.setName("Test Pane " + juce::String(layoutManager.getPaneCount() + 1));
        pane.setOrderIndex(static_cast<int>(layoutManager.getPaneCount()));
        layoutManager.addPane(pane);
    }
}

PaneId findPaneInColumn(const std::vector<Pane>& panes, int column)
{
    for (const auto& p : panes)
    {
        if (p.getColumnIndex() == column)
            return p.getId();
    }
    return PaneId::invalid();
}

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
nlohmann::json testMovePaneForward(PaneLayoutManager& layoutManager, OscilPluginEditor& editor)
{
    nlohmann::json test;
    test["name"] = "MovePaneForward";
    auto panesBefore = layoutManager.getPanes();
    juce::String firstPaneId = panesBefore[0].getId().id;
    layoutManager.movePane(panesBefore[0].getId(), 2);
    editor.resized();
    auto panesAfter = layoutManager.getPanes();
    test["passed"] = (panesAfter[2].getId().id == firstPaneId);
    test["details"] = "First pane should move to last position";
    layoutManager.movePane(panesAfter[2].getId(), 0);
    return test;
}

nlohmann::json testMovePaneBackward(PaneLayoutManager& layoutManager, OscilPluginEditor& editor)
{
    nlohmann::json test;
    test["name"] = "MovePaneBackward";
    auto panesBefore = layoutManager.getPanes();
    juce::String lastPaneId = panesBefore[2].getId().id;
    layoutManager.movePane(panesBefore[2].getId(), 0);
    editor.resized();
    auto panesAfter = layoutManager.getPanes();
    test["passed"] = (panesAfter[0].getId().id == lastPaneId);
    test["details"] = "Last pane should move to first position";
    layoutManager.movePane(panesAfter[0].getId(), 2);
    return test;
}

nlohmann::json testMovePaneAdjacent(PaneLayoutManager& layoutManager, OscilPluginEditor& editor)
{
    nlohmann::json test;
    test["name"] = "MovePaneAdjacent";
    auto panesBefore = layoutManager.getPanes();
    juce::String middlePaneId = panesBefore[1].getId().id;
    layoutManager.movePane(panesBefore[1].getId(), 0);
    editor.resized();
    auto panesAfter = layoutManager.getPanes();
    test["passed"] = (panesAfter[0].getId().id == middlePaneId);
    test["details"] = "Middle pane should move to first position";
    return test;
}

nlohmann::json testCrossColumnMove0to1(OscilState& state, PaneLayoutManager& layoutManager, OscilPluginEditor& editor)
{
    nlohmann::json test;
    test["name"] = "CrossColumnMove_0to1";
    state.setColumnLayout(ColumnLayout::Double);
    editor.resized();
    auto panesBefore = layoutManager.getPanes();
    PaneId paneToMove = findPaneInColumn(panesBefore, 0);
    if (paneToMove.isValid())
    {
        layoutManager.movePaneToColumn(paneToMove, 1, 0);
        editor.resized();
        const Pane* movedPane = layoutManager.getPane(paneToMove);
        test["passed"] = movedPane && movedPane->getColumnIndex() == 1;
        test["details"] = "Pane should move from column 0 to column 1";
        layoutManager.movePaneToColumn(paneToMove, 0, 0);
    }
    else
    {
        test["passed"] = false;
        test["details"] = "No pane found in column 0";
    }
    return test;
}

nlohmann::json testCrossColumnMove1to0(PaneLayoutManager& layoutManager, OscilPluginEditor& editor)
{
    nlohmann::json test;
    test["name"] = "CrossColumnMove_1to0";
    auto panesBefore = layoutManager.getPanes();
    PaneId paneToMove = findPaneInColumn(panesBefore, 1);
    if (paneToMove.isValid())
    {
        layoutManager.movePaneToColumn(paneToMove, 0, 0);
        editor.resized();
        const Pane* movedPane = layoutManager.getPane(paneToMove);
        test["passed"] = movedPane && movedPane->getColumnIndex() == 0;
        test["details"] = "Pane should move from column 1 to column 0";
    }
    else
    {
        PaneId col0Pane = findPaneInColumn(panesBefore, 0);
        if (col0Pane.isValid())
        {
            layoutManager.movePaneToColumn(col0Pane, 1, 0);
            layoutManager.movePaneToColumn(col0Pane, 0, 0);
            editor.resized();
            const Pane* movedPane = layoutManager.getPane(col0Pane);
            test["passed"] = movedPane && movedPane->getColumnIndex() == 0;
            test["details"] = "Pane should move back to column 0";
        }
        else
        {
            test["passed"] = false;
            test["details"] = "Could not find pane for cross-column test";
        }
    }
    return test;
}

nlohmann::json testThreeColumnCrossMove(OscilState& state, PaneLayoutManager& layoutManager, OscilPluginEditor& editor)
{
    nlohmann::json test;
    test["name"] = "ThreeColumnCrossMove";
    state.setColumnLayout(ColumnLayout::Triple);
    editor.resized();
    auto panesBefore = layoutManager.getPanes();
    if (panesBefore.size() >= 3)
    {
        PaneId paneToMove = panesBefore[0].getId();
        layoutManager.movePaneToColumn(paneToMove, 2, 0);
        editor.resized();
        const Pane* movedPane = layoutManager.getPane(paneToMove);
        test["passed"] = movedPane && movedPane->getColumnIndex() == 2;
        test["details"] = "Pane should move to column 2 in 3-column layout";
        state.setColumnLayout(ColumnLayout::Single);
        editor.resized();
    }
    else
    {
        test["passed"] = false;
        test["details"] = "Not enough panes for three-column test";
    }
    return test;
}

} // namespace

void TestRunnerHandler::handleRunDragDropTest(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json tests = nlohmann::json::array();

        auto& state = editor_.getProcessor().getState();
        auto& layoutManager = state.getLayoutManager();
        ensureMinPanes(layoutManager, 3);

        tests.push_back(testMovePaneForward(layoutManager, editor_));
        tests.push_back(testMovePaneBackward(layoutManager, editor_));
        tests.push_back(testMovePaneAdjacent(layoutManager, editor_));
        tests.push_back(testCrossColumnMove0to1(state, layoutManager, editor_));
        tests.push_back(testCrossColumnMove1to0(layoutManager, editor_));
        tests.push_back(testThreeColumnCrossMove(state, layoutManager, editor_));

        countPassed(tests, response);
        return response;
    });

    res.set_content(result.dump(), "application/json");
}

} // namespace oscil
