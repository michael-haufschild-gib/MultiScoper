/*
    Oscil Test Harness - Screenshot Capture
    Captures screenshots of UI elements
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "TestElementRegistry.h"

namespace oscil::test
{

/**
 * Captures screenshots of UI components for visual verification.
 */
class TestScreenshot
{
public:
    TestScreenshot() = default;
    ~TestScreenshot() = default;

    /**
     * Capture a screenshot of a component by test ID
     */
    bool captureElement(const juce::String& elementId, const juce::File& outputFile);

    /**
     * Capture a screenshot of any component
     */
    bool captureComponent(juce::Component* component, const juce::File& outputFile);

    /**
     * Capture the entire window containing the given component
     */
    bool captureWindow(juce::Component* component, const juce::File& outputFile);

    /**
     * Capture all visible windows
     */
    bool captureAllWindows(const juce::File& outputDir, const juce::String& prefix = "window");

    /**
     * Get image of a component (doesn't save to file)
     */
    juce::Image getComponentImage(juce::Component* component);

    /**
     * Verify that a region has non-uniform pixels (something is rendered)
     */
    bool hasRenderedContent(const juce::Image& image, juce::Rectangle<int> region = {});

    /**
     * Get average color in a region
     */
    juce::Colour getAverageColor(const juce::Image& image, juce::Rectangle<int> region = {});

    /**
     * Get the peak amplitude from a waveform image (assumes bright on dark)
     */
    float estimateWaveformAmplitude(const juce::Image& image);

    /**
     * Save image to file (PNG format)
     */
    static bool saveImage(const juce::Image& image, const juce::File& file);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestScreenshot)
};

} // namespace oscil::test
