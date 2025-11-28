/*
    Oscil - Color Swatches Component Implementation
*/

#include "ui/components/OscilColorSwatches.h"

namespace oscil
{

OscilColorSwatches::OscilColorSwatches()
    : hoverSpring_(SpringPresets::stiff())
{
    setWantsKeyboardFocus(true);

    theme_ = ThemeManager::getInstance().getCurrentTheme();
    ThemeManager::getInstance().addListener(this);

    hoverSpring_.position = 0.0f;
}

OscilColorSwatches::OscilColorSwatches(const juce::String& testId)
    : OscilColorSwatches()
{
    setTestId(testId);
}

void OscilColorSwatches::registerTestId()
{
    OSCIL_REGISTER_TEST_ID(testId_);
}

OscilColorSwatches::~OscilColorSwatches()
{
    ThemeManager::getInstance().removeListener(this);
    stopTimer();
}

void OscilColorSwatches::setColors(const std::vector<juce::Colour>& colors)
{
    colors_ = colors;

    if (selectedIndex_ >= static_cast<int>(colors_.size()))
        selectedIndex_ = -1;

    repaint();
}

void OscilColorSwatches::addColor(juce::Colour color)
{
    colors_.push_back(color);
    repaint();
}

void OscilColorSwatches::clearColors()
{
    colors_.clear();
    selectedIndex_ = -1;
    hoveredIndex_ = -1;
    repaint();
}

void OscilColorSwatches::setSelectedIndex(int index, bool notify)
{
    if (index < -1 || index >= static_cast<int>(colors_.size()))
        return;

    if (selectedIndex_ == index)
        return;

    selectedIndex_ = index;
    repaint();

    if (notify && onColorSelected && index >= 0)
        onColorSelected(index, colors_[static_cast<size_t>(index)]);
}

juce::Colour OscilColorSwatches::getSelectedColor() const
{
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(colors_.size()))
        return colors_[static_cast<size_t>(selectedIndex_)];

    return juce::Colour();
}

void OscilColorSwatches::setSelectedColor(juce::Colour color, bool notify)
{
    for (int i = 0; i < static_cast<int>(colors_.size()); ++i)
    {
        if (colors_[static_cast<size_t>(i)] == color)
        {
            setSelectedIndex(i, notify);
            return;
        }
    }
}

void OscilColorSwatches::setSwatchSize(int size)
{
    if (swatchSize_ != size)
    {
        swatchSize_ = size;
        repaint();
    }
}

void OscilColorSwatches::setSpacing(int spacing)
{
    if (spacing_ != spacing)
    {
        spacing_ = spacing;
        repaint();
    }
}

void OscilColorSwatches::setColumns(int cols)
{
    if (columns_ != cols)
    {
        columns_ = cols;
        repaint();
    }
}

void OscilColorSwatches::setShowCheckmark(bool show)
{
    if (showCheckmark_ != show)
    {
        showCheckmark_ = show;
        repaint();
    }
}

int OscilColorSwatches::getColumnCount() const
{
    if (columns_ > 0)
        return columns_;

    // Auto-calculate based on width
    int width = getWidth();
    if (width <= 0)
        return 8;  // Default

    return std::max(1, (width + spacing_) / (swatchSize_ + spacing_));
}

int OscilColorSwatches::getRowCount() const
{
    int cols = getColumnCount();
    return static_cast<int>((colors_.size() + static_cast<size_t>(cols) - 1) / static_cast<size_t>(cols));
}

int OscilColorSwatches::getPreferredWidth() const
{
    int cols = columns_ > 0 ? columns_ : 8;
    return cols * swatchSize_ + (cols - 1) * spacing_;
}

int OscilColorSwatches::getPreferredHeight() const
{
    int cols = getColumnCount();
    int rows = static_cast<int>((colors_.size() + static_cast<size_t>(cols) - 1) / static_cast<size_t>(cols));
    return rows * swatchSize_ + (rows - 1) * spacing_;
}

void OscilColorSwatches::paint(juce::Graphics& g)
{
    for (int i = 0; i < static_cast<int>(colors_.size()); ++i)
    {
        auto bounds = getSwatchBounds(i);
        paintSwatch(g, i, bounds);
    }

    // Focus ring around focused swatch
    if (hasFocus_ && focusedIndex_ >= 0 && focusedIndex_ < static_cast<int>(colors_.size()))
    {
        auto bounds = getSwatchBounds(focusedIndex_).toFloat();
        g.setColour(theme_.controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
        g.drawRoundedRectangle(
            bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET),
            ComponentLayout::RADIUS_SM,
            ComponentLayout::FOCUS_RING_WIDTH
        );
    }
}

void OscilColorSwatches::paintSwatch(juce::Graphics& g, int index, juce::Rectangle<int> bounds)
{
    auto color = colors_[static_cast<size_t>(index)];
    bool isSelected = (index == selectedIndex_);
    bool isHovered = (index == hoveredIndex_);

    // Expand slightly on hover
    if (isHovered)
    {
        bounds = bounds.expanded(2);
    }

    // Draw checker pattern for transparent colors
    if (color.getAlpha() < 255)
    {
        int checkerSize = 4;
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
    }

    // Color fill
    g.setColour(color);
    g.fillRoundedRectangle(bounds.toFloat(), ComponentLayout::RADIUS_SM);

    // Border
    auto borderColour = isSelected ? theme_.controlActive
                      : color.getBrightness() > 0.9f ? theme_.controlBorder
                      : color.darker(0.3f);

    g.setColour(borderColour);
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), ComponentLayout::RADIUS_SM,
                           isSelected ? 2.0f : 1.0f);

    // Selection checkmark
    if (isSelected && showCheckmark_)
        paintCheckmark(g, bounds);
}

void OscilColorSwatches::paintCheckmark(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto color = colors_[static_cast<size_t>(selectedIndex_)];

    // Choose white or black checkmark based on color brightness
    auto checkColor = color.getBrightness() > 0.5f ? juce::Colours::black : juce::Colours::white;

    g.setColour(checkColor);

    float cx = static_cast<float>(bounds.getCentreX());
    float cy = static_cast<float>(bounds.getCentreY());
    float size = static_cast<float>(bounds.getWidth()) * 0.25f;

    juce::Path checkPath;
    checkPath.startNewSubPath(cx - size, cy);
    checkPath.lineTo(cx - size * 0.3f, cy + size * 0.7f);
    checkPath.lineTo(cx + size, cy - size * 0.5f);

    g.strokePath(checkPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
}

void OscilColorSwatches::resized()
{
    // Nothing to do
}

int OscilColorSwatches::getSwatchAtPosition(juce::Point<int> pos) const
{
    int cols = getColumnCount();

    int col = pos.x / (swatchSize_ + spacing_);
    int row = pos.y / (swatchSize_ + spacing_);

    if (col < 0 || col >= cols)
        return -1;

    int index = row * cols + col;

    if (index < 0 || index >= static_cast<int>(colors_.size()))
        return -1;

    // Check if actually within the swatch bounds
    auto swatchBounds = getSwatchBounds(index);
    if (swatchBounds.contains(pos))
        return index;

    return -1;
}

juce::Rectangle<int> OscilColorSwatches::getSwatchBounds(int index) const
{
    int cols = getColumnCount();

    int col = index % cols;
    int row = index / cols;

    int x = col * (swatchSize_ + spacing_);
    int y = row * (swatchSize_ + spacing_);

    return juce::Rectangle<int>(x, y, swatchSize_, swatchSize_);
}

void OscilColorSwatches::mouseDown(const juce::MouseEvent& e)
{
    int index = getSwatchAtPosition(e.getPosition());

    if (index >= 0)
    {
        setSelectedIndex(index);
        focusedIndex_ = index;
    }
}

void OscilColorSwatches::mouseMove(const juce::MouseEvent& e)
{
    int newHovered = getSwatchAtPosition(e.getPosition());

    if (newHovered != hoveredIndex_)
    {
        hoveredIndex_ = newHovered;
        repaint();

        // Notify for live preview
        if (onColorHovered && newHovered >= 0)
            onColorHovered(colors_[static_cast<size_t>(newHovered)]);
    }
}

void OscilColorSwatches::mouseExit(const juce::MouseEvent&)
{
    if (hoveredIndex_ != -1)
    {
        hoveredIndex_ = -1;
        repaint();

        // Notify hover ended (revert preview)
        if (onColorHovered && selectedIndex_ >= 0)
            onColorHovered(colors_[static_cast<size_t>(selectedIndex_)]);
    }
}

bool OscilColorSwatches::keyPressed(const juce::KeyPress& key)
{
    if (colors_.empty())
        return false;

    int cols = getColumnCount();
    int newFocus = focusedIndex_;

    if (key == juce::KeyPress::leftKey)
        newFocus = std::max(0, focusedIndex_ - 1);
    else if (key == juce::KeyPress::rightKey)
        newFocus = std::min(static_cast<int>(colors_.size()) - 1, focusedIndex_ + 1);
    else if (key == juce::KeyPress::upKey)
        newFocus = std::max(0, focusedIndex_ - cols);
    else if (key == juce::KeyPress::downKey)
        newFocus = std::min(static_cast<int>(colors_.size()) - 1, focusedIndex_ + cols);
    else if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey)
    {
        setSelectedIndex(focusedIndex_);
        return true;
    }

    if (newFocus != focusedIndex_)
    {
        focusedIndex_ = newFocus;
        repaint();

        // Preview on keyboard nav
        if (onColorHovered)
            onColorHovered(colors_[static_cast<size_t>(focusedIndex_)]);

        return true;
    }

    return false;
}

void OscilColorSwatches::focusGained(FocusChangeType)
{
    hasFocus_ = true;

    if (selectedIndex_ >= 0)
        focusedIndex_ = selectedIndex_;
    else if (focusedIndex_ < 0 && !colors_.empty())
        focusedIndex_ = 0;

    repaint();
}

void OscilColorSwatches::focusLost(FocusChangeType)
{
    hasFocus_ = false;
    repaint();
}

void OscilColorSwatches::timerCallback()
{
    hoverSpring_.update(AnimationTiming::FRAME_DURATION_60FPS);

    if (hoverSpring_.isSettled())
        stopTimer();

    repaint();
}

void OscilColorSwatches::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    repaint();
}

std::unique_ptr<juce::AccessibilityHandler> OscilColorSwatches::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this,
        juce::AccessibilityRole::group
    );
}

} // namespace oscil
