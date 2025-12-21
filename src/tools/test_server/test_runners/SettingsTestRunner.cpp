#include "tools/test_server/test_runners/SettingsTestRunner.h"
#include "plugin/PluginProcessor.h"
#include "ui/layout/PaneComponent.h"
#include "ui/panels/WaveformComponent.h"
#include "core/OscilState.h"
#include "core/Oscillator.h"

namespace oscil
{

nlohmann::json SettingsTestRunner::runTests()
{
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
}

}
