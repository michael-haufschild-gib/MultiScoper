#include "tools/test_server/test_runners/WaveformTestRunner.h"
#include "plugin/PluginProcessor.h"
#include "ui/layout/PaneComponent.h"
#include "ui/panels/WaveformComponent.h"
#include "core/OscilState.h"
#include "core/Oscillator.h"
#include "core/SharedCaptureBuffer.h"
#include "core/DecimatingCaptureBuffer.h"
#include <cmath>

namespace oscil
{

nlohmann::json WaveformTestRunner::runTests()
{
    nlohmann::json response;
    nlohmann::json tests = nlohmann::json::array();

    auto& state = editor_.getProcessor().getState();
    auto captureBuffer = editor_.getProcessor().getCaptureBuffer();
    auto decimatingBuffer = std::dynamic_pointer_cast<DecimatingCaptureBuffer>(captureBuffer);

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
        
        if (decimatingBuffer) decimatingBuffer->write(testBuffer, metadata);

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
        
        if (decimatingBuffer) decimatingBuffer->write(testBuffer, metadata);

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
        metadata.isPlaying = true;
        
        if (decimatingBuffer) decimatingBuffer->write(testBuffer, metadata);

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
}

}