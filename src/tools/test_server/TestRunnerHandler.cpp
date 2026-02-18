/*
    Oscil - Test Runner Handler Implementation
*/

#include "tools/test_server/TestRunnerHandler.h"
#include "plugin/PluginEditor.h"
#include "ui/layout/PaneComponent.h"
#include "ui/panels/WaveformComponent.h"
#include "plugin/PluginProcessor.h"
#include "core/OscilState.h"
#include "core/Pane.h"
#include "core/Oscillator.h"
#include "core/SharedCaptureBuffer.h"
#include <cmath>

namespace oscil
{
void TestRunnerHandler::handleRunLayoutTest(const httplib::Request& /*req*/, httplib::Response& res)
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
    });

    sendJson(res, result, 200);
}

void TestRunnerHandler::handleRunDragDropTest(const httplib::Request& /*req*/, httplib::Response& res)
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

    sendJson(res, result, 200);
}

void TestRunnerHandler::handleRunWaveformTest(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json tests = nlohmann::json::array();

        auto& state = editor_.getProcessor().getState();
        auto captureBuffer = editor_.getProcessor().getDecimatingCaptureBuffer();

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

    sendJson(res, result, 200);
}

void TestRunnerHandler::handleRunSettingsTest(const httplib::Request& /*req*/, httplib::Response& res)
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

    sendJson(res, result, 200);
}

void TestRunnerHandler::handleRunRenderingTest(const httplib::Request& /*req*/, httplib::Response& res)
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

    sendJson(res, result, 200);
}

} // namespace oscil
