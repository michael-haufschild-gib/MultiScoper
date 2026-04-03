/*
    Oscil - Pane Body Implementation
*/

#include "ui/layout/pane/PaneBody.h"

#include "core/OscilState.h"
#include "core/interfaces/IAudioDataProvider.h"
#include "core/interfaces/IInstanceRegistry.h"

#include "rendering/ShaderRegistry.h"

namespace oscil
{

PaneBody::PaneBody(IAudioDataProvider& dataProvider, IInstanceRegistry& instanceRegistry, IThemeService& themeService,
                   ShaderRegistry& shaderRegistry)
    : dataProvider_(dataProvider)
    , instanceRegistry_(instanceRegistry)
    , themeService_(themeService)
{
    setTestId("pane_body");

    // Create waveform stack
    waveformStack_ = std::make_unique<WaveformStack>(dataProvider, themeService, shaderRegistry);
    addAndMakeVisible(*waveformStack_);

    // Create crosshair overlay (always visible, follows mouse)
    crosshairOverlay_ = std::make_unique<CrosshairOverlay>(themeService);
    addChildComponent(*crosshairOverlay_);
    crosshairOverlay_->setVisible(false); // Hidden until mouse enters

    // Create stats overlay
    statsOverlay_ = std::make_unique<StatsOverlay>(themeService);
    statsOverlay_->setPosition(PaneOverlay::Position::TopRight);
    statsOverlay_->onResetStats = [this]() { resetStats(); };
    addChildComponent(*statsOverlay_);
    // Overlay starts invisible - use setVisibleAnimated(true) via setStatsVisible to show
}

PaneBody::~PaneBody() { stopTimer(); }

void PaneBody::paint(juce::Graphics& g)
{
#if OSCIL_ENABLE_OPENGL
    // In GPU mode, the OpenGL renderer handles the background.
    // Only fill if GPU rendering is disabled to avoid occluding the GL content.
    if (dataProvider_.getState().isGpuRenderingEnabled())
        return;
#endif

    const auto& theme = themeService_.getCurrentTheme();
    g.fillAll(theme.backgroundPrimary);
}

void PaneBody::resized()
{
    auto bounds = getLocalBounds().reduced(PADDING);

    // Waveform stack takes the full area (minus padding)
    if (waveformStack_)
        waveformStack_->setBounds(bounds);

    // Crosshair overlay should match the waveform area exactly
    if (crosshairOverlay_)
        crosshairOverlay_->setBounds(bounds);

    // Stats overlay positions itself within the waveform area
    if (statsOverlay_)
        statsOverlay_->updatePositionInParent(bounds);
}

void PaneBody::mouseMove(const juce::MouseEvent& event) { updateCrosshairPosition(event.getPosition()); }

void PaneBody::mouseEnter(const juce::MouseEvent& event)
{
    if (crosshairOverlay_)
    {
        crosshairOverlay_->setVisible(true);
        crosshairOverlay_->setCrosshairVisible(true);
    }
    updateCrosshairPosition(event.getPosition());
}

void PaneBody::mouseExit(const juce::MouseEvent& /*event*/)
{
    if (crosshairOverlay_)
    {
        crosshairOverlay_->setCrosshairVisible(false);
        crosshairOverlay_->setVisible(false);
    }
}

void PaneBody::updateCrosshairPosition(juce::Point<int> pos)
{
    if (!crosshairOverlay_)
        return;

    // Crosshair expects position relative to itself.
    auto localPoint = pos - crosshairOverlay_->getPosition();
    crosshairOverlay_->setMousePosition(localPoint);

    // Calculate time/amplitude values using the local point
    float timeMs = 0.0f;
    float ampDb = 0.0f;
    calculateCrosshairValues(localPoint, timeMs, ampDb);

    crosshairOverlay_->setTimeValue(timeMs);
    crosshairOverlay_->setAmplitudeValue(ampDb);
}

void PaneBody::calculateCrosshairValues(juce::Point<int> localPos, float& timeMs, float& ampDb)
{
    if (!crosshairOverlay_)
        return;

    auto bounds = crosshairOverlay_->getLocalBounds();

    // Guard against zero or invalid dimensions to prevent division by zero and NaN
    int width = bounds.getWidth();
    int height = bounds.getHeight();
    if (width <= 0 || height <= 0)
        return;

    // Guard against invalid sample rate or display samples
    if (sampleRate_ <= 0 || displaySamples_ <= 0)
        return;

    // Calculate time based on X position
    float normalizedX = static_cast<float>(localPos.x) / static_cast<float>(width);
    normalizedX = juce::jlimit(0.0f, 1.0f, normalizedX);

    float totalTimeMs = (static_cast<float>(displaySamples_) / static_cast<float>(sampleRate_)) * 1000.0f;
    timeMs = normalizedX * totalTimeMs;

    // Calculate amplitude based on Y position
    float normalizedY = static_cast<float>(localPos.y) / static_cast<float>(height);
    normalizedY = juce::jlimit(0.0f, 1.0f, normalizedY);

    // Linear interpolation between +6dB and -60dB
    constexpr float MAX_DB = 6.0f;
    constexpr float MIN_DB = -60.0f;
    ampDb = MAX_DB - (normalizedY * (MAX_DB - MIN_DB));
}

// Oscillator management (delegates to WaveformStack)

void PaneBody::addOscillator(const Oscillator& oscillator)
{
    if (waveformStack_)
        waveformStack_->addOscillator(oscillator);
}

void PaneBody::removeOscillator(const OscillatorId& oscillatorId)
{
    if (waveformStack_)
        waveformStack_->removeOscillator(oscillatorId);
}

void PaneBody::clearOscillators()
{
    if (waveformStack_)
        waveformStack_->clearOscillators();
}

void PaneBody::updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId)
{
    if (waveformStack_)
        waveformStack_->updateOscillatorSource(oscillatorId, newSourceId);
}

void PaneBody::updateOscillator(const OscillatorId& oscillatorId, ProcessingMode mode, bool visible)
{
    if (waveformStack_)
        waveformStack_->updateOscillator(oscillatorId, mode, visible);
}

void PaneBody::updateOscillatorName(const OscillatorId& oscillatorId, const juce::String& name)
{
    if (waveformStack_)
        waveformStack_->updateOscillatorName(oscillatorId, name);
}

void PaneBody::updateOscillatorColor(const OscillatorId& oscillatorId, juce::Colour color)
{
    if (waveformStack_)
        waveformStack_->updateOscillatorColor(oscillatorId, color);
}

void PaneBody::updateOscillatorFull(const Oscillator& oscillator)
{
    if (waveformStack_)
        waveformStack_->updateOscillatorFull(oscillator);
}

size_t PaneBody::getOscillatorCount() const { return waveformStack_ ? waveformStack_->getOscillatorCount() : 0; }

// Display settings (delegates to WaveformStack)

void PaneBody::setShowGrid(bool enabled)
{
    if (waveformStack_)
        waveformStack_->setShowGrid(enabled);
}

void PaneBody::setGridConfig(const GridConfiguration& config)
{
    if (waveformStack_)
        waveformStack_->setGridConfig(config);
}

void PaneBody::setAutoScale(bool enabled)
{
    if (waveformStack_)
        waveformStack_->setAutoScale(enabled);
}

void PaneBody::setHoldDisplay(bool enabled)
{
    if (waveformStack_)
        waveformStack_->setHoldDisplay(enabled);
}

void PaneBody::setGainDb(float dB)
{
    if (waveformStack_)
        waveformStack_->setGainDb(dB);
}

void PaneBody::setDisplaySamples(int samples)
{
    displaySamples_ = samples;
    if (waveformStack_)
        waveformStack_->setDisplaySamples(samples);
}

void PaneBody::setSampleRate(int sampleRate)
{
    if (sampleRate > 0)
        sampleRate_ = sampleRate;
}

void PaneBody::requestWaveformRestartAtTimestamp(int64_t timelineSampleTimestamp)
{
    if (waveformStack_)
        waveformStack_->requestRestartAtTimestamp(timelineSampleTimestamp);
}

// Highlight oscillator

void PaneBody::highlightOscillator(const OscillatorId& oscillatorId)
{
    if (waveformStack_)
        waveformStack_->highlightOscillator(oscillatorId);
}

// Overlay control

void PaneBody::setStatsVisible(bool visible)
{
    if (statsOverlay_)
    {
        statsOverlay_->setVisibleAnimated(visible);
        if (visible)
            startTimerHz(STATS_UPDATE_HZ);
        else
            stopTimer();
    }
}

bool PaneBody::isStatsVisible() const { return statsOverlay_ && statsOverlay_->isVisible(); }

void PaneBody::timerCallback()
{
    if (isStatsVisible() && isShowing())
    {
        updateStats();
    }
    else
    {
        // Stop when stats are hidden OR when the component is not showing.
        // visibilityChanged() restarts the timer when we become visible again.
        stopTimer();
    }
}

void PaneBody::visibilityChanged()
{
    if (isShowing() && isStatsVisible())
        startTimerHz(STATS_UPDATE_HZ);
}

void PaneBody::updateStats()
{
    if (!waveformStack_ || !statsOverlay_)
        return;

    std::vector<OscillatorStats> stats;
    size_t count = waveformStack_->getOscillatorCount();

    for (size_t i = 0; i < count; ++i)
    {
        const auto* osc = waveformStack_->getOscillatorAt(i);
        if (!osc)
            continue;

        auto sourceId = osc->getSourceId();
        if (!sourceId.isValid())
            continue;

        auto sourceInfo = instanceRegistry_.getSource(sourceId);
        if (!sourceInfo.has_value() || !sourceInfo->analysisEngine)
            continue;

        OscillatorStats os;
        os.name = osc->getName();

        // Copy metrics manually from atomics to snapshot
        auto copyMetrics = [](const ChannelMetrics& src, MetricSnapshot& dest) {
            dest.rmsDb = src.rmsDb.load(std::memory_order_relaxed);
            dest.peakDb = src.peakDb.load(std::memory_order_relaxed);
            dest.crestFactorDb = src.crestFactorDb.load(std::memory_order_relaxed);
            dest.dcOffset = src.dcOffset.load(std::memory_order_relaxed);
            dest.attackTimeMs = src.attackTimeMs.load(std::memory_order_relaxed);
            dest.decayTimeMs = src.decayTimeMs.load(std::memory_order_relaxed);
            dest.maxPeakDb = src.maxPeakDb.load(std::memory_order_relaxed);
        };

        const auto& srcMetrics = sourceInfo->analysisEngine->getMetrics();
        copyMetrics(srcMetrics.left, os.left);
        copyMetrics(srcMetrics.right, os.right);

        os.hasSignal = (os.left.rmsDb > -90.0f || os.right.rmsDb > -90.0f);

        stats.push_back(std::move(os));
    }

    statsOverlay_->updateStats(stats);
}

void PaneBody::resetStats()
{
    if (!waveformStack_)
        return;
    size_t count = waveformStack_->getOscillatorCount();

    for (size_t i = 0; i < count; ++i)
    {
        const auto* osc = waveformStack_->getOscillatorAt(i);
        if (!osc)
            continue;

        auto sourceId = osc->getSourceId();
        if (!sourceId.isValid())
            continue;

        auto sourceInfo = instanceRegistry_.getSource(sourceId);
        if (sourceInfo.has_value() && sourceInfo->analysisEngine)
        {
            sourceInfo->analysisEngine->resetAccumulated();
        }
    }
}

} // namespace oscil
