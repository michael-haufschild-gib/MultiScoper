/*
    Oscil - Color Picker Component Implementation
*/

#include "ui/components/OscilColorPicker.h"

namespace oscil
{

OscilColorPicker::OscilColorPicker(IThemeService& themeService, const juce::String& testId)
    : ThemedComponent(themeService)
{
    if (testId.isNotEmpty())
        setTestId(testId);
    currentColor_ = juce::Colours::red;
    originalColor_ = currentColor_;

    // Create hex input
    hexInput_ = std::make_unique<juce::TextEditor>();
    hexInput_->setMultiLine(false);
    hexInput_->setInputRestrictions(9, "#0123456789ABCDEFabcdef");
    hexInput_->onReturnKey = [this]() {
        auto text = hexInput_->getText();
        if (text.startsWithChar('#'))
            text = text.substring(1);

        if (text.length() >= 6)
        {
            auto color = juce::Colour::fromString("FF" + text.substring(0, 6));
            if (text.length() >= 8)
                color = juce::Colour::fromString(text.substring(6, 8) + text.substring(0, 6));

            setColor(color);
        }
    };
    addAndMakeVisible(*hexInput_);
    updateHexField();
}

void OscilColorPicker::registerTestId()
{
    OSCIL_REGISTER_TEST_ID(testId_);
}

OscilColorPicker::~OscilColorPicker()
{
    // ScopedAnimators handle completion on destruction
}

void OscilColorPicker::setColor(juce::Colour color, bool notify)
{
    currentColor_ = color;

    hue_ = color.getHue();
    saturation_ = color.getSaturation();
    brightness_ = color.getBrightness();
    alpha_ = color.getFloatAlpha();

    updateHexField();
    repaint();

    if (notify && onColorChanged)
        onColorChanged(currentColor_);
}

void OscilColorPicker::setOriginalColor(juce::Colour color)
{
    originalColor_ = color;
    repaint();
}

void OscilColorPicker::setMode(Mode mode)
{
    if (mode_ != mode)
    {
        mode_ = mode;
        repaint();
    }
}

void OscilColorPicker::setShowAlpha(bool show)
{
    if (showAlpha_ != show)
    {
        showAlpha_ = show;
        resized();
    }
}

void OscilColorPicker::setShowHexInput(bool show)
{
    if (showHexInput_ != show)
    {
        showHexInput_ = show;
        hexInput_->setVisible(show);
        resized();
    }
}

void OscilColorPicker::setShowPreview(bool show)
{
    if (showPreview_ != show)
    {
        showPreview_ = show;
        resized();
    }
}

int OscilColorPicker::getPreferredHeight() const
{
    int height = GRADIENT_SIZE + SLIDER_SPACING + SLIDER_HEIGHT;  // Gradient + hue slider

    if (showAlpha_)
        height += SLIDER_SPACING + SLIDER_HEIGHT;

    if (showPreview_ || showHexInput_)
        height += SLIDER_SPACING + PREVIEW_HEIGHT;

    return height;
}

void OscilColorPicker::paint(juce::Graphics& g)
{
    // Guard against zero-size painting
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

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
    float indicatorX = static_cast<float>(bounds.getX()) + saturation_ * static_cast<float>(bounds.getWidth());
    float indicatorY = static_cast<float>(bounds.getY()) + (1.0f - brightness_) * static_cast<float>(bounds.getHeight());

    g.setColour(juce::Colours::white);
    g.drawEllipse(indicatorX - 6, indicatorY - 6, 12, 12, 2.0f);
    g.setColour(juce::Colours::black);
    g.drawEllipse(indicatorX - 5, indicatorY - 5, 10, 10, 1.0f);
}

void OscilColorPicker::paintWheelMode(juce::Graphics& g)
{
    auto bounds = getGradientBounds();
    
    // Check if cache is stale: bounds changed, image invalid, or mode changed from square to wheel
    bool needsUpdate = cachedGradientBounds_ != bounds ||
                       !cachedGradientImage_.isValid() ||
                       !cachedIsWheelMode_;  // Was in square mode, need to regenerate for wheel

    if (needsUpdate)
    {
        updateGradientCache(bounds);
    }

    g.drawImage(cachedGradientImage_, bounds.toFloat());
    
    // Indicator
    float radius = static_cast<float>(std::min(bounds.getWidth(), bounds.getHeight())) / 2.0f;
    float indicatorAngle = (hue_ * 2.0f * juce::MathConstants<float>::pi) - juce::MathConstants<float>::pi;
    float indicatorDist = saturation_ * radius;
    
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    
    float indicatorX = cx + std::cos(indicatorAngle) * indicatorDist;
    float indicatorY = cy + std::sin(indicatorAngle) * indicatorDist;

    g.setColour(juce::Colours::white);
    g.drawEllipse(indicatorX - 6, indicatorY - 6, 12, 12, 2.0f);
    g.setColour(juce::Colours::black);
    g.drawEllipse(indicatorX - 5, indicatorY - 5, 10, 10, 1.0f);
}

void OscilColorPicker::updateGradientCache(const juce::Rectangle<int>& bounds)
{
    // Guard against zero or negative dimensions
    if (bounds.getWidth() <= 0 || bounds.getHeight() <= 0)
        return;

    cachedGradientImage_ = juce::Image(juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
    cachedGradientBounds_ = bounds;

    if (mode_ == Mode::Square)
    {
        cachedHue_ = hue_;
        cachedIsWheelMode_ = false;

        juce::Image::BitmapData data(cachedGradientImage_, juce::Image::BitmapData::writeOnly);

        float heightF = static_cast<float>(std::max(1, bounds.getHeight()));
        float widthF = static_cast<float>(std::max(1, bounds.getWidth()));

        for (int y = 0; y < bounds.getHeight(); ++y)
        {
            float brightness = 1.0f - (static_cast<float>(y) / heightF);

            for (int x = 0; x < bounds.getWidth(); ++x)
            {
                float saturation = static_cast<float>(x) / widthF;
                auto color = juce::Colour::fromHSV(hue_, saturation, brightness, 1.0f);
                data.setPixelColour(x, y, color);
            }
        }
    }
    else
    {
        cachedHue_ = -10.0f; // Not used in wheel mode
        cachedIsWheelMode_ = true;
        float radius = std::max(1.0f, static_cast<float>(std::min(bounds.getWidth(), bounds.getHeight())) / 2.0f);
        float cx = static_cast<float>(bounds.getWidth()) / 2.0f;
        float cy = static_cast<float>(bounds.getHeight()) / 2.0f;

        juce::Image::BitmapData data(cachedGradientImage_, juce::Image::BitmapData::writeOnly);

        for (int y = 0; y < bounds.getHeight(); ++y)
        {
            for (int x = 0; x < bounds.getWidth(); ++x)
            {
                float dx = static_cast<float>(x) - cx;
                float dy = static_cast<float>(y) - cy;
                float distance = std::sqrt(dx * dx + dy * dy);

                if (distance <= radius)
                {
                    float angle = std::atan2(dy, dx);
                    float hue = (angle + juce::MathConstants<float>::pi) /
                               (2.0f * juce::MathConstants<float>::pi);
                    float sat = distance / radius;

                    auto color = juce::Colour::fromHSV(hue, sat, brightness_, 1.0f);
                    data.setPixelColour(x, y, color);
                }
            }
        }
    }
}

void OscilColorPicker::paintHueSlider(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (bounds.getWidth() <= 0 || bounds.getHeight() <= 0)
        return;

    // Hue gradient
    float widthF = static_cast<float>(std::max(1, bounds.getWidth()));
    for (int x = 0; x < bounds.getWidth(); ++x)
    {
        float hue = static_cast<float>(x) / widthF;
        g.setColour(juce::Colour::fromHSV(hue, 1.0f, 1.0f, 1.0f));
        g.fillRect(bounds.getX() + x, bounds.getY(), 1, bounds.getHeight());
    }

    // Border
    g.setColour(getTheme().controlBorder);
    g.drawRect(bounds);

    // Indicator
    float indicatorX = static_cast<float>(bounds.getX()) + hue_ * static_cast<float>(bounds.getWidth());
    g.setColour(juce::Colours::white);
    g.fillRect(static_cast<int>(indicatorX) - 2, bounds.getY() - 2, 4, bounds.getHeight() + 4);
    g.setColour(juce::Colours::black);
    g.drawRect(static_cast<int>(indicatorX) - 2, bounds.getY() - 2, 4, bounds.getHeight() + 4);
}

void OscilColorPicker::paintAlphaSlider(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Checker background
    int checkerSize = 6;
    for (int y = bounds.getY(); y < bounds.getBottom(); y += checkerSize)
    {
        for (int x = bounds.getX(); x < bounds.getRight(); x += checkerSize)
        {
            bool isWhite = ((x - bounds.getX()) / checkerSize + (y - bounds.getY()) / checkerSize) % 2 == 0;
            g.setColour(isWhite ? juce::Colours::white : juce::Colours::lightgrey);
            g.fillRect(x, y,
                std::min(checkerSize, bounds.getRight() - x),
                std::min(checkerSize, bounds.getBottom() - y));
        }
    }

    // Alpha gradient
    auto baseColor = juce::Colour::fromHSV(hue_, saturation_, brightness_, 1.0f);
    float widthF = static_cast<float>(std::max(1, bounds.getWidth()));
    for (int x = 0; x < bounds.getWidth(); ++x)
    {
        float alpha = static_cast<float>(x) / widthF;
        g.setColour(baseColor.withAlpha(alpha));
        g.fillRect(bounds.getX() + x, bounds.getY(), 1, bounds.getHeight());
    }

    // Border
    g.setColour(getTheme().controlBorder);
    g.drawRect(bounds);

    // Indicator
    float indicatorX = static_cast<float>(bounds.getX()) + alpha_ * static_cast<float>(bounds.getWidth());
    g.setColour(juce::Colours::white);
    g.fillRect(static_cast<int>(indicatorX) - 2, bounds.getY() - 2, 4, bounds.getHeight() + 4);
    g.setColour(juce::Colours::black);
    g.drawRect(static_cast<int>(indicatorX) - 2, bounds.getY() - 2, 4, bounds.getHeight() + 4);
}

void OscilColorPicker::paintPreview(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    int halfWidth = bounds.getWidth() / 2;

    // Original color
    auto originalBounds = bounds.withWidth(halfWidth);
    g.setColour(originalColor_);
    g.fillRect(originalBounds);

    // Current color
    auto currentBounds = bounds.withX(bounds.getX() + halfWidth).withWidth(halfWidth);

    // Checker for alpha
    if (currentColor_.getAlpha() < 255)
    {
        int checkerSize = 6;
        for (int y = currentBounds.getY(); y < currentBounds.getBottom(); y += checkerSize)
        {
            for (int x = currentBounds.getX(); x < currentBounds.getRight(); x += checkerSize)
            {
                bool isWhite = ((x - currentBounds.getX()) / checkerSize + (y - currentBounds.getY()) / checkerSize) % 2 == 0;
                g.setColour(isWhite ? juce::Colours::white : juce::Colours::lightgrey);
                g.fillRect(x, y,
                    std::min(checkerSize, currentBounds.getRight() - x),
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

juce::Rectangle<int> OscilColorPicker::getGradientBounds() const
{
    return juce::Rectangle<int>(0, 0, getWidth(), GRADIENT_SIZE);
}

juce::Rectangle<int> OscilColorPicker::getHueSliderBounds() const
{
    return juce::Rectangle<int>(0, GRADIENT_SIZE + SLIDER_SPACING, getWidth(), SLIDER_HEIGHT);
}

juce::Rectangle<int> OscilColorPicker::getAlphaSliderBounds() const
{
    int y = GRADIENT_SIZE + SLIDER_SPACING + SLIDER_HEIGHT + SLIDER_SPACING;
    return juce::Rectangle<int>(0, y, getWidth(), SLIDER_HEIGHT);
}

juce::Rectangle<int> OscilColorPicker::getPreviewBounds() const
{
    int y = GRADIENT_SIZE + SLIDER_SPACING + SLIDER_HEIGHT;
    if (showAlpha_)
        y += SLIDER_SPACING + SLIDER_HEIGHT;
    y += SLIDER_SPACING;

    int width = showHexInput_ ? getWidth() / 2 - 4 : getWidth();
    return juce::Rectangle<int>(0, y, width, PREVIEW_HEIGHT);
}

juce::Rectangle<int> OscilColorPicker::getHexInputBounds() const
{
    auto previewBounds = getPreviewBounds();
    return juce::Rectangle<int>(previewBounds.getRight() + 8, previewBounds.getY(),
                                 getWidth() - previewBounds.getRight() - 8, PREVIEW_HEIGHT);
}

void OscilColorPicker::resized()
{
    // Guard against zero or negative dimensions
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    if (showHexInput_)
        hexInput_->setBounds(getHexInputBounds());
}

OscilColorPicker::DragTarget OscilColorPicker::getDragTarget(juce::Point<int> pos) const
{
    if (getGradientBounds().contains(pos))
        return DragTarget::Gradient;

    if (getHueSliderBounds().expanded(4).contains(pos))
        return DragTarget::Hue;

    if (showAlpha_ && getAlphaSliderBounds().expanded(4).contains(pos))
        return DragTarget::Alpha;

    return DragTarget::None;
}

void OscilColorPicker::mouseDown(const juce::MouseEvent& e)
{
    currentDragTarget_ = getDragTarget(e.getPosition());

    switch (currentDragTarget_)
    {
        case DragTarget::Gradient:
            handleGradientDrag(e.getPosition());
            break;
        case DragTarget::Hue:
            handleHueDrag(e.getPosition());
            break;
        case DragTarget::Alpha:
            handleAlphaDrag(e.getPosition());
            break;
        case DragTarget::None:
            break;
    }
}

void OscilColorPicker::mouseDrag(const juce::MouseEvent& e)
{
    switch (currentDragTarget_)
    {
        case DragTarget::Gradient:
            handleGradientDrag(e.getPosition());
            break;
        case DragTarget::Hue:
            handleHueDrag(e.getPosition());
            break;
        case DragTarget::Alpha:
            handleAlphaDrag(e.getPosition());
            break;
        case DragTarget::None:
            break;
    }
}

void OscilColorPicker::mouseUp(const juce::MouseEvent&)
{
    if (currentDragTarget_ != DragTarget::None && onColorChanged)
        onColorChanged(currentColor_);

    currentDragTarget_ = DragTarget::None;
}

void OscilColorPicker::handleGradientDrag(juce::Point<int> pos)
{
    auto bounds = getGradientBounds();

    // Guard against division by zero if bounds are invalid
    if (bounds.getWidth() <= 0 || bounds.getHeight() <= 0)
        return;

    saturation_ = std::clamp(static_cast<float>(pos.x - bounds.getX()) / static_cast<float>(bounds.getWidth()), 0.0f, 1.0f);
    brightness_ = std::clamp(1.0f - static_cast<float>(pos.y - bounds.getY()) / static_cast<float>(bounds.getHeight()), 0.0f, 1.0f);

    updateFromHSV();
}

void OscilColorPicker::handleHueDrag(juce::Point<int> pos)
{
    auto bounds = getHueSliderBounds();

    // Guard against division by zero if bounds are invalid
    if (bounds.getWidth() <= 0)
        return;

    hue_ = std::clamp(static_cast<float>(pos.x - bounds.getX()) / static_cast<float>(bounds.getWidth()), 0.0f, 1.0f);
    updateFromHSV();
}

void OscilColorPicker::handleAlphaDrag(juce::Point<int> pos)
{
    auto bounds = getAlphaSliderBounds();

    // Guard against division by zero if bounds are invalid
    if (bounds.getWidth() <= 0)
        return;

    alpha_ = std::clamp(static_cast<float>(pos.x - bounds.getX()) / static_cast<float>(bounds.getWidth()), 0.0f, 1.0f);
    updateFromHSV();
}

void OscilColorPicker::updateFromHSV()
{
    currentColor_ = juce::Colour::fromHSV(hue_, saturation_, brightness_, alpha_);
    updateHexField();
    repaint();

    if (onColorChanging)
        onColorChanging(currentColor_);
}

void OscilColorPicker::updateHexField()
{
    juce::String hex = "#" + currentColor_.toDisplayString(showAlpha_);
    hexInput_->setText(hex, juce::dontSendNotification);
}

} // namespace oscil
