/*
    Oscil Test Harness - Screenshot & Visual Verification Implementation
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
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return {};

    return getComponentImage(component);
}

// ================== Image Comparison ==================

ImageComparisonResult TestScreenshot::compareImages(const juce::Image& image1,
                                                     const juce::Image& image2,
                                                     int tolerance)
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
        result.diffBounds = juce::Rectangle<int>(minDiffX, minDiffY,
                                                  maxDiffX - minDiffX + 1,
                                                  maxDiffY - minDiffY + 1);
    }

    result.similarity = 1.0f - (static_cast<float>(result.differentPixels) / result.totalPixels);
    result.match = (result.differentPixels == 0);

    return result;
}

ImageComparisonResult TestScreenshot::compareElementToBaseline(const juce::String& elementId,
                                                                const juce::File& baselineFile,
                                                                int tolerance)
{
    auto currentImage = getElementImage(elementId);
    auto baselineImage = loadImage(baselineFile);

    return compareImages(currentImage, baselineImage, tolerance);
}

juce::Image TestScreenshot::generateDiffImage(const juce::Image& image1,
                                               const juce::Image& image2,
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

// ================== Waveform Verification ==================

WaveformAnalysis TestScreenshot::analyzeWaveform(const juce::Image& image,
                                                  juce::Colour backgroundColor)
{
    WaveformAnalysis result;

    if (image.isNull())
        return result;

    int width = image.getWidth();
    int height = image.getHeight();
    int centerY = height / 2;

    int totalNonBackground = 0;
    float maxAmplitude = 0.0f;

    // Color histogram for finding dominant waveform color
    std::map<uint32_t, int> colorCounts;

    int lastY = -1;
    int zeroCrossings = 0;

    for (int x = 0; x < width; x += 2)
    {
        int waveformY = -1;

        for (int y = 0; y < height; ++y)
        {
            auto pixel = image.getPixelAt(x, y);

            // Check if this is a non-background pixel
            if (!colorsMatch(pixel, backgroundColor, 30))
            {
                totalNonBackground++;

                // Track waveform position for amplitude
                if (waveformY < 0)
                    waveformY = y;

                // Count colors for dominant color detection
                uint32_t colorKey = (pixel.getRed() / 16) * 256 * 256 +
                                    (pixel.getGreen() / 16) * 256 +
                                    (pixel.getBlue() / 16);
                colorCounts[colorKey]++;

                // Track amplitude
                float distanceFromCenter = std::abs(y - centerY) / static_cast<float>(centerY);
                maxAmplitude = std::max(maxAmplitude, distanceFromCenter);
            }
        }

        // Count zero crossings (waveform position crossing center)
        if (waveformY >= 0)
        {
            if (lastY >= 0)
            {
                bool prevAbove = lastY < centerY;
                bool currAbove = waveformY < centerY;
                if (prevAbove != currAbove)
                    zeroCrossings++;
            }
            lastY = waveformY;
        }
    }

    result.detected = (totalNonBackground > (width * height * 0.01f)); // At least 1% non-background
    result.amplitude = maxAmplitude;
    result.activity = static_cast<float>(totalNonBackground) / (width * height);
    result.zeroCrossings = zeroCrossings;

    // Find dominant color
    if (!colorCounts.empty())
    {
        auto maxIt = std::max_element(colorCounts.begin(), colorCounts.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

        uint32_t key = maxIt->first;
        result.dominantColor = juce::Colour(
            static_cast<uint8_t>((key / (256 * 256)) * 16),
            static_cast<uint8_t>(((key / 256) % 256) * 16),
            static_cast<uint8_t>((key % 256) * 16)
        );
    }

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

bool TestScreenshot::verifyWaveformColor(const juce::String& elementId,
                                          juce::Colour expectedColor,
                                          int tolerance)
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

            if (brightness > 0.3f)
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

    return juce::Colour(
        static_cast<uint8_t>(totalR / count),
        static_cast<uint8_t>(totalG / count),
        static_cast<uint8_t>(totalB / count)
    );
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
    return juce::Colour(
        static_cast<uint8_t>((key >> 16) & 0xFF),
        static_cast<uint8_t>((key >> 8) & 0xFF),
        static_cast<uint8_t>(key & 0xFF)
    );
}

std::map<uint32_t, int> TestScreenshot::getColorHistogram(const juce::Image& image,
                                                           juce::Rectangle<int> region,
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

            // Bucket the color values
            uint8_t r = static_cast<uint8_t>((pixel.getRed() / bucketSize) * bucketSize);
            uint8_t g = static_cast<uint8_t>((pixel.getGreen() / bucketSize) * bucketSize);
            uint8_t b = static_cast<uint8_t>((pixel.getBlue() / bucketSize) * bucketSize);

            uint32_t key = (static_cast<uint32_t>(r) << 16) |
                           (static_cast<uint32_t>(g) << 8) |
                           static_cast<uint32_t>(b);

            histogram[key]++;
        }
    }

    return histogram;
}

bool TestScreenshot::verifyBackgroundColor(const juce::String& elementId,
                                            juce::Colour expectedColor,
                                            int tolerance)
{
    auto image = getElementImage(elementId);
    if (image.isNull())
        return false;

    // Sample corners and edges for background color
    std::vector<juce::Point<int>> samplePoints = {
        {2, 2},
        {image.getWidth() - 3, 2},
        {2, image.getHeight() - 3},
        {image.getWidth() - 3, image.getHeight() - 3}
    };

    int matchCount = 0;
    for (const auto& point : samplePoints)
    {
        auto pixel = image.getPixelAt(point.x, point.y);
        if (colorsMatch(pixel, expectedColor, tolerance))
            matchCount++;
    }

    // At least 3 of 4 corners should match
    return matchCount >= 3;
}

bool TestScreenshot::verifyContainsColor(const juce::String& elementId,
                                          juce::Colour expectedColor,
                                          float minCoverage,
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

    // Adjust for sampling stride
    float coverage = static_cast<float>(matchingPixels * 4) / totalPixels;
    return coverage >= minCoverage;
}

bool TestScreenshot::colorsMatch(juce::Colour c1, juce::Colour c2, int tolerance)
{
    return std::abs(c1.getRed() - c2.getRed()) <= tolerance &&
           std::abs(c1.getGreen() - c2.getGreen()) <= tolerance &&
           std::abs(c1.getBlue() - c2.getBlue()) <= tolerance;
}

// ================== Component State Verification ==================

juce::Rectangle<int> TestScreenshot::getElementBounds(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return {};

    return component->getBounds();
}

bool TestScreenshot::verifyElementBounds(const juce::String& elementId,
                                          int expectedWidth, int expectedHeight,
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
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return false;

    if (!component->isVisible())
        return false;

    // Check if within parent bounds
    auto* parent = component->getParentComponent();
    if (parent == nullptr)
        return true; // Top-level component

    auto bounds = component->getBounds();
    auto parentBounds = parent->getLocalBounds();

    // Check if at least partially visible
    return parentBounds.intersects(bounds);
}

// ================== Baseline Management ==================

bool TestScreenshot::saveBaseline(const juce::String& elementId,
                                   const juce::File& baselineDir,
                                   const juce::String& name)
{
    auto image = getElementImage(elementId);
    if (image.isNull())
        return false;

    juce::String filename = name.isEmpty() ? elementId : name;
    filename = filename.replaceCharacters(":/\\", "___"); // Sanitize

    auto file = baselineDir.getChildFile(filename + ".png");
    return saveImage(image, file);
}

juce::Image TestScreenshot::loadBaseline(const juce::File& baselineFile)
{
    return loadImage(baselineFile);
}

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
