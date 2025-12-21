#include "tools/test_server/test_runners/RenderingTestRunner.h"
#include "ui/layout/PaneComponent.h"

namespace oscil
{

nlohmann::json RenderingTestRunner::runTests()
{
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
}

}
