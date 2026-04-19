/*
    MultiScoper - Color Swatches Component Implementation
*/

#include "ui/components/MultiScoperColorSwatches.h"

#include "ui/components/SurfacePainter.h"

#include <utility>

namespace multiscoper
{

MultiScoperColorSwatches::MultiScoperColorSwatches(IThemeService& themeService, const juce::String& testId)
    : ThemedComponent(themeService)
    , hoverSpring_(SpringPresets::fast())
{
    if (testId.isNotEmpty())
        setTestId(testId);

    setWantsKeyboardFocus(true);
    hoverSpring_.position = 0.0f;
}

void MultiScoperColorSwatches::registerTestId() { MULTISCOPER_REGISTER_TEST_ID(testId_); }

MultiScoperColorSwatches::~MultiScoperColorSwatches() { stopTimer(); }

void MultiScoperColorSwatches::setColors(const std::vector<juce::Colour>& colors)
{
    colors_ = colors;

    if (std::cmp_greater_equal(selectedIndex_, colors_.size()))
        selectedIndex_ = -1;

    repaint();
}

void MultiScoperColorSwatches::addColor(juce::Colour color)
{
    colors_.push_back(color);
    repaint();
}

void MultiScoperColorSwatches::clearColors()
{
    colors_.clear();
    selectedIndex_ = -1;
    hoveredIndex_ = -1;
    repaint();
}

void MultiScoperColorSwatches::setSelectedIndex(int index, bool notify)
{
    if (index < -1 || std::cmp_greater_equal(index, colors_.size()))
        return;

    if (selectedIndex_ == index)
        return;

    selectedIndex_ = index;
    repaint();

    if (notify && onColorSelected && index >= 0)
        onColorSelected(index, colors_[static_cast<size_t>(index)]);
}

juce::Colour MultiScoperColorSwatches::getSelectedColor() const
{
    if (selectedIndex_ >= 0 && std::cmp_less(selectedIndex_, colors_.size()))
        return colors_[static_cast<size_t>(selectedIndex_)];

    return {};
}

void MultiScoperColorSwatches::setSelectedColor(juce::Colour color, bool notify)
{
    for (int i = 0; std::cmp_less(i, colors_.size()); ++i)
    {
        if (colors_[static_cast<size_t>(i)] == color)
        {
            setSelectedIndex(i, notify);
            return;
        }
    }
}

void MultiScoperColorSwatches::setSwatchSize(int size)
{
    if (swatchSize_ != size)
    {
        swatchSize_ = size;
        repaint();
    }
}

void MultiScoperColorSwatches::setSpacing(int spacing)
{
    if (spacing_ != spacing)
    {
        spacing_ = spacing;
        repaint();
    }
}

void MultiScoperColorSwatches::setColumns(int cols)
{
    if (columns_ != cols)
    {
        columns_ = cols;
        repaint();
    }
}

void MultiScoperColorSwatches::setShowCheckmark(bool show)
{
    if (showCheckmark_ != show)
    {
        showCheckmark_ = show;
        repaint();
    }
}

int MultiScoperColorSwatches::getColumnCount() const
{
    if (columns_ > 0)
        return columns_;

    // Auto-calculate based on width
    int const width = getWidth();
    if (width <= 0)
        return 8; // Default

    return std::max(1, (width + spacing_) / (swatchSize_ + spacing_));
}

int MultiScoperColorSwatches::getRowCount() const
{
    int const cols = getColumnCount();
    return static_cast<int>((colors_.size() + static_cast<size_t>(cols) - 1) / static_cast<size_t>(cols));
}

int MultiScoperColorSwatches::getPreferredWidth() const
{
    int const cols = columns_ > 0 ? columns_ : 8;
    return (cols * swatchSize_) + ((cols - 1) * spacing_);
}

int MultiScoperColorSwatches::getPreferredHeight() const
{
    if (colors_.empty())
        return 0;

    int const cols = getColumnCount();
    int const rows = static_cast<int>((colors_.size() + static_cast<size_t>(cols) - 1) / static_cast<size_t>(cols));
    return (rows * swatchSize_) + ((rows - 1) * spacing_);
}

void MultiScoperColorSwatches::paint(juce::Graphics& g)
{
    for (int i = 0; std::cmp_less(i, colors_.size()); ++i)
    {
        auto bounds = getSwatchBounds(i);
        paintSwatch(g, i, bounds);
    }

    // Focus ring around focused swatch — accent-colored via SurfacePainter
    if (hasFocus_ && focusedIndex_ >= 0 && std::cmp_less(focusedIndex_, colors_.size()))
    {
        auto focusBounds = getSwatchBounds(focusedIndex_).toFloat();
        SurfacePainter::paintFocusRing(g, focusBounds, 0.0f, getSurface().accent);
    }
}

void MultiScoperColorSwatches::paintSwatch(juce::Graphics& g, int index, juce::Rectangle<int> bounds)
{
    auto color = colors_[static_cast<size_t>(index)];
    bool const isSelected = (index == selectedIndex_);
    bool const isHovered = (index == hoveredIndex_);

    // Expand slightly on hover
    if (isHovered)
    {
        bounds = bounds.expanded(2);
    }

    // Draw checker pattern for transparent colors
    if (color.getAlpha() < 255)
    {
        SurfacePainter::paintCheckerboard(g, bounds, 4);
    }

    // Color fill (flat rectangle)
    g.setColour(color);
    g.fillRect(bounds);

    // Border — use accent for selection, glass borderSubtle otherwise
    const auto& glass = getSurface();
    auto borderColour = isSelected                     ? glass.accent
                        : color.getBrightness() > 0.9f ? glass.borderSubtle
                                                       : color.darker(0.3f);

    g.setColour(borderColour);
    g.drawRect(bounds, isSelected ? 2 : 1);

    // Selection ring for selected swatch
    if (isSelected)
    {
        SurfacePainter::paintFocusRing(g, bounds.toFloat(), 0.0f, glass.accent, 1.5f, 1.5f);
    }

    // Selection checkmark
    if (isSelected && showCheckmark_)
        paintCheckmark(g, bounds);
}

void MultiScoperColorSwatches::paintCheckmark(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto color = colors_[static_cast<size_t>(selectedIndex_)];

    // Choose white or black checkmark based on color brightness
    auto checkColor = color.getBrightness() > 0.5f ? juce::Colours::black : juce::Colours::white;

    g.setColour(checkColor);

    auto cx = static_cast<float>(bounds.getCentreX());
    auto cy = static_cast<float>(bounds.getCentreY());
    float const size = static_cast<float>(bounds.getWidth()) * 0.25f;

    juce::Path checkPath;
    checkPath.startNewSubPath(cx - size, cy);
    checkPath.lineTo(cx - (size * 0.3f), cy + (size * 0.7f));
    checkPath.lineTo(cx + size, cy - (size * 0.5f));

    g.strokePath(checkPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void MultiScoperColorSwatches::resized()
{
    // Nothing to do
}

int MultiScoperColorSwatches::getSwatchAtPosition(juce::Point<int> pos) const
{
    int const cols = getColumnCount();

    int const col = pos.x / (swatchSize_ + spacing_);
    int const row = pos.y / (swatchSize_ + spacing_);

    if (col < 0 || col >= cols)
        return -1;

    int const index = (row * cols) + col;

    if (index < 0 || std::cmp_greater_equal(index, colors_.size()))
        return -1;

    // Check if actually within the swatch bounds
    auto swatchBounds = getSwatchBounds(index);
    if (swatchBounds.contains(pos))
        return index;

    return -1;
}

juce::Rectangle<int> MultiScoperColorSwatches::getSwatchBounds(int index) const
{
    int const cols = getColumnCount();

    int const col = index % cols;
    int const row = index / cols;

    int const x = col * (swatchSize_ + spacing_);
    int const y = row * (swatchSize_ + spacing_);

    return {x, y, swatchSize_, swatchSize_};
}

void MultiScoperColorSwatches::mouseDown(const juce::MouseEvent& e)
{
    int const index = getSwatchAtPosition(e.getPosition());

    if (index >= 0)
    {
        setSelectedIndex(index);
        focusedIndex_ = index;
    }
}

void MultiScoperColorSwatches::mouseMove(const juce::MouseEvent& e)
{
    int const newHovered = getSwatchAtPosition(e.getPosition());

    if (newHovered != hoveredIndex_)
    {
        hoveredIndex_ = newHovered;
        repaint();

        // Notify for live preview
        if (onColorHovered && newHovered >= 0)
            onColorHovered(colors_[static_cast<size_t>(newHovered)]);
    }
}

void MultiScoperColorSwatches::mouseExit(const juce::MouseEvent& /*event*/)
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

bool MultiScoperColorSwatches::keyPressed(const juce::KeyPress& key)
{
    if (colors_.empty())
        return false;

    int const cols = getColumnCount();
    int newFocus = focusedIndex_;

    if (key == juce::KeyPress::leftKey)
    {
        newFocus = std::max(0, focusedIndex_ - 1);
    }
    else if (key == juce::KeyPress::rightKey)
    {
        newFocus = std::min(static_cast<int>(colors_.size()) - 1, focusedIndex_ + 1);
    }
    else if (key == juce::KeyPress::upKey)
    {
        newFocus = std::max(0, focusedIndex_ - cols);
    }
    else if (key == juce::KeyPress::downKey)
    {
        newFocus = std::min(static_cast<int>(colors_.size()) - 1, focusedIndex_ + cols);
    }
    // Space/return not consumed — they must pass through to the DAW for
    // transport shortcuts. Use mouse to select a swatch.

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

void MultiScoperColorSwatches::focusGained(FocusChangeType /*cause*/)
{
    hasFocus_ = true;

    if (selectedIndex_ >= 0)
        focusedIndex_ = selectedIndex_;
    else if (focusedIndex_ < 0 && !colors_.empty())
        focusedIndex_ = 0;

    repaint();
}

void MultiScoperColorSwatches::focusLost(FocusChangeType /*cause*/)
{
    hasFocus_ = false;
    repaint();
}

void MultiScoperColorSwatches::timerCallback()
{
    hoverSpring_.update(AnimationTiming::FRAME_DURATION_60FPS);

    if (hoverSpring_.isSettled())
        stopTimer();

    repaint();
}

std::unique_ptr<juce::AccessibilityHandler> MultiScoperColorSwatches::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(*this, juce::AccessibilityRole::group);
}

} // namespace multiscoper
