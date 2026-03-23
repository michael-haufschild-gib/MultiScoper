/*
    Oscil - Software Grid Renderer Implementation
    CPU-based grid rendering extracted from WaveformComponent
*/

#include "ui/panels/SoftwareGridRenderer.h"

#include "ui/theme/ThemeManager.h"

#include <cmath>

namespace oscil
{

SoftwareGridRenderer::SoftwareGridRenderer(IThemeService& themeService) : themeService_(themeService) {}

void SoftwareGridRenderer::setShowGrid(bool show)
{
    showGrid_ = show;
    gridConfig_.enabled = show;
}

void SoftwareGridRenderer::setGridConfig(const GridConfiguration& config)
{
    gridConfig_ = config;
    showGrid_ = config.enabled;
}

SoftwareGridRenderer::GridDivisions SoftwareGridRenderer::resolveGridDivisions(NoteInterval interval, int beatsPerBar)
{
    GridDivisions result;

    switch (interval)
    {
        case NoteInterval::WHOLE:
            result.count = beatsPerBar;
            result.barBased = true;
            break;
        case NoteInterval::TWO_BARS:
            result.count = 2;
            result.barBased = true;
            break;
        case NoteInterval::THREE_BARS:
            result.count = 3;
            result.barBased = true;
            break;
        case NoteInterval::FOUR_BARS:
            result.count = 4;
            result.barBased = true;
            break;
        case NoteInterval::EIGHT_BARS:
            result.count = 8;
            result.barBased = true;
            break;
        default:
            result.count = 4;
            result.barBased = false;
            break;
    }

    if (result.count <= 0)
        result.count = 1;
    return result;
}

void SoftwareGridRenderer::render(juce::Graphics& g, juce::Rectangle<int> bounds, ProcessingMode processingMode)
{
    if (!showGrid_)
        return;

    const auto& theme = themeService_.getCurrentTheme();

    if (processingMode == ProcessingMode::FullStereo)
    {
        int halfHeight = bounds.getHeight() / 2;
        auto topBounds = bounds.removeFromTop(halfHeight);
        auto bottomBounds = bounds;

        drawChannelGrid(g, topBounds);
        drawChannelGrid(g, bottomBounds);

        g.setColour(theme.gridMajor);
        g.drawHorizontalLine(topBounds.getBottom(), static_cast<float>(bounds.getX()),
                             static_cast<float>(bounds.getRight()));

        g.setColour(theme.textSecondary.withAlpha(0.6f));
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        g.drawText("L", topBounds.removeFromLeft(20).removeFromTop(14).translated(4, 2),
                   juce::Justification::centredLeft);
        g.drawText("R", bottomBounds.removeFromLeft(20).removeFromTop(14).translated(4, 2),
                   juce::Justification::centredLeft);
    }
    else
    {
        drawChannelGrid(g, bounds);
    }
}

void SoftwareGridRenderer::drawChannelGrid(juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto& theme = themeService_.getCurrentTheme();
    int height = area.getHeight();
    float centerY = area.getCentreY();

    // Minor horizontal lines (8 divisions)
    g.setColour(theme.gridMinor);
    const int numMinorLines = 8;
    for (int i = 1; i < numMinorLines; ++i)
    {
        float y = area.getY() + (static_cast<float>(i) / numMinorLines) * static_cast<float>(height);
        if (std::abs(y - centerY) > 1.0f)
            g.drawHorizontalLine(static_cast<int>(y), static_cast<float>(area.getX()),
                                 static_cast<float>(area.getRight()));
    }

    // Major horizontal lines (top/bottom quarters)
    g.setColour(theme.gridMajor);
    g.drawHorizontalLine(static_cast<int>(area.getY() + height * 0.25f), static_cast<float>(area.getX()),
                         static_cast<float>(area.getRight()));
    g.drawHorizontalLine(static_cast<int>(area.getY() + height * 0.75f), static_cast<float>(area.getX()),
                         static_cast<float>(area.getRight()));

    // Zero line
    g.setColour(theme.gridZeroLine);
    g.drawHorizontalLine(static_cast<int>(centerY), static_cast<float>(area.getX()),
                         static_cast<float>(area.getRight()));

    // Vertical lines
    if (gridConfig_.timingMode == TimingMode::TIME)
        drawTimeVerticalLines(g, area);
    else
        drawMelodicVerticalLines(g, area);
}

void SoftwareGridRenderer::drawTimeVerticalLines(juce::Graphics& g, juce::Rectangle<int> area) const
{
    const auto& theme = themeService_.getCurrentTheme();
    int width = area.getWidth();

    float durationMs = gridConfig_.visibleDurationMs;
    if (durationMs <= 0.0001f)
        durationMs = 1.0f;

    float targetStep = durationMs / 8.0f;
    if (targetStep <= 0.0f)
        targetStep = 1.0f;
    float magnitude = std::pow(10.0f, std::floor(std::log10(targetStep)));
    if (magnitude <= 0.0f || !std::isfinite(magnitude))
        magnitude = 1.0f;
    float normalizedStep = targetStep / magnitude;

    float stepSize;
    if (normalizedStep < 2.0f)
        stepSize = 1.0f * magnitude;
    else if (normalizedStep < 5.0f)
        stepSize = 2.0f * magnitude;
    else
        stepSize = 5.0f * magnitude;

    g.setColour(theme.gridMajor.withAlpha(0.5f));

    for (float t = stepSize; t < durationMs; t += stepSize)
    {
        float x = area.getX() + (t / durationMs) * static_cast<float>(width);
        g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
    }
}

void SoftwareGridRenderer::drawMelodicVerticalLines(juce::Graphics& g, juce::Rectangle<int> area) const
{
    const auto& theme = themeService_.getCurrentTheme();
    int width = area.getWidth();

    auto div = resolveGridDivisions(gridConfig_.noteInterval, gridConfig_.timeSigNumerator);
    float widthPerDiv = static_cast<float>(width) / static_cast<float>(div.count);

    // Major division lines
    for (int i = 1; i < div.count; ++i)
    {
        float x = area.getX() + i * widthPerDiv;
        g.setColour(div.barBased ? theme.gridMajor : theme.gridMajor.withAlpha(0.6f));
        g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
    }

    // Sub-beat lines within each bar for multi-bar modes
    if (div.barBased && gridConfig_.noteInterval >= NoteInterval::TWO_BARS && widthPerDiv > 40.0f)
    {
        int subBeatsPerDiv = std::max(1, gridConfig_.timeSigNumerator);
        float subBeatWidth = widthPerDiv / static_cast<float>(subBeatsPerDiv);
        g.setColour(theme.gridMinor.withAlpha(0.3f));

        for (int i = 0; i < div.count; ++i)
        {
            float baseX = area.getX() + i * widthPerDiv;
            for (int j = 1; j < subBeatsPerDiv; ++j)
            {
                float x = baseX + j * subBeatWidth;
                g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()),
                                   static_cast<float>(area.getBottom()));
            }
        }
    }
}

juce::String SoftwareGridRenderer::formatTimeLabel(float ms)
{
    if (ms >= 1000.0f)
    {
        float seconds = ms / 1000.0f;
        if (seconds >= 10.0f)
            return juce::String(static_cast<int>(seconds)) + "s";
        else
            return juce::String(seconds, 1) + "s";
    }
    else if (ms >= 1.0f)
    {
        if (ms >= 100.0f)
            return juce::String(static_cast<int>(ms)) + "ms";
        else if (ms >= 10.0f)
            return juce::String(ms, 0) + "ms";
        else
            return juce::String(ms, 1) + "ms";
    }
    else
    {
        return juce::String(ms * 1000.0f, 0) + "us";
    }
}

float SoftwareGridRenderer::calculateGridStepSize(float totalDuration, int targetDivisions)
{
    if (totalDuration <= 0.0f || targetDivisions <= 0)
        return totalDuration;

    float targetStep = totalDuration / static_cast<float>(targetDivisions);

    float magnitude = std::pow(10.0f, std::floor(std::log10(targetStep)));
    if (magnitude <= 0.0f || !std::isfinite(magnitude))
        return targetStep;

    float normalizedStep = targetStep / magnitude;

    float niceStep;
    if (normalizedStep < 1.5f)
        niceStep = 1.0f * magnitude;
    else if (normalizedStep < 3.5f)
        niceStep = 2.0f * magnitude;
    else if (normalizedStep < 7.5f)
        niceStep = 5.0f * magnitude;
    else
        niceStep = 10.0f * magnitude;

    return niceStep;
}

void SoftwareGridRenderer::renderLabels(juce::Graphics& g, juce::Rectangle<int> bounds, ProcessingMode processingMode)
{
    const auto& theme = themeService_.getCurrentTheme();

    g.setFont(juce::FontOptions(9.0f));
    g.setColour(theme.textSecondary.withAlpha(0.7f));

    int height = bounds.getHeight();
    bool isStereo = (processingMode == ProcessingMode::FullStereo);
    const int leftMargin = 22;

    // Time/beat axis labels
    if (gridConfig_.timingMode == TimingMode::TIME)
        drawTimeAxisLabels(g, bounds, leftMargin);
    else
        drawMelodicAxisLabels(g, bounds, leftMargin);

    // Amplitude axis labels
    g.setColour(theme.textSecondary.withAlpha(0.6f));

    if (isStereo)
    {
        int halfHeight = height / 2;
        auto topBounds = bounds.withHeight(halfHeight);
        auto bottomBounds = bounds.withTrimmedTop(halfHeight);

        drawAmplitudeLabels(g, topBounds, true, "L");
        drawAmplitudeLabels(g, bottomBounds, true, "R");
    }
    else
    {
        drawAmplitudeLabels(g, bounds, false, "");
    }
}

void SoftwareGridRenderer::drawTimeAxisLabels(juce::Graphics& g, juce::Rectangle<int> bounds, int leftMargin) const
{
    int width = bounds.getWidth();
    float durationMs = gridConfig_.visibleDurationMs;
    if (durationMs <= 0.0001f)
        durationMs = 500.0f;

    float stepSize = calculateGridStepSize(durationMs, 6);

    g.drawText("0", bounds.getX() + leftMargin, bounds.getBottom() - 14, 30, 12, juce::Justification::centredLeft);

    for (float t = stepSize; t < durationMs - stepSize * 0.1f; t += stepSize)
    {
        float xRatio = t / durationMs;
        int x = bounds.getX() + static_cast<int>(xRatio * static_cast<float>(width));

        juce::String label = formatTimeLabel(t);
        int labelWidth = 40;
        g.drawText(label, x - labelWidth / 2, bounds.getBottom() - 14, labelWidth, 12, juce::Justification::centred);
    }

    juce::String totalLabel = formatTimeLabel(durationMs);
    int totalLabelWidth = 50;
    g.drawText(totalLabel, bounds.getRight() - totalLabelWidth - 2, bounds.getBottom() - 14, totalLabelWidth, 12,
               juce::Justification::centredRight);
}

void SoftwareGridRenderer::drawMelodicAxisLabels(juce::Graphics& g, juce::Rectangle<int> bounds, int leftMargin) const
{
    int width = bounds.getWidth();
    auto div = resolveGridDivisions(gridConfig_.noteInterval, gridConfig_.timeSigNumerator);
    float widthPerDiv = static_cast<float>(width) / static_cast<float>(div.count);

    if (widthPerDiv >= 25.0f)
    {
        for (int i = 0; i < div.count; ++i)
        {
            int x = bounds.getX() + static_cast<int>(i * widthPerDiv);
            juce::String label = juce::String(i + 1);

            int labelWidth = 20;
            int xPos = (i == 0) ? x + leftMargin : x - labelWidth / 2;
            auto justify = (i == 0) ? juce::Justification::centredLeft : juce::Justification::centred;

            g.drawText(label, xPos, bounds.getBottom() - 14, labelWidth, 12, justify);
        }
    }
}

void SoftwareGridRenderer::drawAmplitudeLabels(juce::Graphics& g, juce::Rectangle<int> area, bool showChannelLabel,
                                               const juce::String& channelLabel)
{
    int centerY = area.getCentreY();

    g.drawText("+1", area.getX() + 2, area.getY() + 2, 20, 12, juce::Justification::centredLeft);
    g.drawText("0", area.getX() + 2, centerY - 6, 20, 12, juce::Justification::centredLeft);
    g.drawText("-1", area.getX() + 2, area.getBottom() - 26, 20, 12, juce::Justification::centredLeft);

    if (showChannelLabel && !channelLabel.isEmpty())
    {
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        g.drawText(channelLabel, area.getRight() - 18, area.getY() + 2, 14, 12, juce::Justification::centredRight);
        g.setFont(juce::FontOptions(9.0f));
    }
}

} // namespace oscil
