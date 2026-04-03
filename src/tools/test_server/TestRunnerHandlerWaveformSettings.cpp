/*
    Oscil - Test Runner Handler - Waveform & Settings Tests
*/

#include "core/OscilState.h"
#include "core/Oscillator.h"
#include "core/Pane.h"
#include "core/SharedCaptureBuffer.h"
#include "ui/layout/PaneComponent.h"
#include "ui/panels/WaveformComponent.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "tools/test_server/TestRunnerHandler.h"

#include <cmath>

namespace oscil
{

namespace
{
void ensureTestOscillator(OscilState& state, OscilPluginEditor& editor)
{
    if (!state.getOscillators().empty())
        return;

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
    editor.resized();
}

juce::AudioBuffer<float> generateSineBuffer(int numSamples, float amplitude)
{
    juce::AudioBuffer<float> buffer(2, numSamples);
    float phase = 0.0f;
    float const phaseInc = (2.0f * juce::MathConstants<float>::pi * 440.0f) / 44100.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float const sample = std::sin(phase) * amplitude;
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
        phase += phaseInc;
    }
    return buffer;
}

CaptureFrameMetadata testMetadata(int numSamples)
{
    CaptureFrameMetadata m;
    m.sampleRate = 44100.0;
    m.numChannels = 2;
    m.numSamples = numSamples;
    m.isPlaying = true;
    return m;
}

WaveformComponent* getFirstWaveform(const OscilPluginEditor& editor)
{
    const auto& panes = editor.getPaneComponents();
    if (panes.empty() || !panes[0] || panes[0]->getOscillatorCount() == 0)
        return nullptr;
    return panes[0]->getWaveformAt(0);
}
nlohmann::json testSineWaveRendering(const std::shared_ptr<SharedCaptureBuffer>& captureBuffer,
                                     const OscilPluginEditor& editor)
{
    nlohmann::json test;
    test["name"] = "SineWaveRendering";
    auto buf = generateSineBuffer(4096, 0.8f);
    captureBuffer->write(buf, testMetadata(4096));
    auto* wf = getFirstWaveform(editor);
    bool hasData = false;
    float peak = 0.0f;
    if (wf)
    {
        wf->forceUpdateWaveformData();
        hasData = wf->hasWaveformData();
        peak = wf->getPeakLevel();
    }
    test["passed"] = hasData && peak > 0.5f;
    test["details"] = "Sine wave should generate waveform data with peak > 0.5";
    return test;
}

nlohmann::json testSilenceRendering(const std::shared_ptr<SharedCaptureBuffer>& captureBuffer,
                                    const OscilPluginEditor& editor)
{
    nlohmann::json test;
    test["name"] = "SilenceRendering";
    juce::AudioBuffer<float> buf(2, 4096);
    buf.clear();
    captureBuffer->write(buf, testMetadata(4096));
    auto* wf = getFirstWaveform(editor);
    float peak = 1.0f;
    if (wf)
    {
        wf->forceUpdateWaveformData();
        peak = wf->getPeakLevel();
    }
    test["passed"] = peak < 0.01f;
    test["details"] = "Silence should produce near-zero peak level";
    return test;
}

nlohmann::json testHighAmplitudeRendering(const std::shared_ptr<SharedCaptureBuffer>& captureBuffer,
                                          const OscilPluginEditor& editor)
{
    nlohmann::json test;
    test["name"] = "HighAmplitudeRendering";
    auto buf = generateSineBuffer(4096, 1.0f);
    captureBuffer->write(buf, testMetadata(4096));
    auto* wf = getFirstWaveform(editor);
    float peak = 0.0f;
    if (wf)
    {
        wf->forceUpdateWaveformData();
        peak = wf->getPeakLevel();
    }
    test["passed"] = peak > 0.9f;
    test["details"] = "Full amplitude sine should produce peak > 0.9";
    return test;
}

nlohmann::json testToggleSetting(OscilPluginEditor& editor, const char* name, const char* details,
                                 std::function<void(bool)> setter, std::function<bool()> getter)
{
    nlohmann::json test;
    test["name"] = name;
    setter(false);
    editor.repaint();
    bool const off = !getter();
    setter(true);
    editor.repaint();
    bool const on = getter();
    test["passed"] = off && on;
    test["details"] = details;
    return test;
}

nlohmann::json testGainAdjustment(PaneComponent* pane, WaveformComponent* waveform, OscilPluginEditor& editor)
{
    nlohmann::json test;
    test["name"] = "GainAdjustment";
    pane->setGainDb(6.0f);
    editor.repaint();
    bool const gain6ok = std::abs(waveform->getGainLinear() - 1.995f) < 0.1f;
    pane->setGainDb(-6.0f);
    editor.repaint();
    bool const gainM6ok = std::abs(waveform->getGainLinear() - 0.501f) < 0.1f;
    pane->setGainDb(0.0f);
    test["passed"] = gain6ok && gainM6ok;
    test["details"] = "Gain dB conversion should be accurate";
    return test;
}

nlohmann::json testColumnLayoutChange(OscilState& state, OscilPluginEditor& editor)
{
    nlohmann::json test;
    test["name"] = "ColumnLayoutChange";
    state.setColumnLayout(ColumnLayout::Single);
    editor.resized();
    bool const s = state.getLayoutManager().getColumnCount() == 1;
    state.setColumnLayout(ColumnLayout::Double);
    editor.resized();
    bool const d = state.getLayoutManager().getColumnCount() == 2;
    state.setColumnLayout(ColumnLayout::Triple);
    editor.resized();
    bool const t = state.getLayoutManager().getColumnCount() == 3;
    state.setColumnLayout(ColumnLayout::Single);
    editor.resized();
    test["passed"] = s && d && t;
    test["details"] = "Column layout changes should be applied correctly";
    return test;
}

} // namespace

void TestRunnerHandler::handleRunWaveformTest(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json tests = nlohmann::json::array();

        auto& state = editor_.getProcessor().getState();
        auto captureBuffer = editor_.getProcessor().getCaptureBuffer();
        ensureTestOscillator(state, editor_);

        tests.push_back(testSineWaveRendering(captureBuffer, editor_));
        tests.push_back(testSilenceRendering(captureBuffer, editor_));
        tests.push_back(testHighAmplitudeRendering(captureBuffer, editor_));

        TestServerHandlerBase::countTestResults(tests, response);
        return response;
    });

    res.set_content(result.dump(), "application/json");
}

void TestRunnerHandler::handleRunSettingsTest(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;
        nlohmann::json tests = nlohmann::json::array();

        auto& state = editor_.getProcessor().getState();
        ensureTestOscillator(state, editor_);

        const auto& paneComponents = editor_.getPaneComponents();
        if (paneComponents.empty() || !paneComponents[0] || paneComponents[0]->getOscillatorCount() == 0)
        {
            response["error"] = "No waveform component available for testing";
            return response;
        }

        auto* pane = paneComponents[0].get();
        auto* waveform = pane->getWaveformAt(0);

        tests.push_back(testToggleSetting(
            editor_, "GridToggle", "Grid setting should propagate to waveform component",
            [pane](bool v) { pane->setShowGrid(v); }, [waveform]() { return waveform->isGridVisible(); }));

        tests.push_back(testToggleSetting(
            editor_, "AutoScaleToggle", "AutoScale setting should propagate", [pane](bool v) { pane->setAutoScale(v); },
            [waveform]() { return waveform->isAutoScaleEnabled(); }));

        tests.push_back(testToggleSetting(
            editor_, "HoldDisplayToggle", "HoldDisplay setting should propagate",
            [pane](bool v) { pane->setHoldDisplay(v); }, [waveform]() { return waveform->isHoldDisplayEnabled(); }));

        tests.push_back(testGainAdjustment(pane, waveform, editor_));
        tests.push_back(testColumnLayoutChange(state, editor_));

        TestServerHandlerBase::countTestResults(tests, response);
        return response;
    });

    res.set_content(result.dump(), "application/json");
}

} // namespace oscil
