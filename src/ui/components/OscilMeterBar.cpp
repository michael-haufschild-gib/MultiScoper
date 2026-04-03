/*
    Oscil - Meter Bar Component Implementation
*/

#include "ui/components/OscilMeterBar.h"

#include <algorithm>
#include <cmath>

namespace oscil
{

OscilMeterBar::OscilMeterBar(IThemeService& themeService) : ThemedComponent(themeService) { startTimerHz(TIMER_HZ); }

OscilMeterBar::OscilMeterBar(IThemeService& themeService, const juce::String& testId) : OscilMeterBar(themeService)
{
    setTestId(testId);
}

void OscilMeterBar::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

OscilMeterBar::~OscilMeterBar() { stopTimer(); }

void OscilMeterBar::setLevel(float level)
{
    leftLevel_ = level;
    rightLevel_ = level;

    // Check for clipping
    if (level > 1.0f)
        leftClip_ = rightClip_ = true;

    // Update peak hold
    if (level > leftPeakHold_)
    {
        leftPeakHold_ = level;
        leftPeakHoldCounter_ =
            static_cast<int>(static_cast<float>(peakHoldTimeMs_) * 0.001f * static_cast<float>(TIMER_HZ));
    }
}

void OscilMeterBar::setLevels(float left, float right)
{
    leftLevel_ = left;
    rightLevel_ = right;

    if (left > 1.0f)
        leftClip_ = true;
    if (right > 1.0f)
        rightClip_ = true;

    if (left > leftPeakHold_)
    {
        leftPeakHold_ = left;
        leftPeakHoldCounter_ =
            static_cast<int>(static_cast<float>(peakHoldTimeMs_) * 0.001f * static_cast<float>(TIMER_HZ));
    }

    if (right > rightPeakHold_)
    {
        rightPeakHold_ = right;
        rightPeakHoldCounter_ =
            static_cast<int>(static_cast<float>(peakHoldTimeMs_) * 0.001f * static_cast<float>(TIMER_HZ));
    }
}

void OscilMeterBar::setRMSLevel(float rms) { leftRMS_ = rightRMS_ = rms; }

void OscilMeterBar::setRMSLevels(float leftRMS, float rightRMS)
{
    leftRMS_ = leftRMS;
    rightRMS_ = rightRMS;
}

void OscilMeterBar::setOrientation(Orientation orientation)
{
    if (orientation_ != orientation)
    {
        orientation_ = orientation;
        repaint();
    }
}

void OscilMeterBar::setMeterType(MeterType type)
{
    if (meterType_ != type)
    {
        meterType_ = type;
        repaint();
    }
}

void OscilMeterBar::setStereo(bool stereo)
{
    if (stereo_ != stereo)
    {
        stereo_ = stereo;
        repaint();
    }
}

void OscilMeterBar::setPeakHoldTime(int milliseconds) { peakHoldTimeMs_ = milliseconds; }

void OscilMeterBar::setPeakDecayRate(float decayPerSecond) { peakDecayRate_ = decayPerSecond; }

void OscilMeterBar::setMinDb(float db) { minDb_ = db; }

void OscilMeterBar::setMaxDb(float db) { maxDb_ = db; }

void OscilMeterBar::setShowScale(bool show)
{
    if (showScale_ != show)
    {
        showScale_ = show;
        repaint();
    }
}

void OscilMeterBar::resetClip()
{
    leftClip_ = rightClip_ = false;
    repaint();
}

void OscilMeterBar::resetPeakHold()
{
    leftPeakHold_ = rightPeakHold_ = 0.0f;
    leftPeakHoldCounter_ = rightPeakHoldCounter_ = 0;
}

int OscilMeterBar::getPreferredWidth() const
{
    int const meterWidth = stereo_ ? STEREO_WIDTH : MONO_WIDTH;
    return meterWidth + (showScale_ ? SCALE_WIDTH : 0);
}

int OscilMeterBar::getPreferredHeight() const { return 100; }

void OscilMeterBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(getTheme().backgroundSecondary);
    g.fillRect(bounds);

    int const scaleWidth = showScale_ ? SCALE_WIDTH : 0;
    auto meterBounds = bounds.withWidth(bounds.getWidth() - scaleWidth);

    if (stereo_)
    {
        auto leftBounds = meterBounds.removeFromLeft(meterBounds.getWidth() / 2);
        auto rightBounds = meterBounds;

        paintMeter(g, leftBounds.reduced(1), leftSmooth_, leftRMS_, leftPeakHold_, leftClip_);
        paintMeter(g, rightBounds.reduced(1), rightSmooth_, rightRMS_, rightPeakHold_, rightClip_);
    }
    else
    {
        paintMeter(g, meterBounds.reduced(1), leftSmooth_, leftRMS_, leftPeakHold_, leftClip_);
    }

    if (showScale_)
    {
        auto scaleBounds = bounds.removeFromRight(SCALE_WIDTH);
        paintScale(g, scaleBounds);
    }
}

void OscilMeterBar::paintMeterVertical(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                                       juce::ColourGradient& gradient, float levelPos, float rmsPos, float peakPos,
                                       bool clip)
{
    gradient.point1 = {static_cast<float>(bounds.getX()), static_cast<float>(bounds.getBottom())};
    gradient.point2 = {static_cast<float>(bounds.getX()), static_cast<float>(bounds.getY())};
    gradient.isRadial = false;

    int const levelHeight = static_cast<int>(levelPos * static_cast<float>(bounds.getHeight()));
    if (levelHeight > 0)
    {
        g.setGradientFill(gradient);
        g.fillRect(bounds.withTop(bounds.getBottom() - levelHeight));
    }

    if (meterType_ == MeterType::PeakWithRMS)
    {
        int const rmsHeight = static_cast<int>(rmsPos * static_cast<float>(bounds.getHeight()));
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.fillRect(bounds.getX(), bounds.getBottom() - rmsHeight, bounds.getWidth(), 2);
    }

    int const peakY = bounds.getBottom() - static_cast<int>(peakPos * static_cast<float>(bounds.getHeight()));
    g.setColour(getTheme().textPrimary);
    g.fillRect(bounds.getX(), peakY - 1, bounds.getWidth(), 2);

    if (clip)
    {
        g.setColour(getTheme().statusError);
        g.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth(), 4);
    }
}

void OscilMeterBar::paintMeterHorizontal(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                                         juce::ColourGradient& gradient, float levelPos, float peakPos, bool clip)
{
    gradient.point1 = {static_cast<float>(bounds.getX()), static_cast<float>(bounds.getY())};
    gradient.point2 = {static_cast<float>(bounds.getRight()), static_cast<float>(bounds.getY())};
    gradient.isRadial = false;

    int const levelWidth = static_cast<int>(levelPos * static_cast<float>(bounds.getWidth()));
    if (levelWidth > 0)
    {
        g.setGradientFill(gradient);
        g.fillRect(bounds.withWidth(levelWidth));
    }

    int const peakX = bounds.getX() + static_cast<int>(peakPos * static_cast<float>(bounds.getWidth()));
    g.setColour(getTheme().textPrimary);
    g.fillRect(peakX - 1, bounds.getY(), 2, bounds.getHeight());

    if (clip)
    {
        g.setColour(getTheme().statusError);
        g.fillRect(bounds.getRight() - 4, bounds.getY(), 4, bounds.getHeight());
    }
}

void OscilMeterBar::paintMeter(juce::Graphics& g, juce::Rectangle<int> bounds, float level, float rms, float peakHold,
                               bool clip)
{
    float const levelPos = levelToPosition(std::min(level, 1.0f));
    float const rmsPos = levelToPosition(std::min(rms, 1.0f));
    float const peakPos = levelToPosition(std::min(peakHold, 1.0f));

    juce::ColourGradient gradient;
    gradient.addColour(0.0, getTheme().statusActive);
    gradient.addColour(0.7, getTheme().statusWarning);
    gradient.addColour(1.0, getTheme().statusError);

    if (orientation_ == Orientation::Vertical)
        paintMeterVertical(g, bounds, gradient, levelPos, rmsPos, peakPos, clip);
    else
        paintMeterHorizontal(g, bounds, gradient, levelPos, peakPos, clip);
}

void OscilMeterBar::paintScale(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(getTheme().textSecondary);
    g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));

    // Draw dB markers
    static const std::vector<float> markers = {0.0f, -3.0f, -6.0f, -12.0f, -24.0f, -48.0f};

    for (float const db : markers)
    {
        if (db < minDb_ || db > maxDb_)
            continue;

        float const level = dbToLevel(db);
        float const pos = levelToPosition(level);

        int const y = bounds.getBottom() - static_cast<int>(pos * static_cast<float>(bounds.getHeight()));

        g.drawText(juce::String(static_cast<int>(db)), bounds.getX(), y - 6, bounds.getWidth(), 12,
                   juce::Justification::centredLeft);

        // Tick mark
        g.fillRect(bounds.getX(), y, 3, 1);
    }
}

float OscilMeterBar::levelToPosition(float level) const
{
    if (level <= 0.0f)
        return 0.0f;

    float db = 20.0f * std::log10(level);
    db = std::clamp(db, minDb_, maxDb_);

    // Guard against division by zero if range is invalid
    float const range = maxDb_ - minDb_;
    if (std::abs(range) < 0.001f)
        return 0.5f;

    return (db - minDb_) / range;
}

float OscilMeterBar::dbToLevel(float db) const { return std::pow(10.0f, db / 20.0f); }

juce::Colour OscilMeterBar::getLevelColour(float normalizedLevel) const
{
    // Green -> Yellow -> Red gradient
    if (normalizedLevel < 0.7f)
        return getTheme().statusActive; // Green
    if (normalizedLevel < 0.9f)
        return getTheme().statusWarning; // Yellow
    else
        return getTheme().statusError; // Red
}

void OscilMeterBar::resized()
{
    // Nothing to do
}

void OscilMeterBar::mouseDown(const juce::MouseEvent& /*event*/)
{
    // Click to reset clip
    resetClip();
}

void OscilMeterBar::timerCallback()
{
    // Smooth level display
    leftSmooth_ += (leftLevel_ - leftSmooth_) * SMOOTH_FACTOR;
    rightSmooth_ += (rightLevel_ - rightSmooth_) * SMOOTH_FACTOR;

    // Decay levels
    leftLevel_ *= 0.9f;
    rightLevel_ *= 0.9f;
    leftRMS_ *= 0.95f;
    rightRMS_ *= 0.95f;

    // Peak hold decay
    if (leftPeakHoldCounter_ > 0)
    {
        leftPeakHoldCounter_--;
    }
    else
    {
        leftPeakHold_ -= peakDecayRate_ * FRAME_TIME_SEC;
        leftPeakHold_ = std::max<float>(leftPeakHold_, 0);
    }

    if (rightPeakHoldCounter_ > 0)
    {
        rightPeakHoldCounter_--;
    }
    else
    {
        rightPeakHold_ -= peakDecayRate_ * FRAME_TIME_SEC;
        rightPeakHold_ = std::max<float>(rightPeakHold_, 0);
    }

    repaint();
}

} // namespace oscil
