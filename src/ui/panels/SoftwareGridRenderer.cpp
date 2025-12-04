/*
    Oscil - Software Grid Renderer Implementation
    CPU-based grid rendering extracted from WaveformComponent
*/

#include "ui/panels/SoftwareGridRenderer.h"
#include "ui/theme/ThemeManager.h"
#include <cmath>

namespace oscil
{

SoftwareGridRenderer::SoftwareGridRenderer(IThemeService& themeService)
    : themeService_(themeService)
{
}

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

void SoftwareGridRenderer::render(juce::Graphics& g,
                                  juce::Rectangle<int> bounds,
                                  ProcessingMode processingMode)
{
    if (!showGrid_)
        return;

    const auto& theme = themeService_.getCurrentTheme();

    if (processingMode == ProcessingMode::FullStereo)
    {
        // Stereo: Two separate grids
        int halfHeight = bounds.getHeight() / 2;

        // Left Channel (Top)
        auto topBounds = bounds.removeFromTop(halfHeight);
        drawChannelGrid(g, topBounds);

        // Right Channel (Bottom)
        auto bottomBounds = bounds; // Remaining
        drawChannelGrid(g, bottomBounds);

        // Separator
        g.setColour(theme.gridMajor);
        g.drawHorizontalLine(topBounds.getBottom(), static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));

        // Labels
        g.setColour(theme.textSecondary.withAlpha(0.6f));
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        g.drawText("L", topBounds.removeFromLeft(20).removeFromTop(14).translated(4, 2), juce::Justification::centredLeft);
        g.drawText("R", bottomBounds.removeFromLeft(20).removeFromTop(14).translated(4, 2), juce::Justification::centredLeft);
    }
    else
    {
        // Mono: Single grid
        drawChannelGrid(g, bounds);
    }
}

void SoftwareGridRenderer::drawChannelGrid(juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto& theme = themeService_.getCurrentTheme();

    int width = area.getWidth();
    int height = area.getHeight();
    float centerY = area.getCentreY();

    // --- Horizontal Lines ---

    // Minor horizontal lines (8 divisions)
    g.setColour(theme.gridMinor);
    const int numMinorLines = 8;
    for (int i = 1; i < numMinorLines; ++i)
    {
        float y = area.getY() + (static_cast<float>(i) / numMinorLines) * static_cast<float>(height);
        if (std::abs(y - centerY) > 1.0f) // Don't draw over zero line
            g.drawHorizontalLine(static_cast<int>(y), static_cast<float>(area.getX()), static_cast<float>(area.getRight()));
    }

    // Major horizontal lines (top/bottom quarters)
    g.setColour(theme.gridMajor);
    g.drawHorizontalLine(static_cast<int>(area.getY() + height * 0.25f), static_cast<float>(area.getX()), static_cast<float>(area.getRight()));
    g.drawHorizontalLine(static_cast<int>(area.getY() + height * 0.75f), static_cast<float>(area.getX()), static_cast<float>(area.getRight()));

    // Zero line
    g.setColour(theme.gridZeroLine);
    g.drawHorizontalLine(static_cast<int>(centerY), static_cast<float>(area.getX()), static_cast<float>(area.getRight()));

    // --- Vertical Lines ---

    if (gridConfig_.timingMode == TimingMode::TIME)
    {
        // Time-based grid
        float durationMs = gridConfig_.visibleDurationMs;
        if (durationMs <= 0.0001f) durationMs = 1.0f;

        float targetStep = durationMs / 8.0f;
        float magnitude = std::pow(10.0f, std::floor(std::log10(targetStep)));
        float normalizedStep = targetStep / magnitude;

        float stepSize;
        if (normalizedStep < 2.0f) stepSize = 1.0f * magnitude;
        else if (normalizedStep < 5.0f) stepSize = 2.0f * magnitude;
        else stepSize = 5.0f * magnitude;

        g.setColour(theme.gridMajor.withAlpha(0.5f));

        for (float t = stepSize; t < durationMs; t += stepSize)
        {
            float x = area.getX() + (t / durationMs) * static_cast<float>(width);
            g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
        }
    }
    else // MELODIC Mode - aligned with labels
    {
        int numDivisions = 4;
        bool isBarBased = false;
        int beatsPerBar = gridConfig_.timeSigNumerator;

        switch (gridConfig_.noteInterval)
        {
            // Single note intervals - show 4 subdivisions
            case NoteInterval::THIRTY_SECOND:
            case NoteInterval::SIXTEENTH:
            case NoteInterval::TWELFTH:
            case NoteInterval::EIGHTH:
            case NoteInterval::TRIPLET_EIGHTH:
            case NoteInterval::DOTTED_EIGHTH:
            case NoteInterval::QUARTER:
            case NoteInterval::TRIPLET_QUARTER:
            case NoteInterval::DOTTED_QUARTER:
            case NoteInterval::HALF:
            case NoteInterval::TRIPLET_HALF:
            case NoteInterval::DOTTED_HALF:
                numDivisions = 4;
                break;
            // Bar-based intervals
            case NoteInterval::WHOLE:
                numDivisions = beatsPerBar;
                isBarBased = true;
                break;
            case NoteInterval::TWO_BARS:
                numDivisions = 2;
                isBarBased = true;
                break;
            case NoteInterval::THREE_BARS:
                numDivisions = 3;
                isBarBased = true;
                break;
            case NoteInterval::FOUR_BARS:
                numDivisions = 4;
                isBarBased = true;
                break;
            case NoteInterval::EIGHT_BARS:
                numDivisions = 8;
                isBarBased = true;
                break;
            default:
                numDivisions = 4;
                break;
        }

        float widthPerDiv = static_cast<float>(width) / static_cast<float>(numDivisions);

        // Draw major division lines
        for (int i = 1; i < numDivisions; ++i)
        {
            float x = area.getX() + i * widthPerDiv;

            if (isBarBased)
                g.setColour(theme.gridMajor);
            else
                g.setColour(theme.gridMajor.withAlpha(0.6f));

            g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
        }

        // For bar-based modes, add sub-beat lines within each bar
        if (isBarBased && gridConfig_.noteInterval >= NoteInterval::TWO_BARS && widthPerDiv > 40.0f)
        {
            int subBeatsPerDiv = beatsPerBar;
            float subBeatWidth = widthPerDiv / static_cast<float>(subBeatsPerDiv);
            g.setColour(theme.gridMinor.withAlpha(0.3f));

            for (int i = 0; i < numDivisions; ++i)
            {
                float baseX = area.getX() + i * widthPerDiv;
                for (int j = 1; j < subBeatsPerDiv; ++j)
                {
                    float x = baseX + j * subBeatWidth;
                    g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()), static_cast<float>(area.getBottom()));
                }
            }
        }
    }
}

juce::String SoftwareGridRenderer::formatTimeLabel(float ms)
{
    if (ms >= 1000.0f)
    {
        // Show in seconds for values >= 1s
        float seconds = ms / 1000.0f;
        if (seconds >= 10.0f)
            return juce::String(static_cast<int>(seconds)) + "s";
        else
            return juce::String(seconds, 1) + "s";
    }
    else if (ms >= 1.0f)
    {
        // Show in ms for values >= 1ms
        if (ms >= 100.0f)
            return juce::String(static_cast<int>(ms)) + "ms";
        else if (ms >= 10.0f)
            return juce::String(ms, 0) + "ms";
        else
            return juce::String(ms, 1) + "ms";
    }
    else
    {
        // Show in microseconds for very small values
        return juce::String(ms * 1000.0f, 0) + "us";
    }
}

float SoftwareGridRenderer::calculateGridStepSize(float totalDuration, int targetDivisions)
{
    if (totalDuration <= 0.0f || targetDivisions <= 0)
        return totalDuration;

    float targetStep = totalDuration / static_cast<float>(targetDivisions);

    // Find the order of magnitude
    float magnitude = std::pow(10.0f, std::floor(std::log10(targetStep)));
    if (magnitude <= 0.0f || !std::isfinite(magnitude))
        return targetStep;

    float normalizedStep = targetStep / magnitude;

    // Snap to nice values: 1, 2, 5, 10
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

void SoftwareGridRenderer::renderLabels(juce::Graphics& g,
                                       juce::Rectangle<int> bounds,
                                       ProcessingMode processingMode)
{
    const auto& theme = themeService_.getCurrentTheme();

    // Use a small, readable font for labels
    g.setFont(juce::FontOptions(9.0f));
    g.setColour(theme.textSecondary.withAlpha(0.7f));

    int width = bounds.getWidth();
    int height = bounds.getHeight();

    bool isStereo = (processingMode == ProcessingMode::FullStereo);

    // Reserve space for amplitude labels on the left (avoid overlap)
    const int leftMargin = 22;

    // === TIME AXIS LABELS (bottom of waveform area) ===
    if (gridConfig_.timingMode == TimingMode::TIME)
    {
        float durationMs = gridConfig_.visibleDurationMs;
        if (durationMs <= 0.0001f)
            durationMs = 500.0f; // Default fallback

        // Calculate nice step size for ~6-8 labels
        float stepSize = calculateGridStepSize(durationMs, 6);

        // Draw "0" at the left edge (offset to avoid amplitude label overlap)
        g.drawText("0", bounds.getX() + leftMargin, bounds.getBottom() - 14, 30, 12,
                   juce::Justification::centredLeft);

        // Draw time labels at each step
        for (float t = stepSize; t < durationMs - stepSize * 0.1f; t += stepSize)
        {
            float xRatio = t / durationMs;
            int x = bounds.getX() + static_cast<int>(xRatio * static_cast<float>(width));

            juce::String label = formatTimeLabel(t);
            int labelWidth = 40;

            // Center the label on the grid line
            g.drawText(label, x - labelWidth / 2, bounds.getBottom() - 14, labelWidth, 12,
                       juce::Justification::centred);
        }

        // Draw total duration at right edge
        juce::String totalLabel = formatTimeLabel(durationMs);
        int totalLabelWidth = 50;
        g.drawText(totalLabel, bounds.getRight() - totalLabelWidth - 2, bounds.getBottom() - 14,
                   totalLabelWidth, 12, juce::Justification::centredRight);
    }
    else // MELODIC mode
    {
        // Determine the number of major divisions and what they represent
        int numDivisions = 4;  // Number of major grid divisions
        bool isBarBased = false;
        int beatsPerBar = gridConfig_.timeSigNumerator;

        switch (gridConfig_.noteInterval)
        {
            // Single note intervals - show subdivisions within the note
            case NoteInterval::THIRTY_SECOND:
            case NoteInterval::SIXTEENTH:
            case NoteInterval::TWELFTH:
                numDivisions = 4; // Show 4 subdivisions within the note
                break;
            case NoteInterval::EIGHTH:
            case NoteInterval::TRIPLET_EIGHTH:
            case NoteInterval::DOTTED_EIGHTH:
                numDivisions = 4;
                break;
            case NoteInterval::QUARTER:
            case NoteInterval::TRIPLET_QUARTER:
            case NoteInterval::DOTTED_QUARTER:
                numDivisions = 4; // 4 subdivisions of a quarter note (16ths)
                break;
            case NoteInterval::HALF:
            case NoteInterval::TRIPLET_HALF:
            case NoteInterval::DOTTED_HALF:
                numDivisions = 4; // 4 subdivisions of a half note (8ths)
                break;
            // Bar-based intervals - show beats or bars
            case NoteInterval::WHOLE:
                numDivisions = beatsPerBar; // Show beat divisions within the bar
                isBarBased = true;
                break;
            case NoteInterval::TWO_BARS:
                numDivisions = 2; // 2 bars = 2 divisions
                isBarBased = true;
                break;
            case NoteInterval::THREE_BARS:
                numDivisions = 3;
                isBarBased = true;
                break;
            case NoteInterval::FOUR_BARS:
                numDivisions = 4;
                isBarBased = true;
                break;
            case NoteInterval::EIGHT_BARS:
                numDivisions = 8;
                isBarBased = true;
                break;
            default:
                numDivisions = 4;
                break;
        }

        float widthPerDiv = static_cast<float>(width) / static_cast<float>(numDivisions);

        // Draw division labels if there's enough space
        if (widthPerDiv >= 25.0f)
        {
            for (int i = 0; i < numDivisions; ++i)
            {
                int x = bounds.getX() + static_cast<int>(i * widthPerDiv);

                juce::String label;
                if (isBarBased)
                {
                    if (gridConfig_.noteInterval == NoteInterval::WHOLE)
                    {
                        // For 1 bar, show beat numbers (1, 2, 3, 4)
                        label = juce::String(i + 1);
                    }
                    else
                    {
                        // For multi-bar, show bar numbers
                        label = juce::String(i + 1);
                    }
                }
                else
                {
                    // For sub-beat intervals, show fraction positions
                    // e.g., for quarter note with 4 divisions: 1, 2, 3, 4
                    label = juce::String(i + 1);
                }

                int labelWidth = 20;
                int xPos = (i == 0) ? x + leftMargin : x - labelWidth / 2;
                auto justify = (i == 0) ? juce::Justification::centredLeft : juce::Justification::centred;

                g.drawText(label, xPos, bounds.getBottom() - 14, labelWidth, 12, justify);
            }
        }

        // Note: BPM and note interval indicators removed per user request
        // Only X-axis (time/beat) and Y-axis (amplitude) labels are shown
    }

    // === AMPLITUDE AXIS LABELS (left side) ===
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

void SoftwareGridRenderer::drawAmplitudeLabels(juce::Graphics& g,
                                              juce::Rectangle<int> area,
                                              bool showChannelLabel,
                                              const juce::String& channelLabel)
{
    const auto& theme = themeService_.getCurrentTheme();
    int centerY = area.getCentreY();

    // Draw +1 at top
    g.drawText("+1", area.getX() + 2, area.getY() + 2, 20, 12, juce::Justification::centredLeft);

    // Draw 0 at center
    g.drawText("0", area.getX() + 2, centerY - 6, 20, 12, juce::Justification::centredLeft);

    // Draw -1 at bottom (offset up to avoid overlap with time axis labels)
    g.drawText("-1", area.getX() + 2, area.getBottom() - 26, 20, 12, juce::Justification::centredLeft);

    // Draw channel label if stereo
    if (showChannelLabel && !channelLabel.isEmpty())
    {
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        g.drawText(channelLabel, area.getRight() - 18, area.getY() + 2, 14, 12,
                   juce::Justification::centredRight);
        g.setFont(juce::FontOptions(9.0f)); // Reset font
    }
}

} // namespace oscil
