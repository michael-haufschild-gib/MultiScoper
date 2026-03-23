/*
    Oscil Test Harness - Screenshot Capture & Image Comparison
*/

#include "TestScreenshot.h"

#include <juce_graphics/juce_graphics.h>

#include <algorithm>
#include <cmath>

namespace oscil::test
{

// Forward declaration for macOS native window capture (defined in TestScreenshotMac.mm)
#if JUCE_MAC
juce::Image captureNativeWindowMac(juce::Component* component, const juce::String& outputPath = juce::String());
#endif

// ================== Screenshot Capture ==================

bool TestScreenshot::captureElement(const juce::String& elementId, const juce::File& outputFile)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    return captureComponent(component, outputFile);
}

bool TestScreenshot::captureComponent(juce::Component* component, const juce::File& outputFile)
{
    if (component == nullptr)
        return false;

    auto image = getComponentImage(component);
    if (image.isNull())
        return false;

    return saveImage(image, outputFile);
}

bool TestScreenshot::captureWindow(juce::Component* component, const juce::File& outputFile)
{
    if (component == nullptr)
        return false;

    auto* topLevel = component->getTopLevelComponent();
    if (topLevel == nullptr)
        return false;

#if JUCE_MAC
    // Use native window capture on macOS to include OpenGL content
    // Pass the output path directly so screencapture writes the file
    outputFile.getParentDirectory().createDirectory();
    auto image = captureNativeWindowMac(topLevel, outputFile.getFullPathName());
    if (!image.isNull())
        return outputFile.existsAsFile();
    // Fall back to component snapshot if native capture fails
#endif

    return captureComponent(topLevel, outputFile);
}

bool TestScreenshot::captureAllWindows(const juce::File& outputDir, const juce::String& prefix)
{
    if (!outputDir.isDirectory())
    {
        if (!outputDir.createDirectory())
            return false;
    }

    int windowIndex = 0;
    for (int i = 0; i < juce::Desktop::getInstance().getNumComponents(); ++i)
    {
        auto* component = juce::Desktop::getInstance().getComponent(i);
        if (component != nullptr && component->isVisible())
        {
            auto file = outputDir.getChildFile(prefix + "_" + juce::String(windowIndex++) + ".png");
            captureComponent(component, file);
        }
    }

    return windowIndex > 0;
}

juce::Image TestScreenshot::getComponentImage(juce::Component* component)
{
    if (component == nullptr)
        return {};

    return component->createComponentSnapshot(component->getLocalBounds(), true, 1.0f);
}

juce::Image TestScreenshot::getElementImage(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return {};

    return getComponentImage(component);
}

// ================== Image Comparison ==================

ImageComparisonResult TestScreenshot::compareImages(const juce::Image& image1, const juce::Image& image2, int tolerance)
{
    ImageComparisonResult result;

    if (image1.isNull() || image2.isNull())
        return result;

    // Images must be same size
    if (image1.getWidth() != image2.getWidth() || image1.getHeight() != image2.getHeight())
        return result;

    int width = image1.getWidth();
    int height = image1.getHeight();
    result.totalPixels = width * height;

    int minDiffX = width, minDiffY = height;
    int maxDiffX = 0, maxDiffY = 0;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            auto p1 = image1.getPixelAt(x, y);
            auto p2 = image2.getPixelAt(x, y);

            if (!colorsMatch(p1, p2, tolerance))
            {
                result.differentPixels++;
                minDiffX = std::min(minDiffX, x);
                minDiffY = std::min(minDiffY, y);
                maxDiffX = std::max(maxDiffX, x);
                maxDiffY = std::max(maxDiffY, y);
            }
        }
    }

    if (result.differentPixels > 0)
    {
        result.diffBounds = juce::Rectangle<int>(minDiffX, minDiffY, maxDiffX - minDiffX + 1, maxDiffY - minDiffY + 1);
    }

    result.similarity = 1.0f - (static_cast<float>(result.differentPixels) / result.totalPixels);
    result.match = (result.differentPixels == 0);

    return result;
}

ImageComparisonResult TestScreenshot::compareElementToBaseline(const juce::String& elementId,
                                                               const juce::File& baselineFile, int tolerance)
{
    auto currentImage = getElementImage(elementId);
    auto baselineImage = loadImage(baselineFile);

    return compareImages(currentImage, baselineImage, tolerance);
}

juce::Image TestScreenshot::generateDiffImage(const juce::Image& image1, const juce::Image& image2,
                                              juce::Colour diffColor)
{
    if (image1.isNull() || image2.isNull())
        return {};

    if (image1.getWidth() != image2.getWidth() || image1.getHeight() != image2.getHeight())
        return {};

    int width = image1.getWidth();
    int height = image1.getHeight();

    juce::Image diffImage(juce::Image::ARGB, width, height, true);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            auto p1 = image1.getPixelAt(x, y);
            auto p2 = image2.getPixelAt(x, y);

            if (colorsMatch(p1, p2, 5))
            {
                // Same - show dimmed version of original
                diffImage.setPixelAt(x, y, p1.withAlpha(0.3f));
            }
            else
            {
                // Different - highlight with diff color
                diffImage.setPixelAt(x, y, diffColor);
            }
        }
    }

    return diffImage;
}

} // namespace oscil::test
