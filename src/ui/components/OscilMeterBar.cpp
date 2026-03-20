/*
    Oscil - Meter Bar Component Implementation
*/

#include "ui/components/OscilMeterBar.h"
#include <cmath>

namespace oscil
{

OscilMeterBar::OscilMeterBar(IThemeService& themeService)
    : ThemedComponent(themeService)
{
    startTimerHz(TIMER_HZ);
}

OscilMeterBar::OscilMeterBar(IThemeService& themeService, const juce::String& testId)
    : OscilMeterBar(themeService)
{
    setTestId(testId);
}

void OscilMeterBar::registerTestId()
{
    OSCIL_REGISTER_TEST_ID(testId_);
}

OscilMeterBar::~OscilMeterBar()
{
    stopTimer();
}

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
        leftPeakHoldCounter_ = static_cast<int>(peakHoldTimeMs_ * 0.001f * TIMER_HZ);
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
        leftPeakHoldCounter_ = static_cast<int>(peakHoldTimeMs_ * 0.001f * TIMER_HZ);
    }

    if (right > rightPeakHold_)
    {
        rightPeakHold_ = right;
        rightPeakHoldCounter_ = static_cast<int>(peakHoldTimeMs_ * 0.001f * TIMER_HZ);
    }
}

void OscilMeterBar::setRMSLevel(float rms)
{
    leftRMS_ = rightRMS_ = rms;
}

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

void OscilMeterBar::setPeakHoldTime(int milliseconds)
{
    peakHoldTimeMs_ = milliseconds;
}

void OscilMeterBar::setPeakDecayRate(float decayPerSecond)
{
    peakDecayRate_ = decayPerSecond;
}

void OscilMeterBar::setMinDb(float db)
{
    minDb_ = db;
}

void OscilMeterBar::setMaxDb(float db)
{
    maxDb_ = db;
}

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
    int meterWidth = stereo_ ? STEREO_WIDTH : MONO_WIDTH;
    return meterWidth + (showScale_ ? SCALE_WIDTH : 0);
}

int OscilMeterBar::getPreferredHeight() const
{
    return 100;
}

void OscilMeterBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(getTheme().backgroundSecondary);
    g.fillRect(bounds);

    int scaleWidth = showScale_ ? SCALE_WIDTH : 0;
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

void OscilMeterBar::paintMeter(juce::Graphics& g, juce::Rectangle<int> bounds,
                                float level, float rms, float peakHold, bool clip)
{
    float levelPos = levelToPosition(std::min(level, 1.0f));
    float rmsPos = levelToPosition(std::min(rms, 1.0f));
    float peakPos = levelToPosition(std::min(peakHold, 1.0f));

    // Create gradient for the full meter range
    juce::ColourGradient gradient;
    
    // Define gradient colors (Green -> Yellow -> Red)
    gradient.addColour(0.0, getTheme().statusActive);   // Bottom/Left
    gradient.addColour(0.7, getTheme().statusWarning);  // Mid
    gradient.addColour(1.0, getTheme().statusError);    // Top/Right

    if (orientation_ == Orientation::Vertical)
    {
        // Setup vertical gradient (Bottom to Top)
        gradient.point1 = { static_cast<float>(bounds.getX()), static_cast<float>(bounds.getBottom()) };
        gradient.point2 = { static_cast<float>(bounds.getX()), static_cast<float>(bounds.getY()) };
        gradient.isRadial = false;

        // Level fill
        int levelHeight = static_cast<int>(levelPos * bounds.getHeight());
        if (levelHeight > 0)
        {
            auto levelBounds = bounds.withTop(bounds.getBottom() - levelHeight);
            g.setGradientFill(gradient);
            g.fillRect(levelBounds);
        }

        // RMS indicator (if showing both)
        if (meterType_ == MeterType::PeakWithRMS)
        {
            int rmsHeight = static_cast<int>(rmsPos * bounds.getHeight());
            auto rmsBounds = bounds.withTop(bounds.getBottom() - rmsHeight);

            g.setColour(juce::Colours::white.withAlpha(0.3f));
            g.fillRect(rmsBounds.getX(), rmsBounds.getY(), rmsBounds.getWidth(), 2);
        }

        // Peak hold indicator
        int peakY = bounds.getBottom() - static_cast<int>(peakPos * bounds.getHeight());
        g.setColour(getTheme().textPrimary);
        g.fillRect(bounds.getX(), peakY - 1, bounds.getWidth(), 2);

        // Clip indicator
        if (clip)
        {
            g.setColour(getTheme().statusError);
            g.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth(), 4);
        }
    }
    else
    {
        // Setup horizontal gradient (Left to Right)
        gradient.point1 = { static_cast<float>(bounds.getX()), static_cast<float>(bounds.getY()) };
        gradient.point2 = { static_cast<float>(bounds.getRight()), static_cast<float>(bounds.getY()) };
        gradient.isRadial = false;

        // Horizontal orientation
        int levelWidth = static_cast<int>(levelPos * bounds.getWidth());
        if (levelWidth > 0)
        {
            auto levelBounds = bounds.withWidth(levelWidth);
            g.setGradientFill(gradient);
            g.fillRect(levelBounds);
        }

        // Peak hold
        int peakX = bounds.getX() + static_cast<int>(peakPos * bounds.getWidth());
        g.setColour(getTheme().textPrimary);
        g.fillRect(peakX - 1, bounds.getY(), 2, bounds.getHeight());

        // Clip
        if (clip)
        {
            g.setColour(getTheme().statusError);
            g.fillRect(bounds.getRight() - 4, bounds.getY(), 4, bounds.getHeight());
        }
    }
}

void OscilMeterBar::paintScale(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(getTheme().textSecondary);
    g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));

    // Draw dB markers
    static const std::vector<float> markers = {0.0f, -3.0f, -6.0f, -12.0f, -24.0f, -48.0f};

    for (float db : markers)
    {
        if (db < minDb_ || db > maxDb_)
            continue;

        float level = dbToLevel(db);
        float pos = levelToPosition(level);

        int y = bounds.getBottom() - static_cast<int>(pos * bounds.getHeight());

        g.drawText(juce::String(static_cast<int>(db)),
                   bounds.getX(), y - 6, bounds.getWidth(), 12,
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
    float range = maxDb_ - minDb_;
    if (std::abs(range) < 0.001f)
        return 0.5f;

    return (db - minDb_) / range;
}

float OscilMeterBar::dbToLevel(float db) const
{
    return std::pow(10.0f, db / 20.0f);
}

juce::Colour OscilMeterBar::getLevelColour(float normalizedLevel) const
{
    // Green -> Yellow -> Red gradient
    if (normalizedLevel < 0.7f)
        return getTheme().statusActive;  // Green
    else if (normalizedLevel < 0.9f)
        return getTheme().statusWarning;  // Yellow
    else
        return getTheme().statusError;    // Red
}

void OscilMeterBar::resized()
{
    // Nothing to do
}

void OscilMeterBar::mouseDown(const juce::MouseEvent&)
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
        if (leftPeakHold_ < 0) leftPeakHold_ = 0;
    }

    if (rightPeakHoldCounter_ > 0)
    {
        rightPeakHoldCounter_--;
    }
    else
    {
        rightPeakHold_ -= peakDecayRate_ * FRAME_TIME_SEC;
        if (rightPeakHold_ < 0) rightPeakHold_ = 0;
    }

    repaint();
}

} // namespace oscil
