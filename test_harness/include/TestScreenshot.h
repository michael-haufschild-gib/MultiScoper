/*
    Oscil Test Harness - Screenshot Capture & Visual Verification
    Captures screenshots and provides visual verification utilities
*/

#pragma once

#include "TestElementRegistry.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace oscil::test
{

/**
 * Result of an image comparison
 */
struct ImageComparisonResult
{
    bool match = false;              // True if images match within tolerance
    float similarity = 0.0f;         // 0.0 to 1.0 (1.0 = identical)
    int differentPixels = 0;         // Number of pixels that differ
    int totalPixels = 0;             // Total pixels compared
    juce::Rectangle<int> diffBounds; // Bounding box of differences
};

/**
 * Result of waveform analysis
 */
struct WaveformAnalysis
{
    bool detected = false;      // True if waveform content detected
    float amplitude = 0.0f;     // Peak amplitude (0.0 to 1.0)
    float activity = 0.0f;      // Amount of non-zero content (0.0 to 1.0)
    int zeroCrossings = 0;      // Estimated zero crossings (frequency indicator)
    juce::Colour dominantColor; // Most common waveform color
};

/**
 * Result of theme color verification
 */
struct ThemeColorVerification
{
    bool pass = false;                                      // True if all colors match expected
    std::vector<std::pair<juce::String, bool>> colorChecks; // Component -> pass/fail
    juce::String errorMessage;                              // Description of first failure
};

/**
 * Captures screenshots of UI components for visual verification.
 */
class TestScreenshot
{
public:
    TestScreenshot() = default;
    ~TestScreenshot() = default;

    // ================== Screenshot Capture ==================

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
     * Get image of a component by test ID
     */
    juce::Image getElementImage(const juce::String& elementId);

    // ================== Image Comparison ==================

    /**
     * Compare two images with configurable tolerance
     * @param image1 First image
     * @param image2 Second image
     * @param tolerance Allowed color difference per channel (0-255)
     * @return Comparison result with similarity metrics
     */
    ImageComparisonResult compareImages(const juce::Image& image1, const juce::Image& image2, int tolerance = 5);

    /**
     * Compare element screenshot against baseline file
     */
    ImageComparisonResult compareElementToBaseline(const juce::String& elementId, const juce::File& baselineFile,
                                                   int tolerance = 5);

    /**
     * Generate a diff image highlighting differences
     */
    juce::Image generateDiffImage(const juce::Image& image1, const juce::Image& image2,
                                  juce::Colour diffColor = juce::Colours::red);

    // ================== Waveform Verification ==================

    /**
     * Analyze waveform rendering in an image
     * @param image Image to analyze
     * @param backgroundColor Expected background color
     * @return Analysis results
     */
    WaveformAnalysis analyzeWaveform(const juce::Image& image, juce::Colour backgroundColor = juce::Colours::black);

    /**
     * Verify waveform is rendered (not flat/empty)
     */
    bool verifyWaveformRendered(const juce::String& elementId, float minAmplitude = 0.05f);

    /**
     * Verify waveform color matches expected
     */
    bool verifyWaveformColor(const juce::String& elementId, juce::Colour expectedColor, int tolerance = 20);

    /**
     * Verify that a region has non-uniform pixels (something is rendered)
     */
    bool hasRenderedContent(const juce::Image& image, juce::Rectangle<int> region = {});

    /**
     * Get the peak amplitude from a waveform image (assumes bright on dark)
     */
    float estimateWaveformAmplitude(const juce::Image& image);

    // ================== Color Verification ==================

    /**
     * Get average color in a region
     */
    juce::Colour getAverageColor(const juce::Image& image, juce::Rectangle<int> region = {});

    /**
     * Get dominant color in a region (most common color)
     */
    juce::Colour getDominantColor(const juce::Image& image, juce::Rectangle<int> region = {});

    /**
     * Get color histogram for a region
     * @return Map of color -> count
     */
    std::map<uint32_t, int> getColorHistogram(const juce::Image& image, juce::Rectangle<int> region = {},
                                              int bucketSize = 8);

    /**
     * Verify element has expected background color
     */
    bool verifyBackgroundColor(const juce::String& elementId, juce::Colour expectedColor, int tolerance = 10);

    /**
     * Verify element contains expected color (anywhere in region)
     */
    bool verifyContainsColor(const juce::String& elementId, juce::Colour expectedColor, float minCoverage = 0.01f,
                             int tolerance = 20);

    /**
     * Check if two colors match within tolerance
     */
    static bool colorsMatch(juce::Colour c1, juce::Colour c2, int tolerance);

    // ================== Component State Verification ==================

    /**
     * Get component bounds by testId
     */
    juce::Rectangle<int> getElementBounds(const juce::String& elementId);

    /**
     * Verify element is visible and has expected size
     */
    bool verifyElementBounds(const juce::String& elementId, int expectedWidth, int expectedHeight, int tolerance = 5);

    /**
     * Verify element is within parent bounds (not clipped off-screen)
     */
    bool verifyElementVisible(const juce::String& elementId);

    // ================== Baseline Management ==================

    /**
     * Save baseline image for future comparisons
     */
    bool saveBaseline(const juce::String& elementId, const juce::File& baselineDir, const juce::String& name = {});

    /**
     * Load baseline image
     */
    juce::Image loadBaseline(const juce::File& baselineFile);

    // ================== Utility ==================

    /**
     * Save image to file (PNG format)
     */
    static bool saveImage(const juce::Image& image, const juce::File& file);

    /**
     * Load image from file
     */
    static juce::Image loadImage(const juce::File& file);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestScreenshot)
};

} // namespace oscil::test
