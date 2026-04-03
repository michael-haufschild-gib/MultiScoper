/*
    Oscil - Color Picker Painting & Gradient Cache
    (Core setup, events, and interaction are in OscilColorPicker.cpp)
*/

#include "ui/components/OscilColorPicker.h"

namespace oscil
{

void OscilColorPicker::paint(juce::Graphics& g)
{
    if (mode_ == Mode::Square)
        paintSquareMode(g);
    else
        paintWheelMode(g);

    paintHueSlider(g, getHueSliderBounds());

    if (showAlpha_)
        paintAlphaSlider(g, getAlphaSliderBounds());

    if (showPreview_)
        paintPreview(g, getPreviewBounds());
}

void OscilColorPicker::paintSquareMode(juce::Graphics& g)
{
    auto bounds = getGradientBounds();

    // Update cache if needed
    if (std::abs(cachedHue_ - hue_) > 0.001f || cachedGradientBounds_ != bounds || !cachedGradientImage_.isValid())
    {
        updateGradientCache(bounds);
    }

    g.drawImage(cachedGradientImage_, bounds.toFloat());

    // Border
    g.setColour(getTheme().controlBorder);
    g.drawRect(bounds);

    // Indicator
    float const indicatorX = static_cast<float>(bounds.getX()) + (saturation_ * static_cast<float>(bounds.getWidth()));
    float const indicatorY =
        static_cast<float>(bounds.getY()) + ((1.0f - brightness_) * static_cast<float>(bounds.getHeight()));

    g.setColour(juce::Colours::white);
    g.drawEllipse(indicatorX - 6, indicatorY - 6, 12, 12, 2.0f);
    g.setColour(juce::Colours::black);
    g.drawEllipse(indicatorX - 5, indicatorY - 5, 10, 10, 1.0f);
}

void OscilColorPicker::paintWheelMode(juce::Graphics& g)
{
    auto bounds = getGradientBounds();

    // Check if cache is stale: bounds changed, image invalid, or mode changed from square to wheel
    bool const needsUpdate = cachedGradientBounds_ != bounds || !cachedGradientImage_.isValid() ||
                             !cachedIsWheelMode_; // Was in square mode, need to regenerate for wheel

    if (needsUpdate)
    {
        updateGradientCache(bounds);
    }

    g.drawImage(cachedGradientImage_, bounds.toFloat());

    // Indicator
    float const radius = static_cast<float>(std::min(bounds.getWidth(), bounds.getHeight())) / 2.0f;
    float const indicatorAngle = (hue_ * 2.0f * juce::MathConstants<float>::pi) - juce::MathConstants<float>::pi;
    float const indicatorDist = saturation_ * radius;

    float const cx = static_cast<float>(bounds.getCentreX());
    float const cy = static_cast<float>(bounds.getCentreY());

    float const indicatorX = cx + (std::cos(indicatorAngle) * indicatorDist);
    float const indicatorY = cy + (std::sin(indicatorAngle) * indicatorDist);

    g.setColour(juce::Colours::white);
    g.drawEllipse(indicatorX - 6, indicatorY - 6, 12, 12, 2.0f);
    g.setColour(juce::Colours::black);
    g.drawEllipse(indicatorX - 5, indicatorY - 5, 10, 10, 1.0f);
}

void OscilColorPicker::updateSquareGradient(const juce::Rectangle<int>& bounds)
{
    cachedHue_ = hue_;
    cachedIsWheelMode_ = false;

    juce::Image::BitmapData const data(cachedGradientImage_, juce::Image::BitmapData::writeOnly);
    float const heightF = static_cast<float>(std::max(1, bounds.getHeight()));
    float const widthF = static_cast<float>(std::max(1, bounds.getWidth()));

    for (int y = 0; y < bounds.getHeight(); ++y)
    {
        float const brightness = 1.0f - (static_cast<float>(y) / heightF);
        for (int x = 0; x < bounds.getWidth(); ++x)
        {
            float const saturation = static_cast<float>(x) / widthF;
            data.setPixelColour(x, y, juce::Colour::fromHSV(hue_, saturation, brightness, 1.0f));
        }
    }
}

void OscilColorPicker::updateWheelGradient(const juce::Rectangle<int>& bounds)
{
    cachedHue_ = -10.0f;
    cachedIsWheelMode_ = true;
    float const radius = std::max(1.0f, static_cast<float>(std::min(bounds.getWidth(), bounds.getHeight())) / 2.0f);
    float const cx = static_cast<float>(bounds.getWidth()) / 2.0f;
    float const cy = static_cast<float>(bounds.getHeight()) / 2.0f;

    juce::Image::BitmapData const data(cachedGradientImage_, juce::Image::BitmapData::writeOnly);

    for (int y = 0; y < bounds.getHeight(); ++y)
    {
        for (int x = 0; x < bounds.getWidth(); ++x)
        {
            float const dx = static_cast<float>(x) - cx;
            float const dy = static_cast<float>(y) - cy;
            float const distance = std::sqrt((dx * dx) + (dy * dy));

            if (distance <= radius)
            {
                float const angle = std::atan2(dy, dx);
                float const hue = (angle + juce::MathConstants<float>::pi) / (2.0f * juce::MathConstants<float>::pi);
                float const sat = distance / radius;
                data.setPixelColour(x, y, juce::Colour::fromHSV(hue, sat, brightness_, 1.0f));
            }
        }
    }
}

void OscilColorPicker::updateGradientCache(const juce::Rectangle<int>& bounds)
{
    if (bounds.getWidth() <= 0 || bounds.getHeight() <= 0)
        return;

    cachedGradientImage_ = juce::Image(juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
    cachedGradientBounds_ = bounds;

    if (mode_ == Mode::Square)
        updateSquareGradient(bounds);
    else
        updateWheelGradient(bounds);
}

void OscilColorPicker::paintHueSlider(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (bounds.getWidth() <= 0 || bounds.getHeight() <= 0)
        return;

    // Hue gradient
    float const widthF = static_cast<float>(std::max(1, bounds.getWidth()));
    for (int x = 0; x < bounds.getWidth(); ++x)
    {
        float const hue = static_cast<float>(x) / widthF;
        g.setColour(juce::Colour::fromHSV(hue, 1.0f, 1.0f, 1.0f));
        g.fillRect(bounds.getX() + x, bounds.getY(), 1, bounds.getHeight());
    }

    // Border
    g.setColour(getTheme().controlBorder);
    g.drawRect(bounds);

    // Indicator
    float const indicatorX = static_cast<float>(bounds.getX()) + (hue_ * static_cast<float>(bounds.getWidth()));
    g.setColour(juce::Colours::white);
    g.fillRect(static_cast<int>(indicatorX) - 2, bounds.getY() - 2, 4, bounds.getHeight() + 4);
    g.setColour(juce::Colours::black);
    g.drawRect(static_cast<int>(indicatorX) - 2, bounds.getY() - 2, 4, bounds.getHeight() + 4);
}

void OscilColorPicker::paintAlphaSlider(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Checker background
    int const checkerSize = 6;
    for (int y = bounds.getY(); y < bounds.getBottom(); y += checkerSize)
    {
        for (int x = bounds.getX(); x < bounds.getRight(); x += checkerSize)
        {
            bool const isWhite = (((x - bounds.getX()) / checkerSize) + ((y - bounds.getY()) / checkerSize)) % 2 == 0;
            g.setColour(isWhite ? juce::Colours::white : juce::Colours::lightgrey);
            g.fillRect(x, y, std::min(checkerSize, bounds.getRight() - x),
                       std::min(checkerSize, bounds.getBottom() - y));
        }
    }

    // Alpha gradient
    auto baseColor = juce::Colour::fromHSV(hue_, saturation_, brightness_, 1.0f);
    float const widthF = static_cast<float>(std::max(1, bounds.getWidth()));
    for (int x = 0; x < bounds.getWidth(); ++x)
    {
        float const alpha = static_cast<float>(x) / widthF;
        g.setColour(baseColor.withAlpha(alpha));
        g.fillRect(bounds.getX() + x, bounds.getY(), 1, bounds.getHeight());
    }

    // Border
    g.setColour(getTheme().controlBorder);
    g.drawRect(bounds);

    // Indicator
    float const indicatorX = static_cast<float>(bounds.getX()) + (alpha_ * static_cast<float>(bounds.getWidth()));
    g.setColour(juce::Colours::white);
    g.fillRect(static_cast<int>(indicatorX) - 2, bounds.getY() - 2, 4, bounds.getHeight() + 4);
    g.setColour(juce::Colours::black);
    g.drawRect(static_cast<int>(indicatorX) - 2, bounds.getY() - 2, 4, bounds.getHeight() + 4);
}

void OscilColorPicker::paintPreview(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    int const halfWidth = bounds.getWidth() / 2;

    // Original color
    auto originalBounds = bounds.withWidth(halfWidth);
    g.setColour(originalColor_);
    g.fillRect(originalBounds);

    // Current color
    auto currentBounds = bounds.withX(bounds.getX() + halfWidth).withWidth(halfWidth);

    // Checker for alpha
    if (currentColor_.getAlpha() < 255)
    {
        int const checkerSize = 6;
        for (int y = currentBounds.getY(); y < currentBounds.getBottom(); y += checkerSize)
        {
            for (int x = currentBounds.getX(); x < currentBounds.getRight(); x += checkerSize)
            {
                bool const isWhite =
                    (((x - currentBounds.getX()) / checkerSize) + ((y - currentBounds.getY()) / checkerSize)) % 2 == 0;
                g.setColour(isWhite ? juce::Colours::white : juce::Colours::lightgrey);
                g.fillRect(x, y, std::min(checkerSize, currentBounds.getRight() - x),
                           std::min(checkerSize, currentBounds.getBottom() - y));
            }
        }
    }

    g.setColour(currentColor_);
    g.fillRect(currentBounds);

    // Border
    g.setColour(getTheme().controlBorder);
    g.drawRect(bounds);
}

} // namespace oscil
