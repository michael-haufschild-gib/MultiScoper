/*
    MultiScoper - Stats Overlay Implementation
*/

#include "ui/layout/pane/overlays/StatsOverlay.h"

#include "ui/components/ListItemIcons.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Typography.h"

#include <utility>

namespace multiscoper
{

StatsOverlay::StatsOverlay(IThemeService& themeService) : PaneOverlay(themeService, "statsOverlay")
{
    // Route testId registration through PaneOverlay's testId-aware
    // constructor so the harness can observe overlay visibility without
    // relying on a hit-test on child elements.  Previously `setTestId`
    // stored the id but never invoked the registration hook, leaving the
    // overlay invisible to the test harness element registry.
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
    // 12pt mono — tighter than canonical 15pt readout; layout-tuned for stats table density.
    statsDisplay_->setFont(Typography::readout().withHeight(12.0f));
    // Seed the palette from the current theme; onThemeChanged keeps it in sync afterwards.
    // Safe even though onThemeChanged is virtual: StatsOverlay has no subclass that overrides
    // it, and the call runs after statsDisplay_ is constructed so the theme write targets a
    // valid object. NOLINT pacifies clang-analyzer's chain trace from the constructor.
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.VirtualCall)
    onThemeChanged(getThemeService().getCurrentTheme());
    addChildComponent(*statsDisplay_); // Hidden until visible

    // Reset Button
    resetButton_ = std::make_unique<MultiScoperButton>(getThemeService(), "", "statsOverlay_resetBtn");
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

    // Keep the text colour in sync with the live fade opacity. The base text
    // colour (hue) is already applied in onThemeChanged; here we only rewrite
    // the alpha so fade animations look correct. A no-op when the overlay is
    // fully hidden.
    float const opacity = getCurrentOpacity();
    if (opacity <= 0.0f || statsDisplay_ == nullptr)
        return;

    statsDisplay_->setColour(juce::TextEditor::textColourId,
                             getThemeService().getCurrentTheme().textPrimary.withAlpha(opacity));
}

void StatsOverlay::onThemeChanged(const ColorTheme& newTheme)
{
    // Update the TextEditor's palette on actual theme changes rather than on
    // every paint tick. Background/outline are always transparent — the
    // overlay's own backdrop paints them — but textColourId picks up the
    // palette's primary text colour.
    if (statsDisplay_ == nullptr)
        return;
    statsDisplay_->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    statsDisplay_->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    statsDisplay_->setColour(juce::TextEditor::textColourId, newTheme.textPrimary);
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
    int const width = LABEL_COLUMN_WIDTH + (numOscillators_ * DATA_COLUMN_WIDTH) + (PADDING * 2);
    int const height = HEADER_HEIGHT + (ROW_HEIGHT * TOTAL_TABLE_ROWS) + PADDING;
    return {0, 0, width, height};
}

void StatsOverlay::updateStats(const std::vector<OscillatorStats>& stats)
{
    // Check if layout needs update (oscillator count changed → overlay size changed)
    if (std::cmp_not_equal(stats.size(), numOscillators_))
    {
        numOscillators_ = static_cast<int>(stats.size());
        if (getParentComponent())
            getParentComponent()->resized();
    }

    if (statsDisplay_ == nullptr)
        return;

    // Rebuild table content
    juce::String const tableText = formatTable(stats);
    juce::String const oldText = statsDisplay_->getText();

    // Identical text: skip the setText entirely to avoid caret/selection churn.
    if (oldText == tableText)
        return;

    // Content changed. juce::TextEditor::setText wipes selection + caret + scroll
    // unconditionally, which at a 15 Hz update rate makes drag-selecting any metric
    // for copy/paste impossible. Snapshot the selection around the setText call and
    // restore it when the structure (row/column count) is stable — approximated by
    // text length equality, which holds for cell-value swaps within a fixed grid.
    auto const oldSelection = statsDisplay_->getHighlightedRegion();
    int const oldCaret = statsDisplay_->getCaretPosition();
    bool const structureStable = oldText.length() == tableText.length();

    statsDisplay_->setText(tableText, /*sendTextChangeMessage=*/false);

    if (structureStable)
    {
        if (!oldSelection.isEmpty())
            statsDisplay_->setHighlightedRegion(oldSelection);
        else
            statsDisplay_->setCaretPosition(oldCaret);
    }
}

juce::String StatsOverlay::formatTable(const std::vector<OscillatorStats>& stats)
{
    juce::String s = juce::String("").paddedRight(' ', LABEL_CHAR_WIDTH);
    for (const auto& osc : stats)
        s << " " << osc.name.substring(0, DATA_CHAR_WIDTH - 1).paddedRight(' ', DATA_CHAR_WIDTH);
    s << "\n";

    auto appendRow = [&](const juce::String& label, auto valueGetter) {
        s << label.paddedRight(' ', LABEL_CHAR_WIDTH);
        for (const auto& osc : stats)
            s << " " << valueGetter(osc).paddedRight(' ', DATA_CHAR_WIDTH);
        s << "\n";
    };

    auto maxLR = [](const OscillatorStats& o, float MetricSnapshot::* f) { return std::max(o.left.*f, o.right.*f); };

    appendRow("Peak", [&](const OscillatorStats& o) { return formatDb(maxLR(o, &MetricSnapshot::peakDb)); });
    appendRow("Max Pk", [&](const OscillatorStats& o) { return formatDb(maxLR(o, &MetricSnapshot::maxPeakDb)); });
    appendRow("RMS", [&](const OscillatorStats& o) { return formatDb(maxLR(o, &MetricSnapshot::rmsDb)); });
    appendRow("Crest", [&](const OscillatorStats& o) { return formatDb(maxLR(o, &MetricSnapshot::crestFactorDb)); });
    appendRow("DC", [&](const OscillatorStats& o) {
        float const dc = std::max(std::abs(o.left.dcOffset), std::abs(o.right.dcOffset));
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

} // namespace multiscoper
