/*
    Oscil - Stats Overlay Implementation
*/

#include "ui/layout/pane/overlays/StatsOverlay.h"

#include "ui/components/ListItemIcons.h"
#include "ui/theme/ThemeManager.h"

namespace oscil
{

StatsOverlay::StatsOverlay(IThemeService& themeService) : PaneOverlay(themeService)
{
    setTestId("statsOverlay");
    setupComponents();
}

StatsOverlay::StatsOverlay(IThemeService& themeService, const juce::String& testId) : PaneOverlay(themeService, testId)
{
    setupComponents();
}

void StatsOverlay::setupComponents()
{
    setPosition(Position::TopRight);
    setClickThrough(false);
    setInterceptsMouseClicks(true, true);

    // Text Editor for stats table (read-only, selectable)
    statsDisplay_ = std::make_unique<juce::TextEditor>("statsDisplay");
    statsDisplay_->setMultiLine(true);
    statsDisplay_->setReadOnly(true);
    statsDisplay_->setScrollbarsShown(false);
    statsDisplay_->setCaretVisible(false);
    statsDisplay_->setPopupMenuEnabled(true); // Allow right-click copy
    statsDisplay_->setJustification(juce::Justification::topLeft);
    statsDisplay_->setBorder(juce::BorderSize<int>(0));
    statsDisplay_->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    statsDisplay_->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    statsDisplay_->setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    addChildComponent(*statsDisplay_); // Hidden until visible

    // Reset Button
    resetButton_ = std::make_unique<OscilButton>(themeService_, "", "statsOverlay_resetBtn");
    resetButton_->setVariant(ButtonVariant::Icon);
    resetButton_->setIconPath(ListItemIcons::createRedoIcon(static_cast<float>(RESET_BUTTON_SIZE)));
    resetButton_->setTooltip("Reset Accumulated Stats");
    resetButton_->onClick = [this]() {
        if (onResetStats)
            onResetStats();
    };
    addChildComponent(*resetButton_);
}

void StatsOverlay::resized()
{
    auto bounds = getContentBounds();

    // Reset button at top-right
    auto headerRow = bounds.removeFromTop(HEADER_HEIGHT);
    resetButton_->setBounds(headerRow.removeFromRight(RESET_BUTTON_SIZE + 4).withWidth(RESET_BUTTON_SIZE));

    // Stats display fills the rest
    statsDisplay_->setBounds(bounds);
}

void StatsOverlay::paint(juce::Graphics& g)
{
    PaneOverlay::paint(g);

    float opacity = getCurrentOpacity();
    if (opacity <= 0.0f)
        return;

    const auto& theme = themeService_.getCurrentTheme();

    if (statsDisplay_)
        statsDisplay_->setColour(juce::TextEditor::textColourId, theme.textPrimary.withAlpha(opacity));
}

void StatsOverlay::onAnimationVisibilityChanged(bool becameVisible)
{
    if (statsDisplay_)
        statsDisplay_->setVisible(becameVisible);
    if (resetButton_)
        resetButton_->setVisible(becameVisible);
}

juce::Rectangle<int> StatsOverlay::getPreferredContentSize() const
{
    int width = LABEL_COLUMN_WIDTH + (numOscillators_ * DATA_COLUMN_WIDTH) + PADDING * 2;
    int height = HEADER_HEIGHT + (ROW_HEIGHT * 8) + PADDING; // 1 column header + 7 metric rows
    return {0, 0, width, height};
}

void StatsOverlay::updateStats(const std::vector<OscillatorStats>& stats)
{
    // Check if layout needs update
    if (static_cast<int>(stats.size()) != numOscillators_)
    {
        numOscillators_ = static_cast<int>(stats.size());
        if (getParentComponent())
            updatePositionInParent(getParentComponent()->getLocalBounds().reduced(4)); // Re-layout
    }

    // Rebuild table content
    juce::String tableText = formatTable(stats);

    // Only update if changed to avoid caret/selection reset flicker
    if (statsDisplay_ && statsDisplay_->getText() != tableText)
    {
        statsDisplay_->setText(tableText);
    }
}

juce::String StatsOverlay::formatTable(const std::vector<OscillatorStats>& stats)
{
    static constexpr int colChars = 10;
    static constexpr int labelChars = 8;

    juce::String s = juce::String("").paddedRight(' ', labelChars);
    for (const auto& osc : stats)
        s << " " << osc.name.substring(0, colChars - 1).paddedRight(' ', colChars);
    s << "\n";

    auto appendRow = [&](const juce::String& label, auto valueGetter) {
        s << label.paddedRight(' ', labelChars);
        for (const auto& osc : stats)
            s << " " << valueGetter(osc).paddedRight(' ', colChars);
        s << "\n";
    };

    auto maxLR = [](const OscillatorStats& o, float MetricSnapshot::* f) { return std::max(o.left.*f, o.right.*f); };

    appendRow("Peak", [&](const OscillatorStats& o) { return formatDb(maxLR(o, &MetricSnapshot::peakDb)); });
    appendRow("Max Pk", [&](const OscillatorStats& o) { return formatDb(maxLR(o, &MetricSnapshot::maxPeakDb)); });
    appendRow("RMS", [&](const OscillatorStats& o) { return formatDb(maxLR(o, &MetricSnapshot::rmsDb)); });
    appendRow("Crest", [&](const OscillatorStats& o) { return formatDb(maxLR(o, &MetricSnapshot::crestFactorDb)); });
    appendRow("DC", [&](const OscillatorStats& o) {
        float dc = std::max(std::abs(o.left.dcOffset), std::abs(o.right.dcOffset));
        return dc > 0.001f ? formatPercent(dc) : juce::String("-");
    });
    appendRow("Attack", [&](const OscillatorStats& o) { return formatMs(maxLR(o, &MetricSnapshot::attackTimeMs)); });
    appendRow("Decay", [&](const OscillatorStats& o) { return formatMs(maxLR(o, &MetricSnapshot::decayTimeMs)); });

    return s;
}

juce::String StatsOverlay::formatDb(float dB)
{
    if (dB < -90.0f)
        return "-inf";
    return juce::String(dB, 1) + " dB";
}

juce::String StatsOverlay::formatPercent(float value) { return juce::String(value * 100.0f, 1) + "%"; }

juce::String StatsOverlay::formatMs(float ms)
{
    if (ms < 0.1f)
        return "-";
    return juce::String(ms, 0) + " ms";
}

juce::String StatsOverlay::getDisplayedText() const
{
    return statsDisplay_ ? statsDisplay_->getText() : juce::String();
}

} // namespace oscil
