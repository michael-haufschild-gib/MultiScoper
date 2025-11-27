/*
    Oscil Test Harness - Screenshot Implementation
*/

#include "TestScreenshot.h"
#include <juce_graphics/juce_graphics.h>

namespace oscil::test
{

bool TestScreenshot::captureElement(const juce::String& elementId, const juce::File& outputFile)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
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

    // Find top-level window
    auto* topLevel = component->getTopLevelComponent();
    if (topLevel == nullptr)
        return false;

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

bool TestScreenshot::hasRenderedContent(const juce::Image& image, juce::Rectangle<int> region)
{
    if (image.isNull())
        return false;

    if (region.isEmpty())
        region = image.getBounds();

    // Check if there's variation in pixel colors (not a solid fill)
    juce::Colour firstPixel = image.getPixelAt(region.getX(), region.getY());
    int differentPixels = 0;

    for (int y = region.getY(); y < region.getBottom(); y += 4)
    {
        for (int x = region.getX(); x < region.getRight(); x += 4)
        {
            auto pixel = image.getPixelAt(x, y);
            if (pixel != firstPixel)
            {
                differentPixels++;
                if (differentPixels > 10)
                    return true; // Found enough variation
            }
        }
    }

    return differentPixels > 5;
}

juce::Colour TestScreenshot::getAverageColor(const juce::Image& image, juce::Rectangle<int> region)
{
    if (image.isNull())
        return juce::Colours::black;

    if (region.isEmpty())
        region = image.getBounds();

    int64_t totalR = 0, totalG = 0, totalB = 0;
    int count = 0;

    for (int y = region.getY(); y < region.getBottom(); ++y)
    {
        for (int x = region.getX(); x < region.getRight(); ++x)
        {
            auto pixel = image.getPixelAt(x, y);
            totalR += pixel.getRed();
            totalG += pixel.getGreen();
            totalB += pixel.getBlue();
            count++;
        }
    }

    if (count == 0)
        return juce::Colours::black;

    return juce::Colour(
        static_cast<uint8_t>(totalR / count),
        static_cast<uint8_t>(totalG / count),
        static_cast<uint8_t>(totalB / count)
    );
}

float TestScreenshot::estimateWaveformAmplitude(const juce::Image& image)
{
    if (image.isNull())
        return 0.0f;

    int height = image.getHeight();
    int centerY = height / 2;

    // Find the max distance from center where bright pixels appear
    float maxAmplitude = 0.0f;

    for (int x = 0; x < image.getWidth(); x += 2)
    {
        for (int y = 0; y < height; ++y)
        {
            auto pixel = image.getPixelAt(x, y);

            // Check if this is a "waveform" pixel (brighter than background)
            float brightness = pixel.getBrightness();
            if (brightness > 0.3f) // Threshold for waveform detection
            {
                float distanceFromCenter = std::abs(y - centerY) / static_cast<float>(centerY);
                maxAmplitude = std::max(maxAmplitude, distanceFromCenter);
            }
        }
    }

    return maxAmplitude;
}

bool TestScreenshot::saveImage(const juce::Image& image, const juce::File& file)
{
    if (image.isNull())
        return false;

    // Ensure parent directory exists
    file.getParentDirectory().createDirectory();

    juce::FileOutputStream stream(file);
    if (!stream.openedOk())
        return false;

    juce::PNGImageFormat pngFormat;
    return pngFormat.writeImageToStream(image, stream);
}

} // namespace oscil::test
