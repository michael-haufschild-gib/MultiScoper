/*
    Oscil - Crosshair Overlay Implementation
*/

#include "ui/layout/pane/overlays/CrosshairOverlay.h"
#include "ui/theme/ThemeManager.h"
#include <cmath>

namespace oscil
{

CrosshairOverlay::CrosshairOverlay(IThemeService& themeService)
    : themeService_(themeService)
{
    setOpaque(false);
    setInterceptsMouseClicks(false, false);  // Always click-through

    themeService_.addListener(this);
}

CrosshairOverlay::CrosshairOverlay(IThemeService& themeService, const juce::String& testId)
    : CrosshairOverlay(themeService)
{
#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_REGISTER_TEST_ID(testId.toRawUTF8());
#else
    juce::ignoreUnused(testId);
#endif
}

CrosshairOverlay::~CrosshairOverlay()
{
    themeService_.removeListener(this);
}

void CrosshairOverlay::setMousePosition(juce::Point<int> pos)
{
    if (mousePos_ != pos)
    {
        mousePos_ = pos;
        if (crosshairVisible_)
            repaint();
    }
}

void CrosshairOverlay::setTimeValue(float timeMs)
{
    if (std::abs(timeMs_ - timeMs) > 0.001f)
    {
        timeMs_ = timeMs;
        if (crosshairVisible_)
            repaint();
    }
}

void CrosshairOverlay::setAmplitudeValue(float ampDb)
{
    if (std::abs(ampDb_ - ampDb) > 0.01f)
    {
        ampDb_ = ampDb;
        if (crosshairVisible_)
            repaint();
    }
}

void CrosshairOverlay::setCrosshairVisible(bool visible)
{
    if (crosshairVisible_ != visible)
    {
        crosshairVisible_ = visible;
        repaint();
    }
}

void CrosshairOverlay::paint(juce::Graphics& g)
{
    if (!crosshairVisible_)
        return;

    paintCrosshairLines(g);
    paintTooltip(g);
}

void CrosshairOverlay::paintCrosshairLines(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();
    auto bounds = getLocalBounds();

    g.setColour(theme.crosshairLine.withAlpha(LINE_OPACITY));

    // Vertical line
    g.drawVerticalLine(mousePos_.x, 0.0f, static_cast<float>(bounds.getHeight()));

    // Horizontal line
    g.drawHorizontalLine(mousePos_.y, 0.0f, static_cast<float>(bounds.getWidth()));
}

void CrosshairOverlay::paintTooltip(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();
    auto tooltipBounds = calculateTooltipBounds();

    // Background
    g.setColour(theme.backgroundSecondary.withAlpha(TOOLTIP_BACKGROUND_OPACITY));
    g.fillRoundedRectangle(tooltipBounds.toFloat(), TOOLTIP_CORNER_RADIUS);

    // Border
    g.setColour(theme.controlBorder);
    g.drawRoundedRectangle(tooltipBounds.toFloat().reduced(0.5f), TOOLTIP_CORNER_RADIUS, 1.0f);

    // Content area
    auto contentBounds = tooltipBounds.reduced(TOOLTIP_PADDING);

    // Time row
    auto timeRowBounds = contentBounds.removeFromTop(TOOLTIP_ROW_HEIGHT);
    auto timeLabelBounds = timeRowBounds.removeFromLeft(TOOLTIP_LABEL_WIDTH);
    auto timeValueBounds = timeRowBounds;

    g.setColour(theme.textSecondary);
    g.setFont(juce::FontOptions(ComponentLayout::FONT_SIZE_SMALL));
    g.drawText("Time:", timeLabelBounds, juce::Justification::centredLeft);

    g.setColour(theme.textPrimary);
    g.setFont(juce::FontOptions(ComponentLayout::FONT_SIZE_SMALL).withStyle("Bold"));
    g.drawText(juce::String(timeMs_, 2) + " ms", timeValueBounds, juce::Justification::centredRight);

    // Amplitude row
    auto ampRowBounds = contentBounds.removeFromTop(TOOLTIP_ROW_HEIGHT);
    auto ampLabelBounds = ampRowBounds.removeFromLeft(TOOLTIP_LABEL_WIDTH);
    auto ampValueBounds = ampRowBounds;

    g.setColour(theme.textSecondary);
    g.setFont(juce::FontOptions(ComponentLayout::FONT_SIZE_SMALL));
    g.drawText("Amp:", ampLabelBounds, juce::Justification::centredLeft);

    g.setColour(theme.textPrimary);
    g.setFont(juce::FontOptions(ComponentLayout::FONT_SIZE_SMALL).withStyle("Bold"));
    g.drawText(juce::String(ampDb_, 1) + " dB", ampValueBounds, juce::Justification::centredRight);
}

juce::Rectangle<int> CrosshairOverlay::calculateTooltipBounds() const
{
    int tooltipWidth = TOOLTIP_LABEL_WIDTH + TOOLTIP_VALUE_WIDTH + 2 * TOOLTIP_PADDING;
    int tooltipHeight = 2 * TOOLTIP_ROW_HEIGHT + 2 * TOOLTIP_PADDING;

    auto bounds = getLocalBounds();

    // Default position: offset to the right and below cursor
    int x = mousePos_.x + TOOLTIP_OFFSET_X;
    int y = mousePos_.y + TOOLTIP_OFFSET_Y;

    // Flip horizontally if would go off right edge
    if (x + tooltipWidth > bounds.getRight())
    {
        x = mousePos_.x - TOOLTIP_OFFSET_X - tooltipWidth;
    }

    // Flip vertically if would go off bottom edge
    if (y + tooltipHeight > bounds.getBottom())
    {
        y = mousePos_.y - TOOLTIP_OFFSET_Y - tooltipHeight;
    }

    // Clamp to bounds
    x = juce::jmax(bounds.getX(), juce::jmin(x, bounds.getRight() - tooltipWidth));
    y = juce::jmax(bounds.getY(), juce::jmin(y, bounds.getBottom() - tooltipHeight));

    return { x, y, tooltipWidth, tooltipHeight };
}

bool CrosshairOverlay::hitTest(int /*x*/, int /*y*/)
{
    // Always click-through
    return false;
}

void CrosshairOverlay::themeChanged(const ColorTheme& /*newTheme*/)
{
    repaint();
}

} // namespace oscil
