/*
    Oscil Test Harness - Screenshot Waveform & Color Analysis
*/

#include "TestScreenshot.h"

#include <juce_graphics/juce_graphics.h>

#include <algorithm>
#include <cmath>
#include <map>

namespace oscil::test
{

namespace
{

// Resolve the dominant waveform color from a binned color histogram
juce::Colour findDominantColor(const std::map<uint32_t, int>& colorCounts)
{
    if (colorCounts.empty())
        return juce::Colours::black;

    auto maxIt = std::max_element(colorCounts.begin(), colorCounts.end(),
                                  [](const auto& a, const auto& b) { return a.second < b.second; });

    uint32_t key = maxIt->first;
    return juce::Colour(static_cast<uint8_t>((key / (256 * 256)) * 16), static_cast<uint8_t>(((key / 256) % 256) * 16),
                        static_cast<uint8_t>((key % 256) * 16));
}

} // anonymous namespace

// ================== Waveform Verification ==================

// Scan a single image column for waveform pixels. Returns the topmost waveform Y, or -1 if none.
int scanColumnForWaveform(const juce::Image& image, int x, int height, int centerY, juce::Colour backgroundColor,
                          int& totalNonBackground, float& maxAmplitude, std::map<uint32_t, int>& colorCounts)
{
    int waveformY = -1;
    for (int y = 0; y < height; ++y)
    {
        auto pixel = image.getPixelAt(x, y);
        if (!TestScreenshot::colorsMatch(pixel, backgroundColor, 30))
        {
            totalNonBackground++;
            if (waveformY < 0)
                waveformY = y;

            uint32_t colorKey =
                (pixel.getRed() / 16) * 256 * 256 + (pixel.getGreen() / 16) * 256 + (pixel.getBlue() / 16);
            colorCounts[colorKey]++;

            if (centerY > 0)
            {
                float dist = std::abs(y - centerY) / static_cast<float>(centerY);
                maxAmplitude = std::max(maxAmplitude, dist);
            }
        }
    }
    return waveformY;
}

WaveformAnalysis TestScreenshot::analyzeWaveform(const juce::Image& image, juce::Colour backgroundColor)
{
    WaveformAnalysis result;
    if (image.isNull())
        return result;

    int width = image.getWidth();
    int height = image.getHeight();
    int centerY = height / 2;
    int totalNonBackground = 0;
    float maxAmplitude = 0.0f;
    std::map<uint32_t, int> colorCounts;
    int lastY = -1, zeroCrossings = 0;

    for (int x = 0; x < width; x += 2)
    {
        int waveformY = scanColumnForWaveform(image, x, height, centerY, backgroundColor, totalNonBackground,
                                              maxAmplitude, colorCounts);
        if (waveformY >= 0 && lastY >= 0 && (lastY < centerY) != (waveformY < centerY))
            zeroCrossings++;
        if (waveformY >= 0)
            lastY = waveformY;
    }

    result.detected = (totalNonBackground > (width * height * 0.01f));
    result.amplitude = maxAmplitude;
    result.activity = static_cast<float>(totalNonBackground) / (width * height);
    result.zeroCrossings = zeroCrossings;
    result.dominantColor = findDominantColor(colorCounts);
    return result;
}

bool TestScreenshot::verifyWaveformRendered(const juce::String& elementId, float minAmplitude)
{
    auto image = getElementImage(elementId);
    if (image.isNull())
        return false;

    auto analysis = analyzeWaveform(image);
    return analysis.detected && analysis.amplitude >= minAmplitude;
}

bool TestScreenshot::verifyWaveformColor(const juce::String& elementId, juce::Colour expectedColor, int tolerance)
{
    auto image = getElementImage(elementId);
    if (image.isNull())
        return false;

    auto analysis = analyzeWaveform(image);
    if (!analysis.detected)
        return false;

    return colorsMatch(analysis.dominantColor, expectedColor, tolerance);
}

bool TestScreenshot::hasRenderedContent(const juce::Image& image, juce::Rectangle<int> region)
{
    if (image.isNull())
        return false;

    if (region.isEmpty())
        region = image.getBounds();

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
                    return true;
            }
        }
    }

    return differentPixels > 5;
}

float TestScreenshot::estimateWaveformAmplitude(const juce::Image& image)
{
    if (image.isNull())
        return 0.0f;

    int height = image.getHeight();
    int centerY = height / 2;
    float maxAmplitude = 0.0f;

    for (int x = 0; x < image.getWidth(); x += 2)
    {
        for (int y = 0; y < height; ++y)
        {
            auto pixel = image.getPixelAt(x, y);
            float brightness = pixel.getBrightness();

            if (brightness > 0.3f && centerY > 0)
            {
                float distanceFromCenter = std::abs(y - centerY) / static_cast<float>(centerY);
                maxAmplitude = std::max(maxAmplitude, distanceFromCenter);
            }
        }
    }

    return maxAmplitude;
}

// ================== Color Verification ==================

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

    return juce::Colour(static_cast<uint8_t>(totalR / count), static_cast<uint8_t>(totalG / count),
                        static_cast<uint8_t>(totalB / count));
}

juce::Colour TestScreenshot::getDominantColor(const juce::Image& image, juce::Rectangle<int> region)
{
    if (image.isNull())
        return juce::Colours::black;

    if (region.isEmpty())
        region = image.getBounds();

    auto histogram = getColorHistogram(image, region, 16);

    if (histogram.empty())
        return juce::Colours::black;

    auto maxIt = std::max_element(histogram.begin(), histogram.end(),
                                  [](const auto& a, const auto& b) { return a.second < b.second; });

    uint32_t key = maxIt->first;
    return juce::Colour(static_cast<uint8_t>((key >> 16) & 0xFF), static_cast<uint8_t>((key >> 8) & 0xFF),
                        static_cast<uint8_t>(key & 0xFF));
}

std::map<uint32_t, int> TestScreenshot::getColorHistogram(const juce::Image& image, juce::Rectangle<int> region,
                                                          int bucketSize)
{
    std::map<uint32_t, int> histogram;

    if (image.isNull())
        return histogram;

    if (region.isEmpty())
        region = image.getBounds();

    for (int y = region.getY(); y < region.getBottom(); ++y)
    {
        for (int x = region.getX(); x < region.getRight(); ++x)
        {
            auto pixel = image.getPixelAt(x, y);

            uint8_t r = static_cast<uint8_t>((pixel.getRed() / bucketSize) * bucketSize);
            uint8_t g = static_cast<uint8_t>((pixel.getGreen() / bucketSize) * bucketSize);
            uint8_t b = static_cast<uint8_t>((pixel.getBlue() / bucketSize) * bucketSize);

            uint32_t key =
                (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);

            histogram[key]++;
        }
    }

    return histogram;
}

bool TestScreenshot::verifyBackgroundColor(const juce::String& elementId, juce::Colour expectedColor, int tolerance)
{
    auto image = getElementImage(elementId);
    if (image.isNull())
        return false;

    std::vector<juce::Point<int>> samplePoints = {
        {2, 2}, {image.getWidth() - 3, 2}, {2, image.getHeight() - 3}, {image.getWidth() - 3, image.getHeight() - 3}};

    int matchCount = 0;
    for (const auto& point : samplePoints)
    {
        auto pixel = image.getPixelAt(point.x, point.y);
        if (colorsMatch(pixel, expectedColor, tolerance))
            matchCount++;
    }

    return matchCount >= 3;
}

bool TestScreenshot::verifyContainsColor(const juce::String& elementId, juce::Colour expectedColor, float minCoverage,
                                         int tolerance)
{
    auto image = getElementImage(elementId);
    if (image.isNull())
        return false;

    int matchingPixels = 0;
    int totalPixels = image.getWidth() * image.getHeight();

    for (int y = 0; y < image.getHeight(); y += 2)
    {
        for (int x = 0; x < image.getWidth(); x += 2)
        {
            auto pixel = image.getPixelAt(x, y);
            if (colorsMatch(pixel, expectedColor, tolerance))
                matchingPixels++;
        }
    }

    float coverage = static_cast<float>(matchingPixels * 4) / totalPixels;
    return coverage >= minCoverage;
}

bool TestScreenshot::colorsMatch(juce::Colour c1, juce::Colour c2, int tolerance)
{
    return std::abs(c1.getRed() - c2.getRed()) <= tolerance && std::abs(c1.getGreen() - c2.getGreen()) <= tolerance &&
           std::abs(c1.getBlue() - c2.getBlue()) <= tolerance;
}

// ================== Component State Verification ==================

juce::Rectangle<int> TestScreenshot::getElementBounds(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return {};

    return component->getBounds();
}

bool TestScreenshot::verifyElementBounds(const juce::String& elementId, int expectedWidth, int expectedHeight,
                                         int tolerance)
{
    auto bounds = getElementBounds(elementId);
    if (bounds.isEmpty())
        return false;

    return std::abs(bounds.getWidth() - expectedWidth) <= tolerance &&
           std::abs(bounds.getHeight() - expectedHeight) <= tolerance;
}

bool TestScreenshot::verifyElementVisible(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    if (!component->isVisible())
        return false;

    auto* parent = component->getParentComponent();
    if (parent == nullptr)
        return true;

    auto bounds = component->getBounds();
    auto parentBounds = parent->getLocalBounds();

    return parentBounds.intersects(bounds);
}

// ================== Baseline Management ==================

bool TestScreenshot::saveBaseline(const juce::String& elementId, const juce::File& baselineDir,
                                  const juce::String& name)
{
    auto image = getElementImage(elementId);
    if (image.isNull())
        return false;

    juce::String filename = name.isEmpty() ? elementId : name;
    filename = filename.replaceCharacters(":/\\", "___");

    auto file = baselineDir.getChildFile(filename + ".png");
    return saveImage(image, file);
}

juce::Image TestScreenshot::loadBaseline(const juce::File& baselineFile) { return loadImage(baselineFile); }

// ================== Utility ==================

bool TestScreenshot::saveImage(const juce::Image& image, const juce::File& file)
{
    if (image.isNull())
        return false;

    file.getParentDirectory().createDirectory();

    juce::FileOutputStream stream(file);
    if (!stream.openedOk())
        return false;

    juce::PNGImageFormat pngFormat;
    return pngFormat.writeImageToStream(image, stream);
}

juce::Image TestScreenshot::loadImage(const juce::File& file)
{
    if (!file.existsAsFile())
        return {};

    juce::FileInputStream stream(file);
    if (!stream.openedOk())
        return {};

    juce::PNGImageFormat pngFormat;
    return pngFormat.decodeImage(stream);
}

} // namespace oscil::test
