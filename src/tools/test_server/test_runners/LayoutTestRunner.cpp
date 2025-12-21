#include "tools/test_server/test_runners/LayoutTestRunner.h"
#include "plugin/PluginProcessor.h"
#include "core/OscilState.h"
#include "core/Pane.h"

namespace oscil
{

nlohmann::json LayoutTestRunner::runTests()
{
    nlohmann::json response;
    nlohmann::json tests = nlohmann::json::array();

    auto& state = editor_.getProcessor().getState();
    auto& layoutManager = state.getLayoutManager();

    // Get editor dimensions for testing
    auto editorBounds = editor_.getLocalBounds();
    int toolbarHeight = 40;
    int statusBarHeight = 24;
    int sidebarWidth = 250;

    // Guard against zero/negative dimensions during initialization or small window sizes
    int availableWidth = std::max(100, editorBounds.getWidth() - sidebarWidth);
    int availableHeight = std::max(100, editorBounds.getHeight() - toolbarHeight - statusBarHeight);
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
}

nlohmann::json LayoutTestRunner::runDragDropTests()
{
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
}

}
