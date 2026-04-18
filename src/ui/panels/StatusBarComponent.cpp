/*
    Oscil - Status Bar Component Implementation
*/

#include "ui/panels/StatusBarComponent.h"

#include "ui/components/ComponentConstants.h"
#include "ui/components/TestId.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Typography.h"

namespace oscil
{

namespace
{
// Outer horizontal padding matches the previous single-zone layout so that the
// right-anchored metrics visually align with the original position.
constexpr int kOuterPaddingPx = 10;

// Spec: 8px between right-zone items for a consistent rhythm.
constexpr int kItemSpacingPx = 8;

// Spec: 12px horizontal padding on each side of the 1px separator.
constexpr int kSeparatorPaddingPx = 12;
constexpr int kSeparatorWidthPx = 1;
constexpr int kSeparatorRegionPx = kSeparatorPaddingPx * 2 + kSeparatorWidthPx;

// Preserved per-label widths from the previous layout.
constexpr int kFpsWidthPx = 70;
constexpr int kCpuWidthPx = 80;
constexpr int kMemWidthPx = 90;
constexpr int kOscWidthPx = 60;
constexpr int kSrcWidthPx = 60;
constexpr int kModeWidthPx = 80;
} // namespace

StatusBarComponent::StatusBarComponent(IThemeService& themeService)
    : themeService_(themeService)
    , renderingMode_(detectRenderingMode())
{
    // Opaque because the hint text is painted directly and a background fill
    // is required for correct text rendering.
    setOpaque(true);

    auto createLabel = [this](std::unique_ptr<juce::Label>& label, [[maybe_unused]] const juce::String& testId,
                              juce::Justification justification) {
        label = std::make_unique<juce::Label>();
        label->setFont(Typography::caption());
        label->setJustificationType(justification);
        addAndMakeVisible(*label);
        OSCIL_REGISTER_CHILD_TEST_ID(*label, testId);
    };

    // Metrics labels are right-aligned so their text hugs the separator side;
    // mode label stays right-aligned at the far edge.
    createLabel(fpsLabel_, "statusBar_fps", juce::Justification::centredRight);
    createLabel(cpuLabel_, "statusBar_cpu", juce::Justification::centredRight);
    createLabel(memoryLabel_, "statusBar_mem", juce::Justification::centredRight);
    createLabel(oscillatorLabel_, "statusBar_osc", juce::Justification::centredRight);
    createLabel(sourceLabel_, "statusBar_src", juce::Justification::centredRight);
    createLabel(renderModeLabel_, "statusBar_mode", juce::Justification::centredRight);

    updateFpsLabel();
    updateCpuLabel();
    updateMemoryLabel();
    updateOscillatorLabel();
    updateSourceLabel();
    updateRenderModeLabel();

#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_REGISTER_TEST_ID("statusBar");
#endif
}

RenderingMode StatusBarComponent::detectRenderingMode()
{
#if OSCIL_ENABLE_OPENGL
    return RenderingMode::OpenGL;
#else
    return RenderingMode::Software;
#endif
}

int StatusBarComponent::getRightZoneWidth() const
{
    // Six items separated by five inter-item gaps.
    return kFpsWidthPx + kCpuWidthPx + kMemWidthPx + kOscWidthPx + kSrcWidthPx + kModeWidthPx + (5 * kItemSpacingPx);
}

int StatusBarComponent::getLeftZoneWidth() const
{
    // Total width minus outer padding on both sides, separator region, and the
    // right-zone. Separator region is always reserved so that right-zone
    // position stays stable when the hint toggles on and off.
    const int reserved = (2 * kOuterPaddingPx) + kSeparatorRegionPx + getRightZoneWidth();
    return juce::jmax(0, getWidth() - reserved);
}

void StatusBarComponent::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const auto& theme = themeService_.getCurrentTheme();

    // Flat fill, no border, no rounded corners per spec.
    g.setColour(theme.backgroundSecondary);
    g.fillRect(bounds);

    if (hintText_.isEmpty())
        return;

    const int leftZoneWidth = getLeftZoneWidth();
    if (leftZoneWidth <= 0)
        return;

    // Hint text occupies the left zone, vertically centered. If the zone is
    // too narrow to fit even a bare ellipsis, getElidedHintText returns an
    // empty string — in that case skip the divider as well so the user never
    // sees a naked separator floating over nothing.
    const auto elided = getElidedHintText(static_cast<float>(leftZoneWidth));
    if (elided.isEmpty())
        return;

    g.setColour(theme.textSecondary);
    g.setFont(Typography::caption());

    const juce::Rectangle<int> hintArea(kOuterPaddingPx, 0, leftZoneWidth, getHeight());
    g.drawText(elided, hintArea, juce::Justification::centredLeft, false);

    // Separator drawn only when there is hint content to visually separate.
    const int separatorX = kOuterPaddingPx + leftZoneWidth + kSeparatorPaddingPx;
    g.setColour(theme.divider);
    g.fillRect(juce::Rectangle<int>(separatorX, 0, kSeparatorWidthPx, getHeight()));
}

void StatusBarComponent::resized()
{
    // Right-anchor the metrics group; left zone is handled at paint time so
    // that hint elision adapts to the exact pixel width without re-laying out
    // the metrics whenever the hint text changes.
    auto bounds = getLocalBounds().reduced(kOuterPaddingPx, 0);

    const auto h = bounds.getHeight();

    // Place labels from the far right inward so the order on screen reads
    // [fps][cpu][memory][osc][src][mode] left-to-right within the right zone.
    auto placeFromRight = [&bounds, h](juce::Label& label, int width, bool isLastOnRight) {
        if (!isLastOnRight)
            bounds.removeFromRight(kItemSpacingPx);
        auto area = bounds.removeFromRight(width);
        label.setBounds(area.withSizeKeepingCentre(width, h));
    };

    if (renderModeLabel_)
        placeFromRight(*renderModeLabel_, kModeWidthPx, true);
    if (sourceLabel_)
        placeFromRight(*sourceLabel_, kSrcWidthPx, false);
    if (oscillatorLabel_)
        placeFromRight(*oscillatorLabel_, kOscWidthPx, false);
    if (memoryLabel_)
        placeFromRight(*memoryLabel_, kMemWidthPx, false);
    if (cpuLabel_)
        placeFromRight(*cpuLabel_, kCpuWidthPx, false);
    if (fpsLabel_)
        placeFromRight(*fpsLabel_, kFpsWidthPx, false);
}

void StatusBarComponent::setFps(float fps)
{
    if (std::abs(currentFps_ - fps) > 0.1f)
    {
        currentFps_ = fps;
        updateFpsLabel();
    }
}

void StatusBarComponent::setCpuUsage(float cpu)
{
    if (std::abs(cpuUsage_ - cpu) > 0.1f)
    {
        cpuUsage_ = cpu;
        updateCpuLabel();
    }
}

void StatusBarComponent::setMemoryUsage(float memory)
{
    // Memory label is rendered with one decimal place, so update once the rounded
    // display value can change (half-step of 0.1 MB).
    if (std::abs(memoryUsage_ - memory) >= 0.05f)
    {
        memoryUsage_ = memory;
        updateMemoryLabel();
    }
}

void StatusBarComponent::setOscillatorCount(int count)
{
    if (oscillatorCount_ != count)
    {
        oscillatorCount_ = count;
        updateOscillatorLabel();
    }
}

void StatusBarComponent::setSourceCount(int count)
{
    if (sourceCount_ != count)
    {
        sourceCount_ = count;
        updateSourceLabel();
    }
}

void StatusBarComponent::setRenderingMode(RenderingMode mode)
{
    if (renderingMode_ != mode)
    {
        renderingMode_ = mode;
        updateRenderModeLabel();
    }
}

void StatusBarComponent::setHintText(const juce::String& text)
{
    if (hintText_ == text)
        return;
    hintText_ = text;
    repaint();
}

bool StatusBarComponent::shouldDrawSeparator() const
{
    // Divider visibility must track the actual painted text: if the left zone
    // is too narrow for even a bare ellipsis, getElidedHintText() returns
    // empty and paint() suppresses the divider. Mirror that rule here.
    const int leftZoneWidth = getLeftZoneWidth();
    if (leftZoneWidth <= 0)
        return false;
    return !getElidedHintText(static_cast<float>(leftZoneWidth)).isEmpty();
}

juce::String StatusBarComponent::getElidedHintText(float availableWidth) const
{
    if (hintText_.isEmpty() || availableWidth <= 0.0f)
        return {};

    const auto font = Typography::caption();
    if (juce::GlyphArrangement::getStringWidth(font, hintText_) <= availableWidth)
        return hintText_;

    // Trim from the end and append an ellipsis marker until it fits.
    // A space + three dots reads cleanly with proportional fonts; at very
    // narrow widths we fall back to a bare ellipsis so the paint path always
    // renders a visible hint instead of a naked separator.
    const juce::String suffix(" ...");
    juce::String ellipsis("...");
    const float suffixWidth = juce::GlyphArrangement::getStringWidth(font, suffix);
    const float ellipsisWidth = juce::GlyphArrangement::getStringWidth(font, ellipsis);

    if (ellipsisWidth > availableWidth)
        return {};

    const int n = hintText_.length();
    int lo = 0;
    int hi = n;
    // Binary search the largest prefix whose width plus the `" ..."` suffix
    // fits. If nothing fits with the padded suffix, fall through to the bare
    // ellipsis below — matching the width check we actually gate on.
    while (lo < hi)
    {
        const int mid = lo + ((hi - lo + 1) / 2);
        const float w = juce::GlyphArrangement::getStringWidth(font, hintText_.substring(0, mid)) + suffixWidth;
        if (w <= availableWidth)
            lo = mid;
        else
            hi = mid - 1;
    }

    if (lo <= 0)
        return ellipsis;
    return hintText_.substring(0, lo) + suffix;
}

void StatusBarComponent::updateFpsLabel()
{
    if (!fpsLabel_)
        return;

    const auto& theme = themeService_.getCurrentTheme();
    juce::Colour fpsColour = theme.statusActive;

    if (currentFps_ < 30.0f)
        fpsColour = theme.statusError;
    else if (currentFps_ < 55.0f)
        fpsColour = theme.statusWarning;

    fpsLabel_->setText(juce::String::formatted("%.1f FPS", currentFps_), juce::dontSendNotification);
    fpsLabel_->setColour(juce::Label::textColourId, fpsColour);
}

void StatusBarComponent::updateCpuLabel()
{
    if (!cpuLabel_)
        return;

    const auto& theme = themeService_.getCurrentTheme();
    cpuLabel_->setText(juce::String::formatted("CPU: %.1f%%", cpuUsage_), juce::dontSendNotification);
    cpuLabel_->setColour(juce::Label::textColourId, cpuUsage_ > 10.0f ? theme.statusWarning : theme.textSecondary);
}

void StatusBarComponent::updateMemoryLabel()
{
    if (!memoryLabel_)
        return;

    const auto& theme = themeService_.getCurrentTheme();
    memoryLabel_->setText(juce::String::formatted("Mem: %.1f MB", memoryUsage_), juce::dontSendNotification);
    memoryLabel_->setColour(juce::Label::textColourId, theme.textSecondary);
}

void StatusBarComponent::updateOscillatorLabel()
{
    if (!oscillatorLabel_)
        return;

    const auto& theme = themeService_.getCurrentTheme();
    oscillatorLabel_->setText(juce::String::formatted("Osc: %d", oscillatorCount_), juce::dontSendNotification);
    oscillatorLabel_->setColour(juce::Label::textColourId, theme.textSecondary);
}

void StatusBarComponent::updateSourceLabel()
{
    if (!sourceLabel_)
        return;

    const auto& theme = themeService_.getCurrentTheme();
    sourceLabel_->setText(juce::String::formatted("Src: %d", sourceCount_), juce::dontSendNotification);
    sourceLabel_->setColour(juce::Label::textColourId, theme.textSecondary);
}

void StatusBarComponent::updateRenderModeLabel()
{
    if (!renderModeLabel_)
        return;

    const auto& theme = themeService_.getCurrentTheme();
    juce::String const renderModeText = (renderingMode_ == RenderingMode::OpenGL) ? "OpenGL" : "Software";
    renderModeLabel_->setText(renderModeText, juce::dontSendNotification);
    renderModeLabel_->setColour(juce::Label::textColourId, theme.textSecondary);
}

} // namespace oscil
