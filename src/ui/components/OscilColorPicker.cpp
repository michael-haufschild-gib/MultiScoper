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

void OscilColorPicker::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

OscilColorPicker::~OscilColorPicker() { stopTimer(); }

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
    int height = GRADIENT_SIZE + SLIDER_SPACING + SLIDER_HEIGHT; // Gradient + hue slider

    if (showAlpha_)
        height += SLIDER_SPACING + SLIDER_HEIGHT;

    if (showPreview_ || showHexInput_)
        height += SLIDER_SPACING + PREVIEW_HEIGHT;

    return height;
}

// paint, paintSquareMode, paintWheelMode, updateGradientCache, paintHueSlider,
// paintAlphaSlider, paintPreview are in OscilColorPickerCalc.cpp

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

    saturation_ =
        std::clamp(static_cast<float>(pos.x - bounds.getX()) / static_cast<float>(bounds.getWidth()), 0.0f, 1.0f);
    brightness_ = std::clamp(1.0f - static_cast<float>(pos.y - bounds.getY()) / static_cast<float>(bounds.getHeight()),
                             0.0f, 1.0f);

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

void OscilColorPicker::timerCallback() { stopTimer(); }

} // namespace oscil
